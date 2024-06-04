Dealing with data over sockets is a common activity for embedded devices, and Thunder provides built-in support for a number of different socket types. Using these classes, we can send, receive and monitor for state changes on the sockets.

Thunder supports the following types of socket:

* Unix domain socket
    * Both `SOCK_STREAM` and `SOCK_DGRAM`
* IPv4
* IPv6
* Bluetooth (if Thunder is built with Bluetooth support)
* Netlink
* Packet
* RS232 Serial

The following documentation will cover examples for some common use-cases for socket programming with Thunder.

## Resource Monitor

Thunder provides a mechanism known as the "Resource Monitor" for monitoring file descriptors and sockets. The resource monitor is a singleton constructed at process launch, and uses `poll()` on Linux to listen to provided file descriptors. 

Resource monitor uses a single thread, and it is strongly recommended that plugins use this to monitor file descriptors and sockets instead of spinning up their own thread(s). For out-of-process plugins, the ThunderPlugin host will run its own instance of ResourceMonitor, which can be useful for performance-criticial plugins.

!!! danger
	The same instance of ResourceMonitor is also used for receiving incoming JSON-RPC and COM-RPC messages. Since this is a single thread, it is vitally important not to block the thread with any processing or long-lived task. If the thread is blocked, Thunder will not be able to process any incoming messages, impacting overall performance and responsiveness. Instead, make use of the worker pool to do any processing work.


To add a new entry to the resource monitor, construct an object of type `IResource`, then add it to the monitor by calling

```cpp
Core::ResourceMonitor::Instance().Register(<object>)
```

All `IResource` objects must implement 3 methods

```cpp
// Returns the file descriptor that should be monitored
virtual handle Descriptor() const = 0;

// Return the events that you are interested in (e.g. POLLIN)
virtual uint16_t Events() = 0;

// Invoked (on the resource monitor thread) whenever an event occurs on the file descriptor
virtual void Handle(const uint16_t events) = 0;
```

To view the current resources being monitored by the ResourceMonitor, run Thunder in a foreground terminal and press the **M** key:

```
Resource Monitor Entry states:
============================================================
Currently monitoring: 5 resources
     5 socket:[1123804][I--:---]: LinkType<Thunder::Core::SocketPort, Thunder::Core::IMessage, Thunder::Core::IMessage, Thunder::Core::IPCChannel::IPCFactory&>::HandlerType<Thunder::Core::LinkType<Thunder::Core::SocketPort, Thunder::Core::IMessage, Thunder::Core::IMessage, Thunder::Core::IPCChannel::IPCFactory&>, Thunder::Core::SocketPort>
     8 socket:[1121398][I--:---]: Handler
     9 socket:[1121399][I--:---]: Handler
    10 socket:[1121400][I--:---]: Handler
    11 socket:[1129519][I-H:---]: WebSocketLinkType<Thunder::Core::SocketStream, Thunder::PluginHost::Request, Thunder::Web::Response, Thunder::Core::ProxyPoolType<Thunder::PluginHost::Request>&>::HandlerType<Thunder::Core::SocketStream>
```

Each line contains the following information:

* File descriptor
* File name
* Flags (the events being monitored)
    * `I` = `POLLIN`
    * `O` = `POLLOUT`
    * `H` = `POLLHUP`
* Class name

The same information can be retrieved programmatically by querying the ResourceMonitor singleton:

```c++
Core::ResourceMonitor& monitor = Core::ResourceMonitor::Instance();
Core::ResourceMonitor::Metadata info {};

uint32_t index = 0;
while (monitor.Info(index, info) == true) {
	printf ("%s\n", info.filename);
	index++;
}
```



## Generic Socket Classes

Thunder provides generic templates that support multiple types of socket underneath. This makes it simple to re-use the same code for both unix and TCP sockets for example. These classes also automatically integrate with the resource monitor and provide friendly read/write methods for handling sending and receiving data.

The `Core::NodeId` class represents a generic socket of any type (unix, internet, bluetooth etc) and provides common methods applicable to all sockets. The type of socket is dependant on the specific constructor called.

### Stream vs Datagram Sockets

When working with sockets, there are two main classes of socket:

* Stream sockets (represented in Thunder by `Core::SocketStream`)
* Datagram sockets (represented in Thunder by `Core::SocketDatagram`)

A stream socket is equivalent to TCP - it can be relied on to deliver data in sequence and without duplicates. Receipt of stream messages is guaranteed, and streams are well suited to handling large amounts of data. This will likely be the most common socket type you use.

A datagram socket is equivalent to UDP - they are not guaranteed to be reliable and data may arrive out-of-order or duplicated. Datagrams are considered "connectionless", meaning no explicit connection is established before sending/receiving data.

For stream sockets, since they are very common, Thunder provides generalisations for common data types that will likely be passed over the socket. The following types are supported:

* `StreamTextType` - when the data transferred over the socket will be textual. The template accepts a terminator, which defines how the incoming strings will be split into discrete messages (e.g. null, carriage return, line feed)
* `StreamJSONType` - when the data transferred over the socket will be formatted as JSON documents

To use these types, you must provide an implementation for the following pure virtual methods:

```cpp
virtual void Received(ProxyType<INTERFACE>& element) = 0;
virtual void Send(ProxyType<INTERFACE>& element) = 0;
virtual void StateChange() = 0;
```

Where the proxy-types correspond to the data type (e.g. for StreamJSONType this will be a JSON document). See the worked example below for a demonstration.

## Stream Socket Example

In the below example, we will create two classes to act as server and client applications. By making use of the generic socket classes, the code can then be used for communication across different socket types without needing to write code specifically for each one.

### Server

To create a socket server, we will use the `Core::SocketServerType` class. This handles the creation of our server for us. 

!!! tip
	If using a Unix domain socket, the socket file will be automatically created upon construction and destroyed when the server is destructed, so there is no manual cleanup required

The only thing we need to do is provide an implementation of a client - which will be the code that represents a single connection to the socket. For each connection, a new instance of the client will be created (so N connections = N clients). This client code will be responsible for sending/receiving/monitoring that particular connection.

In this case, we are going to deal with string-based data, so will use the `Core::StreamTextType` template. By specifying the terminator type as `TerminatorCarriageReturn`, we indicate that incoming strings should be split by a carriage return. 

Our client will print a message each time it receives some data over the socket. When the connection is established, we will send the string `Welcome!` back.

```cpp linenums="1"
class Connection : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
private:
    using BaseClass = Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn>;

public:
    /**
     * This constructor is called each time a connection is established with the socket
     *
     * @param[in] connector     The client's connection ID
     * @param[in] remoteId      The socket the client is connected to
     * @param[in] server        A pointer to the server the client is connected to
    */
    Connection(const SOCKET& connector, const Core::NodeId& remoteId, Core::SocketServerType<Connection>* server)
        : BaseClass(false, connector, remoteId, 1024, 1024)
    {
    }

    ~Connection() = default;

    Connection(Connection&&) = delete;
    Connection(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;

    /**
     * Called every time we receive some data over the socket. Since we inherit from StreamTextType,
     * our data is formatted as a string
    */
    void Received(string& text) override
    {
        printf("Received data %s [size %d]\n", text.c_str(), static_cast<uint32_t>(text.size()));
    }

    /**
     * Called when data is sent over the socket
     *
     * This is not the method to call to actually send data, instead use the Submit() method to actually
     * send a string over the socket.
    */
    void Send(const string& text) override
    {
        printf("About to send data %s\n", text.c_str());
    }

    /**
     * Called when the connection changes state (e.g. open/close)
    */
    void StateChange() override
    {
        if (IsOpen()) {
            printf("State change occurred - connection is open\n");
            
            // We have an open connection, send the string "Welcome" back over the socket
            Submit("Welcome!");
        } else {
            printf("State change occurred - connection is closed\n");
        }
    }
};
```

Now we have created our generic client, we can construct a server. Here, the server will start and listen on a unix domain socket at `/tmp/testSocket` for 30 seconds before exiting.

!!! hint
	For UNIX domain sockets, The `Core::NodeId` allows supplying a group and/or permissions in the constructor. For example:
	
	```cpp
	// Create a socket at /tmp/sampleSocket with permissions set to 0755
	Core::NodeId("/tmp/sampleSocket|0755");
	
	// Create a socket with the group set to "administrator" and permissions set to 0655
	Core::NodeId("/tmp/sampleSocket|0655,administrator");
	```

When `Open()` is called, it will register our socket with the resource monitor instance to monitor the socket for data. The `Open()` method takes a timeout time in seconds. If you want to wait forever, then supply `Core::infinite` as the timeout.


```c++ linenums="1"
int main(int argc, char const* argv[])
{
    Core::SocketServerType<Connection> server(Core::NodeId("/tmp/testSocket"));

    server.Open(Core::infinite);
    SleepS(30);
    server.Close(Core::infinite);

    // Must call this at the end of the code to dispose of the resource monitor singleton
    Core::Singleton::Dispose();

    return 0;
}
```

We can test this using the [netcat](https://manpages.ubuntu.com/manpages/focal/man1/nc_openbsd.1.html) utility in Linux to connect to the socket and send the text "Hello World":

```shell
$ nc -U /tmp/testSocket
Welcome!
Hello World
```

 In the server logs, we see the connection being opened, followed by receiving the string "Hello World" sent by netcat. The program then exits, closing and deleting the socket.

```title="Server Log"
[Singleton.h:95](SingletonType)<PID:71858><TID:71858><1>: Singleton constructing ResourceMonitor
State change occurred - connection is open
About to send data 'Welcome!'
Received data 'Hello World' [size 11]
[SocketPort.cpp:1260](Closed)<PID:71858><TID:71862><1>: CLOSED: Remove socket descriptor /tmp/testSocket
State change occurred - connection is closed
[Singleton.cpp:51](Dispose)<PID:71858><TID:71858><1>: Singleton destructing ResourceMonitor
```

By using the generic `Core::NodeId` class, this code can easily be re-purposed to listen on different socket types such as a TCP socket by changing a single line in the `main()` method. In this case, we will create a TCP socket listening on port 8080.

``` c++ linenums="1" hl_lines="3"
int main(int argc, char const* argv[])
{
    Core::SocketServerType<Connection> server(Core::NodeId("localhost:8080"));

    server.Open(Core::infinite);
    SleepS(30);
    server.Close(Core::infinite);

    // Must call this at the end of the code to dispose of the resource monitor singleton
    Core::Singleton::Dispose();

    return 0;
}
```

Again, netcat can be used to connect to the server, this time providing an ip address and port:

```shell
$ nc localhost 8080
Welcome!
Hello World
```

### Client

The code for connecting to a socket is essentially the same as the client code, except we don't use the `SocketServerType`.

First, we create our client class. This is the same as the previous code with the exception of the constructor as we will construct this manually when we want to connect to a socket. In addition, since we will use this class directly, we need to add logic for opening/closing the socket ourselves.

```cpp linenums="1" hl_lines="6-11"
class Connection : public Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn> {
private:
    using BaseClass = Core::StreamTextType<Core::SocketStream, Core::TerminatorCarriageReturn>;

public:
    Connection(const Core::NodeId& socket)
        : BaseClass(false, socket.AnyInterface(), socket, 1024, 1024)
    {
        // Attempt to connect to the socket with a 5 second timeout
        Open(5);
    }

    ~Connection()
    {
        if (IsOpen()) {
            Close(Core::infinite);
        }
    }

    Connection(Connection&&) = delete;
    Connection(const Connection&) = delete;
    Connection& operator=(Connection&&) = delete;
    Connection& operator=(const Connection&) = delete;

    /**
     * Called every time we receive some data over the socket. Since we inherit from StreamTextType,
     * our data is formatted as a string
     */
    void Received(string& text) override
    {
        printf("Received data '%s' [size %d]\n", text.c_str(), static_cast<uint32_t>(text.size()));
    }

    /**
     * Called when data is sent over the socket
     *
     * This is not the method to call to actually send data, instead use the Submit() method to actually
     * send a string over the socket.
     */
    void Send(const string& text) override
    {
        printf("About to send data '%s'\n", text.c_str());
    }

    /**
     * Called when the connection changes state (e.g. open/close)
     */
    void StateChange() override
    {
        if (IsOpen()) {
            printf("State change occurred - connection is open\n");
        } else {
            printf("State change occurred - connection is closed\n");
        }
    }
};
```

To use, construct a `Connection` object with the path to the socket we want to connect to. Once we open the connection we send a string over the socket.

```cpp linenums="1"
int main(int argc, char const* argv[])
{
    {	
    	// Create our connection - will attempt to connect to the socket on construction
    	Connection socketConnection(Core::NodeId("/tmp/otherSocket"));

    	// Send a message over the socket
    	socketConnection.Submit("Hello from Thunder!");
    	SleepS(30);
    }

    // Must call this at the end of the code to dispose of the ResourceManager singleton
    Core::Singleton::Dispose();

    return 0;
}
```

This can again be tested with `netcat`, using the `-l` argument to listen on a given socket

```title="netcat"
❯ nc -l -U /tmp/otherSocket
Hello from Thunder!
```

```title="client logs"
[Singleton.h:95](SingletonType)<PID:81313><TID:81313><1>: Singleton constructing ResourceMonitor
Successfully connected to /tmp/otherSocket
State change occurred - connection is open
About to send data 'Hello from Thunder!'
State change occurred - connection is closed
[Singleton.cpp:51](Dispose)<PID:81313><TID:81313><1>: Singleton destructing ResourceMonitor
```

## Datagram Socket Example

Datagram sockets are connection-less, so do not require code to explicitly track and monitor each connection independently. Instead, we just need to handle a single stream of incoming messages. 

Start by constructing an implementation of `Core::SocketDatagram` that implements the `SendData`, `ReceiveData` and `StateChange` functions. Following RAII principles, the socket will be opened on construction and closed in the destructor.

```cpp linenums="1"
class SocketReader : public Core::SocketDatagram {
public:
    explicit SocketReader(const Core::NodeId& socket)
        : Core::SocketDatagram(true, socket, Core::NodeId(), 1024, 1024)
    {
        // Start listening on the socket and register with resource monitor
        Open(5);
    }

    ~SocketReader()
    {
        Close(Core::infinite);
    }

    SocketReader(SocketReader&&) = delete;
    SocketReader(const SocketReader&) = delete;
    SocketReader& operator=(SocketReader&&) = delete;
    SocketReader& operator=(const SocketReader&) = delete;

    uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
    {
        // Not interested in sending data for this example
        return 0;
    }

    uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
    {
        uint16_t length = 0;

        // Convert incoming data to a string for this example
        string dataString(reinterpret_cast<char*>(dataFrame), receivedSize);
        printf("%s", dataString.c_str());
        return length;
    }

    void StateChange() override
    {
        if (IsOpen()) {
            printf("Socket is open\n");
        } else {
            printf("Socket is closed\n");
        }
    }
};
```

Now, to listen on the socket construct an instance of this SocketReader class

```cpp linenums="1"
int main(int argc, char const* argv[])
{
    {
        SocketReader monitor(Core::NodeId("/tmp/testDgramSocket"));
        SleepS(10);
    }

    // Must call this at the end of the code to dispose of the ResourceManager singleton
    Core::Singleton::Dispose();
    return 0;
}
```

## RS232 Serial

On embedded devices, it is occasionally necessary to send/receive data over an RS232 serial port. Thunder provides the `Core::SerialPort` class for working with RS232 serial.

Similar to other sockets, we create an implementation of `Core::StreamType` (indicating we want to deal with raw binary data, not strings) to handle the read/write. For the below example, we will create an implementation to monitor a serial port and print the received data to the console.

Following RAII principles, the socket will be opened on construction and closed in the destructor.

```c++ linenums="1"
class SerialPortMonitor : public Core::StreamType<Core::SerialPort> {
public:
    SerialPortMonitor(
        const string& deviceName,
        const Core::SerialPort::BaudRate baudrate,
        const Core::SerialPort::Parity parity,
        const Core::SerialPort::DataBits dataBits,
        const Core::SerialPort::StopBits stopBits,
        const Core::SerialPort::FlowControl flowControl,
        const uint32_t bufferSize)
        : Core::StreamType<Core::SerialPort>(deviceName, baudrate, parity, dataBits, stopBits, flowControl, bufferSize, bufferSize)
    {
        // Calling open will register the port with resource monitor
        if (Open(5) != Core::ERROR_NONE) {
            printf("Failed to open serial port @ '%s'\n", deviceName.c_str());
        } else {
            printf("Opened serial port @ '%s'\n", deviceName.c_str());
        }
    }
    
    ~SerialPortMonitor() override
    {
        Close(Core::infinite);
    }

    SerialPortMonitor(SerialPortMonitor&&) = delete;
    SerialPortMonitor(const SerialPortMonitor&) = delete;
    SerialPortMonitor& operator=(SerialPortMonitor&&) = delete;
    SerialPortMonitor& operator=(const SerialPortMonitor&) = delete;

public:
    uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
    {
        return 0;
    }

    uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
    {
        // Convert incoming data to a string for this example
        string dataString(reinterpret_cast<char*>(dataFrame), receivedSize);
        printf("%s", dataString.c_str());
        return 0;
    }

    void StateChange() override
    {
        return;
    }
};
```

Then to construct the port, we provide suitable options (baud rate, flow control, parity etc)

```c++ linenums="1"
int main(int argc, char const* argv[])
{
    // Set up our serial port options (115200-8-N-1)
    // Using a USB serial adapter for this example
    Core::SerialPort::BaudRate baudRate(Core::SerialPort::BaudRate::BAUDRATE_115200);
    Core::SerialPort::Parity parity(Core::SerialPort::NONE);
    Core::SerialPort::DataBits dataBits(Core::SerialPort::DataBits::BITS_8);
    Core::SerialPort::StopBits stopBits(Core::SerialPort::StopBits::BITS_1);
    Core::SerialPort::FlowControl flowControl(Core::SerialPort::FlowControl::OFF);
    const string port = "/dev/ttyUSB1";

    {
        SerialPortMonitor serialPort(port, baudRate, parity, dataBits, stopBits, flowControl, 1024);
        SleepS(10);
    }

    // Must call this at the end of the code to dispose of the ResourceManager singleton
    Core::Singleton::Dispose();

    return 0;
}
```

When run, the code will open the serial port and print any incoming messages to the console.


## Systemd Integration

Systemd allows for the use of `.socket` configuration files, which define the socket(s) a service will listen on. For example:

```title="foo.service"
[Unit]
Description=An example systemd service

[Service]
ExecStart=/usr/bin/foo
```

```title="foo.socket"
[Unit]
Description=An example systemd socket

[Socket]
ListenStream=/var/run/foo.socket
```

In this case, systemd will create the `/var/run/foo.socket`  before the foo service starts. It is now systemd's responsibility to create/destroy the socket, not the application. 

Thunder is aware of this. If it is started as a systemd service and asked to create a socket that systemd is managing, Thunder will call `sd_listen_fds()` to check if the socket is managed by systemd. If it is, it will use that socket instead of creating it afresh.

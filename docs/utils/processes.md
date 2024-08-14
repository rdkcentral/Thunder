Occasionally plugins might need to spawn other processes or get information about other processes running on the device. 

## Process Information

The `Core::ProcessInfo` class can be used to retrieve information about a currently running process. 

```c++
int main(int argc, char const* argv[])
{
    Core::ProcessInfo currentProcess;
    printf("Our PID is %d\n", currentProcess.Id());

    // Take a snapshot of the memory usage
    currentProcess.MemoryStats();
    printf("Currently using %ld KB (PSS) memory\n", currentProcess.PSS());

    // Get stats about another process by PID
    Core::ProcessInfo anotherProcess(1);
    printf("Other process name: %s\n", anotherProcess.Name().c_str());

    Core::Singleton::Dispose();

    return 0;
}

/* Output:
Our PID is 51247
Currently using 1333 KB (PSS) memory
Other process name: systemd
*/
```

### Find Process By Name

The `FindByName` method will search for all currently running processes that match the given name and return them in a list. This can be thought of as an equivalent to the `pgrep` command in Linux

```c++
std::list<Core::ProcessInfo> processes;

// Second argument specifies if we should match exactly
Core::ProcessInfo::FindByName("Thunder", true, processes);
printf("There are currently %d Thunder processes\n", processes.size());

/* Output:
There are currently 1 Thunder processes
*/
```

### Finding Child Processes

The `Children()` method returns an iterator containing all the child processes

```c++
Core::ProcessInfo sampleProcess(29614);

printf("Process %d (%s) has %d children\n", sampleProcess.Id(), sampleProcess.Name().c_str(), sampleProcess.Children().Count());

Core::ProcessInfo::Iterator children = sampleProcess.Children();
while (children.Next()) {
    Core::ProcessInfo child = children.Current();
    printf("* [PID: %d] %s\n", child.Id(), child.Name().c_str());
}

/* Output:
Process 29614 (firefox) has 6 children
    * [PID: 29851] firefox
    * [PID: 29879] firefox
    * [PID: 29987] firefox
    * [PID: 30158] firefox
    * [PID: 30161] firefox
    * [PID: 30165] firefox
*/
```

## Spawning Child Processes

The `Core::Process` class provides a way to spawn a child process in a cross-platform way. It can also handle capturing stdout/error, signals and setting command line options.

### Launch a process

This example shows launching a process, then blocking indefinitely until that process has completed. Once completed, it reads the exit code of the process to determine whether it ran successfully.

The `Core::Process::Options` class can be used to pass command-line options to a process. The first option should always be the name of the process to execute. Additional options can be passed by repeatedly calling `.Add()`.

```c++
Core::Process childProcess(false);

// Run the command "/usr/bin/sleep 5"
Core::Process::Options options("/usr/bin/sleep");
options.Add("5");

uint32_t pid;
if (childProcess.Launch(options, &pid) != Core::ERROR_NONE) {
    printf("Failed to launch process\n");
} else {
    printf("Successfully launched process\n");
    
    // Process is now running, block forever until it exits
    if (childProcess.WaitProcessCompleted(Core::infinite) == Core::ERROR_NONE) {
        printf("Process exited with error code %d\n", childProcess.ExitCode());
    } else {
        printf("Timed out waiting for process to complete\n");
    }
}
```

### Capturing stdout/error

It is often useful to capture the output of a process. If Core::Process is started with capture set to `true`, Thunder will duplicate the stdout/err file descriptors and make the available for reading/writing.

!!! warning
	Thunder does not do any kind of automatic buffering of stdout/err data if capture is enabled. This means if the process produces any output, the output will be stored in the default stdout buffer until it is read back out by your code. On most Linux systems this buffer size is 8K. So you must either:
	 

	* Guarantee the process will not fill the stdout buffer before exiting, then read data from the stdout file descriptor once it quits
	* Continuously drain the stdout buffer whilst the process is running. 

#### Simple Approach

This example shows launching the process, waiting for it to exit, then draining the stdout file descriptor to get the output from the process. This is only suitable since the output from the command is a known size and will not exceed the stdout buffer.

```c++ hl_lines="18-22"
Core::Process childProcess(true);

// Run the command "/usr/bin/echo hello"
Core::Process::Options options("/usr/bin/echo");
options.Add("hello");

uint32_t pid;
if (childProcess.Launch(options, &pid) != Core::ERROR_NONE) {
    printf("Failed to launch process\n");
} else {
    printf("Successfully launched process\n");

    // Process is now running, block forever until it exits
    if (childProcess.WaitProcessCompleted(Core::infinite) == Core::ERROR_NONE) {
        printf("Process exited with error code %d\n", childProcess.ExitCode());

        // Get the output from stdout
        std::string output;
        char buffer[2048];
        while (childProcess.Output(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer)) > 0) {
            output += buffer;
        }

        printf("Stdout from process: %s\n", output.c_str());

    } else {
        printf("Timed out waiting for process to complete\n");
    }
}

/* Output:
Successfully launched process
Process exited with error code 0
Stdout from process: hello
*/
```

#### Resource Monitor Approach

This example registers the application stdout file descriptor with the Thunder resource monitor. The resource monitor will observe the file descriptor and invoke a callback when there is data on the fd for processing. This is strongly recommended instead of creating your own thread for monitoring the output (this applies to any file descriptor/socket, not just this example).

For simplicity, the example will print the data sent to stdout along with the timestamp and PID that produced the message. The sample process executed simply prints a message to stdout and stderr at regular intervals until it's killed.

```c++
#include <core/ResourceMonitor.h>
#include <core/core.h>

using namespace Thunder;

/**
 * @brief A simple class that will monitor the stdout/err from a long-running process
 * and log it
 */
class ProcessOutputMonitor : public Core::IResource {
public:
    enum class OutputType {
        stdout,
        stderr
    };

    ProcessOutputMonitor(uint32_t pid, OutputType outputType, int fd)
        : _pid(pid)
        , _fd(fd)
    {
        switch (outputType) {
        case OutputType::stdout:
            _outputType = "STDOUT";
            break;
        case OutputType::stderr:
            _outputType = "STDERR";
        }
    }
    ~ProcessOutputMonitor() = default;

    ProcessOutputMonitor(ProcessOutputMonitor&&) = delete;
    ProcessOutputMonitor(const ProcessOutputMonitor&) = delete;
    ProcessOutputMonitor& operator=(ProcessOutputMonitor&&) = delete;
    ProcessOutputMonitor& operator=(const ProcessOutputMonitor&) = delete;

    Core::IResource::handle Descriptor() const override
    {
        return _fd;
    }

    uint16_t Events() override
    {
        // We are interested in receiving data from the process
        return (POLLIN);
    }

    void Handle(const uint16_t events) override
    {
        // Got some data from the process
        if ((events & POLLIN) != 0) {
            // Read data from the fd
            ssize_t ret;
            std::string output;
            char buffer[1024] = {};

            do {
                ret = read(_fd, buffer, sizeof(buffer));
                if (ret < 0 && errno != EWOULDBLOCK) {
                    printf("Error %d reading from process output\n", errno);
                } else if (ret > 0) {
                    output += buffer;
                }
            } while (ret > 0);

            // For the example, just print the received data
            printf("[PID: %d][%s][%s]: %s", _pid, _outputType.c_str(), Core::Time::Now().ToRFC1123().c_str(), output.c_str());
        }
    }

private:
    const uint32_t _pid;
    const int _fd;
    std::string _outputType;
};

int main(int argc, char const* argv[])
{
    Core::Process childProcess(true);

    // This sample app will print to stdout and error on regular intervals
    Core::Process::Options options("./sampleProcess");

    uint32_t pid;
    if (childProcess.Launch(options, &pid) != Core::ERROR_NONE) {
        printf("Failed to launch process\n");
    } else {
        printf("Successfully launched process\n");

        // Process is now running, monitor its stdout and stderr
        ProcessOutputMonitor stdoutMonitor(pid, ProcessOutputMonitor::OutputType::stdout, childProcess.Output());
        ProcessOutputMonitor stderrMonitor(pid, ProcessOutputMonitor::OutputType::stderr, childProcess.Error());

        Core::ResourceMonitor::Instance().Register(stdoutMonitor);
        Core::ResourceMonitor::Instance().Register(stderrMonitor);

        // Sleep for 10 seconds
        SleepS(10);

        // Kill the process
        childProcess.Kill(true);

        // Unregister the resource monitor
        Core::ResourceMonitor::Instance().Unregister(stdoutMonitor);
        Core::ResourceMonitor::Instance().Unregister(stderrMonitor);
    }

    Core::Singleton::Dispose();

    return 0;
}

/* Output:
Successfully launched process
[PID: 85574][STDOUT][Thu, 03 Aug 2023 11:49:08 GMT]: Hello from stdout
[PID: 85574][STDERR][Thu, 03 Aug 2023 11:49:08 GMT]: Hello from stderr
[PID: 85574][STDOUT][Thu, 03 Aug 2023 11:49:09 GMT]: Hello from stdout
[PID: 85574][STDERR][Thu, 03 Aug 2023 11:49:09 GMT]: Hello from stderr
[PID: 85574][STDOUT][Thu, 03 Aug 2023 11:49:10 GMT]: Hello from stdout
[PID: 85574][STDERR][Thu, 03 Aug 2023 11:49:10 GMT]: Hello from stderr
*/
```




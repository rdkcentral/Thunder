#define MODULE_NAME COMHacker

#include <core/core.h>
#include <com/com.h>

#include <random>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

constexpr char installPath[] = "/home/bram/Projects/metrological/Thunder/install/";

using namespace Thunder;

class Message : public Core::FrameType<UINT32_MAX, true, uint32_t> {
private:
    using BaseClass = Core::FrameType<UINT32_MAX, true, uint32_t>;

public:
    static constexpr uint8_t SIZE_OFFSET = 0;
    static constexpr uint8_t MESSAGE_ID_OFFSET = (SIZE_OFFSET + sizeof(uint8_t)); // 1
    static constexpr uint8_t IMPLEMENTATION_OFFSET = (MESSAGE_ID_OFFSET + sizeof(uint8_t)); // 2

    Message() = delete;

    Message(const uint16_t length, uint8_t data[])
    {
        Clear();
        SetBuffer(0, length, data);
    }

    Message(const uint8_t type)
        : BaseClass()
    {
        Clear();
        SetNumber(MESSAGE_ID_OFFSET, SetMessageType(operator[](MESSAGE_ID_OFFSET), type));
    }
    ~Message() = default;

    uint32_t Finish(int offset = 0)
    {
        operator[](SIZE_OFFSET) = (Size() - 1) + offset;
        return operator[](SIZE_OFFSET);
    }

private:
    constexpr uint8_t SetMessageType(const uint8_t field, const uint8_t type)
    {
        return (field & ~0xFE) | ((type << 1) & 0xFE);
    }
};

class BinaryConnector : public Core::SocketStream {
public:
    BinaryConnector() = delete;
    BinaryConnector(BinaryConnector&&) = delete;
    BinaryConnector(const BinaryConnector&) = delete;
    BinaryConnector& operator=(BinaryConnector&&) = delete;
    BinaryConnector& operator=(const BinaryConnector&) = delete;

    BinaryConnector(const Core::NodeId& remoteNode)
        : Core::SocketStream(false, remoteNode.AnyInterface(), remoteNode, 1024, 1024)
        , _adminLock()
        , _loaded(0)
        , _signal(false, true)
    {
    }
    ~BinaryConnector() override = default;

public:
    uint32_t WaitForResponse(const uint32_t waitTime)
    {
        return _signal.Lock(waitTime);
    }

    uint16_t Submit(const uint16_t length, const uint8_t buffer[])
    {
        msleep(3);
        _adminLock.Lock();
        uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _loaded), length);
        ::memmove(&(_buffer[_loaded]), buffer, result);
        bool initialDrop = (_loaded == 0);
        _loaded += result;
        _adminLock.Unlock();

        if (initialDrop == true) {
            Trigger();
        }

        return (result);
    }

private:
    // Methods to extract and insert data into the socket buffers
    uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
    {
        uint16_t result = 0;
        _adminLock.Lock();
        if (_loaded > 0) {
            result = std::min(maxSendSize, _loaded);
            ::memcpy(dataFrame, _buffer, result);
            if (result == _loaded) {
                _loaded = 0;
            } else {
                ::memmove(_buffer, &(_buffer[result]), (_loaded - result));
                _loaded -= result;
            }
            // Dump("Send", result, dataFrame);
        }
        _adminLock.Unlock();
        return (result);
    }
    uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
    {
        if (receivedSize == 0) {
            printf("Received is called without any data!\n");
        } else {
            // Dump("Received", receivedSize, dataFrame);
        }

        _signal.SetEvent();

        return (receivedSize);
    }

    // Signal a state change, Opened, Closed or Accepted
    void StateChange() override
    {
        _adminLock.Lock();

        printf("StateChange called. Open = [%s]\n", IsOpen() ? _T("true") : _T("false"));

        _adminLock.Unlock();
    }

private:
    int msleep(long msec)
    {
        struct timespec ts;
        int res;

        if (msec < 0) {
            errno = EINVAL;
            return -1;
        }

        ts.tv_sec = msec / 1000;
        ts.tv_nsec = (msec % 1000) * 1000000;

        do {
            res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);

        return res;
    }

    void Dump(const TCHAR prefix[], const uint16_t length, const uint8_t dataFrame[])
    {
        _adminLock.Lock();

        printf("%s [%d]:\n ", prefix, length);

        printf("\t0\t1\t2\t3\t4\t5\t6\t7");

        if (length > 1) {
            for (uint16_t index = 0; index < (length - 1); index++) {
                if ((index % 8 == 0))
                    printf("\n 0x%04X\t", index);
                printf("%02X\t", dataFrame[index]);
            }
        }
        printf("%02X\n", dataFrame[length - 1]);

        _adminLock.Unlock();
    }

private:
    Core::CriticalSection _adminLock;
    uint16_t _loaded;
    uint8_t _buffer[1024];
    Core::Event _signal;
};

class InvokeMessage : public Message {
    static constexpr uint8_t ID = 2;

public:
    static constexpr uint8_t INTERFACEID_OFFSET = (IMPLEMENTATION_OFFSET + sizeof(Core::instance_id));
    static constexpr uint8_t METHODEID_OFFSET = (INTERFACEID_OFFSET + sizeof(uint32_t));

public:
    InvokeMessage(/* args */)
        : Message(ID)
    {
    }

    ~InvokeMessage(){};

private:
};

class AnnounceMessage : public Message {
    static constexpr uint8_t ID = 1;

public:
    static constexpr uint8_t ID_OFFSET = (IMPLEMENTATION_OFFSET + sizeof(Core::instance_id));
    static constexpr uint8_t INTERFACEID_OFFSET = (ID_OFFSET + sizeof(uint32_t));
    static constexpr uint8_t EXCHANGEID_OFFSET = (INTERFACEID_OFFSET + sizeof(uint32_t));
    static constexpr uint8_t VERSIONID_OFFSET = (EXCHANGEID_OFFSET + sizeof(uint32_t));
    static constexpr uint8_t TYPE_OFFSET = (VERSIONID_OFFSET + sizeof(uint32_t));
    static constexpr uint8_t STRINGS_OFFSET = (TYPE_OFFSET + sizeof(uint8_t));

public:
    AnnounceMessage(/* args */)
        : Message(ID)
    {
    }

    ~AnnounceMessage(){};

private:
};

class ThunderWrapper : public Core::Thread {
public:
    struct ICallback {
        virtual ~ICallback() = default;
        virtual void Signal(const int signo) const = 0;
    };

    class KeyMonitor : public Core::IResource {
    public:
        typedef void (*onKeyPress)(const char key);

        KeyMonitor(int fd, onKeyPress callback)
            : _fd(dup(fd))
            , _callback(callback)
        {
            _origFlags = fcntl(_fd, F_GETFL, 0);
            fcntl(_fd, F_SETFL, (_origFlags | O_NONBLOCK | O_CLOEXEC));

            // get original cooked/canonical mode values
            tcgetattr(fd, &_origMode);
            // set options for raw mode
            struct termios raw = _origMode;

            raw.c_lflag &= ~(ICANON);

            tcsetattr(fd, TCSANOW, &raw);
        }
        ~KeyMonitor()
        {
            // restore original mode
            tcsetattr(_fd, TCSANOW, &_origMode);
            fcntl(_fd, F_SETFL, _origFlags);
            close(_fd);
        };

        KeyMonitor(KeyMonitor&&) = delete;
        KeyMonitor(const KeyMonitor&) = delete;
        KeyMonitor& operator=(KeyMonitor&&) = delete;
        KeyMonitor& operator=(const KeyMonitor&) = delete;

        Core::IResource::handle Descriptor() const override
        {
            return _fd;
        }

        uint16_t Events() override
        {
            // We are interested in receiving data from the process
            return (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
        }

        void Handle(const uint16_t events) override
        {
            // Got some data from the process
            if ((events & POLLIN) != 0) {
                // Read data from the fd
                char buffer;

                memset(&buffer, 0, sizeof(buffer));

                int n = read(Descriptor(), &buffer, sizeof(buffer));

                // For the example, just print the received data
                // printf("[%s]: %c\n", Core::Time::Now().ToRFC1123().c_str(), buffer);

                if ((n > 0) && (_callback != nullptr)) {
                    _callback(buffer);
                }
            }
        }

    private:
        const int _fd;
        int _origFlags;
        struct termios _origMode;
        onKeyPress _callback;
    };

    class ProcessOutputMonitor : public Core::IResource {
    public:
        ProcessOutputMonitor(uint32_t pid, int fd)
            : _pid(pid)
            , _fd(fd)
        {
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
                printf("[PID: %d][%s]: %s", _pid, Core::Time::Now().ToRFC1123().c_str(), output.c_str());
            }
        }

    private:
        const uint32_t _pid;
        const int _fd;
    };

    ThunderWrapper(const string& installPath, const ICallback& callback)
        : Core::Thread(Thread::DefaultStackSize(), "ThunderWrapper")
        , _thunder(true)
        , _options(installPath + string("/usr/bin/Thunder"))
        , _callback(callback)
    {
        FixLDLibraryPaths(installPath + string("/usr/lib"));

        _options.Add("-c").Add(installPath + string("/etc/Thunder/config.json"));

        struct sigaction sa;

        memset(&sa, 0, sizeof(struct sigaction));

        sa.sa_handler = &SignalHandler;

        if (sigaction(SIGINT, &sa, NULL) == -1) {
            printf("Failed to initialize signal handler...\n");
            abort();
        }

        Thread::Init();
    }

    ~ThunderWrapper() = default;

    void Start()
    {
        printf("Start Thunder session");
        return Thread::Run();
    }

    void Stop()
    {
        uint32_t writtenBytes = ::write(_thunder.Input(), "q\n", 2);

        _thunder.WaitProcessCompleted(5000);
        _thunder.Kill(true);

        Thread::Stop();

        printf("Stop Thunder session");

        Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
    }

    virtual uint32_t Worker() override
    {
        _thunder.Launch(_options, &_pid);

        ProcessOutputMonitor stdoutMonitor(_pid, _thunder.Output());
        ProcessOutputMonitor stderrMonitor(_pid, _thunder.Error());

        Core::ResourceMonitor::Instance().Register(stdoutMonitor);
        Core::ResourceMonitor::Instance().Register(stderrMonitor);

        _thunder.WaitProcessCompleted(Core::infinite);

        Core::ResourceMonitor::Instance().Unregister(stdoutMonitor);
        Core::ResourceMonitor::Instance().Unregister(stderrMonitor);

        return 0;
    }

    const uint32_t Pid() const
    {
        return _pid;
    }

    static volatile sig_atomic_t s_signal; // = 0;

private:
    static void SignalHandler(int /*signo*/)
    {
        s_signal = 1;
    }

    void FixLDLibraryPaths(const string& basePath) const
    {
        string newLDLibraryPaths;
        string oldLDLibraryPaths;

        Core::SystemInfo::GetEnvironment(_T("LD_LIBRARY_PATH"), oldLDLibraryPaths);

        // Read currently added LD_LIBRARY_PATH to prefix with _systemRootPath
        if (oldLDLibraryPaths.empty() != true) {
            size_t start = 0;
            size_t end = oldLDLibraryPaths.find(':');
            do {
                newLDLibraryPaths += basePath;
                newLDLibraryPaths += oldLDLibraryPaths.substr(start,
                    ((end != string::npos) ? (end - start + 1) : end));
                start = end;
                if (end != string::npos) {
                    start++;
                    end = oldLDLibraryPaths.find(':', start);
                }
            } while (start != string::npos);
        } else {
            newLDLibraryPaths = basePath;
        }

        Core::SystemInfo::SetEnvironment(_T("LD_LIBRARY_PATH"), newLDLibraryPaths, true);
        printf("Populated New LD_LIBRARY_PATH : %s\n", newLDLibraryPaths.c_str());
    }

private:
    Core::Process _thunder;
    Core::Process::Options _options;
    const ICallback& _callback;
    uint32_t _pid;
};

volatile sig_atomic_t ThunderWrapper::s_signal = 0;

class App : public BinaryConnector {
    class Callback : public ThunderWrapper::ICallback {
    public:
        Callback() = delete;

        Callback(const App& app)
            : _app(app)
        {
        }

        virtual ~Callback() = default;

        void Signal(const int signal) const override
        {
            _app.Signal(signal);
        }

    private:
        const App& _app;
    };

public:
    App(const Core::NodeId& remoteNode)
        : BinaryConnector(remoteNode)
        , _callback(*this)
        , _wrapper(installPath, _callback)
    {
    }

    ~App() = default;

    void Signal(const int signal) const
    {
        printf("Caught signal %d\n", signal);
    }

    void StartTUT()
    {
        Flush();
        _wrapper.Start();
    }

    void StopTUT()
    {
        Close(10);
        _wrapper.Stop();
        Flush();
    }

private:
    Callback _callback;
    ThunderWrapper _wrapper;
};

int main(int argc, char** argv)
{
    int result = 0;
    Core::NodeId destination;

    printf("COMHacker\n");

    if (argc > 2) {
        printf("COMHacker should be started as: COMHacker <server:port>");
        result = 1;
    } else if (argc == 2) {
        destination = Core::NodeId(argv[1]);
    } else {
        destination = Core::NodeId("127.0.0.1:62000");
    }

    std::random_device dev;
    std::mt19937 rng(dev());

    const uint32_t BlockSize = 1234;

    if (result == 0) {
        int character;

        BinaryConnector channel(destination);

        // App channel(destination);
        // channel.StartTUT();

        do {
            SleepMs(10);
            printf(">>");
            character = ::toupper(::getc(stdin));

            switch (character) {
            case 'O': {
                if (channel.IsOpen() == false) {
                    channel.Open(10);
                    printf("Opening channel.\n");
                }
                break;
            }
            case 'C': {
                if (channel.IsOpen() == true) {
                    printf("Closing channel.\n");
                    channel.Close(10);
                }
                break;
            }
            case 'I': {
                static const uint8_t message[] = { 0x01, 0x02, 0x03, 0x04, 0x05 };
                printf("Ingest data.\n");
                channel.Submit(sizeof(message), message);
                break;
            }
            case 'R': {

                static char quit;

                ThunderWrapper::KeyMonitor monitor(0, [](const char c) { quit = c; });

                Core::ResourceMonitor::Instance().Register(monitor);

                printf("Invoking random data... stop by pressing [ESC]\n");

                do {
                    InvokeMessage junkData;
                    std::uniform_int_distribution<std::mt19937::result_type> randomLength(27, BlockSize);
                    std::uniform_int_distribution<std::mt19937::result_type> randomByte(0, 255);

                    uint32_t MessageSize = randomLength(rng);

                    printf("Ingest junk data %d.\n", MessageSize);

                    for (uint32_t i = 2; i < MessageSize; i++) {
                        junkData.SetNumber<uint8_t>(i, randomByte(rng));
                    }

                    junkData.Finish();

                    channel.Submit(junkData.Size(), junkData.Data());
                    channel.WaitForResponse(100);
                } while (quit != 27);

                quit = '\0';

                Core::ResourceMonitor::Instance().Unregister(monitor);

                break;
            }
            case 'N': {
                // causes a Segfault
                InvokeMessage request;

                request.SetNumber<Core::instance_id>(InvokeMessage::IMPLEMENTATION_OFFSET, 0x03); //  invalid pointer access pointer
                request.SetNumber<uint32_t>(InvokeMessage::INTERFACEID_OFFSET, 0x00000030); // interfaceId
                request.SetNumber<uint8_t>(InvokeMessage::METHODEID_OFFSET + sizeof(uint32_t), 0x01); // methodId

                request.Finish();

                channel.Submit(request.Size(), request.Data());

                break;
            }
            case 'P': {
                // causes a Segfault
                InvokeMessage request;

                request.SetNumber<Core::instance_id>(InvokeMessage::IMPLEMENTATION_OFFSET, Core::instance_id(0x00005555557e73a1)); //  invalid pointer access pointer
                request.SetNumber<uint32_t>(InvokeMessage::INTERFACEID_OFFSET, 0x00000030); // interfaceId
                request.SetNumber<uint8_t>(InvokeMessage::METHODEID_OFFSET, 0x01); // methodId AddRef

                request.Finish();

                channel.Submit(request.Size(), request.Data());

                channel.WaitForResponse(100);

                break;
            }
            case 'A': {
                AnnounceMessage msg;

                msg.SetNumber<Core::instance_id>(AnnounceMessage::IMPLEMENTATION_OFFSET, Core::instance_id(0x1)); //  implementation
                msg.SetNumber<uint32_t>(AnnounceMessage::ID_OFFSET, uint32_t(0x00)); // interfaceId
                msg.SetNumber<uint32_t>(AnnounceMessage::EXCHANGEID_OFFSET, 1);
                msg.SetNumber<uint32_t>(AnnounceMessage::VERSIONID_OFFSET, 1);
                msg.SetNumber<uint8_t>(AnnounceMessage::TYPE_OFFSET, 1);

                const uint16_t classNameLength = msg.SetText(AnnounceMessage::STRINGS_OFFSET, "className");
                msg.SetText((AnnounceMessage::STRINGS_OFFSET + classNameLength), "callsign");

                for (uint8_t i = RPC::ID_OFFSET_INTERNAL + 1; i < RPC::ID_EXTERNAL_INTERFACE_OFFSET; i++) {
                    msg.SetNumber<uint32_t>(AnnounceMessage::INTERFACEID_OFFSET, i); // methodId
                    msg.Finish();
                    channel.Submit(msg.Size(), msg.Data());
                    if (channel.WaitForResponse(100) == Core::ERROR_NONE) {

                    } else {
                        printf("Invalid Interface ID: 0x%04X\n", i);
                    }
                }

                break;
            }
            default: {
                if (isalnum(character)) {
                    printf("Use the following keys for actions:\n");
                    printf("O)pen the channel\n");
                    printf("C)lose the channel\n");
                    printf("I)ngest the test string on the channel\n");
                    printf("R)andom junk data\n");

                    printf("Q)uit the application\n\n");
                    printf("The channel is directed towards: [%s]:[%d]\n", destination.HostName().c_str(), destination.PortNumber());
                }
                break;
            }
            }
        } while (character != 'Q');

        printf("Closing channel.\n");

        if (channel.IsOpen() == true) {
            channel.Close(10);
        }
        // channel.StopTUT();
    }

    return (result);
}
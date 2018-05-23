#include "SerialPort.h"

#include "Thread.h"
#include "Sync.h"
#include "Serialization.h"
#include "Singleton.h"

#include <errno.h>

#ifdef __POSIX__
#include <poll.h>
#include <fcntl.h>
#ifdef __APPLE__
#include "NodeId.h"
#define SOCKET signed int
#define SOCKET_ERROR static_cast<signed int>(-1)
#define INVALID_SOCKET static_cast<SOCKET>(-1)
#define __ERRORRESULT__               errno
#endif

#include <sys/ioctl.h>
#define ERRORRESULT errno
#define ERROR_WOULDBLOCK EWOULDBLOCK
#define ERROR_AGAIN EAGAIN
#endif

#ifdef __WIN32__
#define ERRORRESULT ::GetLastError()
#define ERROR_WOULDBLOCK ERROR_IO_PENDING
#define ERROR_AGAIN ERROR_IO_PENDING
#endif

namespace WPEFramework {
namespace Core {
    //////////////////////////////////////////////////////////////////////
    // SerialPort::SerialMonitor
    //////////////////////////////////////////////////////////////////////
    class SerialMonitor {
    private:
        class MonitorWorker : public Core::Thread {
        private:
            MonitorWorker(const MonitorWorker&);
            MonitorWorker& operator=(const MonitorWorker&);

        public:
            MonitorWorker(SerialMonitor& parent)
                : Core::Thread(Thread::DefaultStackSize(), _T("SerialPortMonitor"))
                , _parent(parent)
            {
                Thread::Init();

            }
            virtual ~MonitorWorker()
            {
                Wait(Thread::BLOCKED | Thread::STOPPED | Thread::INITIALIZED, Core::infinite);
            }

        public:
#ifdef __LINUX__
            virtual bool Initialize()
            {
                return ((Thread::Initialize() == true) && (_parent.Initialize() == true));
            }
#endif

            virtual uint32_t Worker()
            {
                return (_parent.Worker());
            }

        private:
            SerialMonitor& _parent;
        };

        static const uint8_t SLOT_ALLOCATION = 8;

        SerialMonitor(const SerialMonitor&);
        SerialMonitor& operator=(const SerialMonitor&);

    public:
        SerialMonitor()
            : m_ThreadInstance(nullptr)
            , m_Admin()
            , m_MonitoredPorts()
            , m_MaxSlots(SLOT_ALLOCATION)
            ,
#ifdef __WIN32__
            m_Slots(static_cast<HANDLE*>(::malloc((sizeof(HANDLE) * 2 * m_MaxSlots) + sizeof(HANDLE))))
            , m_Break(CreateEvent(nullptr, TRUE, FALSE, nullptr))
#endif
#ifdef __LINUX__
                  m_Slots(static_cast<struct pollfd*>(::malloc(sizeof(struct pollfd) * (m_MaxSlots + 1))))
            , m_SignalFD(-1)
#endif
        {
#ifdef __WIN32__
            m_Slots[0] = m_Break;
#endif
        }
        virtual ~SerialMonitor()
        {
            TRACE_L1("SerialPortMonitor: Closing [%d] ports", static_cast<uint32_t>(m_MonitoredPorts.size()));

            // Unregister all serial ports (Close the port !!!) before closing the App !!!!
            ASSERT(m_MonitoredPorts.size() == 0);

            if (m_ThreadInstance != nullptr) {
                m_Admin.Lock();

                m_MonitoredPorts.clear();

                m_ThreadInstance->Block();

                Break();

                m_Admin.Unlock();

                delete m_ThreadInstance;
            }

            ::free(m_Slots);
#ifdef __LINUX__
            if (m_SignalFD != -1) {
                ::close(m_SignalFD);
            }
#endif
#ifdef __WIN32__
            ::CloseHandle(m_Break);
#endif
        }

    public:
        void Register(SerialPort& port)
        {
            m_Admin.Lock();

// Make sure this entry does not exist, only register sockets once !!!
#ifdef __DEBUG__
            std::list<SerialPort*>::const_iterator index = m_MonitoredPorts.begin();

            while ((index != m_MonitoredPorts.end()) && (*index != &port)) {
                index++;
            }
            ASSERT(index == m_MonitoredPorts.end());
#endif

            m_MonitoredPorts.push_back(&port);

#ifdef __WIN32__
            // Start waiting for characters to come in...
            port.Read(0);
#endif

            if (m_MonitoredPorts.size() == 1) {
                if (m_ThreadInstance == nullptr) {
                    m_ThreadInstance = new MonitorWorker(*this);
                }
                m_ThreadInstance->Run();
            }
            else {
                Break();
            }

            m_Admin.Unlock();
        }
        void Unregister(SerialPort& port)
        {
            m_Admin.Lock();

            std::list<SerialPort*>::iterator index = m_MonitoredPorts.begin();

            while ((index != m_MonitoredPorts.end()) && (*index != &port)) {
                index++;
            }

            // Do not try to unregister something that is not registered.
            if (index != m_MonitoredPorts.end()) {
                (*index) = nullptr;
            }

#ifdef __WIN32__
            CancelIo(port.Descriptor());
#endif

            if (m_MonitoredPorts.size() == 0) {
                m_ThreadInstance->Block();
                Break();
            }
            else {
                Break();
            }

            m_Admin.Unlock();
        }

        inline void Break()
        {
#ifdef __APPLE__
            int data = 0;
            ::sendto(m_SignalFD,
                     &data,
                     sizeof(data), 0,
                     m_signalNode,
                     m_signalNode.Size());
#elif defined(__LINUX__)
            ASSERT(m_ThreadInstance != nullptr);

            m_ThreadInstance->Signal(SIGUSR2);
#endif

#ifdef __WIN32__
            ::SetEvent(m_Break);
#endif
        }

    private:
#ifdef __LINUX__
        bool Initialize() {
            int err;

#ifdef __APPLE__
            char filename[] = "/tmp/WPE-communication.XXXXXX";
            char *file = mktemp(filename);

            m_SignalFD = INVALID_SOCKET;

            if ((m_SignalFD = ::socket(AF_UNIX, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
                TRACE_L1("Error on creating socket SOCKET. Error %d", __ERRORRESULT__);
            } else {
                int flags = fcntl(m_SignalFD, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(m_SignalFD, F_SETFL, flags) != 0) {
                    ASSERT(false && "failed to make socket nonblocking ");
                } else {
                    // Do we need to find something to bind to or is it pre-destined
                    m_signalNode = Core::NodeId(file);
                    if (::bind(m_SignalFD, m_signalNode, m_signalNode.Size()) == SOCKET_ERROR) {
                        m_SignalFD = INVALID_SOCKET;
                        ASSERT(false && "failed to bind");
                    }
                }
            }

            err = 0;
#else
            sigset_t sigset;

        /* Create a sigset of all the signals that we're interested in */
        err = sigemptyset(&sigset);
        ASSERT (err == 0);
        err = sigaddset(&sigset, SIGUSR2);
        ASSERT (err == 0);

        /* We must block the signals in order for signalfd to receive them */
        err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
        assert(err == 0);

        /* Create the signalfd */
        m_SignalFD = signalfd(-1, &sigset, 0);

#endif
            ASSERT(m_SignalFD != -1);

            m_Slots[0].fd = m_SignalFD;
            m_Slots[0].events = POLLIN;
            m_Slots[0].revents = 0;

            return (err == 0);
        }
#endif
#ifdef __LINUX__
        uint32_t Worker()
        {
            uint32_t delay = 0;

            // Add entries not in the Array before we start !!!
            m_Admin.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if ((m_MonitoredPorts.size() + 1) > m_MaxSlots) {
                m_MaxSlots = ((((m_MonitoredPorts.size() + 1) / SLOT_ALLOCATION) + 1) * SLOT_ALLOCATION);

                ::free(m_Slots);

                // Resize the array to fit..
                m_Slots = static_cast<struct pollfd*>(::malloc(sizeof(struct pollfd) * m_MaxSlots));

                m_Slots[0].fd = m_SignalFD;
                m_Slots[0].events = POLLIN;
                m_Slots[0].revents = 0;
            }

            int filledFileDescriptors = 1;
            std::list<SerialPort*>::iterator index = m_MonitoredPorts.begin();

            // Fill in all entries required/updated..
            while (index != m_MonitoredPorts.end()) {
                SerialPort* port = (*index);

                if (port == nullptr) {
                    index = m_MonitoredPorts.erase(index);

                    if (m_MonitoredPorts.size() == 0) {
                        m_ThreadInstance->Block();
                        delay = Core::infinite;
                    }
                }
                else {
                    m_Slots[filledFileDescriptors].fd = port->m_Descriptor;
                    m_Slots[filledFileDescriptors].events = POLLIN | ((*index)->m_State & SerialPort::WRITE);
                    m_Slots[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    index++;
                }
            }

            if (filledFileDescriptors > 1) {
                m_Admin.Unlock();

                int result = poll(m_Slots, filledFileDescriptors, -1);

                m_Admin.Lock();

                if (result == -1) {
                    TRACE_L1("poll failed with error <%d>", ERRORRESULT);
                }
                else if (m_Slots[0].revents & POLLIN) {
                    /* We have a valid signal, read the info from the fd */

#ifdef __APPLE__
                    int info;
#else
                    struct signalfd_siginfo info;
#endif
                    uint32_t VARIABLE_IS_NOT_USED bytes = read(m_SignalFD, &info, sizeof(info));

                    ASSERT(bytes == sizeof(info));

                    // Clear the signal port..
                    m_Slots[0].revents = 0;
                }
            }
            else {
                m_ThreadInstance->Block();
                delay = Core::infinite;
            }

            // We are only interested in the filedescriptors that have a corresponding client.
            // We also know that once a file descriptor is not found, we handled them all...
            int fd_index = 1;
            index = m_MonitoredPorts.begin();

            while ((index != m_MonitoredPorts.end()) && (fd_index < filledFileDescriptors)) {
                SerialPort* port = *index;

                if (port == nullptr) {
                    index = m_MonitoredPorts.erase(index);

                    if (m_MonitoredPorts.size() == 0) {
                        m_ThreadInstance->Block();
                        delay = Core::infinite;
                    }
                }
                else {
                    int fd = port->Descriptor();

                    // Find the current file descriptor in our array..
                    while ((fd_index < filledFileDescriptors) && (m_Slots[fd_index].fd != fd)) {
                        fd_index++;
                    }

                    if (fd_index < filledFileDescriptors) {
                        uint16_t result = m_Slots[fd_index].revents;

                        if (result != 0) {
                            if (port->IsOpen() == true) {
                                if ((result & POLLOUT) != 0) {
                                    port->Write();
                                }
                                if ((result & POLLIN) != 0) {
                                    port->Read();
                                }
                            }
                            else {
                                port->Close(0);
                            }
                        }

                        if ((*index) == nullptr) {
                            index = m_MonitoredPorts.erase(index);

                            if (m_MonitoredPorts.size() == 0) {
                                m_ThreadInstance->Block();
                                delay = Core::infinite;
                            }
                        }
                        else {
                            // We could have been triggered to write new data, if so, do so :-)
                            if ((port->m_State & SerialPort::WRITESLOT) != 0) {
                                port->Write();
                            }

                            index++;
                        }
                    }
                }

                fd_index++;
            }

            m_Admin.Unlock();

            return (delay);
        }
#endif

#ifdef __WIN32__
        uint32_t Worker()
        {
            // Add entries not in the Array before we start !!!
            m_Admin.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if ((m_MonitoredPorts.size() + 1) >= m_MaxSlots) {
                m_MaxSlots = ((((static_cast<uint32_t>(m_MonitoredPorts.size()) + 1) / SLOT_ALLOCATION) + 1) * SLOT_ALLOCATION);

                ::free(m_Slots);

                // Resize the array to fit..
                m_Slots = static_cast<HANDLE*>(::malloc((sizeof(HANDLE) * 2 * m_MaxSlots) + sizeof(HANDLE)));

                m_Slots[0] = m_Break;
            }

            int filledSlot = 1;
            std::list<SerialPort*>::iterator index = m_MonitoredPorts.begin();

            // Fill in all entries required/updated..
            while (index != m_MonitoredPorts.end()) {
                SerialPort* port = (*index);

                if (port == nullptr) {
                    index = m_MonitoredPorts.erase(index);

                    if (m_MonitoredPorts.size() == 0) {
                        m_ThreadInstance->Block();
                    }
                }
                else {
                    m_Slots[filledSlot++] = (*index)->m_ReadInfo.hEvent;
                    m_Slots[filledSlot++] = (*index)->m_WriteInfo.hEvent;
                    index++;
                }
            }

            if (filledSlot > 1) {
                m_Admin.Unlock();

                ::WaitForMultipleObjects(filledSlot, m_Slots, FALSE, Core::infinite);

                m_Admin.Lock();

                ::ResetEvent(m_Slots[0]);
            }
            else {
                m_ThreadInstance->Block();
            }

            // We are only interested in events that were fired..
            uint8_t count = 1;
            index = m_MonitoredPorts.begin();

            while (index != m_MonitoredPorts.end()) {
                DWORD info;
                SerialPort* port = (*index);

                if (port == nullptr) {
                    index = m_MonitoredPorts.erase(index);

                    if (m_MonitoredPorts.size() == 0) {
                        m_ThreadInstance->Block();
                    }
                }
                else {
                    if ((::WaitForSingleObject(port->m_ReadInfo.hEvent, 0) == WAIT_OBJECT_0) && (::GetOverlappedResult(port->Descriptor(), &(port->m_ReadInfo), &info, FALSE))) {
                        ::ResetEvent(port->m_ReadInfo.hEvent);

                        ASSERT((info == 0) || (info == 1));

                        port->Read(static_cast<uint16_t>(info));
                    }

                    // We could have been triggered to write new data, if so, do so :-)
                    if ((port->m_State & SerialPort::WRITESLOT) != 0) {
                        ::SetEvent(port->m_WriteInfo.hEvent);
                    }

                    if ((::WaitForSingleObject(port->m_WriteInfo.hEvent, 0) == WAIT_OBJECT_0) && (::GetOverlappedResult(port->Descriptor(), &(port->m_WriteInfo), &info, FALSE))) {
                        ::ResetEvent(port->m_WriteInfo.hEvent);

                        port->Write(static_cast<uint16_t>(info));
                    }

                    if ((*index) == nullptr) {
                        index = m_MonitoredPorts.erase(index);

                        if (m_MonitoredPorts.size() == 0) {
                            m_ThreadInstance->Block();
                        }
                    }
                    else {
                        index++;
                    }
                }
            }

            m_Admin.Unlock();

            return (m_MonitoredPorts.size() == 0 ? Core::infinite : 0);
        }
#endif

    private:
        MonitorWorker* m_ThreadInstance;
        Core::CriticalSection m_Admin;
        std::list<SerialPort*> m_MonitoredPorts;
        uint32_t m_MaxSlots;

#ifdef __LINUX__
        struct pollfd* m_Slots;
        int m_SignalFD;
#endif

#ifdef __WIN32__
        HANDLE* m_Slots;
        HANDLE m_Break;
#endif
#ifdef __APPLE__
        Core::NodeId m_signalNode;
#endif
    };

    static SerialMonitor& g_SerialPortMonitor = SingletonType<SerialMonitor>::Instance();

    //////////////////////////////////////////////////////////////////////
    // SerialPort::Trigger
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // SerialPort
    //////////////////////////////////////////////////////////////////////

    SerialPort::SerialPort()
        : m_syncAdmin()
        , m_PortName()
        , m_State(0)
        , m_SendBufferSize(0)
        , m_ReceiveBufferSize(0)
        , m_SendBuffer(nullptr)
        , m_ReceiveBuffer(nullptr)
        , m_ReadBytes(0)
        , m_SendOffset(0)
        , m_SendBytes(0)
        ,

#ifdef __WIN32__
        m_Descriptor(INVALID_HANDLE_VALUE)
#endif

#ifdef __LINUX__
            m_Descriptor(-1)
#endif
    {
    }
    SerialPort::SerialPort(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const uint16_t sendBufferSize,
        const uint16_t receiveBufferSize)
        : m_syncAdmin()
        , m_PortName(port)
        , m_State(0)
        , m_SendBufferSize(0)
        , m_ReceiveBufferSize(0)
        , m_SendBuffer(nullptr)
        , m_ReceiveBuffer(nullptr)
        , m_ReadBytes(0)
        , m_SendOffset(0)
        , m_SendBytes(0)
        ,

#ifdef __WIN32__
        m_Descriptor(INVALID_HANDLE_VALUE)
#endif

#ifdef __LINUX__
            m_Descriptor(-1)
#endif
    {
        Configuration(port, baudRate, parity, dataBits, stopBits, sendBufferSize, receiveBufferSize);
    }

    /* virtual */ SerialPort::~SerialPort()
    {
// Make sure the socket is closed before you destruct. Otherwise
// the virtuals might be called, which are destructed at this point !!!!
#ifdef __LINUX__
        ASSERT(m_Descriptor == -1);
#endif
#ifdef __WIN32__
        ASSERT(m_Descriptor == INVALID_HANDLE_VALUE);
#endif

        ::free(m_SendBuffer);

#ifdef __WIN32__
        ::CloseHandle(m_ReadInfo.hEvent);
        ::CloseHandle(m_WriteInfo.hEvent);
#endif
    }

#ifdef __LINUX__
    void SerialPort::BufferAlignment(int descriptor VARIABLE_IS_NOT_USED)
#endif
#ifdef __WIN32__
        void SerialPort::BufferAlignment(HANDLE descriptor)
#endif
    {
        uint32_t receiveBuffer = m_ReceiveBufferSize;
        uint32_t sendBuffer = m_SendBufferSize;

        if ((receiveBuffer != 0) || (sendBuffer != 0)) {
            uint8_t* allocatedMemory = static_cast<uint8_t*>(::malloc(m_SendBufferSize + receiveBuffer));
            if (sendBuffer != 0) {
                m_SendBuffer = allocatedMemory;
            }
            if (receiveBuffer != 0) {
                m_ReceiveBuffer = &(allocatedMemory[sendBuffer]);
            }
        }
    }

    bool SerialPort::Configuration(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const uint16_t sendBufferSize,
        const uint16_t receiveBufferSize)
    {
#ifdef __LINUX__
        if (m_Descriptor == -1)
#endif
#ifdef __WIN32__
            if (m_Descriptor == INVALID_HANDLE_VALUE)
#endif
            {
                m_PortName = port;
                m_SendBufferSize = sendBufferSize;
                m_ReceiveBufferSize = receiveBufferSize;

#ifdef __LINUX__
                cfsetispeed(&m_PortSettings, baudRate); // set baud rates for in
                cfsetospeed(&m_PortSettings, baudRate); // and out

                m_PortSettings.c_cflag &= ~(PARENB | PARODD | CSTOPB | CS5 | CS6 | CS7 | CS8); // Clear all relevant bits
                m_PortSettings.c_cflag |= parity | stopBits | dataBits; // Set the requested bits
#endif
#ifdef __WIN32__
                m_PortSettings.DCBlength = sizeof(DCB);
                m_PortSettings.BaudRate = baudRate;
                m_PortSettings.ByteSize = dataBits;
                m_PortSettings.Parity = parity;
                m_PortSettings.StopBits = stopBits;
                ::memset(&m_ReadInfo, 0, sizeof(OVERLAPPED));
                ::memset(&m_WriteInfo, 0, sizeof(OVERLAPPED));
                m_ReadInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
                m_WriteInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif

                ASSERT((m_SendBufferSize != 0) || (m_ReceiveBufferSize != 0));

                if (m_SendBuffer != nullptr) {
                    ::free(m_SendBuffer);
                }

                uint8_t* allocatedMemory = static_cast<uint8_t*>(::malloc(m_SendBufferSize + m_ReceiveBufferSize));
                if (m_SendBufferSize != 0) {
                    m_SendBuffer = allocatedMemory;
                }
                if (m_ReceiveBufferSize != 0) {
                    m_ReceiveBuffer = &(allocatedMemory[m_SendBufferSize]);
                }

                return (true);
            }
        return (false);
    }

    uint32_t SerialPort::Open(uint32_t /* waitTime */)
    {
        uint32_t result = 0;

        m_syncAdmin.Lock();

#ifdef __POSIX__
        if (m_Descriptor == -1) {
            std::string convertedPortName;
            Core::ToString(m_PortName.c_str(), convertedPortName);

            m_Descriptor = open(convertedPortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);

            result = errno;

            if (m_Descriptor != -1) {
                int flags = fcntl(m_Descriptor, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(m_Descriptor, F_SETFL, flags) != 0) {
                    TRACE_L4("Error on port socket F_SETFL call. Error %d", ERRORRESULT);

                    result = errno;

                    close(m_Descriptor);

                    m_Descriptor = -1;
                }
                else {
                    tcsetattr(m_Descriptor, TCSANOW, &m_PortSettings);

                    BufferAlignment(m_Descriptor);

                    g_SerialPortMonitor.Register(*this);

                    m_State = SerialPort::OPEN;
                    StateChange();

                    result = OK;
                }
            }
        }
#endif

#ifdef __WIN32__
        if (m_Descriptor == INVALID_HANDLE_VALUE) {
            m_Descriptor = ::CreateFile(m_PortName.c_str(),
                GENERIC_READ | GENERIC_WRITE, //access ( read and write)
                0, //(share) 0:cannot share the COM port
                0, //security  (None)
                OPEN_EXISTING, // creation : open_existing
                FILE_FLAG_OVERLAPPED, // we want overlapped operation
                0 // no templates file for COM port...
                );

            result = GetLastError();

            if (m_Descriptor != INVALID_HANDLE_VALUE) {
                DCB currentSettings;

                if (::GetCommState(m_Descriptor, &currentSettings) == false) {
                    result = GetLastError();
                }
                else {
                    currentSettings.BaudRate = m_PortSettings.BaudRate;
                    currentSettings.Parity = m_PortSettings.Parity;
                    currentSettings.StopBits = m_PortSettings.StopBits;
                    currentSettings.ByteSize = m_PortSettings.ByteSize;

                    if (!::SetCommState(m_Descriptor, &currentSettings)) {
                        result = GetLastError();
                    }
                    else {
                        BufferAlignment(m_Descriptor);

                        m_ReadBytes = 0;
                        m_State = SerialPort::OPEN;
                        g_SerialPortMonitor.Register(*this);

                        StateChange();

                        result = OK;
                    }
                }
            }
        }
#endif

        m_syncAdmin.Unlock();

        return (result);
    }

    uint32_t SerialPort::Close(uint32_t /* waitTime */)
    {
        m_syncAdmin.Lock();

#ifdef __LINUX__

        if (m_Descriptor != -1) {
            // Before we delete the descriptor, get ride of the Trigger
            // subscribtion.
            g_SerialPortMonitor.Unregister(*this);

            close(m_Descriptor);

            m_Descriptor = -1;

            m_State = 0;
            StateChange();
        }
#endif

#ifdef __WIN32__
        if (m_Descriptor != INVALID_HANDLE_VALUE) {
            g_SerialPortMonitor.Unregister(*this);

            ::CloseHandle(m_Descriptor);

            m_Descriptor = INVALID_HANDLE_VALUE;

            m_State = 0;
            StateChange();
        }
#endif

        m_syncAdmin.Unlock();

        return (OK);
    }

    void SerialPort::Trigger()
    {
        m_syncAdmin.Lock();

        if ((m_State & (SerialPort::OPEN | SerialPort::EXCEPTION | SerialPort::WRITESLOT)) == SerialPort::OPEN) {
 		m_State |= WRITESLOT;
        	g_SerialPortMonitor.Break();
        }

        m_syncAdmin.Unlock();
    }

#ifdef __WIN32__
    void SerialPort::Write(const uint16_t writtenBytes)
    {
        m_syncAdmin.Lock();

        m_State &= (~(SerialPort::WRITE|SerialPort::WRITESLOT));

        ASSERT(((m_SendBytes == 0) && (m_SendOffset == 0)) || ((writtenBytes + m_SendOffset) <= m_SendBytes));

        if (m_SendBytes != 0) {
            m_SendOffset += writtenBytes;
        }

        do {
            if (m_SendOffset == m_SendBytes) {
                m_SendBytes = SendData(m_SendBuffer, m_SendBufferSize);
                m_SendOffset = 0;
            }

            if (m_SendOffset < m_SendBytes) {
                DWORD sendSize;

                // Sockets are non blocking the Send buffer size is equal to the buffer size. We only send
                // if the buffer free (SEND flag) is active, so the buffer should always fit.
                if (::WriteFile(m_Descriptor,
                        &m_SendBuffer[m_SendOffset],
                        m_SendBytes - m_SendOffset,
                        &sendSize, &m_WriteInfo)
                    != FALSE) {
                    m_SendOffset += static_cast<uint16_t>(sendSize);
                }
                else {
                    uint32_t l_Result = ERRORRESULT;

                    m_State |= (SerialPort::WRITE);

                    if ((l_Result != ERROR_WOULDBLOCK) && (l_Result != ERROR_AGAIN)) {
                        m_State |= SerialPort::EXCEPTION;

                        StateChange();
                    }
                }
            }
        } while (((m_State & (SerialPort::WRITE | SerialPort::EXCEPTION)) == 0) && (m_SendBytes != 0));

        m_syncAdmin.Unlock();
    }

    void SerialPort::Read(const uint16_t readBytes)
    {
        m_syncAdmin.Lock();

        ASSERT((readBytes == 1) || (readBytes == 0));

        if (readBytes == 1) {
            m_ReceiveBuffer[m_ReadBytes++] = m_CharBuffer;
        }

        m_State &= (~SerialPort::READ);

        while ((m_State & (SerialPort::READ | SerialPort::EXCEPTION | SerialPort::OPEN)) == SerialPort::OPEN) {
            DWORD readBytes;
            uint32_t l_Size = static_cast<uint32_t>(~0);

            ASSERT(m_ReadBytes <= m_ReceiveBufferSize);

            if (m_ReadBytes == m_ReceiveBufferSize) {
                m_ReadBytes = 0;
            }

            // Read the actual data from the port.
            if (ReadFile(m_Descriptor, &m_CharBuffer, 1, &readBytes, &m_ReadInfo) == 0) {
                uint32_t reason = ERRORRESULT;

                m_State |= SerialPort::READ;

                if ((reason != ERROR_WOULDBLOCK) && (reason != ERROR_INPROGRESS)) {
                    m_State |= SerialPort::EXCEPTION;
                    StateChange();
                }
            }
            else if (readBytes == 0) {
                // nothing to read wait on the next trigger..
                m_State |= SerialPort::READ;
            }
            else {
                ASSERT(readBytes == 1);
                m_ReceiveBuffer[m_ReadBytes] = m_CharBuffer;
                m_ReadBytes++;
            }

            if (m_ReadBytes != 0) {
                uint16_t handledBytes = ReceiveData(m_ReceiveBuffer, m_ReadBytes);

                ASSERT(m_ReadBytes >= handledBytes);

                m_ReadBytes -= handledBytes;

                if ((m_ReadBytes != 0) && (handledBytes != 0)) {
                    // Oops not all data was consumed, Lets remove the read data
                    ::memcpy(m_ReceiveBuffer, &m_ReceiveBuffer[handledBytes], m_ReadBytes);
                }
            }
        }

        m_syncAdmin.Unlock();
    }
#endif

#ifdef __POSIX__
    void SerialPort::Write()
    {
        m_syncAdmin.Lock();

        m_State &= (~(SerialPort::WRITE|SerialPort::WRITESLOT));

        do {
            if (m_SendOffset == m_SendBytes) {
                m_SendBytes = SendData(m_SendBuffer, m_SendBufferSize);
                m_SendOffset = 0;
            }

            if (m_SendOffset < m_SendBytes) {
                uint32_t sendSize;

                sendSize = write(m_Descriptor, reinterpret_cast<const char*>(&m_SendBuffer[m_SendOffset]),
                    m_SendBytes - m_SendOffset);

                if (sendSize != static_cast<uint32_t>(-1)) {
                    m_SendOffset += sendSize;
                }
                else {
                    uint32_t l_Result = ERRORRESULT;

                    if ((l_Result == ERROR_WOULDBLOCK) || (l_Result == ERROR_AGAIN)) {
                        m_State |= (SerialPort::WRITE);
                    }
                    else {
                        m_State |= SerialPort::EXCEPTION;

                        StateChange();
                    }
                }
            }
        } while (((m_State & (SerialPort::WRITE | SerialPort::EXCEPTION)) == 0) && (m_SendBytes != 0));

        m_syncAdmin.Unlock();
    }

    void SerialPort::Read()
    {
        m_syncAdmin.Lock();

        m_State &= (~SerialPort::READ);

        while ((m_State & (SerialPort::READ | SerialPort::EXCEPTION | SerialPort::OPEN)) == SerialPort::OPEN) {
            uint32_t l_Size;

            ASSERT(m_ReadBytes <= m_ReceiveBufferSize);

            if (m_ReadBytes == m_ReceiveBufferSize) {
                m_ReadBytes = 0;
            }

            // Read the actual data from the port.
            l_Size = ::read(m_Descriptor, reinterpret_cast<char*>(&m_ReceiveBuffer[m_ReadBytes]), m_ReceiveBufferSize - m_ReadBytes);

            if ((l_Size != static_cast<uint32_t>(~0)) && (l_Size != 0)) {
                m_ReadBytes += l_Size;

                if (m_ReadBytes != 0) {
                    uint16_t handledBytes = ReceiveData(m_ReceiveBuffer, m_ReadBytes);

                    ASSERT(m_ReadBytes >= handledBytes);

                    m_ReadBytes -= handledBytes;

                    if ((m_ReadBytes != 0) && (handledBytes != 0)) {
                        // Oops not all data was consumed, Lets remove the read data
                        ::memcpy(m_ReceiveBuffer, &m_ReceiveBuffer[handledBytes], m_ReadBytes);
                    }
                }
            }
            else {
                uint32_t l_Result = ERRORRESULT;

                m_State |= SerialPort::READ;

                if ((l_Result != ERROR_WOULDBLOCK) && (l_Result != ERROR_INPROGRESS) && (l_Result != 0)) {
                    m_State |= SerialPort::EXCEPTION;
                    StateChange();
                }
            }
        }

        m_syncAdmin.Unlock();
    }
#endif
}
} // namespace Solution::Core

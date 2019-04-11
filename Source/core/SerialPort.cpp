#include "SerialPort.h"

#include "Serialization.h"
#include "Singleton.h"
#include "Sync.h"
#include "Thread.h"

#include <errno.h>

#ifdef __POSIX__
#include <fcntl.h>
#include <poll.h>
#ifdef __APPLE__
#include "NodeId.h"
#define SOCKET signed int
#define SOCKET_ERROR static_cast<signed int>(-1)
#define INVALID_SOCKET static_cast<SOCKET>(-1)
#define __ERRORRESULT__ errno
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

    static constexpr uint32_t SLEEPSLOT_TIME = 100;

//////////////////////////////////////////////////////////////////////
// SerialPort::SerialMonitor
//////////////////////////////////////////////////////////////////////
#ifdef __WIN32__
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
            , m_Slots(static_cast<HANDLE*>(::malloc((sizeof(HANDLE) * 2 * m_MaxSlots) + sizeof(HANDLE))))
            , m_Break(CreateEvent(nullptr, TRUE, FALSE, nullptr))
        {
            m_Slots[0] = m_Break;
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
            ::CloseHandle(m_Break);
        }

    public:
        void Monitor(SerialPort& port)
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

            // Start waiting for characters to come in...
            port.Read(0);

            Break();

            if (m_MonitoredPorts.size() == 1) {
                if (m_ThreadInstance == nullptr) {
                    m_ThreadInstance = new MonitorWorker(*this);
                }
                m_ThreadInstance->Run();
            }

            m_Admin.Unlock();
        }
        inline void Break()
        {
            ::SetEvent(m_Break);
        }

    private:
        uint32_t Worker()
        {
            uint32_t delay = 0;

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

                if (port->IsOpen() == false) {
                    index = m_MonitoredPorts.erase(index);
                    port->Closed();
                } else {
                    if (port->IsOpen() == true) {
                        port->Opened();
                    }
                    m_Slots[filledSlot++] = (*index)->m_ReadInfo.hEvent;
                    m_Slots[filledSlot++] = (*index)->m_WriteInfo.hEvent;
                    index++;
                }
            }

            if (filledSlot <= 1) {
                m_ThreadInstance->Block();
                delay = Core::infinite;
            } else {
                m_Admin.Unlock();

                ::WaitForMultipleObjects(filledSlot, m_Slots, FALSE, Core::infinite);

                m_Admin.Lock();

                ::ResetEvent(m_Slots[0]);

                // We are only interested in events that were fired..
                index = m_MonitoredPorts.begin();

                while (index != m_MonitoredPorts.end()) {
                    DWORD info;
                    SerialPort* port = (*index);

                    if (port != nullptr) {
                        if ((::WaitForSingleObject(port->m_ReadInfo.hEvent, 0) == WAIT_OBJECT_0) && (::GetOverlappedResult(port->Descriptor(), &(port->m_ReadInfo), &info, FALSE))) {
                            ::ResetEvent(port->m_ReadInfo.hEvent);

                            ASSERT((info == 0) || (info == 1));

                            port->Read(static_cast<uint16_t>(info));
                        }

                        if ((::WaitForSingleObject(port->m_WriteInfo.hEvent, 0) == WAIT_OBJECT_0) && (::GetOverlappedResult(port->Descriptor(), &(port->m_WriteInfo), &info, FALSE))) {
                            ::ResetEvent(port->m_WriteInfo.hEvent);
                            port->Write(static_cast<uint16_t>(info));
                        }
                    }

                    index++;
                }
            }

            m_Admin.Unlock();

            return (delay);
        }

    private:
        MonitorWorker* m_ThreadInstance;
        Core::CriticalSection m_Admin;
        std::list<SerialPort*> m_MonitoredPorts;
        uint32_t m_MaxSlots;
        HANDLE* m_Slots;
        HANDLE m_Break;
    };

    static SerialMonitor& g_SerialPortMonitor = SingletonType<SerialMonitor>::Instance();
#endif

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
#ifdef __WIN32__
        ::memset(&m_ReadInfo, 0, sizeof(OVERLAPPED));
        ::memset(&m_WriteInfo, 0, sizeof(OVERLAPPED));
        m_ReadInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        m_WriteInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif
    }
    SerialPort::SerialPort(const string& port)
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
#ifdef __WIN32__
        ::memset(&m_ReadInfo, 0, sizeof(OVERLAPPED));
        ::memset(&m_WriteInfo, 0, sizeof(OVERLAPPED));
        m_ReadInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        m_WriteInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif
    }
    SerialPort::SerialPort(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const FlowControl flowControl,
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
#ifdef __WIN32__
        ::memset(&m_ReadInfo, 0, sizeof(OVERLAPPED));
        ::memset(&m_WriteInfo, 0, sizeof(OVERLAPPED));
        m_ReadInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
        m_WriteInfo.hEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
#endif
        Configuration(port, baudRate, parity, dataBits, stopBits, flowControl, sendBufferSize, receiveBufferSize);
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
        if (m_SendBuffer != nullptr) {
            ::free(m_SendBuffer);
        }

#ifdef __WIN32__
        ::CloseHandle(m_ReadInfo.hEvent);
        ::CloseHandle(m_WriteInfo.hEvent);
#endif
    }

    bool SerialPort::Configuration(
        const BaudRate baudRate,
        const FlowControl flowControl,
        const uint16_t sendBufferSize,
        const uint16_t receiveBufferSize)
    {
#ifdef __LINUX__
        if (m_Descriptor != -1) {
            ::tcgetattr(m_Descriptor, &m_PortSettings);
        }
#endif
#ifdef __WIN32__
        if (m_Descriptor != INVALID_HANDLE_VALUE) {
            ::GetCommState(m_Descriptor, &m_PortSettings);
        }
#endif

#ifdef __LINUX__
        cfmakeraw(&m_PortSettings);

        cfsetispeed(&m_PortSettings, baudRate); // set baud rates for in
        cfsetospeed(&m_PortSettings, baudRate); // and out
        m_PortSettings.c_cflag |= CLOCAL;

        if (flowControl == OFF) {
            m_PortSettings.c_cflag &= ~CRTSCTS;
            m_PortSettings.c_iflag &= ~IXON;
        } else if (flowControl == SOFTWARE) {
            m_PortSettings.c_cflag &= ~CRTSCTS;
            m_PortSettings.c_iflag |= IXON;
        } else if (flowControl == HARDWARE) {
            m_PortSettings.c_cflag |= CRTSCTS;
            m_PortSettings.c_iflag &= (~IXON);
        }
        if (m_Descriptor != -1) {
            ::tcsetattr(m_Descriptor, TCSANOW, &m_PortSettings);
            ::tcflush(m_Descriptor, TCIOFLUSH);
        }
#endif

#ifdef __WIN32__
        m_PortSettings.DCBlength = sizeof(DCB);
        m_PortSettings.BaudRate = baudRate;
        m_PortSettings.ByteSize = BITS_8;
        m_PortSettings.Parity = NONE;
        m_PortSettings.StopBits = BITS_1;
        if (flowControl == OFF) {
            m_PortSettings.fOutX = FALSE;
            m_PortSettings.fInX = FALSE;
            m_PortSettings.fDtrControl = DTR_CONTROL_DISABLE;
            m_PortSettings.fRtsControl = RTS_CONTROL_DISABLE;
            m_PortSettings.fOutxCtsFlow = FALSE;
            m_PortSettings.fOutxDsrFlow = FALSE;
        } else if (flowControl == SOFTWARE) {
            m_PortSettings.fOutX = TRUE;
            m_PortSettings.fInX = TRUE;
            m_PortSettings.fDtrControl = DTR_CONTROL_DISABLE;
            m_PortSettings.fRtsControl = RTS_CONTROL_DISABLE;
            m_PortSettings.fOutxCtsFlow = FALSE;
            m_PortSettings.fOutxDsrFlow = FALSE;
        } else if (flowControl == HARDWARE) {
            m_PortSettings.fOutX = FALSE;
            m_PortSettings.fInX = FALSE;
            m_PortSettings.fDtrControl = DTR_CONTROL_HANDSHAKE;
            m_PortSettings.fRtsControl = RTS_CONTROL_HANDSHAKE;
            m_PortSettings.fOutxCtsFlow = TRUE;
            m_PortSettings.fOutxDsrFlow = TRUE;
        }
        if (m_Descriptor != INVALID_HANDLE_VALUE) {
            ::SetCommState(m_Descriptor, &m_PortSettings);
        }
#endif

        m_syncAdmin.Lock();

        m_SendBufferSize = sendBufferSize;
        m_ReceiveBufferSize = receiveBufferSize;

        ASSERT((m_SendBufferSize != 0) && (m_ReceiveBufferSize != 0));

        if (m_SendBuffer != nullptr) {
            ::free(m_SendBuffer);
        }

        uint8_t* allocatedMemory = static_cast<uint8_t*>(::malloc(m_SendBufferSize + m_ReceiveBufferSize));
        if (m_SendBufferSize != -1) {
            m_SendBuffer = allocatedMemory;
        }
        if (m_ReceiveBufferSize != -1) {
            m_ReceiveBuffer = &(allocatedMemory[m_SendBufferSize]);
        }

        m_syncAdmin.Unlock();

        return (true);
    }

    bool SerialPort::Configuration(
        const string& port,
        const BaudRate baudRate,
        const Parity parity,
        const DataBits dataBits,
        const StopBits stopBits,
        const FlowControl flowControl,
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
                m_PortSettings.c_cflag |= CLOCAL;

                if (flowControl == OFF) {
                    m_PortSettings.c_cflag &= ~CRTSCTS;
                    m_PortSettings.c_iflag &= ~IXON;
                } else if (flowControl == SOFTWARE) {
                    m_PortSettings.c_cflag &= ~CRTSCTS;
                    m_PortSettings.c_iflag |= IXON;
                } else if (flowControl == HARDWARE) {
                    m_PortSettings.c_cflag |= CRTSCTS;
                    m_PortSettings.c_iflag &= (~IXON);
                }

#endif
#ifdef __WIN32__
                m_PortSettings.DCBlength = sizeof(DCB);
                m_PortSettings.BaudRate = baudRate;
                m_PortSettings.ByteSize = dataBits;
                m_PortSettings.Parity = parity;
                m_PortSettings.StopBits = stopBits;
                if (flowControl == OFF) {
                    m_PortSettings.fOutX = FALSE;
                    m_PortSettings.fInX = FALSE;
                    m_PortSettings.fDtrControl = DTR_CONTROL_DISABLE;
                    m_PortSettings.fRtsControl = RTS_CONTROL_DISABLE;
                    m_PortSettings.fOutxCtsFlow = FALSE;
                    m_PortSettings.fOutxDsrFlow = FALSE;
                } else if (flowControl == SOFTWARE) {
                    m_PortSettings.fOutX = TRUE;
                    m_PortSettings.fInX = TRUE;
                    m_PortSettings.fDtrControl = DTR_CONTROL_DISABLE;
                    m_PortSettings.fRtsControl = RTS_CONTROL_DISABLE;
                    m_PortSettings.fOutxCtsFlow = FALSE;
                    m_PortSettings.fOutxDsrFlow = FALSE;
                } else if (flowControl == HARDWARE) {
                    m_PortSettings.fOutX = FALSE;
                    m_PortSettings.fInX = FALSE;
                    m_PortSettings.fDtrControl = DTR_CONTROL_HANDSHAKE;
                    m_PortSettings.fRtsControl = RTS_CONTROL_HANDSHAKE;
                    m_PortSettings.fOutxCtsFlow = TRUE;
                    m_PortSettings.fOutxDsrFlow = TRUE;
                }
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

            m_Descriptor = open(convertedPortName.c_str(), O_RDWR | O_NOCTTY);

            result = errno;

            if (m_Descriptor != -1) {
                int flags = fcntl(m_Descriptor, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(m_Descriptor, F_SETFL, flags) != 0) {
                    TRACE_L4("Error on port socket F_SETFL call. Error %d", ERRORRESULT);

                    result = errno;

                    close(m_Descriptor);

                    m_Descriptor = -1;
                } else {

                    if (m_SendBuffer != nullptr) {
                        tcsetattr(m_Descriptor, TCSANOW, &m_PortSettings);
                    }

                    tcflush(m_Descriptor, TCIOFLUSH);

                    m_State = SerialPort::OPEN;
                    ResourceMonitor::Instance().Register(*this);

                    result = Core::ERROR_NONE;
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
                } else {
                    if (m_SendBuffer != nullptr) {
                        currentSettings.BaudRate = m_PortSettings.BaudRate;
                        currentSettings.Parity = m_PortSettings.Parity;
                        currentSettings.StopBits = m_PortSettings.StopBits;
                        currentSettings.ByteSize = m_PortSettings.ByteSize;

                        ::SetCommState(m_Descriptor, &currentSettings);
                    }
                    m_ReadBytes = 0;
                    m_State = SerialPort::OPEN;

                    g_SerialPortMonitor.Monitor(*this);

                    result = Core::ERROR_NONE;
                }
            }
        }
#endif

        m_syncAdmin.Unlock();

        return (result);
    }

    uint32_t SerialPort::Close(uint32_t waitTime)
    {
        m_syncAdmin.Lock();

#ifdef __LINUX__

        if (m_Descriptor != -1) {
            // Before we delete the descriptor, get ride of the Trigger
            // subscribtion.
            m_State |= SerialPort::EXCEPTION;
            m_State &= ~SerialPort::OPEN;
            close(m_Descriptor);
            m_Descriptor = -1;
            ResourceMonitor::Instance().Break();

            m_syncAdmin.Unlock();

            WaitForClosure(waitTime);
        } else {
#endif

#ifdef __WIN32__
            if (m_Descriptor != INVALID_HANDLE_VALUE) {

                m_State |= SerialPort::EXCEPTION;
                m_State &= ~SerialPort::OPEN;
                ::CloseHandle(m_Descriptor);
                m_Descriptor = INVALID_HANDLE_VALUE;
                g_SerialPortMonitor.Break();

                m_syncAdmin.Unlock();

                WaitForClosure(waitTime);
            } else {
#endif

                m_syncAdmin.Unlock();
            }

            return (Core::ERROR_NONE);
        }

        uint32_t SerialPort::WaitForClosure(const uint32_t time) const
        {
            // If we build in release, we do not want to "hang" forever, forcefull close after 20S waiting...
#ifdef __DEBUG__
            uint32_t waiting = time; // Expect time in MS
            uint32_t reportSlot = 0;
#else
    uint32_t waiting = (time == Core::infinite ? 20000 : time);
#endif

            // Right, a wait till connection is closed is requested..
            while ((waiting > 0) && (m_State != 0)) {
                // Make sure we aren't in the monitor thread waiting for close completion.
                ASSERT(Core::Thread::ThreadId() != ResourceMonitor::Instance().Id());

                uint32_t sleepSlot = (waiting > SLEEPSLOT_TIME ? SLEEPSLOT_TIME : waiting);

                m_syncAdmin.Unlock();

                // Right, lets sleep in slices of <= SLEEPSLOT_TIME ms
                SleepMs(sleepSlot);

                m_syncAdmin.Lock();

#ifdef __DEBUG__
                if ((++reportSlot & 0x1F) == 0) {
                    TRACE_L1("Currently waiting for Socket Closure. Current State [0x%X]", m_State);
                }
                waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
#else
        waiting -= sleepSlot;
#endif
            }
            return (m_State == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
        }

        void SerialPort::Trigger()
        {
            m_syncAdmin.Lock();

#ifdef __WIN32__
            if ((m_State & (SerialPort::OPEN | SerialPort::EXCEPTION)) == SerialPort::OPEN) {
                ::SetEvent(m_WriteInfo.hEvent);
            }
#else
    if ((m_State & (SerialPort::OPEN | SerialPort::EXCEPTION | SerialPort::WRITESLOT)) == SerialPort::OPEN) {
        m_State |= SerialPort::WRITESLOT;
        ResourceMonitor::Instance().Break();
    }
#endif

            m_syncAdmin.Unlock();
        }

#ifndef __WIN32__
        /* virtual */ uint16_t SerialPort::Events()
        {
            uint16_t result = POLLIN;
            if ((m_State & SerialPort::OPEN) == 0) {
                result = 0;
                Closed();
            } else if ((m_State & (SerialPort::OPEN | SerialPort::READ | SerialPort::WRITE)) == SerialPort::OPEN) {
                Opened();
                Write();
            } else if ((m_State & SerialPort::WRITE) != 0) {
                result |= POLLOUT;
            }
            return (result);
        }

        /* virtual */ void SerialPort::Handle(const uint16_t flags)
        {

            bool breakIssued = ((m_State & SerialPort::WRITESLOT) != 0);

            if (((flags != 0) || (breakIssued == true)) && ((m_State & SerialPort::OPEN) != 0)) {

                if (((flags & POLLOUT) != 0) || (breakIssued == true)) {
                    Write();
                }
                if ((flags & POLLIN) != 0) {
                    Read();
                }
            }
            Read();
        }

#else
void SerialPort::Write(const uint16_t writtenBytes)
{
    m_syncAdmin.Lock();

    m_State &= (~(SerialPort::WRITE));

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
            } else {
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
        } else if (readBytes == 0) {
            // nothing to read wait on the next trigger..
            m_State |= SerialPort::READ;
        } else {
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

            m_State &= (~(SerialPort::WRITE | SerialPort::WRITESLOT));

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
                    } else {
                        uint32_t l_Result = ERRORRESULT;

                        if ((l_Result == ERROR_WOULDBLOCK) || (l_Result == ERROR_AGAIN)) {
                            m_State |= (SerialPort::WRITE);
                        } else {
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
                } else {
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

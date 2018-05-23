#include "SocketPort.h"
#include "Thread.h"
#include "Sync.h"
#include "Singleton.h"
#include "Timer.h"

#ifdef __POSIX__
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <net/if.h>
#define __ERRORRESULT__               errno
#define __ERROR_AGAIN__               EAGAIN
#define __ERROR_WOULDBLOCK__          EWOULDBLOCK
#define __ERROR_INPROGRESS__          EINPROGRESS
#define __ERROR_ISCONN__              EISCONN
#define __ERROR_CONNRESET__           ECONNRESET
#define __ERROR_NETWORK_UNREACHABLE__ ENETUNREACH
#endif


#ifdef __APPLE__
#include <sys/event.h>
#elif defined(__LINUX__)
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <execinfo.h>
#endif

#ifdef __WIN32__
#include <Winsock2.h>
#include <ws2tcpip.h>
#define __ERRORRESULT__               ::WSAGetLastError ()
#define __ERROR_WOULDBLOCK__          WSAEWOULDBLOCK
#define __ERROR_AGAIN__               WSAEALREADY
#define __ERROR_INPROGRESS__          WSAEINPROGRESS
#define __ERROR_ISCONN__              WSAEISCONN
#define __ERROR_CONNRESET__           WSAECONNRESET
#define __ERROR_NETWORK_UNREACHABLE__ WSAENETUNREACH
#pragma warning(disable: 4355) // 'this' used in initializer list
#endif

#define MAX_LISTEN_QUEUE 	64

namespace WPEFramework {
namespace Core {
static uint32_t SLEEPSLOT_TIME = 100;

#ifdef __DEBUG__
static const char WATCHDOG_THREAD_NAME[] = "SocketWatchDog";
#endif

//////////////////////////////////////////////////////////////////////
// SocketPort::Initialization
//////////////////////////////////////////////////////////////////////

#ifdef __WIN32__
class WinSocketInitializer {
public:
    WinSocketInitializer() {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
        wVersionRequested = MAKEWORD(2, 2);

        err = WSAStartup(wVersionRequested, &wsaData);
        if(err != 0) {
            /* Tell the user that we could not find a usable */
            /* Winsock DLL.                                  */
            printf("WSAStartup failed with error: %d\n", err);
            exit(1);
        }

        /* Confirm that the WinSock DLL supports 2.2.*/
        /* Note that if the DLL supports versions greater    */
        /* than 2.2 in addition to 2.2, it will still return */
        /* 2.2 in wVersion since that is the version we      */
        /* requested.                                        */

        if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            /* Tell the user that we could not find a usable */
            /* WinSock DLL.                                  */
            printf("Could not find a usable version of Winsock.dll\n");
            WSACleanup();
            exit(1);
        } else {
            printf("The Winsock 2.2 dll was found okay\n");
        }
    }

    ~WinSocketInitializer() {
        /* then call WSACleanup when down using the Winsock dll */
        WSACleanup();
    }

    bool IsInitialized() const {
        return (true);
    }
};

static WinSocketInitializer g_SocketInitializer;
#endif

inline void DestroySocket(SOCKET& socket) {
#ifdef __LINUX__
    ::close(socket);
#endif

#ifdef __WIN32__
    ::closesocket(socket);
#endif

    socket = INVALID_SOCKET;
}

bool SetNonBlocking (SOCKET socket) {
#ifdef __WIN32__
    unsigned long l_Value = 1;
    if(ioctlsocket(socket, FIONBIO, &l_Value) != 0) {
        TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", __ERRORRESULT__);
    } else {
        return(true);
    }
#endif

#ifdef __POSIX__
    if(fcntl(socket, F_SETOWN, getpid()) == -1) {
        TRACE_L1("Setting Process ID failed. <%d>", __ERRORRESULT__);
    } else {
        int flags = fcntl(socket, F_GETFL, 0) | O_NONBLOCK;

        if (fcntl(socket, F_SETFL, flags) != 0) {
            TRACE_L1("Error on port socket F_SETFL call. Error %d", __ERRORRESULT__);
        } else {
            return (true);
        }
    }
#endif

    return (false);
}

#ifdef __DEBUG__
class SocketTimeOutHandler
{
public:
   SocketTimeOutHandler()
      : _socketMonitorThread(0)
   {
   }

   SocketTimeOutHandler(ThreadId socketMonitorThread)
      : _socketMonitorThread(socketMonitorThread)
   {
   }

   SocketTimeOutHandler(const SocketTimeOutHandler& rhs)
      : _socketMonitorThread(rhs._socketMonitorThread)
   {
   }

   uint32_t Expired()
   {
      fprintf(stderr, "===> SocketPort monitor thread is in deadlock!\nStack:\n");
      #if defined(__LINUX__) && !defined(__APPLE__)
      void* addresses[_AllocatedStackEntries];
      int addressCount = ::GetCallStack(_socketMonitorThread, addresses, _AllocatedStackEntries);
      backtrace_symbols_fd(addresses, addressCount, fileno(stderr));
      #endif

      return Core::infinite;
   }

private:
	ThreadId _socketMonitorThread;

   static const int _AllocatedStackEntries = 20;
};
#endif // __DEBUG__

//////////////////////////////////////////////////////////////////////
// SocketPort::SocketMonitor
//////////////////////////////////////////////////////////////////////

class SocketMonitor {
private:
#ifdef __LINUX__
    static const uint8_t  FILE_DESCRIPTOR_ALLOCATION = 64;
    static const uint32_t MAX_FILE_DESCRIPTORS = 3000;
#endif

    SocketMonitor(const SocketMonitor&) = delete;
    SocketMonitor& operator= (const SocketMonitor&) = delete;

    static constexpr uint32_t WatchDogStackSize = 1024 * 1024; // Was 64K

    class MonitorWorker : public Core::Thread {
    private:
        MonitorWorker (const MonitorWorker&) = delete;
        MonitorWorker& operator= (const MonitorWorker&) = delete;

    public:
        MonitorWorker(SocketMonitor& parent) :
            Core::Thread(Thread::DefaultStackSize(), _T("SocketPortMonitor")),
            _parent(parent) {
            Thread::Init();
#ifdef __WIN32__
            if (g_SocketInitializer.IsInitialized() == false) {
                TRACE_L1("SocketPortMonitor: Thread ID [%llu]", (uint64_t) Id());
            }
#else
            TRACE_L1("SocketPortMonitor: Thread ID [%llu]", (uint64_t) Id());
#endif
        }
        virtual ~MonitorWorker () {
            Wait(Thread::BLOCKED|Thread::STOPPED, Core::infinite);
        }

    public:
#ifdef __LINUX__
        virtual bool Initialize() {
            return ((Thread::Initialize() == true) && (_parent.Initialize() == true));
        }
#endif
        virtual uint32_t Worker() {
            return (_parent.Worker());
        }
    private:
        SocketMonitor& _parent;
    };

public:
    SocketMonitor() :
        m_ThreadInstance(nullptr),
        m_Admin(),
        m_MonitoredPorts(),
#ifdef SOCKET_TEST_VECTORS
        m_MonitorRuns(0),
#endif

#ifdef __WIN32__
        m_Action(WSACreateEvent())
#else
        m_MaxFileDescriptors(FILE_DESCRIPTOR_ALLOCATION),
        m_FDArray(static_cast<struct pollfd*>(::malloc(sizeof(struct pollfd) * (m_MaxFileDescriptors+1)))),
        m_SignalFD(-1)
#endif

#ifdef __DEBUG__
        , _watchDog(WatchDogStackSize, WATCHDOG_THREAD_NAME)
#endif
    {
    }
    ~SocketMonitor() {
        TRACE_L1("SocketPortMonitor: Closing [%d] sockets", static_cast<uint32_t>(m_MonitoredPorts.size()));

        // all sockets should be gone !!! Close sockets !!!) before closing the App !!!!
        ASSERT (m_MonitoredPorts.size() == 0);

        if (m_ThreadInstance != nullptr) {
            m_Admin.Lock();

            m_MonitoredPorts.clear();

            m_ThreadInstance->Block();
            Break();

            m_Admin.Unlock();

            delete m_ThreadInstance;
        }

#ifdef __LINUX__
        ::free(m_FDArray);
        if (m_SignalFD != -1) {
            ::close(m_SignalFD);
        }
#endif
#ifdef __WIN32__
        WSACloseEvent(m_Action);
#endif
    }

public:
    inline ::ThreadId Id () const {
        return (m_ThreadInstance != nullptr ? m_ThreadInstance->Id() : 0);
    }
#ifdef __WIN32__
    void Update (SocketPort& port) {
        m_Admin.Lock();

        // We are moving with the socket to -> Listning or to Connected...
        if ((port.m_State & SocketPort::OPEN) == 0) {
            ::WSAEventSelect(port.Socket(), m_Action, FD_CLOSE|FD_CONNECT);
        } else if ((port.m_State & SocketPort::ACCEPT) != 0) {
            ::WSAEventSelect(port.Socket(), m_Action, FD_CLOSE|FD_ACCEPT);
        } else {
            ::WSAEventSelect(port.Socket(), m_Action, FD_CLOSE|FD_READ|FD_WRITE);
        }

        m_Admin.Unlock();
    }
#endif
    void Monitor(SocketPort& port) {
        m_Admin.Lock();

        // Make sure this entry does not exist, only register sockets once !!!
#ifdef __DEBUG__
        std::list<SocketPort*>::const_iterator index = m_MonitoredPorts.begin();

        while ( (index != m_MonitoredPorts.end()) && (*index != &port) ) {
            index++;
        }
        ASSERT(index == m_MonitoredPorts.end());
#endif

        m_MonitoredPorts.push_back(&port);

        ASSERT ((port.m_State & SocketPort::MONITOR) == 0);

        if (m_MonitoredPorts.size() == 1) {
            if (m_ThreadInstance == nullptr) {
                m_ThreadInstance = new MonitorWorker(*this);

                // Wait till we are at least initialized
                m_ThreadInstance->Wait (Thread::BLOCKED|Thread::STOPPED);
            }

            m_ThreadInstance->Run();
        }
        else {
            Break();
        }

        m_Admin.Unlock();
    }

    inline ::ThreadId GetThreadId() {
        return m_ThreadInstance->Id();
    }

#ifdef __DEBUG__
    inline uint32_t SocketsInState(const SocketPort::enumState state) const {
        switch (state) {
        case SocketPort::ACCEPT:
            return (m_States[0]);
        case SocketPort::SHUTDOWN:
            return (m_States[1]);
        case SocketPort::OPEN:
            return (m_States[2]);
        case SocketPort::EXCEPTION:
            return (m_States[3]);
        case SocketPort::LINK:
            return (m_States[4]);
        case SocketPort::MONITOR:
            return (m_States[5]);
        default:
            break;
        }
        return (0);
    }
#endif

#ifdef SOCKET_TEST_VECTORS
    inline uint32_t MonitorRuns () const {
        return (m_MonitorRuns);
    }
#endif

    inline void Suspend(SocketPort& port) {
#ifdef __LINUX__
        shutdown(port.Socket(), SHUT_RDWR);
#endif

#ifdef __WIN32__
        ::WSAEventSelect(port.Socket(), m_Action, FD_CLOSE);
        shutdown(port.Socket(), SD_BOTH);
#endif
    }

    inline void Break() {

#ifdef __APPLE__

        int data = 0;
        ::sendto(m_SignalFD,
                 &data,
                 sizeof(data), 0,
                 m_signalNode,
                 m_signalNode.Size());
#elif defined(__LINUX__)
        ASSERT (m_ThreadInstance != nullptr);

        m_ThreadInstance->Signal(SIGUSR2);
#endif

#ifdef __WIN32__
        ::WSASetEvent(m_Action);
#endif
    };

private:
#ifdef __LINUX__
    bool Initialize() {
        int err;


#ifdef __APPLE__
        char filename[] = "/tmp/WPE-communication.XXXXXX";
        char *file = mktemp(filename);

        m_SignalFD = INVALID_SOCKET;

        if ((m_SignalFD = ::socket(AF_UNIX, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
            err = __ERRORRESULT__;
            TRACE_L1("Error on creating socket SOCKET. Error %d", __ERRORRESULT__);
        } else if (SetNonBlocking (m_SignalFD) == false) {
            err = 1;
            m_SignalFD = INVALID_SOCKET;
            ASSERT(false && "failed to make socket nonblocking ");
        }
        else {
            // Do we need to find something to bind to or is it pre-destined
            m_signalNode = Core::NodeId(file);
            if (::bind(m_SignalFD, m_signalNode, m_signalNode.Size()) == SOCKET_ERROR) {
                err = __ERRORRESULT__;
                m_SignalFD = INVALID_SOCKET;
                ASSERT(false && "failed to bind");
            }
            else {
                err = 0;
            }
        }

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

        m_FDArray[0].fd = m_SignalFD;
        m_FDArray[0].events = POLLIN;
        m_FDArray[0].revents = 0;

        return (err == 0);
    }
#endif

#ifdef __LINUX__
    uint32_t  Worker() {
        uint32_t delay = 0;

#ifdef SOCKET_TEST_VECTORS
        m_MonitorRuns++;
#endif

        // Add entries not in the Array before we start !!!
        m_Admin.Lock();

        // Do we have enough space to allocate all file descriptors ?
        if ((m_MonitoredPorts.size() + 1) > m_MaxFileDescriptors) {
            m_MaxFileDescriptors = ((((m_MonitoredPorts.size() + 1) / FILE_DESCRIPTOR_ALLOCATION) + 1) * FILE_DESCRIPTOR_ALLOCATION);

            ::free (m_FDArray);

            // Resize the array to fit..
            m_FDArray = static_cast<struct pollfd*>(::malloc(sizeof(struct pollfd) * m_MaxFileDescriptors));

            m_FDArray[0].fd = m_SignalFD;
            m_FDArray[0].events = POLLIN;
            m_FDArray[0].revents = 0;
        }

        int filledFileDescriptors = 1;
        std::list<SocketPort*>::iterator index = m_MonitoredPorts.begin();


#ifdef __DEBUG__
        // Determine the states of the sockets, start at 0..
        ::memset (m_States, 0, sizeof (m_States));
#endif

        // Fill in all entries required/updated..
        while (index != m_MonitoredPorts.end()) {
            SocketPort* port = (*index);

            ASSERT (port != nullptr);

            // It is the first time we are going to pick this one up..
            if ((port->m_State & SocketPort::MONITOR) == 0) {
                port->m_State |= SocketPort::MONITOR;

                if ((port->m_State & (SocketPort::OPEN|SocketPort::ACCEPT)) == SocketPort::OPEN) {
                    port->Opened();
                }
                if ( (port->IsOpen ()) && ((port->m_State & SocketPort::WRITESLOT) != 0) ) {
                    port->Write();
                }
            }

            m_FDArray[filledFileDescriptors].fd = port->Socket();
            m_FDArray[filledFileDescriptors].events = POLLIN|((port->m_State & SocketPort::LINK) != 0 ? POLLHUP : 0)|((port->m_State & SocketPort::WRITE) != 0 ? POLLOUT : 0);
            m_FDArray[filledFileDescriptors].revents = 0;
            filledFileDescriptors++;

#ifdef __DEBUG__
            // Determine the states of the sockets and their count..
            //
            // [0] = ACCEPT    = 0x04,
            // [1] = SHUTDOWN  = 0x08,
            // [2] = OPEN      = 0x10,
            // [3] = EXCEPTION = 0x20,
            // [4] = LINK      = 0x40 (TCP v.s. UDP sockets, here are the TCP socket, diff from 5 are the UDP)
            // [5] = MONITOR   = 0x80 (Equals the number of socket available to the system to monitor.

            m_States[0] += ((port->m_State & SocketPort::ACCEPT)    == 0 ? 0 : 1);
            m_States[1] += ((port->m_State & SocketPort::SHUTDOWN)  == 0 ? 0 : 1);
            m_States[2] += ((port->m_State & SocketPort::OPEN)      == 0 ? 0 : 1);
            m_States[3] += ((port->m_State & SocketPort::EXCEPTION) == 0 ? 0 : 1);
            m_States[4] += ((port->m_State & SocketPort::LINK)      == 0 ? 0 : 1);
#endif

            index++;
        }

#ifdef __DEBUG__
        m_States[5] = filledFileDescriptors;
#endif

        if (filledFileDescriptors > 1) {
            m_Admin.Unlock();

            int result = poll(m_FDArray, filledFileDescriptors, -1);

            m_Admin.Lock();

            if (result ==  -1) {
                TRACE_L1 ("poll failed with error <%d>", __ERRORRESULT__);

            } else if (m_FDArray[0].revents & POLLIN) {
#ifdef __APPLE__
                int info;
#else
                /* We have a valid signal, read the info from the fd */
                struct signalfd_siginfo info;
#endif
                uint32_t VARIABLE_IS_NOT_USED bytes = read(m_SignalFD, &info, sizeof(info));
                ASSERT(bytes == sizeof(info) || bytes == 0);

                // Clear the signal port..
                m_FDArray[0].revents = 0;
            }

            // We are only interested in the filedescriptors that have a corresponding client.
            // We also know that once a file descriptor is not found, we handled them all...
            int fd_index = 1;
            index = m_MonitoredPorts.begin();

            while ( fd_index  < filledFileDescriptors ) {
                ASSERT (index != m_MonitoredPorts.end());

                SocketPort* socket = *index;

                // As we are the only ones that take out the SocketPorts from the list, we
                // always make sure that the iterator is on the right spot/filedescriptor.
                ASSERT (socket->Socket() == m_FDArray[fd_index].fd);

                uint16_t flagsSet = m_FDArray[fd_index].revents;

#ifdef __DEBUG__
                // This time is in milli seconds. Arm for 2S.
                _watchDog.Arm(2000, SocketTimeOutHandler(Thread::ThreadId()));
#endif

                if(flagsSet != 0) {
                    if (socket->IsListening()) {
                        if ( (flagsSet & POLLIN) == POLLIN) {
                            // This triggeres an Addition of clients
                            socket->Accepted();
                        }
                    } else if (socket->IsOpen ()) {
                        if (  (flagsSet & POLLOUT) != 0) {
                            socket->Write();
                        }
                        if ( (flagsSet & POLLIN) != 0) {
                            socket->Read();
                        }
                    } else if (socket->IsConnecting()) {
                        if ( (flagsSet & POLLOUT) != 0) {
                            socket->Opened();
                        }
                    }
                }

                if ( (socket->IsOpen ()) && ((socket->m_State & SocketPort::WRITESLOT) != 0) ) {
                    socket->Write();
                }
                if ((((flagsSet & POLLHUP) != 0) || (socket->IsForcedClosing() == true)) && (socket->Closed() == true))  {
                    index = m_MonitoredPorts.erase (index);
                } else {
                    index++;
                }

#ifdef __DEBUG__
                _watchDog.Reset();
#endif
 
                fd_index++;
            }
        } else {
            m_ThreadInstance->Block();
            delay = Core::infinite;
        }

        m_Admin.Unlock();

        return (delay);
    }
#endif

#ifdef __WIN32__
    uint32_t  Worker() {
        std::list<SocketPort*>::iterator index;

#ifdef SOCKET_TEST_VECTORS
        m_MonitorRuns++;
#endif
#ifdef __DEBUG__
        // Determine the states of the sockets, start at 0..
        ::memset (m_States, 0, sizeof (m_States));
#endif

        m_Admin.Lock();

        // Now iterate over the sockets and determine their states..
        index = m_MonitoredPorts.begin();

        while (index != m_MonitoredPorts.end()) {

#ifdef __DEBUG__
            // Determine the states of the sockets and their count..
            //
            // [0] = ACCEPT    = 0x04,
            // [1] = SHUTDOWN  = 0x08,
            // [2] = OPEN      = 0x10,
            // [3] = EXCEPTION = 0x20,
            // [4] = LINK      = 0x40 (TCP v.s. UDP sockets, here are the TCP socket, diff from 5 are the UDP)
            // [5] = MONITOR   = 0x80 (Equals the number of socket available to the system to monitor.

            m_States[0] += (((*index)->m_State & SocketPort::ACCEPT)    == 0 ? 0 : 1);
            m_States[1] += (((*index)->m_State & SocketPort::SHUTDOWN)  == 0 ? 0 : 1);
            m_States[2] += (((*index)->m_State & SocketPort::OPEN)      == 0 ? 0 : 1);
            m_States[3] += (((*index)->m_State & SocketPort::EXCEPTION) == 0 ? 0 : 1);
            m_States[4] += (((*index)->m_State & SocketPort::LINK)      == 0 ? 0 : 1);
#endif
            // It is the first time we are going to pick this one up..
            if (((*index)->m_State & SocketPort::MONITOR) == 0) {

                (*index)->m_State |= SocketPort::MONITOR;

                if (((*index)->m_State & SocketPort::OPEN) == 0) {
                    ::WSAEventSelect((*index)->Socket(), m_Action, FD_CLOSE|FD_CONNECT);
                } else {
                    if (((*index)->m_State & SocketPort::ACCEPT) != 0) {
                        ::WSAEventSelect((*index)->Socket(), m_Action, FD_CLOSE|FD_ACCEPT);
                    } else {
                        ::WSAEventSelect((*index)->Socket(), m_Action, FD_CLOSE|FD_READ|FD_WRITE);

                        // We are observing, so it is safe to trigger a state change now.
                        (*index)->Opened();
                    }
                }

				if (((*index)->IsOpen()) && (((*index)->m_State & SocketPort::WRITESLOT) != 0)) {
					(*index)->Write();
				}
            }
            index++;
        }

#ifdef __DEBUG__
        m_States[5] = m_MonitoredPorts.size();
#endif

        m_Admin.Unlock();

        WaitForSingleObject(m_Action, Core::infinite);

        m_Admin.Lock();

        // Find all "pending" sockets and signal them..
        index = m_MonitoredPorts.begin();

        ::WSAResetEvent(m_Action);

        while (index != m_MonitoredPorts.end()) {
            SocketPort* socket = (*index);

            ASSERT (index != m_MonitoredPorts.end());
            ASSERT (socket != nullptr);

            WSANETWORKEVENTS networkEvents;

            // Re-enable monitoring for the next round..
            ::WSAEnumNetworkEvents(socket->Socket(), nullptr, &networkEvents);

            uint16_t result = static_cast<uint16_t>(networkEvents.lNetworkEvents);

#ifdef __DEBUG__
                // This time is in milli seconds. Arm for 2S.
                _watchDog.Arm(2000, SocketTimeOutHandler(Thread::ThreadId()));
#endif

            if(result != 0) {

                if ( (result & FD_ACCEPT) != 0) {
                    // This triggeres an Addition of clients
                    socket->Accepted();
                } else if (socket->IsOpen()) {
                    if ( (result & FD_WRITE) != 0) {
                        socket->Write();
                    }
                    if ( (result & FD_READ) != 0) {
                        socket->Read();
                    }
                } else if ( (result & FD_CONNECT) != 0) {
                    ::WSAEventSelect((*index)->Socket(), m_Action, FD_CLOSE|FD_READ|FD_WRITE);
                    socket->Opened();
                }
            }

            if ( (socket->IsOpen()) && ((socket->m_State & SocketPort::WRITESLOT) != 0) ) {
                socket->Write();
            }

            if ( (((result & FD_CLOSE) != 0) || (socket->IsForcedClosing() == true)) && (socket->Closed() == true) ) {
                index = m_MonitoredPorts.erase (index);
            } else {
                index++;
            }

#ifdef __DEBUG__
                _watchDog.Reset();
#endif
        }

        bool socketToObserve = (m_MonitoredPorts.size() > 0);

        if (socketToObserve == false) {
            m_ThreadInstance->Block();
        }

        m_Admin.Unlock();

        return (socketToObserve == false ? Core::infinite : 0);
    }

#endif

private:
    MonitorWorker* m_ThreadInstance;
    mutable Core::CriticalSection     m_Admin;
    std::list<SocketPort*>        m_MonitoredPorts;
#ifdef SOCKET_TEST_VECTORS
    uint32_t			      m_MonitorRuns;
#endif

#ifdef __LINUX__
    uint32_t          m_MaxFileDescriptors;
    struct pollfd*  m_FDArray;
    int             m_SignalFD;
#endif

#ifdef __WIN32__
    HANDLE          m_Action;
#endif
#ifdef __DEBUG__
    uint32_t		m_States[8];

    WatchDogType<SocketTimeOutHandler> _watchDog;
#endif
#ifdef __APPLE__
    Core::NodeId m_signalNode;
#endif
};


static SocketMonitor& g_SocketMonitor = SingletonType<SocketMonitor>::Instance();

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef __DEBUG__
/* static */ uint32_t SocketPort::GetCallStack(void* buffers[], uint32_t length) {
    return (::GetCallStack(g_SocketMonitor.Id(), buffers, length));
}
#endif

uint32_t SocketPort::WaitForOpen (const uint32_t time) const {

    // Make sure the state does not change in the mean time.
    m_syncAdmin.Lock();

    uint32_t waiting = (time == Core::infinite ? Core::infinite : time); // Expect time in MS.

    // Right, a wait till connection is closed is requested..
    while ( (waiting > 0) && (IsOpen() == false) ) {
        // Make sure we aren't in the monitor thread waiting for close completion.
        ASSERT(Core::Thread::ThreadId() != g_SocketMonitor.GetThreadId());

        uint32_t sleepSlot = (waiting > SLEEPSLOT_TIME ?  SLEEPSLOT_TIME : waiting);

        m_syncAdmin.Unlock();

        // Right, lets sleep in slices of 100 ms
        SleepMs (sleepSlot);

        m_syncAdmin.Lock();

        waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
    }

    uint32_t result = (((time == 0) || (IsOpen() == true)) ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);

    m_syncAdmin.Unlock();

    return (result);
}

uint32_t SocketPort::WaitForClosure (const uint32_t time) const {
    // If we build in release, we do not want to "hang" forever, forcefull close after 20S waiting...
#ifdef __DEBUG__
    uint32_t waiting = time; // Expect time in MS
    uint32_t reportSlot = 0;
#else
    uint32_t waiting = (time == Core::infinite ? 20000 : time);
#endif

    // Right, a wait till connection is closed is requested..
    while ( (waiting > 0) && (IsClosed() == false) ) {
        // Make sure we aren't in the monitor thread waiting for close completion.
        ASSERT(Core::Thread::ThreadId() != g_SocketMonitor.GetThreadId());

        uint32_t sleepSlot = (waiting > SLEEPSLOT_TIME ? SLEEPSLOT_TIME : waiting);

        m_syncAdmin.Unlock();

        // Right, lets sleep in slices of <= SLEEPSLOT_TIME ms
        SleepMs (sleepSlot);

        m_syncAdmin.Lock();

#ifdef __DEBUG__
        if ((++reportSlot  & 0x1F) == 0) {
            TRACE_L1("Currently waiting for Socket Closure. Current State [0x%X]", m_State);
        }
        waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
#else
        waiting -= sleepSlot;
#endif
    }
    return (IsClosed() ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
}

/* static */ uint32_t SocketPort::SocketsInState(const enumState state) {
#ifdef __DEBUG__
    return (g_SocketMonitor.SocketsInState(state));
#else
    DEBUG_VARIABLE(state);
    return (0);
#endif
}

#ifdef SOCKET_TEST_VECTORS
/* static */ uint32_t SocketPort::MonitorRuns() {
    return (g_SocketMonitor.MonitorRuns());
}
#endif

SocketPort::SocketPort(
    const enumType     socketType,
    const NodeId&      refLocalNode,
    const NodeId&      refremoteNode,
    const uint16_t       nSendBufferSize,
    const uint16_t       nReceiveBufferSize) :
    m_LocalNode(refLocalNode),
    m_RemoteNode(refremoteNode),
    m_ReceiveBufferSize(nReceiveBufferSize),
    m_SendBufferSize(nSendBufferSize),
    m_SocketType(socketType),
    m_Socket(INVALID_SOCKET),
    m_syncAdmin(),
    m_State(0),
    m_ReceivedNode(),
    m_SendBuffer(nullptr),
    m_ReceiveBuffer(nullptr) {
    TRACE_L5("Constructor SocketPort (NodeId&) <0x%X>", TRACE_POINTER(this));
}

SocketPort::SocketPort(
    const enumType     socketType,
    const SOCKET&      refConnector,
    const NodeId&	   remoteNode,
    const uint16_t       nSendBufferSize,
    const uint16_t       nReceiveBufferSize) :
    m_LocalNode(remoteNode.AnyInterface()),
    m_RemoteNode(remoteNode),
    m_ReceiveBufferSize(nReceiveBufferSize),
    m_SendBufferSize(nSendBufferSize),
    m_SocketType(socketType),
    m_Socket(refConnector),
    m_syncAdmin(),
    m_State(0),
    m_ReceivedNode(),
    m_SendBuffer(nullptr),
    m_ReceiveBuffer(nullptr) {
    NodeId::SocketInfo  localAddress;
    socklen_t           localSize  = sizeof(localAddress);

    ASSERT (refConnector != INVALID_SOCKET);

    if ( (SetNonBlocking (m_Socket) == false) || (::getsockname(m_Socket, (struct sockaddr*) &localAddress, &localSize) == SOCKET_ERROR) ) {
        DestroySocket(m_Socket);

        TRACE_L5("Error on preparing the port for communication. Error %d", __ERRORRESULT__);
    } else {
        m_LocalNode = localAddress;

        BufferAlignment(m_Socket);

        m_State = SocketPort::LINK|SocketPort::OPEN|SocketPort::READ;
    }
}

SocketPort::~SocketPort() {
    TRACE_L5("Destructor SocketPort <0x%X>", TRACE_POINTER(this));

    // Make sure the socket is closed before you destruct. Otherwise
    // the virtuals might be called, which are destructed at this point !!!!
    ASSERT (m_Socket == INVALID_SOCKET);

    ::free(m_SendBuffer);
}

uint32_t SocketPort::TTL() const
{
	uint32_t value;

#ifdef __WIN32__
	int size = sizeof(value);
	if (getsockopt(m_Socket, IPPROTO_IP, IP_TTL, reinterpret_cast<char*>(&value), &size) != 0)
#else
	socklen_t size = sizeof(value);
	if (getsockopt(m_Socket, SOL_IP, IP_TTL, reinterpret_cast<char*>(&value), &size) != 0)
#endif
		return (static_cast<uint32_t>(~0));

	return (value);
}

uint32_t SocketPort::TTL(const uint8_t ttl)
{
	uint32_t result = Core::ERROR_NONE;
	uint32_t value = ttl;
#ifdef __WIN32__
	if (setsockopt(m_Socket, IPPROTO_IP, IP_TTL, reinterpret_cast<char*>(&value), sizeof(value)) != 0)
#else
	if (setsockopt(m_Socket, SOL_IP, IP_TTL, reinterpret_cast<char*>(&value), sizeof(value)) != 0)
#endif
		result = __ERRORRESULT__;

	return (result);

}

bool SocketPort::Broadcast(const bool enabled) {
    uint32_t flag = (enabled ? 1 : 0);

    /* set the broadcast option - we need this to listen to broadcast messages */
    if (::setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char *>(&flag), sizeof(flag)) != 0) {
        TRACE_L1 ("Error: Could not set broadcast option on socket, error: %d\n", __ERRORRESULT__);
        return(false);
    }

    return (true);

}

void SocketPort::BufferAlignment (SOCKET socket) {
    socklen_t valueLength = sizeof(int);
    int value;
    uint32_t receiveBuffer = m_ReceiveBufferSize;
    uint32_t sendBuffer = m_SendBufferSize;

    if (m_ReceiveBufferSize == static_cast<uint16_t>(~0)) {
        ::getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*) &value, &valueLength);

        receiveBuffer = static_cast<uint32_t>(value);

        TRACE_L1("Receive buffer size. %d", receiveBuffer);
    } else if ( (receiveBuffer != 0) && (::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*) &receiveBuffer, sizeof(receiveBuffer)) == SOCKET_ERROR) ) {
        TRACE_L1("Error could not set Receive buffer size (%d).", receiveBuffer);
    }

    if (m_SendBufferSize == static_cast<uint16_t>(~0)) {
        ::getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*) &value, &valueLength);

        sendBuffer = static_cast<uint32_t>(value);

        TRACE_L1("Send buffer size. %d", sendBuffer);
    } else  if ( (sendBuffer != 0) && (::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*) &sendBuffer, sizeof(sendBuffer)) == SOCKET_ERROR) ) {
        TRACE_L1("Error could not set Send buffer size (%d).", sendBuffer);
    }

    if ( (receiveBuffer != 0) || (sendBuffer != 0) ) {
        uint8_t* allocatedMemory =  static_cast<uint8_t*> (::malloc(sendBuffer + receiveBuffer));
        if (sendBuffer != 0) {
            m_SendBuffer = allocatedMemory;
        }
        if (receiveBuffer != 0) {
            m_ReceiveBuffer = &(allocatedMemory[sendBuffer]);
        }
    }
}

SOCKET SocketPort::ConstructSocket(NodeId& localNode, const string& specificInterface) {
    ASSERT (localNode.IsValid() == true);

    SOCKET l_Result = INVALID_SOCKET;

#ifndef __WIN32__
    // Check if domain path already exists, if so remove.
    if ((localNode.Type() == NodeId::TYPE_DOMAIN) && (m_SocketType == SocketPort::LISTEN)) {
        if (access(localNode.HostName().c_str(), F_OK) != -1 ) {
            TRACE_L1 ("Found out domain path already exists, deleting: %s", localNode.HostName().c_str());
            remove(localNode.HostName().c_str());
        }
    }
#endif

    if ((l_Result = ::socket(localNode.Type(), SocketMode(), localNode.Extension())) == INVALID_SOCKET) {
	TRACE_L1("Error on creating socket SOCKET. Error %d", __ERRORRESULT__);
    } else if (SetNonBlocking (l_Result) == false) {
#ifdef __WIN32__
        ::closesocket(l_Result);
#else
        ::close(l_Result);
#endif
        l_Result = INVALID_SOCKET;
    } else if ((localNode.Type() == NodeId::TYPE_IPV4) || (localNode.Type() == NodeId::TYPE_IPV6)) {
        // set SO_REUSEADDR on a socket to true (1): allows other sockets to bind() to this
        // port, unless there is an active listening socket bound to the port already. This
        // enables you to get around those "Address already in use" error messages when you
        // try to restart your server after a crash.
        int optval = 1;
        socklen_t optionLength = sizeof(int);

        ::setsockopt(l_Result, SOL_SOCKET, SO_REUSEADDR, (const char*) &optval, optionLength);
    }
#ifndef __WIN32__
    else if ((localNode.Type() == NodeId::TYPE_DOMAIN) && (m_SocketType == SocketPort::LISTEN)) {
        // The effect of SO_REUSEADDR  but then on Domain Sockets :-)
        if (unlink (localNode.HostName().c_str()) == -1) {
            int report = __ERRORRESULT__;

            if (report != 2) {

                ::close(l_Result);
                l_Result = INVALID_SOCKET;

                TRACE_L1("Error on unlinking domain socket. Error %d", report);
            }
        }
    }
#endif

#ifndef __WIN32__
	// See if we need to bind to a specific interface.
    if ( (l_Result != INVALID_SOCKET) && (specificInterface.empty() == false)) {

	struct ifreq interface;
	strncpy(interface.ifr_ifrn.ifrn_name, specificInterface.c_str(), IFNAMSIZ);

        if (::setsockopt(l_Result, SOL_SOCKET, SO_BINDTODEVICE, (const char *) &interface, sizeof(interface)) < 0) {

            TRACE_L1("Error binding socket to an interface. Error %d", __ERRORRESULT__);

            ::close(l_Result);
            l_Result = INVALID_SOCKET;

        }
    }
#endif

    if (l_Result != INVALID_SOCKET) {
        // Do we need to find something to bind to or is it pre-destined
        // Bind is called in the following situations:
        //  - domain socket server
        //  - domain socket datagram client
        //  - IPV4 or IPV6 UDP server
        //  - IPV4 or IPV6 TCP server with port set
        if ( (SocketMode() != SOCK_STREAM) || (m_SocketType == SocketPort::LISTEN) ||
             (((localNode.Type() == NodeId::TYPE_IPV4) || (localNode.Type() == NodeId::TYPE_IPV6)) && (localNode.PortNumber() != 0)) ) {
            if (::bind(l_Result, static_cast<const NodeId&>(localNode), localNode.Size()) != SOCKET_ERROR) {
                BufferAlignment(l_Result);
                return (l_Result);
            } else {
                printf ("LINE: Error binding: %d\n", __ERRORRESULT__);
                TRACE_L1("Error on port socket BIND. Error %d", __ERRORRESULT__);
            }
        } else {
            BufferAlignment(l_Result);

            return (l_Result);
        }

        DestroySocket(l_Result);
    }


    return (INVALID_SOCKET);
}

uint32_t SocketPort::Open(const uint32_t waitTime, const string& specificInterface) {
    uint32_t nStatus = Core::ERROR_ILLEGAL_STATE;

	m_ReadBytes = 0;
	m_SendBytes = 0;
	m_SendOffset = 0;

    if ((m_State & (SocketPort::LINK|SocketPort::OPEN|SocketPort::MONITOR)) == (SocketPort::LINK|SocketPort::OPEN)) {
        // Open up an accepted socket, but not yet added to the monitor.
        nStatus = Core::ERROR_NONE;
    } else {
        ASSERT ((m_Socket == INVALID_SOCKET) && (m_State == 0));

        if ( (m_SocketType == SocketPort::RAW) || (m_SocketType == SocketPort::STREAM) ) {
            if (m_LocalNode.IsValid() == false) {
                m_LocalNode = m_RemoteNode.Origin();
            }
        }

        ASSERT (((m_SocketType != LISTEN) || (m_LocalNode.IsValid() == true)) && ((m_SocketType != STREAM) || (m_RemoteNode.Type () == m_LocalNode.Type())));

        m_Socket = ConstructSocket(m_LocalNode, specificInterface);

        if(m_Socket != INVALID_SOCKET) {
            if( (m_SocketType == DATAGRAM) || ((m_SocketType == RAW) && (m_RemoteNode.IsValid() == false)) ) {
                m_State = SocketPort::OPEN|SocketPort::READ;

                nStatus = Core::ERROR_NONE;
            } else if(m_SocketType == LISTEN) {
                if(::listen(m_Socket, MAX_LISTEN_QUEUE) == SOCKET_ERROR) {
                    TRACE_L5("Error on port socket LISTEN. Error %d", __ERRORRESULT__);
                } else {
                    // Trigger state to Open
                    m_State = SocketPort::OPEN|SocketPort::ACCEPT;

                    nStatus  = Core::ERROR_NONE;
                }
            } else {
                if(::connect(m_Socket, static_cast<const NodeId&>(m_RemoteNode), m_RemoteNode.Size()) != SOCKET_ERROR) {
                    m_State = SocketPort::LINK|SocketPort::OPEN|SocketPort::READ;
                    nStatus = Core::ERROR_NONE;
                } else {
                    int l_Result = __ERRORRESULT__;

                    if ((l_Result == __ERROR_WOULDBLOCK__) || (l_Result == __ERROR_AGAIN__) || (l_Result == __ERROR_INPROGRESS__)) {
                        m_State = SocketPort::LINK|SocketPort::WRITE;
                        nStatus = Core::ERROR_INPROGRESS;
                    } else if (l_Result == __ERROR_ISCONN__) {
                        nStatus = Core::ERROR_ALREADY_CONNECTED;
                    } else if (l_Result == __ERROR_NETWORK_UNREACHABLE__) {
                        nStatus = Core::ERROR_UNREACHABLE_NETWORK;
                    } else {
                        TRACE_L1 ("Connect failed, error: %d", l_Result);
                        nStatus = Core::ERROR_ASYNC_FAILED;
                    }
                }
            }
        }
    }

    if ( (nStatus == Core::ERROR_NONE) || (nStatus == Core::ERROR_INPROGRESS) ) {
        g_SocketMonitor.Monitor(*this);

        if (nStatus == Core::ERROR_INPROGRESS) {
            if (waitTime > 0) {
                // We are good to go, we just have to wait till we are connected..
                nStatus = WaitForOpen(waitTime);
            } else if (IsOpen() == true) {
                nStatus = Core::ERROR_NONE;
            }
        }

    } else {
        DestroySocket(m_Socket);
    }


    return (nStatus);
}

uint32_t SocketPort::Close(const uint32_t waitTime) {

    // Make sure the state does not change in the mean time.
    m_syncAdmin.Lock();

    bool closed = IsClosed();

    if (m_Socket != INVALID_SOCKET) {

        if ((m_State != 0) && ((m_State & SHUTDOWN) == 0)) {

            if ( ((m_State & LINK) == 0) || ((m_State & OPEN) == 0) ) {
                // This is a connectionless link, do not expect a close from the otherside.
                // No use to wait on anything !!, Signal a FORCED CLOSURE (EXCEPTION && SHUTDOWN)
                m_State |= (SHUTDOWN|EXCEPTION);

                g_SocketMonitor.Break();
            } else {
            	m_State |= SHUTDOWN;

                // Block new data from coming in, signal the other side that we close !!
                g_SocketMonitor.Suspend(*this);
            }
        }

        if (waitTime > 0) {
            closed = (WaitForClosure(waitTime) == Core::ERROR_NONE);

            if (closed == false) {
                // Make this a forced close !!!
                m_State |= EXCEPTION;

                // We probably did not get a response from the otherside on the close
                // sloppy but let's forcefully close it
                g_SocketMonitor.Break();

                closed = (WaitForClosure(Core::infinite) == Core::ERROR_NONE);

                ASSERT (closed == true);
            }
        }

    }

    m_syncAdmin.Unlock();

    return (closed ? Core::ERROR_NONE : Core::ERROR_PENDING_SHUTDOWN);
}

void SocketPort::Trigger() {
    m_syncAdmin.Lock();

    if ((m_State & (SocketPort::SHUTDOWN|SocketPort::OPEN|SocketPort::EXCEPTION)) == SocketPort::OPEN) {

        m_State |= SocketPort::WRITESLOT;
        g_SocketMonitor.Break();
    }
    m_syncAdmin.Unlock();
}

void SocketPort::Write() {
    bool dataLeftToSend = true;

    m_syncAdmin.Lock();

    m_State &= (~(SocketPort::WRITE|SocketPort::WRITESLOT));

    while (((m_State & (SocketPort::WRITE|SocketPort::SHUTDOWN|SocketPort::OPEN|SocketPort::EXCEPTION)) == SocketPort::OPEN) && (dataLeftToSend == true)) {
        if (m_SendOffset == m_SendBytes) {
            m_SendBytes = SendData(m_SendBuffer, m_SendBufferSize);
            m_SendOffset = 0;
            dataLeftToSend = (m_SendOffset != m_SendBytes);

            ASSERT (m_SendBytes <= m_SendBufferSize);
        }

        if (dataLeftToSend == true) {
            uint32_t sendSize;

            // Sockets are non blocking the Send buffer size is equal to the buffer size. We only send
            // if the buffer free (SEND flag) is active, so the buffer should always fit.
            if (((m_State & SocketPort::LINK) == 0) && (m_LocalNode.Type() != NodeId::TYPE_NETLINK)) {
                ASSERT (m_RemoteNode.IsValid() == true);

                sendSize = ::sendto(m_Socket,
                                    reinterpret_cast <const char*>(&m_SendBuffer[m_SendOffset]),
                                    m_SendBytes - m_SendOffset, 0,
                                    static_cast<const NodeId&>(m_RemoteNode),
                                    m_RemoteNode.Size());
            } else {
                sendSize = ::send(m_Socket,
                                  reinterpret_cast <const char*>(&m_SendBuffer[m_SendOffset]),
                                  m_SendBytes - m_SendOffset, 0);
            }

            if (sendSize != static_cast<uint32_t>(SOCKET_ERROR)) {
                m_SendOffset += sendSize;
            } else {
                uint32_t l_Result = __ERRORRESULT__;

                if ((l_Result == __ERROR_WOULDBLOCK__) || (l_Result == __ERROR_AGAIN__) || (l_Result == __ERROR_INPROGRESS__)) {
                    m_State |= SocketPort::WRITE;
                } else {
                    m_State |= SocketPort::EXCEPTION;
                    StateChange();
                    printf ("Write exception. %d\n", l_Result);
                }
            }
        }
    }

    m_syncAdmin.Unlock();
}

void SocketPort::Read() {
    m_syncAdmin.Lock();

    m_State &= (~SocketPort::READ);

    while ((m_State & (SocketPort::READ|SocketPort::EXCEPTION|SocketPort::OPEN)) == SocketPort::OPEN) {
        uint32_t l_Size;

        if (m_ReadBytes == m_ReceiveBufferSize) {
            m_ReadBytes = 0;
        }

        // Read the actual data from the port.
        if (((m_State & SocketPort::LINK) == 0) && (m_LocalNode.Type() != NodeId::TYPE_NETLINK)) {
            NodeId::SocketInfo	l_Remote;
            socklen_t        	l_Address = sizeof(l_Remote);

            l_Size = ::recvfrom(m_Socket,
                                reinterpret_cast <char*>(&m_ReceiveBuffer[m_ReadBytes]),
                                m_ReceiveBufferSize-m_ReadBytes, 0, (struct sockaddr*) &l_Remote,
                                &l_Address);

            m_ReceivedNode = l_Remote;
        } else {
            l_Size = ::recv(m_Socket,
                            reinterpret_cast <char*>(&m_ReceiveBuffer[m_ReadBytes]),
                            m_ReceiveBufferSize-m_ReadBytes, 0);
        }

        if (l_Size == 0) {
            if ((m_State & SocketPort::LINK) != 0) {
                // The otherside has closed the connection !!!
                m_State = ((m_State & (~SocketPort::OPEN)) | SocketPort::EXCEPTION);
            }
        } else if (l_Size != static_cast<uint32_t>(SOCKET_ERROR)) {
            m_ReadBytes += l_Size;
        } else {
            uint32_t l_Result = __ERRORRESULT__;

            if ((l_Result == __ERROR_WOULDBLOCK__) || (l_Result == __ERROR_AGAIN__) || (l_Result == __ERROR_INPROGRESS__) || (l_Result == 0)) {
                m_State |= SocketPort::READ;
            } else if (l_Result == __ERROR_CONNRESET__) {
                m_State = ((m_State & (~SocketPort::OPEN)) | SocketPort::EXCEPTION);
            } else if (l_Result != 0) {
                m_State |= SocketPort::EXCEPTION;
                StateChange();
                printf ("Read exception. %d\n", l_Result);
            }
        }

        if (m_ReadBytes != 0) {
            uint16_t handledBytes = ReceiveData(m_ReceiveBuffer, m_ReadBytes);

            ASSERT (m_ReadBytes >= handledBytes);

            m_ReadBytes -= handledBytes;

            if ( (m_ReadBytes != 0) && (handledBytes != 0) ) {
                // Oops not all data was consumed, Lets remove the read data
                ::memcpy(m_ReceiveBuffer, &m_ReceiveBuffer[handledBytes], m_ReadBytes);
            }
        }
    }

    m_syncAdmin.Unlock();
}

bool SocketPort::Closed() {
    bool result = true;

    ASSERT (m_Socket != INVALID_SOCKET);

    m_syncAdmin.Lock();

    // Turn them all off, except for the SHUTDOWN bit, to show whether this was
    // done on our request, or closed from the other side...
    m_State &= SHUTDOWN;

    StateChange();

    m_State &= (~SHUTDOWN);

    if ( m_State != 0 ) {
        result = false;
    } else {
        DestroySocket (m_Socket);
        // Remove socket descriptor for UNIX domain datagram socket.
        if ((m_LocalNode.Type() == NodeId::TYPE_DOMAIN)  && ((m_SocketType == SocketPort::LISTEN) || (SocketMode() != SOCK_STREAM)))
        {
            TRACE_L1("CLOSED: Remove socket descriptor %s", m_LocalNode.HostName().c_str());
#ifdef __WIN32__
            _unlink(m_LocalNode.HostName().c_str());
#else
			unlink(m_LocalNode.HostName().c_str());
#endif
        }
    }

    m_syncAdmin.Unlock();

    return (result);
}

void SocketPort::Opened() {
    m_syncAdmin.Lock();
    m_State = (m_State & (~SocketPort::WRITE)) | SocketPort::OPEN;
    StateChange();
    m_syncAdmin.Unlock();
}

void SocketPort::Accepted() {
    m_syncAdmin.Lock();
    StateChange();
    m_syncAdmin.Unlock();
}

SOCKET SocketPort::Accept (NodeId& remoteId) {
    NodeId::SocketInfo	address;
    socklen_t        	size  = sizeof(address);
    SOCKET              	result;

    if ((result = ::accept(m_Socket, (struct sockaddr*) &address, &size)) != SOCKET_ERROR) {
        // Align the buffer to what is requested
        BufferAlignment(result);

        remoteId = address;
    } else {
        int error = __ERRORRESULT__;
        if ((error !=__ERROR_AGAIN__) && (error != __ERROR_WOULDBLOCK__)) {
            TRACE_L1("Error could not accept a connection. Error: %d.", error);
        }
        result = INVALID_SOCKET;
    }

    return (result);
}

NodeId SocketPort::Accept () {
    NodeId			newConnection;
    SOCKET                  result;

    if ( ((result = Accept (newConnection)) != INVALID_SOCKET) && (SetNonBlocking(result) == true) ) {
        DestroySocket(m_Socket);

        m_Socket = result;
        m_State = (SocketPort::MONITOR|SocketPort::LINK|SocketPort::OPEN|SocketPort::READ|SocketPort::WRITESLOT);

#ifdef __WIN32__
        g_SocketMonitor.Update(*this);
#endif
    }

    return (newConnection);
}

void SocketPort::Listen () {
    string emptyString;

    // Switching to listen is only allowed if the connection is closed
    // and this is a listning socket from origin !!!!
    ASSERT (m_SocketType == SocketPort::LISTEN);
    ASSERT (m_State == 0);
    ASSERT (m_Socket != INVALID_SOCKET);

    // Current socket can be destroyed
    DestroySocket(m_Socket);

    m_Socket = ConstructSocket(m_LocalNode, emptyString);

    if (m_Socket != INVALID_SOCKET) {
        NodeId remoteId;

        if(::listen(m_Socket, MAX_LISTEN_QUEUE) == SOCKET_ERROR) {
            TRACE_L5("Error on port socket LISTEN. Error %d", __ERRORRESULT__);
        } else {
            // Trigger state to Open
            m_State = SocketPort::MONITOR|SocketPort::OPEN|SocketPort::ACCEPT;
#ifdef __WIN32__
            g_SocketMonitor.Update(*this);
#endif
        }
    }
}

bool SocketPort::Join(const NodeId& multicastAddress) {
    const NodeId::SocketInfo& inputInfo = multicastAddress;

    ASSERT (inputInfo.IPV4Socket.sin_family == AF_INET);

    ip_mreq multicastRequest;

    multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
    multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;

    if (::setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP,  reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
        TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
        return (false);
    }

    return (true);
}

bool SocketPort::Leave(const NodeId& multicastAddress) {
    const NodeId::SocketInfo& inputInfo = multicastAddress;

    ASSERT (inputInfo.IPV4Socket.sin_family == AF_INET);

    ip_mreq multicastRequest;

    multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
    multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;

    if (::setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP,  reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
        TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
        return (false);
    }

    return (true);
}

bool SocketPort::Join(const NodeId& multicastAddress, const NodeId& source) {
    const NodeId::SocketInfo& inputInfo = multicastAddress;

    ASSERT (inputInfo.IPV4Socket.sin_family == AF_INET);

#ifdef __WIN32__
    ip_mreq_source multicastRequest;
#else
    ip_mreq_source multicastRequest;

    // TODO, Fix multicast join with interface
    // multicastRequest.imr_interface = in_addr{ 0, 0;
#endif

#ifdef __WIN32__
    multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
#endif

    multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;
    multicastRequest.imr_sourceaddr = static_cast<const NodeId::SocketInfo&>(source).IPV4Socket.sin_addr;

    if (::setsockopt(m_Socket, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,  reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
        TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
        return (false);
    }

    return (true);
}

bool SocketPort::Leave(const NodeId& multicastAddress, const NodeId& source) {
    const NodeId::SocketInfo& inputInfo = multicastAddress;

    ASSERT (inputInfo.IPV4Socket.sin_family == AF_INET);

#ifdef __WIN32__
    ip_mreq_source multicastRequest;
#else
    ip_mreq_source multicastRequest;

    // TODO, Fix multicast join with source
    // multicastRequest.imr_interface = 0;
#endif

#ifdef __WIN32__
    multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
#endif

    multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;
    multicastRequest.imr_sourceaddr = static_cast<const NodeId::SocketInfo&>(source).IPV4Socket.sin_addr;

    if (::setsockopt(m_Socket, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP,  reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
        TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
        return (false);
    }

    return (true);
}

SocketDatagram::SocketDatagram(const bool    rawSocket,
                               const NodeId& localNode,
                               const NodeId& remoteNode,
                               const uint16_t  sendBufferSize,
                               const uint16_t  receiveBufferSize) :
    SocketPort((rawSocket ? SocketPort::RAW : SocketPort::DATAGRAM),localNode, remoteNode, sendBufferSize, receiveBufferSize) {
}

/* virtual */ SocketDatagram::~SocketDatagram() {
}

}
} // namespace Solution::Core

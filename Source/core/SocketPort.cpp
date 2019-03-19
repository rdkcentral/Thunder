#include "SocketPort.h"
#include "ProcessInfo.h"
#include "ResourceMonitor.h"
#include "Singleton.h"
#include "Sync.h"
#include "Thread.h"
#include "Timer.h"

#ifdef __POSIX__
#include <arpa/inet.h>
#include <fcntl.h>
#include <net/if.h>
#define __ERRORRESULT__ errno
#define __ERROR_AGAIN__ EAGAIN
#define __ERROR_WOULDBLOCK__ EWOULDBLOCK
#define __ERROR_INPROGRESS__ EINPROGRESS
#define __ERROR_ISCONN__ EISCONN
#define __ERROR_CONNRESET__ ECONNRESET
#define __ERROR_NETWORK_UNREACHABLE__ ENETUNREACH
#endif

#ifdef __APPLE__
#include <sys/event.h>
#elif defined(__LINUX__)
#include <execinfo.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/signalfd.h>
#endif

#ifdef __WIN32__
#include <Winsock2.h>
#include <ws2tcpip.h>
#define __ERRORRESULT__ ::WSAGetLastError()
#define __ERROR_WOULDBLOCK__ WSAEWOULDBLOCK
#define __ERROR_AGAIN__ WSAEALREADY
#define __ERROR_INPROGRESS__ WSAEINPROGRESS
#define __ERROR_ISCONN__ WSAEISCONN
#define __ERROR_CONNRESET__ WSAECONNRESET
#define __ERROR_NETWORK_UNREACHABLE__ WSAENETUNREACH
#pragma warning(disable : 4355) // 'this' used in initializer list
#endif

namespace WPEFramework {
namespace Core {

#ifdef __DEBUG__
#define WATCHDOG_ENABLED
#endif

    //////////////////////////////////////////////////////////////////////
    // SocketPort::Initialization
    //////////////////////////////////////////////////////////////////////

#ifdef __WIN32__

    namespace {

        class WinSocketInitializer {
        public:
            WinSocketInitializer()
            {
                WORD wVersionRequested;
                WSADATA wsaData;
                int err;

                /* Use the MAKEWORD(lowbyte, highbyte) macro declared in Windef.h */
                wVersionRequested = MAKEWORD(2, 2);

                err = WSAStartup(wVersionRequested, &wsaData);
                if (err != 0) {
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

                if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
                    /* Tell the user that we could not find a usable */
                    /* WinSock DLL.                                  */
                    printf("Could not find a usable version of Winsock.dll\n");
                    WSACleanup();
                    exit(1);
                } else {
                    printf("The Winsock 2.2 dll was found okay\n");
                }
            }

            ~WinSocketInitializer()
            {
                /* then call WSACleanup when down using the Winsock dll */
                WSACleanup();
            }

            bool IsInitialized() const
            {
                return (true);
            }
        };

        static WinSocketInitializer g_SocketInitializer;

    } // Nameless namespace

#endif

    //////////////////////////////////////////////////////////////////////
    // SocketPort::SocketMonitor
    //////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////
    // SocketPort
    //////////////////////////////////////////////////////////////////////

    static constexpr uint32_t MAX_LISTEN_QUEUE = 64;
    static constexpr uint32_t SLEEPSLOT_TIME = 100;

    inline void DestroySocket(SOCKET& socket)
    {
#ifdef __LINUX__
        ::close(socket);
#endif

#ifdef __WIN32__
        ::closesocket(socket);
#endif

        socket = INVALID_SOCKET;
    }

    bool SetNonBlocking(SOCKET socket)
    {
#ifdef __WIN32__
        unsigned long l_Value = 1;
        if (ioctlsocket(socket, FIONBIO, &l_Value) != 0) {
            TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", __ERRORRESULT__);
        } else {
            return (true);
        }
#endif

#ifdef __POSIX__
        if (fcntl(socket, F_SETOWN, getpid()) == -1) {
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
    //////////////////////////////////////////////////////////////////////
    // Construction/Destruction
    //////////////////////////////////////////////////////////////////////

    SocketPort::SocketPort(
        const enumType socketType,
        const NodeId& refLocalNode,
        const NodeId& refremoteNode,
        const uint16_t nSendBufferSize,
        const uint16_t nReceiveBufferSize)
        : m_LocalNode(refLocalNode)
        , m_RemoteNode(refremoteNode)
        , m_ReceiveBufferSize(nReceiveBufferSize)
        , m_SendBufferSize(nSendBufferSize)
        , m_SocketType(socketType)
        , m_Socket(INVALID_SOCKET)
        , m_syncAdmin()
        , m_State(0)
        , m_ReceivedNode()
        , m_SendBuffer(nullptr)
        , m_ReceiveBuffer(nullptr)
    {
        TRACE_L5("Constructor SocketPort (NodeId&) <0x%X>", TRACE_POINTER(this));
    }

    SocketPort::SocketPort(
        const enumType socketType,
        const SOCKET& refConnector,
        const NodeId& remoteNode,
        const uint16_t nSendBufferSize,
        const uint16_t nReceiveBufferSize)
        : m_LocalNode(remoteNode.AnyInterface())
        , m_RemoteNode(remoteNode)
        , m_ReceiveBufferSize(nReceiveBufferSize)
        , m_SendBufferSize(nSendBufferSize)
        , m_SocketType(socketType)
        , m_Socket(refConnector)
        , m_syncAdmin()
        , m_State(0)
        , m_ReceivedNode()
        , m_SendBuffer(nullptr)
        , m_ReceiveBuffer(nullptr)
    {
        NodeId::SocketInfo localAddress;
        socklen_t localSize = sizeof(localAddress);

        ASSERT(refConnector != INVALID_SOCKET);

        if ((SetNonBlocking(m_Socket) == false) || (::getsockname(m_Socket, (struct sockaddr*)&localAddress, &localSize) == SOCKET_ERROR)) {
            DestroySocket(m_Socket);

            TRACE_L5("Error on preparing the port for communication. Error %d", __ERRORRESULT__);
        } else {
            m_LocalNode = localAddress;

            BufferAlignment(m_Socket);

            m_State = SocketPort::LINK | SocketPort::OPEN | SocketPort::READ;
        }
    }

    SocketPort::~SocketPort()
    {
        TRACE_L5("Destructor SocketPort <0x%X>", TRACE_POINTER(this));

        // Make sure the socket is closed before you destruct. Otherwise
        // the virtuals might be called, which are destructed at this point !!!!
        ASSERT(m_Socket == INVALID_SOCKET);

        ::free(m_SendBuffer);
    }

    //////////////////////////////////////////////////////////////////////
    // PUBLIC SocketPort Interface
    //////////////////////////////////////////////////////////////////////

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

    bool SocketPort::Broadcast(const bool enabled)
    {
        uint32_t flag = (enabled ? 1 : 0);

        /* set the broadcast option - we need this to listen to broadcast messages */
        if (::setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&flag), sizeof(flag)) != 0) {
            TRACE_L1("Error: Could not set broadcast option on socket, error: %d\n", __ERRORRESULT__);
            return (false);
        }

        return (true);
    }

    /* virtual */ bool SocketPort::Initialize()
    {
        return (true);
    }

    uint32_t SocketPort::Open(const uint32_t waitTime, const string& specificInterface)
    {
        uint32_t nStatus = Core::ERROR_ILLEGAL_STATE;

        m_ReadBytes = 0;
        m_SendBytes = 0;
        m_SendOffset = 0;

        if ((m_State & (SocketPort::LINK | SocketPort::OPEN | SocketPort::MONITOR)) == (SocketPort::LINK | SocketPort::OPEN)) {
            // Open up an accepted socket, but not yet added to the monitor.
            nStatus = Core::ERROR_NONE;
        } else {
            ASSERT((m_Socket == INVALID_SOCKET) && (m_State == 0));

            if ((m_SocketType == SocketPort::STREAM) || (m_SocketType == SocketPort::SEQUENCED) || (m_SocketType == SocketPort::RAW)) {
                if (m_LocalNode.IsValid() == false) {
                    m_LocalNode = m_RemoteNode.Origin();
                }
            }

            ASSERT(((m_SocketType != LISTEN) || (m_LocalNode.IsValid() == true)) && ((m_SocketType != STREAM) || (m_RemoteNode.Type() == m_LocalNode.Type())));

            m_Socket = ConstructSocket(m_LocalNode, specificInterface);

            if ((m_Socket != INVALID_SOCKET) && (Initialize() == true)) {

                if ((m_SocketType == DATAGRAM) || ((m_SocketType == RAW) && (m_RemoteNode.IsValid() == false))) {
                    m_State = SocketPort::OPEN | SocketPort::READ;

                    nStatus = Core::ERROR_NONE;
                } else if (m_SocketType == LISTEN) {
                    if (::listen(m_Socket, MAX_LISTEN_QUEUE) == SOCKET_ERROR) {
                        TRACE_L5("Error on port socket LISTEN. Error %d", __ERRORRESULT__);
                    } else {
                        // Trigger state to Open
                        m_State = SocketPort::OPEN | SocketPort::ACCEPT;

                        nStatus = Core::ERROR_NONE;
                    }
                } else {
                    if (::connect(m_Socket, static_cast<const NodeId&>(m_RemoteNode), m_RemoteNode.Size()) != SOCKET_ERROR) {
                        m_State = SocketPort::LINK | SocketPort::OPEN | SocketPort::READ;
                        nStatus = Core::ERROR_NONE;
                    } else {
                        int l_Result = __ERRORRESULT__;

                        if ((l_Result == __ERROR_WOULDBLOCK__) || (l_Result == __ERROR_AGAIN__) || (l_Result == __ERROR_INPROGRESS__)) {
                            m_State = SocketPort::LINK | SocketPort::WRITE;
                            nStatus = Core::ERROR_INPROGRESS;
                        } else if (l_Result == __ERROR_ISCONN__) {
                            nStatus = Core::ERROR_ALREADY_CONNECTED;
                        } else if (l_Result == __ERROR_NETWORK_UNREACHABLE__) {
                            nStatus = Core::ERROR_UNREACHABLE_NETWORK;
                        } else {
                            nStatus = Core::ERROR_ASYNC_FAILED;
                        }
                    }
                }
            } else {
                nStatus = Core::ERROR_GENERAL;
            }
        }

        if ((nStatus == Core::ERROR_NONE) || (nStatus == Core::ERROR_INPROGRESS)) {
            m_State |= SocketPort::UPDATE;
            ResourceMonitor::Instance().Register(*this);

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

    uint32_t SocketPort::Close(const uint32_t waitTime)
    {

        // Make sure the state does not change in the mean time.
        m_syncAdmin.Lock();

        bool closed = IsClosed();

        if (m_Socket != INVALID_SOCKET) {

            if ((m_State != 0) && ((m_State & SHUTDOWN) == 0)) {

                if ((m_State & (LINK | OPEN)) != (LINK | OPEN)) {
                    // This is a connectionless link, do not expect a close from the otherside.
                    // No use to wait on anything !!, Signal a FORCED CLOSURE (EXCEPTION && SHUTDOWN)
                    m_State |= (SHUTDOWN | EXCEPTION);

                    ResourceMonitor::Instance().Break();
                } else {
                    m_State |= SHUTDOWN;

// Block new data from coming in, signal the other side that we close !!
#ifdef __LINUX__
                    shutdown(m_Socket, SHUT_RDWR);
#endif

#ifdef __WIN32__
                    shutdown(m_Socket, SD_BOTH);
#endif
                }
            }

            if (waitTime > 0) {
                closed = (WaitForClosure(waitTime) == Core::ERROR_NONE);

                if (closed == false) {
                    // Make this a forced close !!!
                    m_State |= EXCEPTION;

                    // We probably did not get a response from the otherside on the close
                    // sloppy but let's forcefully close it
                    ResourceMonitor::Instance().Break();

                    closed = (WaitForClosure(Core::infinite) == Core::ERROR_NONE);

                    ASSERT(closed == true);
                }
            }
        }

        m_syncAdmin.Unlock();

        return (closed ? Core::ERROR_NONE : Core::ERROR_PENDING_SHUTDOWN);
    }

    void SocketPort::Trigger()
    {
        m_syncAdmin.Lock();

        if ((m_State & (SocketPort::SHUTDOWN | SocketPort::OPEN | SocketPort::EXCEPTION)) == SocketPort::OPEN) {

            m_State |= SocketPort::WRITESLOT;
            ResourceMonitor::Instance().Break();
        }
        m_syncAdmin.Unlock();
    }

    //////////////////////////////////////////////////////////////////////
    // PRIVATE SocketPort interface
    //////////////////////////////////////////////////////////////////////

    void SocketPort::BufferAlignment(SOCKET socket)
    {
        socklen_t valueLength = sizeof(int);
        int value;
        uint32_t receiveBuffer = m_ReceiveBufferSize;
        uint32_t sendBuffer = m_SendBufferSize;

        if (m_ReceiveBufferSize == static_cast<uint16_t>(~0)) {
            ::getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char*)&value, &valueLength);

            receiveBuffer = static_cast<uint32_t>(value);

            TRACE_L1("Receive buffer size. %d", receiveBuffer);
        } else if ((receiveBuffer != 0) && (::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char*)&receiveBuffer, sizeof(receiveBuffer)) == SOCKET_ERROR)) {
            TRACE_L1("Error could not set Receive buffer size (%d).", receiveBuffer);
        }

        if (m_SendBufferSize == static_cast<uint16_t>(~0)) {
            ::getsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char*)&value, &valueLength);

            sendBuffer = static_cast<uint32_t>(value);

            TRACE_L1("Send buffer size. %d", sendBuffer);
        } else if ((sendBuffer != 0) && (::setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char*)&sendBuffer, sizeof(sendBuffer)) == SOCKET_ERROR)) {
            TRACE_L1("Error could not set Send buffer size (%d).", sendBuffer);
        }

        if ((receiveBuffer != 0) || (sendBuffer != 0)) {
            uint8_t* allocatedMemory = static_cast<uint8_t*>(::malloc(sendBuffer + receiveBuffer));
            if (sendBuffer != 0) {
                m_SendBuffer = allocatedMemory;
            }
            if (receiveBuffer != 0) {
                m_ReceiveBuffer = &(allocatedMemory[sendBuffer]);
            }
        }
    }

    SOCKET SocketPort::ConstructSocket(NodeId& localNode, const string& specificInterface)
    {
        ASSERT(localNode.IsValid() == true);

        SOCKET l_Result = INVALID_SOCKET;

#ifndef __WIN32__
        // Check if domain path already exists, if so remove.
        if ((localNode.Type() == NodeId::TYPE_DOMAIN) && (m_SocketType == SocketPort::LISTEN)) {
            if (access(localNode.HostName().c_str(), F_OK) != -1) {
                TRACE_L1("Found out domain path already exists, deleting: %s", localNode.HostName().c_str());
                remove(localNode.HostName().c_str());
            }
        }
#endif

        if ((l_Result = ::socket(localNode.Type(), SocketMode(), localNode.Extension())) == INVALID_SOCKET) {
            TRACE_L1("Error on creating socket SOCKET. Error %d", __ERRORRESULT__);
        } else if (SetNonBlocking(l_Result) == false) {
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

            ::setsockopt(l_Result, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, optionLength);
        }
#ifndef __WIN32__
        else if ((localNode.Type() == NodeId::TYPE_DOMAIN) && (m_SocketType == SocketPort::LISTEN)) {
            // The effect of SO_REUSEADDR  but then on Domain Sockets :-)
            if (unlink(localNode.HostName().c_str()) == -1) {
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
        if ((l_Result != INVALID_SOCKET) && (specificInterface.empty() == false)) {

            struct ifreq interface;
            strncpy(interface.ifr_ifrn.ifrn_name, specificInterface.c_str(), IFNAMSIZ);

            if (::setsockopt(l_Result, SOL_SOCKET, SO_BINDTODEVICE, (const char*)&interface, sizeof(interface)) < 0) {

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
            if ((SocketMode() != SOCK_STREAM) || (m_SocketType == SocketPort::LISTEN) || (((localNode.Type() == NodeId::TYPE_IPV4) || (localNode.Type() == NodeId::TYPE_IPV6)) && (localNode.PortNumber() != 0))) {
                if (::bind(l_Result, static_cast<const NodeId&>(localNode), localNode.Size()) != SOCKET_ERROR) {

#ifndef __WIN32__
                    if ((localNode.Type() == NodeId::TYPE_DOMAIN) && (localNode.Rights() <= 0777)) {
                        if (::chmod(localNode.HostName().c_str(), localNode.Rights()) == 0) {
                            BufferAlignment(l_Result);
                            return (l_Result);
                        } else {
                            TRACE_L1("Error on port socket CHMOD. Error %d", __ERRORRESULT__);
                        }
                    } else
#endif
                    {
                        BufferAlignment(l_Result);
                        return (l_Result);
                    }
                } else {
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

    uint32_t SocketPort::WaitForOpen(const uint32_t time) const
    {

        // Make sure the state does not change in the mean time.
        m_syncAdmin.Lock();

        uint32_t waiting = (time == Core::infinite ? Core::infinite : time); // Expect time in MS.

        // Right, a wait till connection is closed is requested..
        while ((waiting > 0) && (IsOpen() == false)) {
            // Make sure we aren't in the monitor thread waiting for close completion.
            ASSERT(Core::Thread::ThreadId() != ResourceMonitor::Instance().Id());

            uint32_t sleepSlot = (waiting > SLEEPSLOT_TIME ? SLEEPSLOT_TIME : waiting);

            m_syncAdmin.Unlock();

            // Right, lets sleep in slices of 100 ms
            SleepMs(sleepSlot);

            m_syncAdmin.Lock();

            waiting -= (waiting == Core::infinite ? 0 : sleepSlot);
        }

        uint32_t result = (((time == 0) || (IsOpen() == true)) ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);

        m_syncAdmin.Unlock();

        return (result);
    }

    uint32_t SocketPort::WaitForClosure(const uint32_t time) const
    {
        // If we build in release, we do not want to "hang" forever, forcefull close after 20S waiting...
#ifdef __DEBUG__
        uint32_t waiting = time; // Expect time in MS
        uint32_t reportSlot = 0;
#else
        uint32_t waiting = (time == Core::infinite ? 20000 : time);
#endif

        // Right, a wait till connection is closed is requested..
        while ((waiting > 0) && (IsClosed() == false)) {
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
        return (IsClosed() ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
    }

    uint16_t SocketPort::Events()
    {
        uint16_t result = 0;

        if (m_State != 0) {
#ifdef __WIN32__
            result = FD_CLOSE;
#else
            result = POLLIN;
#endif

            // It is the first time we are going to pick this one up..
            if ((m_State & SocketPort::MONITOR) == 0) {
                m_State |= SocketPort::MONITOR;

                if ((m_State & (SocketPort::OPEN | SocketPort::ACCEPT)) == SocketPort::OPEN) {
                    Opened();
                }
            }

            if ((m_State & UPDATE) != 0) {
                m_State ^= UPDATE;

#ifdef __WIN32__
                result |= 0x8000 | ((m_State & SocketPort::ACCEPT) != 0 ? FD_ACCEPT : ((m_State & SocketPort::OPEN) != 0 ? FD_READ | FD_WRITE : FD_CONNECT));
#endif
            }

            if ((IsForcedClosing() == true) && (Closed() == true)) {
                result = 0;
                m_State &= ~SocketPort::MONITOR;
            } else {

                if ((IsOpen()) && ((m_State & SocketPort::WRITESLOT) != 0)) {
                    Write();
                }
#ifdef __LINUX__
                result |= ((m_State & SocketPort::LINK) != 0 ? POLLHUP : 0) | ((m_State & SocketPort::WRITE) != 0 ? POLLOUT : 0);
#endif
            }
        }

        return (result);
    }

    void SocketPort::Handle(const uint16_t flagsSet)
    {

        bool breakIssued = ((m_State & SocketPort::WRITESLOT) != 0);

        if ((flagsSet != 0) || (breakIssued == true)) {

#ifdef __WIN32__
            if ((flagsSet & FD_CLOSE) != 0) {
                Closed();
            } else if (IsListening()) {
                if ((flagsSet & FD_ACCEPT) != 0) {
                    // This triggeres an Addition of clients
                    Accepted();
                }
            } else if (IsOpen()) {
                if (((flagsSet & FD_WRITE) != 0) || (breakIssued == true)) {
                    Write();
                }
                if ((flagsSet & FD_READ) != 0) {
                    Read();
                }
            } else if ((flagsSet & FD_CONNECT) != 0) {
                Opened();
                m_State |= UPDATE;
            }

#else
            if ((flagsSet & POLLHUP) != 0) {
                Closed();
            } else if (IsListening()) {
                if ((flagsSet & POLLIN) != 0) {
                    // This triggeres an Addition of clients
                    Accepted();
                }
            } else if (IsOpen()) {
                if (((flagsSet & POLLOUT) != 0) || (breakIssued == true)) {
                    Write();
                }
                if ((flagsSet & POLLIN) != 0) {
                    Read();
                }
            } else if ((IsConnecting() == true) && ((flagsSet & POLLOUT) != 0)) {
                Opened();
            }
#endif
        }
    }

    void SocketPort::Write()
    {
        bool dataLeftToSend = true;

        m_syncAdmin.Lock();

        m_State &= (~(SocketPort::WRITE | SocketPort::WRITESLOT));

        while (((m_State & (SocketPort::WRITE | SocketPort::SHUTDOWN | SocketPort::OPEN | SocketPort::EXCEPTION)) == SocketPort::OPEN) && (dataLeftToSend == true)) {
            if (m_SendOffset == m_SendBytes) {
                m_SendBytes = SendData(m_SendBuffer, m_SendBufferSize);
                m_SendOffset = 0;
                dataLeftToSend = (m_SendOffset != m_SendBytes);

                ASSERT(m_SendBytes <= m_SendBufferSize);
            }

            if (dataLeftToSend == true) {
                int32_t sendSize;

                // Sockets are non blocking the Send buffer size is equal to the buffer size. We only send
                // if the buffer free (SEND flag) is active, so the buffer should always fit.
                if (((m_State & SocketPort::LINK) == 0) && (m_RemoteNode.IsValid() == true)) {
                    ASSERT(m_RemoteNode.IsValid() == true);

                    sendSize = ::sendto(m_Socket,
                        reinterpret_cast<const char*>(&m_SendBuffer[m_SendOffset]),
                        m_SendBytes - m_SendOffset, 0,
                        static_cast<const NodeId&>(m_RemoteNode),
                        m_RemoteNode.Size());

                } else {
                    sendSize = ::send(m_Socket,
                        reinterpret_cast<const char*>(&m_SendBuffer[m_SendOffset]),
                        m_SendBytes - m_SendOffset, 0);
                }

                if (sendSize >= 0) {
                    m_SendOffset = ((m_State & SocketPort::LINK) != 0 ? m_SendOffset + sendSize : m_SendBytes);
                } else {
                    uint32_t l_Result = __ERRORRESULT__;

                    if ((l_Result == __ERROR_WOULDBLOCK__) || (l_Result == __ERROR_AGAIN__) || (l_Result == __ERROR_INPROGRESS__)) {
                        m_State |= SocketPort::WRITE;
                    } else {
                        printf("Write exception. %d\n", l_Result);
                        m_State |= SocketPort::EXCEPTION;
                        StateChange();
                    }
                }
            }
        }

        m_syncAdmin.Unlock();
    }

    void SocketPort::Read()
    {
        m_syncAdmin.Lock();

        m_State &= (~SocketPort::READ);

        while ((m_State & (SocketPort::READ | SocketPort::EXCEPTION | SocketPort::OPEN)) == SocketPort::OPEN) {
            uint32_t l_Size;

            if (m_ReadBytes == m_ReceiveBufferSize) {
                m_ReadBytes = 0;
            }

            // Read the actual data from the port.
            if (((m_State & SocketPort::LINK) == 0) && (m_LocalNode.Type() != NodeId::TYPE_NETLINK)) {
                NodeId::SocketInfo l_Remote;
                socklen_t l_Address = sizeof(l_Remote);

                l_Size = ::recvfrom(m_Socket,
                    reinterpret_cast<char*>(&m_ReceiveBuffer[m_ReadBytes]),
                    m_ReceiveBufferSize - m_ReadBytes, 0, (struct sockaddr*)&l_Remote,
                    &l_Address);

                m_ReceivedNode = l_Remote;
            } else {
                l_Size = ::recv(m_Socket,
                    reinterpret_cast<char*>(&m_ReceiveBuffer[m_ReadBytes]),
                    m_ReceiveBufferSize - m_ReadBytes, 0);
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
                    printf("Read exception. %d\n", l_Result);
                }
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

    bool SocketPort::Closed()
    {
        bool result = true;

        ASSERT(m_Socket != INVALID_SOCKET);

        m_syncAdmin.Lock();

        // Turn them all off, except for the SHUTDOWN bit, to show whether this was
        // done on our request, or closed from the other side...
        m_State &= SHUTDOWN;

        StateChange();

        m_State &= (~SHUTDOWN);

        if (m_State != 0) {
            result = false;
        } else {
            DestroySocket(m_Socket);
            // Remove socket descriptor for UNIX domain datagram socket.
            if ((m_LocalNode.Type() == NodeId::TYPE_DOMAIN) && ((m_SocketType == SocketPort::LISTEN) || (SocketMode() != SOCK_STREAM))) {
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

    void SocketPort::Opened()
    {
        m_syncAdmin.Lock();
        m_State = (m_State & (~SocketPort::WRITE)) | SocketPort::OPEN;
        StateChange();
        m_syncAdmin.Unlock();
    }

    void SocketPort::Accepted()
    {
        m_syncAdmin.Lock();
        StateChange();
        m_syncAdmin.Unlock();
    }

    SOCKET SocketPort::Accept(NodeId& remoteId)
    {
        NodeId::SocketInfo address;
        socklen_t size = sizeof(address);
        SOCKET result;

        if ((result = ::accept(m_Socket, (struct sockaddr*)&address, &size)) != SOCKET_ERROR) {
            // Align the buffer to what is requested
            BufferAlignment(result);

            remoteId = address;
        } else {
            int error = __ERRORRESULT__;
            if ((error != __ERROR_AGAIN__) && (error != __ERROR_WOULDBLOCK__)) {
                TRACE_L1("Error could not accept a connection. Error: %d.", error);
            }
            result = INVALID_SOCKET;
        }

        return (result);
    }

    NodeId SocketPort::Accept()
    {
        NodeId newConnection;
        SOCKET result;

        if (((result = Accept(newConnection)) != INVALID_SOCKET) && (SetNonBlocking(result) == true)) {
            DestroySocket(m_Socket);

            m_Socket = result;
            m_State = (SocketPort::UPDATE | SocketPort::MONITOR | SocketPort::LINK | SocketPort::OPEN | SocketPort::READ | SocketPort::WRITESLOT);
        }

        return (newConnection);
    }

    void SocketPort::Listen()
    {
        string emptyString;

        // Switching to listen is only allowed if the connection is closed
        // and this is a listning socket from origin !!!!
        ASSERT(m_SocketType == SocketPort::LISTEN);
        ASSERT(m_State == 0);
        ASSERT(m_Socket != INVALID_SOCKET);

        // Current socket can be destroyed
        DestroySocket(m_Socket);

        m_Socket = ConstructSocket(m_LocalNode, emptyString);

        if (m_Socket != INVALID_SOCKET) {
            NodeId remoteId;

            if (::listen(m_Socket, MAX_LISTEN_QUEUE) == SOCKET_ERROR) {
                TRACE_L5("Error on port socket LISTEN. Error %d", __ERRORRESULT__);
            } else {
                // Trigger state to Open
                m_State = SocketPort::UPDATE | SocketPort::MONITOR | SocketPort::OPEN | SocketPort::ACCEPT;
            }
        }
    }

    bool SocketPort::Join(const NodeId& multicastAddress)
    {
        const NodeId::SocketInfo& inputInfo = multicastAddress;

        ASSERT(inputInfo.IPV4Socket.sin_family == AF_INET);

        ip_mreq multicastRequest;

        multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
        multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;

        if (::setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
            TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
            return (false);
        }

        return (true);
    }

    bool SocketPort::Leave(const NodeId& multicastAddress)
    {
        const NodeId::SocketInfo& inputInfo = multicastAddress;

        ASSERT(inputInfo.IPV4Socket.sin_family == AF_INET);

        ip_mreq multicastRequest;

        multicastRequest.imr_interface = static_cast<const NodeId::SocketInfo&>(LocalNode()).IPV4Socket.sin_addr;
        multicastRequest.imr_multiaddr = inputInfo.IPV4Socket.sin_addr;

        if (::setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
            TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
            return (false);
        }

        return (true);
    }

    bool SocketPort::Join(const NodeId& multicastAddress, const NodeId& source)
    {
        const NodeId::SocketInfo& inputInfo = multicastAddress;

        ASSERT(inputInfo.IPV4Socket.sin_family == AF_INET);

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

        if (::setsockopt(m_Socket, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
            TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
            return (false);
        }

        return (true);
    }

    bool SocketPort::Leave(const NodeId& multicastAddress, const NodeId& source)
    {
        const NodeId::SocketInfo& inputInfo = multicastAddress;

        ASSERT(inputInfo.IPV4Socket.sin_family == AF_INET);

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

        if (::setsockopt(m_Socket, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP, reinterpret_cast<const char*>(&multicastRequest), sizeof(multicastRequest)) == SOCKET_ERROR) {
            TRACE_L1("Error could not join a multicast address. Error: %d.", __ERRORRESULT__);
            return (false);
        }

        return (true);
    }

    SocketDatagram::SocketDatagram(const bool rawSocket,
        const NodeId& localNode,
        const NodeId& remoteNode,
        const uint16_t sendBufferSize,
        const uint16_t receiveBufferSize)
        : SocketPort((rawSocket ? SocketPort::RAW : SocketPort::DATAGRAM), localNode, remoteNode, sendBufferSize, receiveBufferSize)
    {
    }

    /* virtual */ SocketDatagram::~SocketDatagram()
    {
    }
}
} // namespace Solution::Core

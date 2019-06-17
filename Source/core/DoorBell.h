#pragma once 

#include "Module.h"
#include "Sync.h"
#include "ResourceMonitor.h"
#include "SocketPort.h"

namespace WPEFramework {
namespace Core {

class EXTERNAL DoorBell {
private:
    class EXTERNAL Connector : public IResource {
    private:
        static constexpr uint8_t SIGNAL_DOORBELL = 0xAA;

    public:
        Connector () = delete;
        Connector (const Connector&) = delete;
        Connector& operator=(const Connector&) = delete;

        Connector(Connector& parent, const Core::NodeId& node)
            : _parent(parent)
            , _doorbell(node)
            , _socket(::socket(_doorbell.Type(), SOCK_DGRAM, 0))
            , _bound(false) {
        }
        virtual ~Connector() {
            if (_bound == true) {
                ResourceMonitor::Instance().Unregister(*this);
            }
        }

    public:
        // _bound is not expected to be thread safe, as this is only called once, by a single thread 
        // who will always do the waiting  for the doorbell to be rang!!
        bool Bind()
        {
            if (_bound == false) {

                #ifndef __WIN32__
                // Check if domain path already exists, if so remove.
                if (_doorbell.Type() == NodeId::TYPE_DOMAIN) {
                    if (access(_doorbell.HostName().c_str(), F_OK) != -1) {
                        TRACE_L1("Found out domain path already exists, deleting: %s", _doorbell.HostName().c_str());
                        remove(_doorbell.HostName().c_str());
                    }
                }
                #endif
                if ((_doorbell.Type() == NodeId::TYPE_IPV4) || (_doorbell.Type() == NodeId::TYPE_IPV6)) {
                    // set SO_REUSEADDR on a socket to true (1): allows other sockets to bind() to this
                    // port, unless there is an active listening socket bound to the port already. This
                    // enables you to get around those "Address already in use" error messages when you
                    // try to restart your server after a crash.
                    int optval = 1;
                    socklen_t optionLength = sizeof(int);

                    ::setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, optionLength);
                }

                #ifdef __WIN32__
                unsigned long l_Value = 1;
                if (ioctlsocket(_socket, FIONBIO, &l_Value) != 0) {
                    TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", __ERRORRESULT__);
                } else {
                    _bound = true;
                }
                #else
                if (fcntl(_socket, F_SETOWN, getpid()) == -1) {
                    TRACE_L1("Setting Process ID failed. <%d>", __ERRORRESULT__);
                } else {
                    int flags = fcntl(_socket, F_GETFL, 0) | O_NONBLOCK;

                    if (fcntl(_socket, F_SETFL, flags) != 0) {
                        TRACE_L1("Error on port socket F_SETFL call. Error %d", __ERRORRESULT__);
                    } else {
                        _bound = true;
                    }
                }
                #endif

                if ((_bound == true) && (::bind(_socket, static_cast<const NodeId&>(_doorbell), _doorbell.Size()) != SOCKET_ERROR)) {

                    #ifndef __WIN32__
                    if ((_doorbell.Type() == NodeId::TYPE_DOMAIN) && (_doorbell.Rights() <= 0777)) {
                        _bound = (::chmod(_doorbell.HostName().c_str(), _doorbell.Rights()) == 0);
                    }
                    #endif

                    if (_bound == true) {
                        ResourceMonitor::Instance().Register(*this);
                    }
                }
                else {
                    _bound = false;
                }
            }    

            return (_bound);
        }
        bool Ring() 
        {
            const char message[] = { SIGNAL_DOORBELL };
            int sendSize = ::sendto(_socket, message, sizeof(message), 0, _doorbell, _doorbell.Size());
            return (sendSize == sizeof(message));
        }

    private:
        virtual IResource::handle Descriptor() const override
        {
            return (static_cast<IResource::handle>(_socket));
        }
        virtual uint16_t Events() override
        {
            #ifdef __WIN32__
            return (FD_READ);
            #else
            return (POLLIN);
            #endif
        }
        virtual void Handle(const uint16_t events) override
        {
            #ifdef __WIN32__
            if ((flagsSet & FD_READ) != 0) {
                Read();
            }
            #else
            if ((flagsSet & POLLIN) != 0) {
                Read();
            }
            #endif
        }
        void Read() {
            int size;
            char buffer[16];

            do {
                int size = ::recv(_socket, buffer, sizeof(buffer), 0);

                if (size != SOCKET_ERROR) {
                    for(int index = 0; index < size; index++)
                    {
                        if (dataFrame[index] == SIGNAL_DOORBELL) {
                            _parent.Ringing();
                        }
                    }
                }

            } while (size == sizeof(buffer));
        }

    private:
        Connector& _parent;
        Core::NodeId& _doorbell;
        SOCKET _socket;
        bool _bound;
    };

public:
    DoorBell() = delete;
    DoorBell(const DoorBell&) = delete;
    DoorBell& operator=(const DoorBell&) = delete;

    DoorBell(const TCHAR sourceName[]) 
        : _connectPoint(*this, Core::NodeId(sourceName)) 
        , _signal(false, true)
    {
    }
    ~DoorBell()
    {
    }

public:
    void Ring() {
        _connectPoint.Ring();
    }
    void Acknowledge() {
        _signal.ResetEvent()
    }
    uint32_t Wait(const uint32_t waitTime) const {

        uint32_t result = ERROR_UNAVAILABLE;

        if (_connectPoint.Bind() == true) {
            result = _signal.Lock(waitTime);
        }

        return (result);
    }

private:
    void Ringing() {
        _signal.SetEvent();
    }

private:
    Connection _connectPoint;
    Core::Event _signal;
};

}
} // namespace Core

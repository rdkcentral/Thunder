 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "AccessControl.h"
#include "DoorBell.h"

#ifdef __WINDOWS__
#include <Winsock2.h>
#include <ws2tcpip.h>
#define __ERRORRESULT__ ::WSAGetLastError()
#else
#define __ERRORRESULT__ errno
#endif

namespace WPEFramework {

namespace Core {

    DoorBell::Connector::Connector(DoorBell& parent, const Core::NodeId& node)
        : _parent(parent)
        , _doorbell(node)
        , _sendSocket(::socket(_doorbell.Type(), SOCK_DGRAM|SOCK_CLOEXEC, 0))
        , _receiveSocket(INVALID_SOCKET)
        , _registered(0)
    {
        #ifdef __WINDOWS__
        unsigned long l_Value = 1;
        if (ioctlsocket(_sendSocket, FIONBIO, &l_Value) != 0) {
            TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", ::WSAGetLastError());
        }
        #else
        int flags = fcntl(_sendSocket, F_GETFL, 0) | O_NONBLOCK;
        if (fcntl(_sendSocket, F_SETFL, flags) != 0) {
            TRACE_L1("SendSocket:Error on port socket F_SETFL call. Error %d", errno);
        }
        #endif  
    }
    /* virtual */ DoorBell::Connector::~Connector()
    {
        if (_receiveSocket != INVALID_SOCKET) {
            Unbind();

#ifdef __WINDOWS__
            ::closesocket(_receiveSocket);
#else
            ::close(_receiveSocket);
#endif
        }
#ifdef __WINDOWS__
        ::closesocket(_sendSocket);
#else
        ::close(_sendSocket);
#endif
    }

    bool DoorBell::Connector::Bind() const
    {
        if (_receiveSocket == INVALID_SOCKET) {
            _receiveSocket = ::socket(_doorbell.Type(), SOCK_DGRAM|SOCK_CLOEXEC, 0);

#ifndef __WINDOWS__
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

                ::setsockopt(_receiveSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, optionLength);
            }

#ifdef __WINDOWS__
            unsigned long l_Value = 1;
            if (ioctlsocket(_receiveSocket, FIONBIO, &l_Value) != 0) {
                TRACE_L1("Error on port socket NON_BLOCKING call. Error %d", ::WSAGetLastError());
                ::closesocket(_receiveSocket);
                _receiveSocket = INVALID_SOCKET;
            }
#else
            if (fcntl(_receiveSocket, F_SETOWN, getpid()) == -1) {
                TRACE_L1("Setting Process ID failed. <%d>", errno);
                ::close(_receiveSocket);
                _receiveSocket = INVALID_SOCKET;
            }
            else {
                int flags = fcntl(_receiveSocket, F_GETFL, 0) | O_NONBLOCK;

                if (fcntl(_receiveSocket, F_SETFL, flags) != 0) {
                    TRACE_L1("Error on port socket F_SETFL call. Error %d", errno);
                    ::close(_receiveSocket);
                    _receiveSocket = INVALID_SOCKET;
                }
            }
#endif

            if (_receiveSocket != INVALID_SOCKET) {

                if (::bind(_receiveSocket, static_cast<const NodeId&>(_doorbell), _doorbell.Size()) == SOCKET_ERROR) {
                    TRACE_L1("Error on socket bind for the doorbell. Error %d", __ERRORRESULT__);
                }
                else {
#ifndef __WINDOWS__
                    if (_doorbell.Type() == NodeId::TYPE_DOMAIN) {
                        if (AccessControl::Apply(_doorbell) != Core::ERROR_NONE) {
                            ::close(_receiveSocket);
                            _receiveSocket = INVALID_SOCKET;
                        }
                    }
#endif

                    if (_receiveSocket != INVALID_SOCKET) {
                        _registered = SocketPort::enumState::UPDATE | 0x01;
                        ResourceMonitor::Instance().Register(*const_cast<Connector*>(this));
                    }
                }
            }
        }
        else if ((_registered & 0x01) == 0) {
            _registered = SocketPort::enumState::UPDATE | 0x01;
            ResourceMonitor::Instance().Register(*const_cast<Connector*>(this));
        }

        return ((_registered & 0x01) != 0);
    }

    void DoorBell::Connector::Unbind() const
    {
        if ((_registered & 0x01) != 0) {
            _registered = SocketPort::enumState::UPDATE;
            ResourceMonitor::Instance().Unregister(*const_cast<Connector*>(this));
        }
	}

    /* virtual */ IResource::handle DoorBell::Connector::Descriptor() const
    {
        return (static_cast<IResource::handle>(_receiveSocket));
    }

    /* virtual */ uint16_t DoorBell::Connector::Events()
    {
#ifdef __WINDOWS__
        uint16_t result = ((_registered & SocketPort::enumState::UPDATE) | FD_READ);
        _registered &= ~SocketPort::enumState::UPDATE;
        return (result);
#else
        return (POLLIN);
#endif
    }

    /* virtual */ void DoorBell::Connector::Handle(const uint16_t events)
    {
#ifdef __WINDOWS__
        if ((events & FD_READ) != 0) {
            Read();
        }
#else
        if ((events & POLLIN) != 0) {
            Read();
        }
#endif
    }

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
    DoorBell::DoorBell(const TCHAR sourceName[])
        : _connectPoint(*this, Core::NodeId(sourceName))
        , _signal(false, true)
    {
    }
POP_WARNING()

    DoorBell::~DoorBell()
    {
        _signal.SetEvent();
    }
}
}

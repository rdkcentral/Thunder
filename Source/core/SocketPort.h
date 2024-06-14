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

// SocketPort.h: interface for the SocketPort class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __SOCKETPORT_H
#define __SOCKETPORT_H

#include "Module.h"
#include "NodeId.h"
#include "Portability.h"
#include "ResourceMonitor.h"
#include "StateTrigger.h"

#ifdef __WINDOWS__
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef __UNIX__
#define SOCKET signed int
#define SOCKET_ERROR static_cast<signed int>(-1)
#define INVALID_SOCKET static_cast<SOCKET>(-1)
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace Thunder {
    namespace Core {
        class EXTERNAL SocketPort : public IResource {
        private:
            // -------------------------------------------------------------------------
            // This object should not be copied, assigned or created with a default
            // constructor. Prevent them from being used, generatoed by the compiler.
            // define them but do not implement them. Compile error and/or link error.
            // -------------------------------------------------------------------------
            SocketPort(const SocketPort& a_RHS) = delete;
            SocketPort& operator=(const SocketPort& a_RHS) = delete;

        public:
            typedef enum {
                READ = 0x001,
                WRITE = 0x002,
                ACCEPT = 0x004,
                SHUTDOWN = 0x008,
                OPEN = 0x010,
                EXCEPTION = 0x020,
                LINK = 0x040,
                MONITOR = 0x080,
                WRITESLOT = 0x100,
                UPDATE = 0x8000

            } enumState;

            typedef enum {
                DATAGRAM,
                STREAM,
                LISTEN,
                RAW,
                SEQUENCED

            } enumType;

        public:
            SocketPort(const enumType socketType,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize);

            SocketPort(const enumType socketType,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize);

            SocketPort(const enumType socketType,
                const SOCKET& connector,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize);

            SocketPort(const enumType socketType,
                const SOCKET& connector,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize);

            ~SocketPort() override;

        public:
            inline uint16_t State() const
            {
                return (m_State.load(Core::memory_order::memory_order_relaxed));
            }
            inline void RemoteNode(const NodeId& remote)
            {
                ASSERT((IsOpen() == false) || (m_SocketType == DATAGRAM));

                m_syncAdmin.Lock();
                m_RemoteNode = remote;
                m_syncAdmin.Unlock();
            }
            inline enumType Type() const
            {
                return (m_SocketType);
            }
            inline bool IsListening() const
            {
                return ((State() & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION | SocketPort::OPEN | SocketPort::LINK | SocketPort::ACCEPT)) == (SocketPort::OPEN | SocketPort::ACCEPT));
            }
            inline bool IsConnecting() const
            {
                return ((State() & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION | SocketPort::OPEN | SocketPort::LINK | SocketPort::ACCEPT)) == SocketPort::LINK);
            }
            inline bool IsSuspended() const
            {
                return ((State() & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION)) == SocketPort::SHUTDOWN);
            }
            inline bool IsForcedClosing() const
            {
                uint16_t state = State();
                return (((state & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION)) == (SocketPort::SHUTDOWN | SocketPort::EXCEPTION)) || ((state & (SocketPort::OPEN | SocketPort::EXCEPTION)) == (SocketPort::EXCEPTION)));
            }
            inline bool IsOpen() const
            {
                return ((State() & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION | SocketPort::OPEN)) == SocketPort::OPEN);
            }
            inline bool IsClosed() const
            {
                return (State() == 0);
            }
            inline bool HasError() const
            {
                return ((State() & (SocketPort::SHUTDOWN | SocketPort::EXCEPTION)) == SocketPort::EXCEPTION);
            }
            inline bool operator==(const SocketPort& RHS) const
            {
                return (m_Socket == RHS.m_Socket);
            }
            inline bool operator!=(const SocketPort& RHS) const
            {
                return (!operator==(RHS));
            }
            inline const NodeId& LocalNode() const
            {
                return (m_LocalNode);
            }
            inline void LocalNode(const Core::NodeId& node)
            {
                m_LocalNode = node;
            }
            inline const NodeId& RemoteNode() const
            {
                return (m_RemoteNode);
            }
            inline string LocalId() const
            {
                return (m_LocalNode.HostAddress());
            }
            inline string RemoteId() const
            {
                return (m_RemoteNode.HostAddress());
            }
            inline const NodeId& ReceivedNode() const
            {
                return (m_ReceivedNode);
            }
            inline uint32_t ReceivedInterface() const {
                return (m_Interface);
            }
            inline uint16_t SendBufferSize() const
            {
                return (m_SendBufferSize);
            }
            inline uint16_t ReceiveBufferSize() const
            {
                return (m_ReceiveBufferSize);
            }
            inline uint32_t SocketSendBufferSize() const
            {
                return (m_SocketSendBufferSize);
            }
            inline uint32_t SocketReceiveBufferSize() const
            {
                return (m_SocketReceiveBufferSize);
            }
            inline void Flush()
            {
                m_syncAdmin.Lock();
                m_ReadBytes = 0;
                m_SendBytes = 0;
                m_SendOffset = 0;
                m_syncAdmin.Unlock();
            }

            uint32_t TTL() const;
            uint32_t TTL(const uint8_t value);

            bool Broadcast(const bool enabled);

            bool Join(const NodeId& multicastAddress);
            bool Leave(const NodeId& multicastAddress);
            bool Join(const NodeId& multicastAddress, const NodeId& source);
            bool Leave(const NodeId& multicastAddress, const NodeId& source);
            inline uint32_t Open(const uint32_t waitTime)
            {
                return (Open(waitTime, emptyString));
            }
            uint32_t Open(const uint32_t waitTime, const string& specificInterface);
            uint32_t Close(const uint32_t waitTime);
            void Trigger();

            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange() = 0;

            // In case of a single connection should be accepted, these methods help
            // changing the socket from a Listening socket to a connected socket and
            // back in case the socket closes.
            // REMARK: These methods should ONLY be called on the StateChange to
            //         Accepted (Accept) and the StateChange to Closed (Listen).
            NodeId Accept();
            void Listen();
            SOCKET Accept(NodeId& remoteId);

        protected:
            virtual uint32_t Initialize();
            virtual int32_t Read(uint8_t buffer[], const uint16_t length) const;
            virtual int32_t Write(const uint8_t buffer[], const uint16_t length);
            void SetError() {
                m_State |= SocketPort::EXCEPTION;
            }
            void Lock() const {
                m_syncAdmin.Lock();
            }
            void Unlock() const {
                m_syncAdmin.Unlock();
            }

        private:
            IResource::handle Descriptor() const override
            {
                return (static_cast<IResource::handle>(m_Socket));
            }
            inline uint32_t SocketMode() const
            {
                return (((m_SocketType == LISTEN) || (m_SocketType == STREAM)) ? SOCK_STREAM : ((m_SocketType == DATAGRAM) ? SOCK_DGRAM : (m_SocketType == SEQUENCED ? SOCK_SEQPACKET : SOCK_RAW)));
            }
            uint16_t Events() override;
            void Handle(const uint16_t events) override;
            bool Closed();
            void Opened();
            void Accepted();
            void Read();
            void Write();
            void BufferAlignment(SOCKET socket);
            SOCKET ConstructSocket(NodeId& localNode, const string& interfaceName);
            uint32_t WaitForOpen(const uint32_t time) const;
            uint32_t WaitForClosure(const uint32_t time) const;
            uint32_t WaitForWriteComplete(const uint32_t time) const;

        private:
            NodeId m_LocalNode;
            NodeId m_RemoteNode;
            uint16_t m_ReceiveBufferSize;
            uint16_t m_SendBufferSize;
            uint32_t m_SocketReceiveBufferSize;
            uint32_t m_SocketSendBufferSize;
            enumType m_SocketType;
            SOCKET m_Socket;
            mutable CriticalSection m_syncAdmin;
            std::atomic<uint16_t> m_State;
            NodeId m_ReceivedNode;
            uint8_t* m_SendBuffer;
            uint8_t* m_ReceiveBuffer;
            uint16_t m_ReadBytes;
            uint16_t m_SendBytes;
            uint16_t m_SendOffset;
            uint32_t m_Interface;
            bool m_SystemdSocket;
        };

        class EXTERNAL SocketStream : public SocketPort {
        private:
            SocketStream() = delete;
            SocketStream(const SocketStream&) = delete;
            SocketStream& operator=(const SocketStream&) = delete;

        public:
            SocketStream(const bool rawSocket,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize)
                : SocketStream(rawSocket, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
            {
            }

            SocketStream(const bool rawSocket,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize)
                : SocketPort((rawSocket ? SocketPort::RAW : SocketPort::STREAM), localNode, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize)
            {
            }

            SocketStream(const bool rawSocket,
                const SOCKET& connector,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize)
                : SocketStream(rawSocket, connector, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
            {
            }

            SocketStream(const bool rawSocket,
                const SOCKET& connector,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize)
                : SocketPort((rawSocket ? SocketPort::RAW : SocketPort::STREAM), connector, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize)
            {
            }

            ~SocketStream() override = default;

        public:
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange() = 0;
        };

        class EXTERNAL SocketDatagram : public SocketPort {
        private:
            SocketDatagram() = delete;
            SocketDatagram(const SocketDatagram&) = delete;
            SocketDatagram& operator=(const SocketDatagram&) = delete;

        public:
            SocketDatagram(const bool rawSocket,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize)
                : SocketDatagram(rawSocket, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
            {
            }

            SocketDatagram(const bool rawSocket,
                const NodeId& localNode,
                const NodeId& remoteNode,
                const uint16_t sendBufferSize,
                const uint16_t receiveBufferSize,
                const uint32_t socketSendBufferSize,
                const uint32_t socketReceiveBufferSize)
                : SocketPort((rawSocket ? SocketPort::RAW : SocketPort::DATAGRAM), localNode, remoteNode, sendBufferSize, receiveBufferSize, socketSendBufferSize, socketReceiveBufferSize)
            {
            }

            ~SocketDatagram() override
            {
            }

        public:
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) = 0;
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) = 0;

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange() = 0;
        };

        class EXTERNAL SocketListner {
        private:
            class EXTERNAL Handler : public SocketPort {
            private:
                Handler() = delete;
                Handler(const Handler&) = delete;
                Handler& operator=(const Handler&) = delete;

            public:
                Handler(SocketListner& parent)
                    : SocketPort(SocketPort::LISTEN, Core::NodeId(), Core::NodeId(), 0, 0)
                    , _parent(parent)
                {
                }
                Handler(SocketListner& parent, const NodeId& refLocalNode)
                    : SocketPort(SocketPort::LISTEN, refLocalNode, refLocalNode.AnyInterface(), 0, 0)
                    , _parent(parent)
                {
                }
                ~Handler()
                {
                }

            public:
                inline void LocalNode(const Core::NodeId& localNode)
                {
                    SocketPort::LocalNode(localNode);
                }
                virtual uint16_t SendData(uint8_t* /* dataFrame */, const uint16_t /* maxSendSize */)
                {
                    // This should not happen on this socket !!!!!
                    ASSERT(false);

                    return (0);
                }
                virtual uint16_t ReceiveData(uint8_t* /* dataFrame */, const uint16_t /* receivedSize */)
                {
                    // This should not happen on this socket !!!!!
                    ASSERT(false);

                    return (0);
                }
                virtual void StateChange()
                {
                    SOCKET newClient;
                    NodeId remoteId;

                    while ((newClient = SocketPort::Accept(remoteId)) != INVALID_SOCKET) {
                        _parent.Accept(newClient, remoteId);
                    }
                }

            private:
                SocketListner& _parent;
            };

            // -------------------------------------------------------------------------
            // This object should not be copied, assigned or created with a default
            // constructor. Prevent them from being used, generatoed by the compiler.
            // define them but do not implement them. Compile error and/or link error.
            // -------------------------------------------------------------------------
            SocketListner(const SocketListner& a_RHS) = delete;
            SocketListner& operator=(const SocketListner& a_RHS) = delete;

        protected:
            PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
                SocketListner()
                : _socket(*this)
            {
                TRACE_L5("Constructor SocketListner <%p>", (this));
            }

        public:
            SocketListner(const NodeId& refLocalNode)
                : _socket(*this, refLocalNode)
            {
                TRACE_L5("Constructor SocketListner <%p>", (this));
            }
            POP_WARNING()

                virtual ~SocketListner()
            {
                TRACE_L5("Destructor SocketListner <%p>", (this));

                _socket.Close(0);
            }

        public:
            inline bool IsListening() const
            {
                return (_socket.IsListening());
            }
            inline uint32_t Open(const uint32_t waitTime)
            {
                return (_socket.Open(waitTime));
            }
            inline uint32_t Close(const uint32_t waitTime)
            {
                return (_socket.Close(waitTime));
            }
            inline bool operator==(const SocketListner& RHS) const
            {
                return (RHS._socket == _socket);
            }
            inline bool operator!=(const SocketListner& RHS) const
            {
                return (RHS._socket != _socket);
            }

            virtual void Accept(SOCKET& newClient, const NodeId& remoteId) = 0;

        protected:
            inline void LocalNode(const Core::NodeId& localNode)
            {

                ASSERT(_socket.IsClosed() == true);

                _socket.LocalNode(localNode);
            }

        private:
            Handler _socket;
        };
    }
} // namespace Core

#endif // __SOCKETPORT_H

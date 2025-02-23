/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Privileged_Request
#endif

#include <core/core.h>

#ifdef __UNIX__
#include <errno.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace Thunder {

namespace Core {
    class PrivilegedRequest {
    public:
        class Descriptor {
        public:
            Descriptor() = delete;
            Descriptor& operator=(Descriptor&&) = delete;
            Descriptor& operator=(const Descriptor&) = delete;

            explicit Descriptor(const int descriptor)
                : _descriptor(-1)
            {
                ASSERT(descriptor != -1);
                if (descriptor != -1) {
#ifndef __WINDOWS__
                    _descriptor = ::dup(descriptor);
#endif
                }
            }
            Descriptor(Descriptor&& move)
                : _descriptor(move._descriptor)
            {
                move._descriptor = -1;
            }
            Descriptor(const Descriptor& copy)
                : _descriptor(-1)
            {
                if (copy._descriptor != -1) {
#ifndef __WINDOWS__
                    _descriptor = ::dup(copy._descriptor);
#endif
                }
            }
            ~Descriptor()
            {
                if (_descriptor != -1) {
#ifndef __WINDOWS__
                    ::close(_descriptor);
#endif
                }
            }

        public:
            operator int() const
            {
                return (_descriptor);
            }
            int Move()
            {
                int result = _descriptor;
                _descriptor = -1;
                return (result);
            }
            void Move(int&& descriptor)
            {
                if (_descriptor != -1) {
#ifndef __WINDOWS__
                    ::close(_descriptor);
#endif
                }
                _descriptor = descriptor;
                descriptor = -1;
            }

        private:
            int _descriptor;
        };

    public:
        using Container = std::vector<Descriptor>;
        static constexpr uint8_t MaxDescriptorsPerRequest = 16; // just an arbitrary number.

        struct ICallback {
            virtual ~ICallback() = default;

            virtual void Request(const uint32_t id, Container& descriptors) = 0;
            virtual void Offer(const uint32_t id, Container&& descriptors) = 0;
        };

    private:
        static constexpr uint8_t MaxFilePathLength = 255; // just an arbitrary number.

        class Connection : public Core::IResource {
        private:
            enum state : uint8_t {
                IDLE,
                REQUEST,
                OFFER,
                LISTENING
            };

#pragma pack(push, 1)
            struct header {
                state modus;
                uint32_t id;
            };
#pragma pack(pop)

        public:
            Connection() = delete;
            Connection(Connection&&) = delete;
            Connection(const Connection&) = delete;
            Connection& operator=(Connection&&) = delete;
            Connection& operator=(const Connection&) = delete;

            Connection(PrivilegedRequest& parent, ICallback* callback)
                : _parent(parent)
                , _state(IDLE)
                , _id(~0)
                , _domainSocket(-1)
                , _connector()
                , _signal(true, true)
                , _callback(callback)
            {
            }
            ~Connection() override
            {
                Close();
            };

        public:
            uint32_t Request(const uint32_t waitTime, const string& connector, const uint32_t id, Container& descriptors)
            {
                uint32_t result = Core::ERROR_INPROGRESS;
                state expected = state::IDLE;

                if (_state.compare_exchange_strong(expected, state::REQUEST) == true) {
                    ASSERT(_domainSocket == -1);

#ifndef __WINDOWS__
                    string clientName(UniqueDomainName(connector));
                    _domainSocket = OpenDomainSocket(clientName);

                    if (_domainSocket == -1) {
                        TRACE_L1("failed to open domain socket: %s", clientName.c_str());
                        result = Core::ERROR_BAD_REQUEST;
                    } else {
                        const Core::NodeId server(connector.c_str());

                        result = Core::ERROR_TIMEDOUT;

                        ResourceMonitor::Instance().Register(*this);

                        _signal.ResetEvent();
                        _id = id;

                        // Send out the request, see if the server has descriptors to respond..
                        Write(state::REQUEST, id, 0, nullptr, server, server.Size());

                        if (_signal.Lock(waitTime) == Core::ERROR_NONE) {
                            result = Core::ERROR_NONE;
                            descriptors = std::move(_descriptors);
                        }

                        ResourceMonitor::Instance().Unregister(*this);

                        ::close(_domainSocket);
                        ::unlink(clientName.c_str());
                        _domainSocket = -1;
                        _id = -1;
                        _descriptors.clear();
                    }

#endif
                    _state = state::IDLE;
                }
                return (result);
            }
            uint32_t Offer(const uint32_t waitTime, const string& connector, const uint32_t id, const Container& descriptors)
            {
                uint32_t result = Core::ERROR_INPROGRESS;
                state expected = state::IDLE;

                if (_state.compare_exchange_strong(expected, state::OFFER) == true) {
                    ASSERT(_domainSocket == -1);

                    result = Core::ERROR_UNAVAILABLE;

#ifndef __WINDOWS__
                    string clientName(UniqueDomainName(connector));
                    _domainSocket = OpenDomainSocket(clientName);

                    if (_domainSocket == -1) {
                        TRACE_L1("failed to open domain socket: %s", clientName.c_str());
                        result = Core::ERROR_BAD_REQUEST;
                    } else {
                        const Core::NodeId server(connector.c_str());

                        result = Core::ERROR_TIMEDOUT;

                        int buffer[MaxDescriptorsPerRequest];
                        uint8_t count = std::min(static_cast<uint8_t>(MaxDescriptorsPerRequest), static_cast<uint8_t>(descriptors.size()));

                        for (uint8_t index = 0; index < count; index++) {
                            buffer[index] = descriptors[index];
                        }

                        ResourceMonitor::Instance().Register(*this);

                        _signal.ResetEvent();
                        _id = id;

                        // Send out the offer, so the server can respond..
                        Write(state::OFFER, id, count, buffer, server, server.Size());

                        if (_signal.Lock(waitTime) == Core::ERROR_NONE) {
                            result = Core::ERROR_NONE;
                        }

                        ResourceMonitor::Instance().Unregister(*this);

                        ::close(_domainSocket);
                        ::unlink(clientName.c_str());
                        _domainSocket = -1;
                        _id = -1;
                    }

#endif
                    _state = state::IDLE;
                }
                return (result);
            }

            uint32_t Open(const string& connector)
            {
                uint32_t result = Core::ERROR_ILLEGAL_STATE;
                state expected = state::IDLE;

                if (_state.compare_exchange_strong(expected, state::LISTENING) == true) {
                    ASSERT(_domainSocket == -1);

#ifndef __WINDOWS__
                    _domainSocket = OpenDomainSocket(connector);

                    if (_domainSocket == -1) {
                        TRACE_L1("failed to open domain socket: %s", connector.c_str());
                        result = Core::ERROR_BAD_REQUEST;
                        _state = state::IDLE;
                    } else {
                        result = Core::ERROR_NONE;
                        _connector = connector;
                        ResourceMonitor::Instance().Register(*this);
                        TRACE_L1("Server running on fd=%d connector=%s", _domainSocket, connector.c_str());
                    }
#endif
                }
                return (result);
            }

            uint32_t Close()
            {
                if (_domainSocket != -1) {
#ifndef __WINDOWS__
                    ResourceMonitor::Instance().Unregister(*this);
                    ::close(_domainSocket);
                    if (_connector.empty() == true) {
                        _signal.SetEvent();
                    } else {
                        ::unlink(_connector.c_str());
                        _connector.clear();
                    }
#endif
                    _domainSocket = -1;
                }
                _state = state::IDLE;
                return (Core::ERROR_NONE);
            }
            handle Descriptor() const override
            {
                return (_domainSocket);
            }
            uint16_t Events() override
            {
                return (POLLIN);
            }
            void Handle(const uint16_t events) override
            {
                if ((events & POLLIN) != 0) {
                    Read();
                }
            }

        private:
#ifndef __WINDOWS__
            static constexpr char Delimiter = '|';

            string UniqueDomainName(const string connector) const
            {
                char* binder = static_cast<char*>(::alloca(connector.length() + 1 + 6 + 1));

                std::size_t delimiter = connector.find(Delimiter);

                if (delimiter == string::npos) {
                    ::strcpy(binder, connector.c_str());
                    ::strcpy(&binder[connector.length()], _T(".XXXXXX"));
                } else {
                    ::memcpy(binder, connector.c_str(), delimiter);
                    ::strcpy(&binder[delimiter], _T(".XXXXXX"));
                }

                ASSERT(strlen(binder) < MaxFilePathLength);

                ::mkstemp(binder);

                return (string(binder));
            }
            int OpenDomainSocket(const string& connector) const
            {
                int fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);

                if (fd >= 0) {
                    int flags = fcntl(fd, F_GETFL, 0) | O_NONBLOCK;

                    if (::fcntl(fd, F_SETFL, flags) != 0) {
                        TRACE_L1("Error on port socket F_SETFL call. Error: %s", strerror(errno));
                        ::close(fd);
                        fd = -1;
                    } else {
                        ::unlink(connector.c_str());

                        const Core::NodeId binder(connector.c_str());

                        if (::bind(fd, binder, binder.Size()) != 0) {
                            TRACE_L1("Error on port socket bind call. Error: %s", strerror(errno));
                            ::close(fd);
                            fd = -1;
                        }
                    }
                }
                return (fd);
            }

            Container MoveToContainer(const uint8_t length, const int* fds) const
            {
                Container result;

                for (uint8_t index = 0; index < length; index++) {
                    ASSERT(fds[index] >= 0);

                    result.emplace_back(fds[index]);
                    ::close(fds[index]);
                }
                return (result);
            }
            uint8_t CopyContainer(const uint8_t length VARIABLE_IS_NOT_USED, int fds[], const Container& container) const
            {
                uint8_t count = 0;

                for (uint8_t index = 0; ((index < static_cast<uint8_t>(container.size())) && (count < MaxDescriptorsPerRequest)); index++) {
                    if (container[index] != -1) {
                        fds[count++] = container[index];
                    }
                }
                return (count);
            }
            uint32_t Load(const struct msghdr* msg, uint8_t& nFds, int fds[]) const
            {
                const struct cmsghdr* cmsg = CMSG_FIRSTHDR(msg);
                uint32_t result = Core::ERROR_NONE;

                if ((cmsg == nullptr) || (cmsg->cmsg_len < CMSG_LEN(sizeof(int)))) {
                    nFds = 0;
                } else if (cmsg->cmsg_level != SOL_SOCKET) {
                    result = Core::ERROR_BAD_REQUEST;
                } else if (cmsg->cmsg_type != SCM_RIGHTS) {
                    result = Core::ERROR_GENERAL;
                } else {
                    const unsigned char* const cmsgData = CMSG_DATA(cmsg);
                    nFds = std::min(static_cast<uint8_t>((cmsg->cmsg_len - sizeof(cmsghdr)) / sizeof(int)), nFds);

                    ::memmove(fds, cmsgData, nFds * sizeof(int));
                }
                return (result);
            }
            uint32_t Write(const state modus, const uint32_t id, const uint8_t nFds, const int fds[], const struct sockaddr* client, const int length)
            {
                uint32_t result = Core::ERROR_NONE;

                struct msghdr msg;
                memset(&msg, 0, sizeof(msg));

                if (nFds > MaxDescriptorsPerRequest) {
                    TRACE_L1("Too much descriptors, sending the first %d.", MaxDescriptorsPerRequest);
                }

                ASSERT(_domainSocket != -1);

                struct header info;
                info.modus = modus;
                info.id = id;

                struct iovec io {
                    &info, sizeof(info)
                };

                TRACE_L1("Sending to: %s", reinterpret_cast<const struct sockaddr_un*>(client)->sun_path);

                int sendBytes;
                msg.msg_name = const_cast<sockaddr*>(client);
                msg.msg_namelen = length;
                msg.msg_iov = &io;
                msg.msg_iovlen = 1;

                if (nFds == 0) {
                    msg.msg_control = nullptr;
                    msg.msg_controllen = 0;

                    sendBytes = sendmsg(_domainSocket, &msg, 0);
                } else {
                    const uint16_t payloadSize((nFds <= MaxDescriptorsPerRequest) ? (sizeof(int) * nFds) : (sizeof(int) * MaxDescriptorsPerRequest));
                    msg.msg_control = static_cast<char*>(ALLOCA(CMSG_SPACE(payloadSize)));
                    ::memset(msg.msg_control, 0, CMSG_SPACE(payloadSize));
                    msg.msg_controllen = CMSG_SPACE(payloadSize);

                    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
                    ASSERT(cmsg != nullptr);

                    cmsg->cmsg_level = SOL_SOCKET;
                    cmsg->cmsg_type = SCM_RIGHTS;
                    cmsg->cmsg_len = CMSG_LEN(payloadSize);

                    ::memmove(CMSG_DATA(cmsg), fds, payloadSize);

                    sendBytes = sendmsg(_domainSocket, &msg, 0);
                }

                if (sendBytes < 0) {
                    TRACE_L1("Error on port socket sendmsg call. Error: %s", strerror(errno));
                    result = Core::ERROR_BAD_REQUEST;
                }

                return result;
            }
#endif
            uint32_t Read()
            {
                ASSERT(_domainSocket != -1);

#ifndef __WINDOWS__
                char buf[CMSG_SPACE(sizeof(int) * MaxDescriptorsPerRequest)];
                memset(buf, 0, sizeof(buf));

                struct msghdr msg;
                struct sockaddr_un client;
                struct header info;
                info.modus = state::LISTENING;
                info.id = 0;
                struct iovec io { &info, sizeof(info) };

                msg.msg_name = &client;
                msg.msg_namelen = sizeof(client);
                msg.msg_iov = &io;
                msg.msg_iovlen = 1;
                msg.msg_control = buf;
                msg.msg_controllen = sizeof(buf);

                int recvBytes = recvmsg(_domainSocket, &msg, 0);

                if (recvBytes < 0) {
                    TRACE_L1("Error on port socket recvfrom call. Error: %s", strerror(errno));
                } else if (recvBytes > 0) {
                    if (_state.load(Core::memory_order::memory_order_relaxed) == state::LISTENING) {
                        if (info.modus == state::OFFER) {
                            int descriptors[MaxDescriptorsPerRequest];
                            uint8_t count = sizeof(descriptors) / sizeof(int);
                            // Send out the offer, so the server can respond..
                            uint32_t result = Load(&msg, count, descriptors);
                            if (result == Core::ERROR_NONE) {
                                Container container = MoveToContainer(count, descriptors); // always needed to properly close the file descriptors
                                if (_callback != nullptr) {
                                    _callback->Offer(info.id, std::move(container));
                                }
                            }
                            Write(state::OFFER, info.id, 0, nullptr, reinterpret_cast<const struct sockaddr*>(&client), msg.msg_namelen);
                        } else if (info.modus == state::REQUEST) {

                            // Honour the request, get a lift of RefCounted File descriptors to return...
                            if (_callback == nullptr) {
                                Write(REQUEST, info.id, 0, nullptr, reinterpret_cast<const struct sockaddr*>(&client), msg.msg_namelen);
                            } else {
                                Container container;
                                _callback->Request(info.id, container);
                                int descriptors[MaxDescriptorsPerRequest];
                                uint8_t count = CopyContainer(MaxDescriptorsPerRequest, descriptors, container);
                                Write(REQUEST, info.id, count, descriptors, reinterpret_cast<const struct sockaddr*>(&client), msg.msg_namelen);
                            }
                        } else {
                            TRACE_L1("Error on port socket recvfrom call. Unknown modus: %d", info.modus);
                        }
                    } else if (_id != info.id) {
                        TRACE_L1("Unexpected response. Not a matching ID.");
                    } else if (_state.load(Core::memory_order::memory_order_relaxed) != info.modus) {
                        TRACE_L1("Unexpected response. Not a matching modus.");
                    } else {

                        int descriptors[MaxDescriptorsPerRequest];
                        uint8_t count = sizeof(descriptors) / sizeof(int);
                        uint32_t result = Load(&msg, count, descriptors);
                        _descriptors = MoveToContainer(count, descriptors);

                        if (result != Core::ERROR_NONE) {
                            TRACE_L1("Could not load the descriptors! Error: [%d].", result);
                        } else {
                            _signal.SetEvent();
                        }
                    }
                }
#endif

                return 0;
            }

        private:
            PrivilegedRequest& _parent;
            std::atomic<state> _state;
            uint32_t _id;
            Container _descriptors;
            int _domainSocket;
            string _connector;
            Core::Event _signal;
            ICallback* _callback;
        };

    public:
        PrivilegedRequest(const PrivilegedRequest&) = delete;
        PrivilegedRequest& operator=(const PrivilegedRequest&) = delete;

        PrivilegedRequest(ICallback* callback = nullptr)
            : _link(*this, callback)
        {
        }
        virtual ~PrivilegedRequest()
        {
            Close();
        }

    public:
        uint32_t Open(const string& identifier)
        {
            return (_link.Open(identifier));
        }
        uint32_t Close()
        {
            return (_link.Close());
        }
        uint32_t Request(const uint32_t waitTime, const string& identifier, const uint32_t requestId, Container& fds)
        {
            uint32_t result;
            if ((result = _link.Request(waitTime, identifier, requestId, fds)) != Core::ERROR_NONE) {
                TRACE_L1("Could not get a privileged request answered.");
            }
            return (result);
        }
        uint32_t Offer(const uint32_t waitTime, const string& identifier, const uint32_t requestId, const Container& fds)
        {
            uint32_t result;
            if ((result = _link.Offer(waitTime, identifier, requestId, fds)) != Core::ERROR_NONE) {
                TRACE_L1("Could not get a privileged offer answered.");
            }
            return (result);
        }

    private:
        Connection _link;
    };
}
} // Namespace Thunder::Core

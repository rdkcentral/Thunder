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

namespace WPEFramework {

namespace Core {
    class PrivilegedRequest {
    public:
        class Descriptor {
        public:
            Descriptor& operator=(const Descriptor&) = delete;

            Descriptor()
                : _descriptor(-1)
            {
            }
            explicit Descriptor(int&& descriptor)
                : _descriptor(descriptor)
            {
                ASSERT(descriptor != -1);
                descriptor = -1;
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
        static constexpr int maxFdsPerRequest = 22; // just an arbitrary number.

    private:
        class Connection : public Core::IResource {
        private:
            enum state : uint8_t {
                IDLE,
                REQUEST,
                LISTENING
            };

        public:
            Connection() = delete;
            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;

            Connection(PrivilegedRequest& parent)
                : _parent(parent)
                , _state(IDLE)
                , _domainSocket(-1)
                , _id(~0)
                , _descriptors(nullptr)
                , _signal(true, true)
            {
            }
            ~Connection() override = default;

        public:
            uint32_t Request(const uint32_t waitTime, const string& connector, const uint32_t id, Container& descriptors)
            {
                uint32_t result = Core::ERROR_INPROGRESS;
                state expected = state::IDLE;

                if (_state.compare_exchange_strong(expected, state::REQUEST) == true) {
                    ASSERT(_domainSocket == -1);

                    result = Core::ERROR_UNAVAILABLE;

#ifndef __WINDOWS__
                    _domainSocket = ::socket(AF_UNIX, SOCK_DGRAM, 0);

                    if (_domainSocket >= 0) {
                        int flags = fcntl(_domainSocket, F_GETFL, 0) | O_NONBLOCK;

                        if (::fcntl(_domainSocket, F_SETFL, flags) != 0) {
                            TRACE_L1("Error on port socket F_SETFL call. Error: %s", strerror(errno));
                            result = Core::ERROR_BAD_REQUEST;
                        } else {
                            result = Core::ERROR_TIMEDOUT;
                            _id = id;

                            // Prepare Client
                            const string clientPath = connector + "-client";

                            ::unlink(clientPath.c_str());

                            const Core::NodeId client(clientPath.c_str());
                            const Core::NodeId server(connector.c_str());

                            if (::bind(_domainSocket, client, client.Size()) == 0) {
                                result = Core::ERROR_NONE;

                                ResourceMonitor::Instance().Register(*this);

                                _signal.ResetEvent();

                                _descriptors = &descriptors;

                                ::sendto(_domainSocket, &id, sizeof(id), 0, server, server.Size());

                                if (_signal.Lock(waitTime) == Core::ERROR_NONE) {
                                    result = Core::ERROR_NONE;
                                }

                                _descriptors = nullptr;

                                ResourceMonitor::Instance().Unregister(*this);
                            } else {
                                TRACE_L1("Bind to \"%s\" failed. Error: %s", clientPath.c_str(), strerror(errno));
                                result = Core::ERROR_BAD_REQUEST;
                            }
                        }

                        ::close(_domainSocket);
                        _domainSocket = -1;
                    } else {
                        TRACE_L1("failed to open domain socket: %s\n", connector.c_str());
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

                    result = Core::ERROR_UNAVAILABLE;

#ifndef __WINDOWS__
                    _domainSocket = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);

                    if (_domainSocket >= 0) {
                        int flags = ::fcntl(_domainSocket, F_GETFL, 0) | O_NONBLOCK;

                        if (::fcntl(_domainSocket, F_SETFL, flags) != 0) {
                            TRACE_L1("Error on port socket F_SETFL call. Error: %s", strerror(errno));
                            result = Core::ERROR_BAD_REQUEST;
                        } else {
                            ::unlink(connector.c_str());

                            const Core::NodeId server(connector.c_str());

                            if (::bind(_domainSocket, server, server.Size()) == 0) {
                                result = Core::ERROR_NONE;
                                ResourceMonitor::Instance().Register(*this);
                            } else {
                                TRACE_L1("Error on port socket bind call. Error: %s", strerror(errno));
                                result = Core::ERROR_BAD_REQUEST;
                            }
                        }
                    } else {
                        TRACE_L1("failed to open domain socket: %s\n", connector.c_str());
                    }

                    if (result != Core::ERROR_NONE) {
                        if (_domainSocket >= 0) {
                            TRACE_L1("Closing socket on error: 0x%04X", result);
                            ::close(_domainSocket);
                            _domainSocket = -1;
                        }
                        _state = state::IDLE;
                    } else {
                        TRACE_L1("Server running on fd=%d connector=%s", _domainSocket, connector.c_str());
                    }
#endif
                }
                return (result);
            }

            uint32_t Close()
            {
                if (_domainSocket != -1) {
                    ResourceMonitor::Instance().Unregister(*this);
#ifndef __WINDOWS__
                    ::close(_domainSocket);
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
            uint32_t Write(const uint32_t id, const uint8_t nFds, const int fds[], struct sockaddr& client, socklen_t length)
            {
                uint32_t result = Core::ERROR_NONE;

                struct msghdr msg;
                memset(&msg, 0, sizeof(msg));

                if (nFds > maxFdsPerRequest) {
                    TRACE_L1("Too much descriptors, sending the first %d.", maxFdsPerRequest);
                }

                const uint16_t payloadSize((nFds <= maxFdsPerRequest) ? (sizeof(int) * nFds) : (sizeof(int) * maxFdsPerRequest));

                char buf[CMSG_SPACE(payloadSize)];
                memset(buf, 0, sizeof(buf));

                uint32_t identifier(id);
                struct iovec io = { .iov_base = &identifier, .iov_len = sizeof(identifier) };

                msg.msg_name = &client;
                msg.msg_namelen = length;
                msg.msg_iov = &io;
                msg.msg_iovlen = 1;
                msg.msg_control = buf;
                msg.msg_controllen = sizeof(buf);

                struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
                cmsg->cmsg_level = SOL_SOCKET;
                cmsg->cmsg_type = SCM_RIGHTS;
                cmsg->cmsg_len = CMSG_LEN(payloadSize);

                memmove(CMSG_DATA(cmsg), fds, payloadSize);

                msg.msg_controllen = CMSG_SPACE(payloadSize);

                ASSERT(_domainSocket != -1);

                int sendBytes = sendmsg(_domainSocket, &msg, 0);

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

                uint32_t result = Core::ERROR_NONE;

#ifndef __WINDOWS__
                struct msghdr msg;
                ::memset(&msg, 0, sizeof(msg));

                if (_state.load(Core::memory_order::memory_order_relaxed) == state::LISTENING) {
                    uint32_t id;
                    struct sockaddr_un client;

                    socklen_t clientLen = sizeof(client);

                    int recvBytes = recvfrom(_domainSocket, &id, sizeof(id), 0, reinterpret_cast<sockaddr*>(&client), &clientLen);

                    if (recvBytes < 0) {
                        TRACE_L1("Error on port socket recvfrom call. Error: %s", strerror(errno));
                        result = Core::ERROR_UNAVAILABLE;
                    } else {
                        int descriptors[64];
                        uint8_t entries = _parent.Service(id, sizeof(descriptors) / sizeof(int), descriptors);
                        Write(id, entries, descriptors, reinterpret_cast<sockaddr&>(client), clientLen);
                    }
                } else {
                    char buf[CMSG_SPACE(sizeof(int) * maxFdsPerRequest)];
                    memset(buf, 0, sizeof(buf));

                    uint32_t identifier;
                    struct iovec io = { .iov_base = &identifier, .iov_len = sizeof(identifier) };

                    msg.msg_name = NULL;
                    msg.msg_namelen = 0;
                    msg.msg_iov = &io;
                    msg.msg_iovlen = 1;
                    msg.msg_control = buf;
                    msg.msg_controllen = sizeof(buf);

                    int recvBytes = recvmsg(_domainSocket, &msg, 0);

                    if (recvBytes < 0) {
                        TRACE_L1("Error on port socket recvmsg call. Error: %s", strerror(errno));
                        result = Core::ERROR_UNAVAILABLE;
                    } else {
                        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);

                        if ((cmsg != nullptr) && (cmsg->cmsg_len >= CMSG_LEN(sizeof(int)))) {
                            if (cmsg->cmsg_level != SOL_SOCKET) {
                                result = Core::ERROR_BAD_REQUEST;
                            } else if (cmsg->cmsg_type != SCM_RIGHTS) {
                                result = Core::ERROR_GENERAL;
                            } else {
                                unsigned char* const cmsgData = CMSG_DATA(cmsg);
                                const uint8_t nFds = ((cmsg->cmsg_len - sizeof(cmsghdr)) / sizeof(int));

                                _descriptors->resize(nFds);

                                for (uint8_t i = 0; i < nFds; i++) {
                                    int fd;
                                    ::memmove(&fd, cmsgData + sizeof(int) * i, sizeof(int));
                                    (*_descriptors)[i].Move(std::move(fd));
                                }

                                _id = identifier;
                                _signal.SetEvent();
                            }
                        } else {
                            TRACE_L1("Error invalid cmsg %p ", cmsg);
                        }
                    }
                }
#endif

                return result;
            }

        private:
            PrivilegedRequest& _parent;
            std::atomic<state> _state;
            int _domainSocket;
            uint32_t _id;
            Container* _descriptors;
            Core::Event _signal;
        };

    public:
        PrivilegedRequest(const PrivilegedRequest&) = delete;
        PrivilegedRequest& operator=(const PrivilegedRequest&) = delete;

        PrivilegedRequest()
            : _link(*this)
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

    private:
        // ToDo: Create separate client
        virtual uint8_t Service(const uint32_t /* id */, const uint8_t /* maxSize */, int[] /*container*/) { return 0; }

    private:
        Connection _link;
    };
}
} // Namespace WPEFramework::Core

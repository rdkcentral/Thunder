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

#pragma once

#ifdef __POSIX__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

namespace Thunder {
namespace Tests {

    // Raw TCP sender for use in socket integration tests.
    //
    // Connects to a TCP endpoint and sends data in controlled chunk sizes.
    // TCP_NODELAY is set so each send() call maps to its own TCP segment,
    // giving the receiver one poll()/read() cycle per chunk. Combined with
    // chunkSize == 1 this lets tests deliver exactly one byte per ReceiveData()
    // invocation, which is the reliable way to trigger and verify parser
    // fragmentation behaviour.
    //
    // Usage:
    //   RawSender sender;
    //   sender.Open("127.0.0.1", 19274);
    //   sender.Send(buf, len);                     // whole buffer in one shot
    //   sender.Send(buf, len, 1);                  // byte by byte, 1000 µs delay
    //   sender.Send(buf, len, 4, 500);             // 4-byte chunks, 500 µs delay
    //   sender.Send(str);                          // convenience overload for string

    class RawSender {
    public:
        RawSender()
            : _fd(-1)
            , _connected(false)
        {
        }

        RawSender(const RawSender&) = delete;
        RawSender& operator=(const RawSender&) = delete;

        ~RawSender()
        {
            Close();
        }

        // Opens a TCP connection to host:port with TCP_NODELAY.
        // Returns true on success.
        bool Open(const std::string& host, uint16_t port)
        {
            Close();

            _fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (_fd < 0) return false;

            const int flag = 1;
            if (::setsockopt(_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) != 0) {
                Close();
                return false;
            }
            struct sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port   = htons(port);
            if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
                Close();
                return false;
            }

            _connected = (::connect(_fd,
                reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0);

            if (!_connected) Close();

            return _connected;
        }

        void Close()
        {
            if (_fd >= 0) {
                ::close(_fd);
                _fd = -1;
            }
            _connected = false;
        }

        bool IsConnected() const
        {
            return _connected;
        }

        // Sends data in chunks of chunkSize bytes with delayUs microseconds between
        // each chunk.
        //
        //   chunkSize == 1          → byte-by-byte, one TCP segment per byte
        //   chunkSize == UINT32_MAX → whole buffer in a single send() (default)
        //   delayUs   == 0          → no sleep between chunks
        //
        // Returns the total number of bytes sent.
        uint32_t Send(const uint8_t* data, uint32_t length,
                      uint32_t chunkSize = UINT32_MAX, uint32_t delayUs = 1000)
        {
            if (!_connected || (data == nullptr) || (length == 0)) return 0;

            uint32_t sent = 0;

            while (sent < length) {
                uint32_t chunk = (chunkSize > (length - sent)) ? (length - sent) : chunkSize;

                int flags = 0;
#if defined(MSG_NOSIGNAL)
                flags |= MSG_NOSIGNAL;
#endif
                const ssize_t result = ::send(_fd, data + sent, chunk, flags);
                if (result <= 0) {
                    Close();
                    break;
                }

                sent += static_cast<uint32_t>(result);

                if ((delayUs > 0) && (chunkSize != UINT32_MAX))
                    std::this_thread::sleep_for(std::chrono::microseconds(delayUs));
            }

            return sent;
        }

        // Convenience overload for string literals and std::string / Thunder string.
        uint32_t Send(const std::string& data,
                      uint32_t chunkSize = UINT32_MAX, uint32_t delayUs = 1000)
        {
            return Send(reinterpret_cast<const uint8_t*>(data.c_str()),
                        static_cast<uint32_t>(data.size()), chunkSize, delayUs);
        }

    private:
        int  _fd;
        bool _connected;
    };

} // Tests
} // Thunder

#endif // __POSIX__

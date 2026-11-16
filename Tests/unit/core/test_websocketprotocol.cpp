/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <websocket/websocket.h>

namespace Thunder {
namespace Tests {
    namespace Core {

        class WebSocketTestLink {
        private:
            WebSocketTestLink(const WebSocketTestLink&) = delete;
            WebSocketTestLink& operator=(const WebSocketTestLink&) = delete;

        public:
            WebSocketTestLink()
                : _closed(false)
                , _closeCount(0)
            {
            }
            virtual ~WebSocketTestLink() = default;

            virtual uint16_t SendData(uint8_t*, const uint16_t)
            {
                return (0);
            }
            virtual uint16_t ReceiveData(uint8_t*, const uint16_t)
            {
                return (0);
            }
            virtual void StateChange()
            {
            }
            bool IsOpen() const
            {
                return (_closed == false);
            }
            bool IsClosed() const
            {
                return (_closed == true);
            }
            bool IsSuspended() const
            {
                return (false);
            }
            uint32_t Close(const uint32_t)
            {
                _closed = true;
                _closeCount++;
                return (::Thunder::Core::ERROR_NONE);
            }
            void Trigger()
            {
            }
            string LocalId() const
            {
                return ("local");
            }
            string RemoteId() const
            {
                return ("remote");
            }
            uint32_t CloseCount() const
            {
                return (_closeCount);
            }

        private:
            bool _closed;
            uint32_t _closeCount;
        };

        class WebSocketTestSocket : public Web::WebSocketLinkType<WebSocketTestLink, Web::Request, Web::Response, Web::WebSocket::RequestAllocator&> {
        private:
            typedef Web::WebSocketLinkType<WebSocketTestLink, Web::Request, Web::Response, Web::WebSocket::RequestAllocator&> BaseClass;

            WebSocketTestSocket(const WebSocketTestSocket&) = delete;
            WebSocketTestSocket& operator=(const WebSocketTestSocket&) = delete;

        public:
            WebSocketTestSocket()
                : BaseClass(false, true, 1, Web::WebSocket::RequestAllocator::Instance())
            {
            }
            ~WebSocketTestSocket() override
            {
                if (Link().IsClosed() == false) {
                    Link().Close(0);
                }
            }

            void LinkBody(::Thunder::Core::ProxyType<Web::Request>&) override
            {
            }
            void Received(::Thunder::Core::ProxyType<Web::Request>&) override
            {
            }
            void Send(const ::Thunder::Core::ProxyType<Web::Response>&) override
            {
            }
            uint16_t ReceiveData(uint8_t* data, const uint16_t size) override
            {
                _received.append(reinterpret_cast<const char*>(data), size);
                return (size);
            }
            uint16_t SendData(uint8_t*, const uint16_t) override
            {
                return (0);
            }
            void StateChange() override
            {
            }
            bool IsIdle() const override
            {
                return (true);
            }
            const string& ReceivedData() const
            {
                return (_received);
            }

        private:
            string _received;
        };

        static void UpgradeToWebSocket(WebSocketTestSocket& socket)
        {
            char request[] = "GET / HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Upgrade: websocket\r\n"
                             "Connection: Upgrade\r\n"
                             "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                             "Sec-WebSocket-Version: 13\r\n\r\n";

            ASSERT_EQ(socket.Link().ReceiveData(reinterpret_cast<uint8_t*>(request), sizeof(request) - 1), sizeof(request) - 1);

            uint8_t response[2048] = {};
            for (uint8_t index = 0; (index < 4) && (socket.IsWebSocket() == false); ++index) {
                const uint16_t size = socket.Link().SendData(response, sizeof(response));
                EXPECT_TRUE((size > 0) || socket.IsWebSocket());
            }
            ASSERT_TRUE(socket.IsWebSocket());
        }

        TEST(WebSocketProtocol, RejectsExtendedLengthControlFrame)
        {
            Web::WebSocket::Protocol decoder(false, false);
            uint8_t frame[] = { 0x89, 126, 0x00, 0x7E };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), sizeof(frame));
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::VIOLATION);
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, Rejects64BitLengthControlFrame)
        {
            Web::WebSocket::Protocol decoder(false, false);
            uint8_t frame[] = { 0x89, 127, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), sizeof(frame));
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::VIOLATION);
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, RejectsMaskedExtendedLengthControlFrame)
        {
            Web::WebSocket::Protocol decoder(false, true);
            uint8_t frame[] = { 0x89, 0xFE, 0x00, 0x7E, 0x01, 0x02, 0x03, 0x04 };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), sizeof(frame));
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::VIOLATION);
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, RejectsPayloadLengthWithHighBitSet)
        {
            Web::WebSocket::Protocol decoder(true, false);
            uint8_t frame[] = { 0x82, 127, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), sizeof(frame));
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::VIOLATION);
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, StreamsPayloadLargerThanSixteenMiB)
        {
            Web::WebSocket::Protocol decoder(true, false);
            uint8_t frame[] = {
                0x82, 127,
                0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x01,
                0xAA, 0xBB
            };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), 10);
            EXPECT_EQ(size, 2);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::BINARY);
            EXPECT_FALSE(decoder.IsCompleteMessage());

            uint8_t continuation[] = { 0xCC, 0xDD, 0xEE };
            size = sizeof(continuation);
            EXPECT_EQ(decoder.Decoder(continuation, size), 0);
            EXPECT_EQ(size, sizeof(continuation));
            EXPECT_FALSE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, RejectsFrameLargerThanPendingState)
        {
            Web::WebSocket::Protocol decoder(true, false);
            uint8_t frame[] = {
                0x82, 127,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00
            };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), sizeof(frame));
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::TOO_BIG);
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, AcceptsMaximumTrackableFrameSize)
        {
            Web::WebSocket::Protocol decoder(true, false);
            uint8_t frame[] = {
                0x82, 127,
                0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                0xAA
            };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), 10);
            EXPECT_EQ(size, 1);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::BINARY);
            EXPECT_FALSE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, RejectsMaskedOversizedFrameBeforeUnmasking)
        {
            Web::WebSocket::Protocol decoder(true, true);
            uint8_t frame[] = {
                0x82, 0xFF,
                0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
                0x01, 0x02, 0x03, 0x04,
                0xAA, 0xBB
            };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), 14);
            EXPECT_EQ(size, 0);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::TOO_BIG);
            EXPECT_EQ(frame[14], 0xAA);
            EXPECT_EQ(frame[15], 0xBB);
        }

        TEST(WebSocketProtocol, StreamsValidControlPayloadAcrossReads)
        {
            Web::WebSocket::Protocol decoder(false, false);
            uint8_t first[] = { 0x89, 5, 'h', 'e' };
            uint16_t size = sizeof(first);

            EXPECT_EQ(decoder.Decoder(first, size), 2);
            EXPECT_EQ(size, 2);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::PING);
            EXPECT_FALSE(decoder.IsCompleteMessage());

            uint8_t second[] = { 'l', 'l', 'o' };
            size = sizeof(second);
            EXPECT_EQ(decoder.Decoder(second, size), 0);
            EXPECT_EQ(size, sizeof(second));
            EXPECT_TRUE(decoder.IsCompleteMessage());
        }

        TEST(WebSocketProtocol, UnmasksValidControlPayload)
        {
            Web::WebSocket::Protocol decoder(false, true);
            uint8_t frame[] = {
                0x89, 0x83,
                0x01, 0x02, 0x03, 0x04,
                static_cast<uint8_t>('o' ^ 0x01),
                static_cast<uint8_t>('n' ^ 0x02),
                static_cast<uint8_t>('e' ^ 0x03)
            };
            uint16_t size = sizeof(frame);

            EXPECT_EQ(decoder.Decoder(frame, size), 6);
            EXPECT_EQ(size, 3);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::PING);
            EXPECT_TRUE(decoder.IsCompleteMessage());
            EXPECT_EQ(frame[6], 'o');
            EXPECT_EQ(frame[7], 'n');
            EXPECT_EQ(frame[8], 'e');
        }

        TEST(WebSocketProtocol, AllowsControlFrameDuringFragmentedMessage)
        {
            Web::WebSocket::Protocol decoder(false, false);
            uint8_t first[] = { 0x01, 2, 'a', 'b' };
            uint16_t size = sizeof(first);

            EXPECT_EQ(decoder.Decoder(first, size), 2);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::TEXT);
            EXPECT_TRUE(decoder.ReceiveInProgress());

            uint8_t ping[] = { 0x89, 0 };
            size = sizeof(ping);
            EXPECT_EQ(decoder.Decoder(ping, size), 2);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::PING);
            EXPECT_TRUE(decoder.ReceiveInProgress());

            uint8_t last[] = { 0x80, 2, 'c', 'd' };
            size = sizeof(last);
            EXPECT_EQ(decoder.Decoder(last, size), 2);
            EXPECT_EQ(decoder.FrameType(), Web::WebSocket::Protocol::TEXT);
            EXPECT_FALSE(decoder.ReceiveInProgress());
        }

        TEST(WebSocketLink, SplitMaskedControlFramePreservesNextFrame)
        {
            WebSocketTestSocket socket;
            UpgradeToWebSocket(socket);

            uint8_t first[] = {
                0x89, 0x83,
                0x01, 0x02, 0x03, 0x04,
                static_cast<uint8_t>('a' ^ 0x01)
            };
            EXPECT_EQ(socket.Link().ReceiveData(first, sizeof(first)), sizeof(first));

            uint8_t second[] = {
                static_cast<uint8_t>('b' ^ 0x02),
                static_cast<uint8_t>('c' ^ 0x03),
                0x81, 0x82,
                0x05, 0x06, 0x07, 0x08,
                static_cast<uint8_t>('O' ^ 0x05),
                static_cast<uint8_t>('K' ^ 0x06)
            };
            EXPECT_EQ(socket.Link().ReceiveData(second, sizeof(second)), sizeof(second));

            EXPECT_EQ(socket.ReceivedData(), "OK");
            EXPECT_EQ(socket.Link().CloseCount(), 0u);
            EXPECT_FALSE(socket.IsClosed());
        }

    } // Core
} // Tests
} // Thunder

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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <websocket/websocket.h>

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    template <typename STREAM>
    class WebSocketClient : public ::Thunder::Web::WebSocketClientType<STREAM> {
    public :

        template <typename... Args>
        WebSocketClient(
              const string& path
            , const string& protocol
            , const string& query
            , const string& origin
            , const bool binary
            , const bool masking
            , Args&&... args
        )
            : ::Thunder::Web::WebSocketClientType<STREAM>(path, protocol, query, origin, binary, masking, /* <Arguments SocketStream> */ false /* raw socket */ , std::forward<Args>(args)... /*</Arguments SocketStream>*/)
            , _deepInspection{ false }
            , _markerReceived{ false }
        {}

        ~WebSocketClient() override = default;

        // Non-idle then data available to send
        bool IsIdle() const override
        {
            // Not sending any data
            return true;
        }

        // Allow for eventfull state updates in this class
        void StateChange() override
        {}

        // Reflects payload, effective after upgrade
        uint16_t SendData(VARIABLE_IS_NOT_USED uint8_t* dataFrame, VARIABLE_IS_NOT_USED const uint16_t maxSendSize) override
        {
            // Not sending any data
            return 0;
        }

        // Reflects payload, effective after upgrade
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
            std::basic_string<uint8_t> message{ dataFrame, receivedSize };

            // IsCompleted requires deep inspection of the data to identify the end of transmission marker

            if (   this->IsCompleted() != false
                && this->IsCompleteMessage() != true
            ) {
                // A subsequent call is required
                _deepInspection= true;
                _markerReceived = dataFrame[receivedSize - 1] == 0xA;
            }

            if (   this->IsCompleted() != false
                && this->IsCompleteMessage() != false
            ) {
                // dataFrame has the 'end' marker, last call
                _deepInspection = false;
                _markerReceived = dataFrame[receivedSize - 1] == 0xA;
            }

            return receivedSize;
        }

        bool DeepInspection() const
        {
            return _deepInspection;
        }

        bool MarkerReceived() const
        {
            return _markerReceived;
        }

    private:
        std::atomic<bool> _deepInspection;
        std::atomic<bool> _markerReceived;
    };

    template <typename STREAM, size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE>
    class WebSocketServer : public ::Thunder::Web::WebSocketServerType<STREAM> {
    public :

        // SocketServerType defines SocketHandler of type SocketListener. SocketListener triggers Accept on StateChange, effectively, calling SocketServerType::Accept(SOCKET, NodeId) which creates a WebSocketServer with these parameters
        WebSocketServer(const SOCKET& socket, const ::Thunder::Core::NodeId remoteNode, ::Thunder::Core::SocketServerType<WebSocketServer<STREAM, SENDBUFFERSIZE, RECEIVEBUFFERSIZE>>*)
            // Initially this should be defined as a regular TCP socket
            : ::Thunder::Web::WebSocketServerType<STREAM>(false /* binary */, false /* masking */, false /* raw socket */, socket, remoteNode, SENDBUFFERSIZE /* send buffer size */, RECEIVEBUFFERSIZE /* receive buffer size */)
            , _post{}
            , _response{}
            , _guard{}
        {
        }

        ~WebSocketServer() override = default;

        // Non-idle then data available to send
        bool IsIdle() const override
        {
            _guard.Lock();

            bool result = _post.size() == 0;

            _guard.Unlock();

            return result;
        }

        // Allow for eventfull state updates in this class
        void StateChange() override
        {}

        // Reflects payload, effective after upgrade
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            size_t count = 0;

            if (   dataFrame != nullptr
                && maxSendSize > 0
                && !IsIdle()
                && ::Thunder::Web::WebSocketServerType<STREAM>::IsOpen()
                && ::Thunder::Web::WebSocketServerType<STREAM>::IsWebSocket() // Redundant, covered by IsOpen
               ) {
                _guard.Lock();

                std::basic_string<uint8_t>& message = _post.front();

                count = std::min(message.size(), static_cast<size_t>(maxSendSize));

                /* void* */ memcpy(dataFrame, message.data(), count);

                if (count == message.size()) {
                    /* iterator */ _post.erase(_post.begin());
                } else {
                    /* this */ message.erase(0, count);

                    // Trigger a call to SendData for remaining data
                    ::Thunder::Web::WebSocketServerType<STREAM>::Link().Trigger();
                }

                _guard.Unlock();
            }

            return count;
        }

        // Reflects payload, effective after upgrade
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override {
            if (receivedSize > 0) {
                _guard.Lock();

                _response.emplace_back(std::basic_string<uint8_t>{ dataFrame, receivedSize });

                _guard.Unlock();
            }

            return receivedSize;
        }

        // Put data in the queue to send (to the (connected) client)
        bool Submit(const std::basic_string<uint8_t>& message)
        {
            _guard.Lock();

            size_t count = _post.size();

            _post.emplace_back(message);

            bool result =  count < _post.size();

            _guard.Unlock();

            // Trigger a call to SendData
            ::Thunder::Web::WebSocketServerType<STREAM>::Link().Trigger();

            return result;
        }

        std::basic_string<uint8_t> Response()
        {
            std::basic_string<uint8_t> message;

            _guard.Lock();

            if (_response.size() > 0) {
                message = _response.front();
                _response.erase(_response.begin());
            } 

            _guard.Unlock();

            return message;
        }

    private:

        std::vector<std::basic_string<uint8_t>> _post; // Send message queue
        std::vector<std::basic_string<uint8_t>> _response; // Receive message queue

        mutable ::Thunder::Core::CriticalSection _guard;
    };

    // WebSocketLinkType identifies the last frame in the WebSocket protocol based on the FIN marker.
    // This marker is at the beginning of the last sent frame. A set FIN marker triggers IsCompleted
    // equal true. Data contained in this last frame is offered, here at the client's ReceiveData.
    // This might happen in multiple offers (calls) and require data inspection at the user to
    // guarantuee all has been received. On the other hand, IsCompleteMessage equals true if no
    // remaining data of the last WebSocket frame is expected. This value is based on the payload
    // lenght value within the fragment having the FIN marker set. This test illustrates that encoding
    // of data in transmissing. Notice the expected return values at the client at lines 288 and 290.

    TEST(METROL_1183, CompletedCompleteMessage)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize { 1024 + 1 };
        constexpr uint16_t receiveBufferSize { 1024 + 1 };

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        constexpr uint16_t nagglesTimeoutMs = 250; // Typical is 200 milliseconds

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            VARIABLE_IS_NOT_USED constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)
            VARIABLE_IS_NOT_USED constexpr bool masking {true};          // Flag set by client to enable masking

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            WebSocketClient<::Thunder::Core::SocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_FALSE(client.DeepInspection());

            EXPECT_TRUE(client.MarkerReceived());

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<::Thunder::Core::SocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            // Do not use '\0' as a marker as std::basic_strng<> assumes it is an end of string

            constexpr uint8_t data[] = { 0x1, 0xF };

            std::basic_string<uint8_t> message;

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                    // Construct a message large enough
                    constexpr size_t length = 1024;

                    ASSERT_GT(it.Client()->Link().SendBufferSize(), length);

                    const size_t count = (length / sizeof(data) + 1 ) * sizeof(data);

                    message.resize(count);

                    for (size_t index = 0; index < count; index += sizeof(data) ) {
                        message.replace(index, sizeof(data), data);
                    }

                    message.resize(length);

                    // Insert a unique marker
                    message.replace(length - 1, 1, 1, 0xA);

                    ASSERT_EQ(message.size(),length);

                    /* bool */ it.Client()->Submit(std::basic_string<uint8_t>{ message.data(), length });
                }
            }

            // Allow some time to receive the response
            SleepMs(maxWaitTimeMs);

            std::reverse(message.begin(), message.end());

            std::basic_string<uint8_t> response;

            response.reserve( message.size() );

            // A simple poll to keep it simple
            for (int8_t retry = 0; retry < maxRetries; ++retry) {
                SleepMs(nagglesTimeoutMs); // Naggle's typical delay, perhaps a bit more

                if (it.IsValid()) {
                    response = it.Client()->Response() + response;
                }
            }

            // Do not expect any response
            EXPECT_FALSE(   response.size() == message.size()
                        || response == message
                       );

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder

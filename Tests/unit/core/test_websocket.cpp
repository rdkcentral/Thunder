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
#include <cryptalgo/cryptalgo.h>

#ifndef SECURESOCKETS_ENABLED
#error "This unit test requires SecureSocketPort"
#endif 

#include "../IPTestAdministrator.h"

#ifdef VOLATILE_PATH
#define XSTR(s) STR(s)
#define STR(s) #s "/"
#else
#define XSTR(s)
#define STR(s)
#endif

#define _VERBOSE

namespace Thunder {
namespace Tests {
namespace Core {

    class CustomSocketStream :  public ::Thunder::Core::SocketStream {
    public:
        CustomSocketStream(
              const SOCKET& socket
            , const ::Thunder::Core::NodeId& localNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : SocketStream(false, socket, localNode, sendBufferSize, receiveBufferSize)
        {
        }

        CustomSocketStream(
              const bool
            , const ::Thunder::Core::NodeId& localNode
            , const ::Thunder::Core::NodeId& remoteNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : SocketStream(false, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
        {
        }

        ~CustomSocketStream()
        {
#ifdef _VERBOSE
            std::cout.flush();
#endif
        }

        // Raw TCP data
        int32_t Read(uint8_t buffer[], const uint16_t length) const override
        { 
#ifdef _VERBOSE
            std::cout << std::dec <<__LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            int32_t count = SocketPort::Read(buffer, length);

#ifdef _VERBOSE
            if (count > 0) {
                std::cout << " |--> buffer ( " << count << "/" << length << " ) = ";
                for (int32_t index = 0; index < count; index++) {
                    std::cout << std::hex << static_cast<int>(buffer[index]) << " ";
                }
                std::cout << "\n";
            }
#endif

            return count;
        }

        // Raw TCP data
        int32_t Write(const uint8_t buffer[], const uint16_t length) override
        {
#ifdef _VERBOSE
            std::cout << std::dec <<__LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            int32_t count = SocketPort::Write(buffer, length);

#ifdef _VERBOSE
            if (count > 0) {
                std::cout << " |--> buffer ( " << count << "/"<< length <<" ) = ";
                for (int32_t index = 0; index < count; index++) {
                    std::cout << std::hex << static_cast<int>(buffer[index]) << " ";
                }
                std::cout << "\n";
            }
#endif

            return count;
        }
    };

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
            : ::Thunder::Web::WebSocketClientType<STREAM>(path, protocol, query, origin, binary, masking, /* <Arguments SocketStream> */ std::forward<Args>(args)... /*</Arguments SocketStream>*/)
        {
        }

        ~WebSocketClient() override = default;

        // Non-idle then data available to send
        bool IsIdle() const override { return _post.size() == 0; }

        // Allow for eventfull state updates in this class
        void StateChange() override
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";

            // Socket port open AND upgraded to WebSocket 
            std::cout << " |--> IsOpen() = " << this->IsOpen() << "\n";
            // Link will not accept new messages as they arrive
            std::cout << " |--> IsSuspended() = " << this->IsSuspended() << "\n";
            // Socket has been closed (removed), link cannot receive new TCP data
            std::cout << " |--> IsClosed() = " << this->IsClosed() << "\n";
            // Regular HTTP connection, no upgraded connection
            std::cout << " |--> IsWebServer() = " << this->IsWebServer() << "\n";
            // Upgrade in progress
            std::cout << " |--> IsUpgrading() = " << this->IsUpgrading() << "\n";
            // Upgraded connection
            std::cout << " |--> IsWebSocket() = " << this->IsWebSocket() << "\n";
            // Finishing frame received
            std::cout << " |--> IsCompleted() = " << this->IsCompleted() << "\n";
#endif
        }

        // Reflects payload, effective after upgrade
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            size_t count = 0;

            if (   dataFrame != nullptr
                && maxSendSize > 0
                && !IsIdle()
                && ::Thunder::Web::WebSocketClientType<STREAM>::IsOpen()
                && ::Thunder::Web::WebSocketClientType<STREAM>::IsWebSocket() // Redundant, covered by IsOpen
                && ::Thunder::Web::WebSocketClientType<STREAM>::IsCompleted()
               ) {
                std::basic_string<uint8_t>& message = _post.front();

                count = std::min(message.size(), static_cast<size_t>(maxSendSize));

                /* void* */ memcpy(dataFrame, message.data(), count);

#ifdef _VERBOSE
                std::cout << " |--> dataFrame ( " << count << "/" << maxSendSize << " ) = ";
                for (int32_t index = 0; index < count; index++) {
                    std::cout << std::hex << static_cast<int>(dataFrame[index]) << " ";
                }
                std::cout << "\n";
#endif

                if (count == message.size()) {
                    /* iterator */ _post.erase(_post.begin());
                } else {
                    /* this */ message.erase(0, count);

                    // Trigger a call to SendData for remaining data
                    ::Thunder::Web::WebSocketClientType<STREAM>::Link().Trigger();
                }
            }

            return count;
        }

        // Reflects payload, effective after upgrade
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";

            if (receivedSize > 0) {
                std::cout << " |--> dataFrame ( " << receivedSize << " ) = ";
                for (int32_t index = 0; index < receivedSize; index++) {
                    std::cout << std::hex << static_cast<int>(dataFrame[index]) << " ";
                }
                std::cout << "\n";
            }
#endif

            // Echo the data in reverse order

            std::basic_string<uint8_t> message{ dataFrame, receivedSize };

            std::reverse(message.begin(), message.end());

            return Submit(message) ? message.size() : 0;
        }

        bool Submit(const std::basic_string<uint8_t>& message)
        {
            size_t count = _post.size();

            _post.emplace_back(message);

            ::Thunder::Web::WebSocketClientType<STREAM>::Link().Trigger();

            return count < _post.size();
        }

    private:

        std::vector<std::basic_string<uint8_t>> _post; // Send message queue
    };

    template <typename STREAM, size_t SENDBUFFERSIZE, size_t RECEIVEBUFFERSIZE>
    class WebSocketServer : public ::Thunder::Web::WebSocketServerType<STREAM> {
    public :

        // SocketServerType defines SocketHandler of type SocketListener. SocketListener triggers Accept on StateChange, effectively, calling SocketServerType::Accept(SOCKET, NodeId) which creates a WebSocketServer with these parameters
        WebSocketServer(const SOCKET& socket, const ::Thunder::Core::NodeId remoteNode, ::Thunder::Core::SocketServerType<WebSocketServer<STREAM, SENDBUFFERSIZE, RECEIVEBUFFERSIZE>>*)
            // Initially this should be defined as a regular TCP socket
            : ::Thunder::Web::WebSocketServerType<STREAM>(false /* binary*/, false /*masking */, socket, remoteNode, SENDBUFFERSIZE /* send buffer size */, RECEIVEBUFFERSIZE /* receive buffer size */)
        {
        }

        ~WebSocketServer() override = default;

        // Non-idle then data available to send
        bool IsIdle() const override { return _post.size() == 0; }

        // Allow for eventfull state updates in this class
        void StateChange() override
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";

            // Socket port open AND upgraded to WebSocket 
            std::cout << " |--> IsOpen() = " << this->IsOpen() << "\n";
            // Link will not accept new messages as they arrive
            std::cout << " |--> IsSuspended() = " << this->IsSuspended() << "\n";
            // Socket has been closed (removed), link cannot receive new TCP data
            std::cout << " |--> IsClosed() = " << this->IsClosed() << "\n";
            // Regular HTTP connection, no upgraded connection
            std::cout << " |--> IsWebServer() = " << this->IsWebServer() << "\n";
            // Upgrade in progress
            std::cout << " |--> IsUpgrading() = " << this->IsUpgrading() << "\n";
            // Upgraded connection
            std::cout << " |--> IsWebSocket() = " << this->IsWebSocket() << "\n";
            // Finishing frame received
            std::cout << " |--> IsCompleted() = " << this->IsCompleted() << "\n";
#endif
        }

        // Reflects payload, effective after upgrade
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            size_t count = 0;

            if (   dataFrame != nullptr
                && maxSendSize > 0
                && !IsIdle()
                && ::Thunder::Web::WebSocketServerType<STREAM>::IsOpen()
                && ::Thunder::Web::WebSocketServerType<STREAM>::IsWebSocket() // Redundant, covered by IsOpen
                && ::Thunder::Web::WebSocketServerType<STREAM>::IsCompleted()
               ) {
                std::basic_string<uint8_t>& message = _post.front();

                count = std::min(message.size(), static_cast<size_t>(maxSendSize));

                /* void* */ memcpy(dataFrame, message.data(), count);

#ifdef _VERBOSE
                std::cout << " |--> dataFrame (" << count << " ) = ";
                for (int32_t index = 0; index < count; index++) {
                    std::cout << std::hex << static_cast<int>(dataFrame[index]) << " ";
                }
                std::cout << "\n";
#endif

                if (count == message.size()) {
                    /* iterator */ _post.erase(_post.begin());
                } else {
                    /* this */ message.erase(0, count);

                    // Trigger a call to SendData for remaining data
                    ::Thunder::Web::WebSocketServerType<STREAM>::Link().Trigger();
                }
            }

            return count;
        }

        // Reflects payload, effective after upgrade
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            if (receivedSize > 0) {
                _response.emplace_back(std::basic_string<uint8_t>{ dataFrame, receivedSize });

#ifdef _VERBOSE
                std::cout << " |--> dataFrame ( " << receivedSize << " ) = ";
                for (int32_t index = 0; index < receivedSize; index++) {
                    std::cout << std::hex << static_cast<int>(dataFrame[index]) << " ";
                }
                std::cout << "\n";
#endif
            }

            return receivedSize;
        }

        // Put data in the queue to send (to the (connected) client)
        bool Submit(const std::basic_string<uint8_t>& message)
        {
            size_t count = _post.size();

            _post.emplace_back(message);

            // Trigger a call to SendData
            ::Thunder::Web::WebSocketServerType<STREAM>::Link().Trigger();

            return count < _post.size();
        }

        std::basic_string<uint8_t> Response()
        {
#ifdef _VERBOSE
            std::cout << std::dec << __LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
#endif

            std::basic_string<uint8_t> message;

            if (_response.size() > 0) {
                message = _response.front();
                _response.erase(_response.begin());

#ifdef _VERBOSE
                std::cout << " |--> message ( " << message.size() << " ) = ";
                for (int32_t index = 0; index < message.size(); index++) {
                    std::cout << std::hex << static_cast<int>(message[index]) << " ";
                }
                std::cout << "\n";
#endif
            } 

            return message;
        }

    private:

        std::vector<std::basic_string<uint8_t>> _post; // Send message queue
        std::vector<std::basic_string<uint8_t>> _response; // Receive message queue
    };

    class CustomSecureSocketStream : public ::Thunder::Crypto::SecureSocketPort {
    private :

        static constexpr char volatilePath[] = XSTR(VOLATILE_PATH);

        // Validat eclient certificate
        class Validator : public ::Thunder::Crypto::SecureSocketPort::IValidator {
        public:

            Validator() = default;
            ~Validator() = default;

            bool Validate(const Certificate& certificate) const override {
                // Print certificate properties
#ifdef _VERBOSE
                std::cout << std::dec <<__LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
                std::cout << " |--> Issuer = " << certificate.Issuer() << "\n";
                std::cout << " |--> Subject = " << certificate.Subject() << "\n";
                std::cout << " |--> Valid from = " << certificate.ValidFrom().ToRFC1123() << "\n";
                std::cout << " |--> Valid until = " << certificate.ValidTill().ToRFC1123() << "\n";
#endif
                return true; // Always accept
            }
        };

   public :

        // In essence, all parameters to SecureSocket are passed to a base class SocketPort
        CustomSecureSocketStream(
              const SOCKET& socket
            , const ::Thunder::Core::NodeId& localNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::CLIENT_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostClient.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostClient.key"}), ::Thunder::Core::SocketPort::STREAM, socket, localNode, sendBufferSize, receiveBufferSize)
            , _validator{}
        {
            // Validate custom (sefl signed) certificates
            uint32_t result = Callback(&_validator);
        }

        CustomSecureSocketStream(
              const bool
            , const ::Thunder::Core::NodeId& localNode
            , const ::Thunder::Core::NodeId& remoteNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::CLIENT_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostClient.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostClient.key"}), ::Thunder::Core::SocketPort::STREAM, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
            , _validator{}
        {
            // Validate custom (self signed) client certificates
            uint32_t result = Callback(&_validator);
        }

        ~CustomSecureSocketStream()
        {
 #ifdef _VERBOSE
            std::cout.flush();
#endif
        }

    private:
        const std::string _prefix;
        Validator _validator;
    };

    /* static */  constexpr char CustomSecureSocketStream::volatilePath[];

    class CustomSecureServerSocketStream : public ::Thunder::Crypto::SecureSocketPort {
    private :

        static constexpr char volatilePath[] = XSTR(VOLATILE_PATH);

    public :

        // In essence, all parameters to SecureSocket are passed to a base class SocketPort
        CustomSecureServerSocketStream(
              const SOCKET& socket
            , const ::Thunder::Core::NodeId& localNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::SERVER_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.key"}), ::Thunder::Core::SocketPort::STREAM, socket, localNode, sendBufferSize, receiveBufferSize)
        {}

        CustomSecureServerSocketStream(
              const bool
            , const ::Thunder::Core::NodeId& localNode
            , const ::Thunder::Core::NodeId& remoteNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::SERVER_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.key"}), ::Thunder::Core::SocketPort::STREAM, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
        {}

        ~CustomSecureServerSocketStream()
        {
#ifdef _VERBOSE
            std::cout.flush();
#endif
        }
    };

    /* static */  constexpr char CustomSecureServerSocketStream::volatilePath[];

    class CustomSecureServerSocketStreamClientValidation : public ::Thunder::Crypto::SecureSocketPort {
    private :

        static constexpr char volatilePath[] = XSTR(VOLATILE_PATH);

        // Validat eclient certificate
        class Validator : public ::Thunder::Crypto::SecureSocketPort::IValidator {
        public:

            Validator() = default;
            ~Validator() = default;

            bool Validate(const Certificate& certificate) const override {
                // Print certificate properties
#ifdef _VERBOSE
                std::cout << std::dec <<__LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
                std::cout << " |--> Issuer = " << certificate.Issuer() << "\n";
                std::cout << " |--> Subject = " << certificate.Subject() << "\n";
                std::cout << " |--> Valid from = " << certificate.ValidFrom().ToRFC1123() << "\n";
                std::cout << " |--> Valid until = " << certificate.ValidTill().ToRFC1123() << "\n";
#endif
                return true; // Always accept
            }
        };

   public :

        // In essence, all parameters to SecureSocket are passed to a base class SocketPort
        CustomSecureServerSocketStreamClientValidation(
              const SOCKET& socket
            , const ::Thunder::Core::NodeId& localNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::SERVER_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.key"}), true, ::Thunder::Core::SocketPort::STREAM, socket, localNode, sendBufferSize, receiveBufferSize)
            , _validator{}
        {
            // Validate custom (sefl signed) certificates
            uint32_t result = Callback(&_validator);
        }

        CustomSecureServerSocketStreamClientValidation(
              const bool
            , const ::Thunder::Core::NodeId& localNode
            , const ::Thunder::Core::NodeId& remoteNode
            , const uint16_t sendBufferSize
            , const uint16_t receiveBufferSize
        )
            : ::Thunder::Crypto::SecureSocketPort(::Thunder::Crypto::SecureSocketPort::context_t::SERVER_CONTEXT, static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.pem"}), static_cast<const std::string&>(std::string{volatilePath} + std::string{"localhostServer.key"}), true, ::Thunder::Core::SocketPort::STREAM, localNode, remoteNode, sendBufferSize, receiveBufferSize, sendBufferSize, receiveBufferSize)
            , _validator{}
        {
            // Validate custom (self signed) client certificates
            uint32_t result = Callback(&_validator);
        }

        ~CustomSecureServerSocketStreamClientValidation()
        {
#ifdef _VERBOSE
            std::cout.flush();
#endif
        }

    private:
        const std::string _prefix;
        Validator _validator;
    };

    /* static */ constexpr char CustomSecureServerSocketStreamClientValidation::volatilePath[];

    TEST(WebSocket, DISABLED_OpeningServerPort)
    {
        const TCHAR localHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        // This is a listening socket as result of using SocketServerType which enables listening
        ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

        ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

        SleepMs(maxWaitTimeMs);

        // Obtain the endpoint at the server side for each (remotely) connected client
        auto it = server.Clients();

        if (it.Next()) {
            // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

            if (it.Client()->IsOpen()) {
                // No data should be transferred to the remote client
            }
        }

        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

    TEST(WebSocket, DISABLED_OpeningClientPort)
    {
        const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
        const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
        const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
        const std::string webSocketOrigin;      // Optional, set by browser clients
        constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
        constexpr bool masking {true};          // Flag set by client to enable masking

        const TCHAR remoteHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        constexpr bool rawSocket {false};

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

        SleepMs(maxWaitTimeMs);

        ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

    TEST(WebSocket, DISABLED_UnsecuredSocketUpgrade)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
//            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(WebSocket, DISABLED_UnsecuredSocketServerPingClientPong)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
//            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                   /* void */ it.Client()->Ping();
                }
            }

            // Allow some time to receive the PONG response
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(WebSocket, DISABLED_UnsecuredSocketServerUnsollicitedPong)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Allow some time to receive the PONG
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
//            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                   /* void */ it.Client()->Pong();
                }
            }

            // Avoid  premature shutdown() at the receiving side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(WebSocket, DISABLED_UnsecuredSocketClientUnsollicitedPong)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            client.Pong();
 
            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
//            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            // Allow some time to receive the PONG
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    } 

    TEST(WebSocket, DISABLED_UnsecuredSocketDataExchange)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            constexpr uint8_t data[] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x3, 0x2, 0x1, 0x0 };

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                    /* bool */ it.Client()->Submit(std::basic_string<uint8_t>{ data, sizeof(data) });
                }
            }

            // Allow some time to receive the response
            SleepMs(maxWaitTimeMs);

            std::basic_string<uint8_t> response{ data, sizeof(data) };
            std::reverse(response.begin(), response.end());

            // A simple poll to keep it simple
            EXPECT_TRUE(   it.IsValid()
                        && (it.Client()->Response() == response)
            );

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    } 

    TEST(WebSocket, DISABLED_UnsecuredSocketMultiFrameDataExchange)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {75};
        constexpr uint16_t receiveBufferSize {75};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        constexpr uint16_t nagglesTimeoutMs = 250; // Typical is 200 milliseconds

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            WebSocketClient<CustomSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

#ifdef _VERBOSE
            std::cout << std::dec <<__LINE__ << " : " << __PRETTY_FUNCTION__ << "\n";
            std::cout << " |--> SendBufferSize = " << client.Link().SendBufferSize() << "\n";
            std::cout << " |--> ReceiveBufferSize  = " << client.Link().ReceiveBufferSize() << "\n";
            std::cout << " |--> SocketSendBufferSize = " << client.Link().SocketSendBufferSize() << "\n";
            std::cout << " |--> SocketReceiveBufferSize = " << client.Link().SocketReceiveBufferSize() << "\n";
#endif

            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            // Do not use '\0' as a marker as std::basic_strng<> assumes it is an end of string

            constexpr uint8_t data[] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x3, 0x2, 0x1, 0x10 };

            std::basic_string<uint8_t> message;

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                    // Construct a message larger than the buffer size to force use of continuation frames
                    size_t count = (it.Client()->Link().SendBufferSize() / sizeof(data) + 1 ) * sizeof(data);

                    ASSERT_LE(count, message.max_size());

                    message.resize(count);

                    for (size_t index = 0; index < count; index += sizeof(data) ) {
                        message.replace(index, sizeof(data), data);
                    }

                    /* bool */ it.Client()->Submit(std::basic_string<uint8_t>{ message.data(), count });
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

            EXPECT_TRUE(   response.size() == message.size()
                        && response == message
                       );

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

    TEST(WebSocket, DISABLED_OpeningSecuredServerPort)
    {
        const TCHAR localHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        // This is a listening socket as result of using SocketServerType which enables listening
        ::Thunder::Core::SocketServerType<WebSocketServer<CustomSecureServerSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

        ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

//        SleepMs(maxWaitTimeMs);

        // Obtain the endpoint at the server side for each (remotely) connected client
        auto it = server.Clients();

        if (it.Next()) {
            // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality
            if (it.Client()->IsOpen()) {
                // No data should be transferred to the remote client
            } else {
           }
        }

        SleepMs(4*maxWaitTimeMs);


        EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

    TEST(WebSocket, DISABLED_OpeningSecuredClientPort)
    {
        const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
        const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
        const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
        const std::string webSocketOrigin;      // Optional, set by browser clients
        constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
        constexpr bool masking {true};          // Flag set by client to enable masking

        const TCHAR remoteHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        constexpr bool rawSocket {false};

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        WebSocketClient<CustomSecureSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

//        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

    TEST(WebSocket, DISABLED_SecuredSocketDataExchange)
    {
        const TCHAR hostName[] {"127.0.0.1"};

        // Some aliases
        const auto& remoteHostName = hostName;
        const auto& localHostName = hostName;

        constexpr uint16_t tcpServerPort {12346};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 8, maxWaitTimeMs = 8000, maxInitTime = 2000;
        constexpr uint8_t maxRetries = 10;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            const std::string webSocketURIPath;     // HTTP URI part, empty path allowed 
            const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
            const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
            const std::string webSocketOrigin;      // Optional, set by browser clients
            constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)  
            constexpr bool masking {true};          // Flag set by client to enable masking

            constexpr bool rawSocket {false};

            const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            WebSocketClient<CustomSecureSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

            ASSERT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            // Avoid  premature shutdown() at the other side
            SleepMs(maxWaitTimeMs);

            EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

            // This is a listening socket as result of using SocketServerType which enables listening
            ::Thunder::Core::SocketServerType<WebSocketServer<CustomSecureServerSocketStream, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

            ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

            // A small delay so the child can be set up
            SleepMs(maxInitTime);

            EXPECT_EQ(testAdmin.Signal(initHandshakeValue, maxRetries), ::Thunder::Core::ERROR_NONE);

            EXPECT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            // Obtain the endpoint at the server side for each (remotely) connected client
            auto it = server.Clients();

            constexpr uint8_t data[] = { 0xF, 0xE, 0xD, 0xC, 0xB, 0xA, 0x9, 0x8, 0x7, 0x6, 0x5, 0x3, 0x2, 0x1, 0x0 };

            if (it.Next()) {
                // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality

                if (it.Client()->IsOpen()) {
                    /* bool */ it.Client()->Submit(std::basic_string<uint8_t>{ data, sizeof(data) });
                }
            }

            // Allow some time to receive the response
            SleepMs(maxWaitTimeMs);

            std::basic_string<uint8_t> response{ data, sizeof(data) };
            std::reverse(response.begin(), response.end());

            // A simple poll to keep it simple
            EXPECT_TRUE(   it.IsValid()
                        && (it.Client()->Response() == response)
            );

            EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    } 

    TEST(WebSocket, DISABLED_SecuredServerPortCertificateRequest)
    {
        const TCHAR localHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId localNode {localHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        // This is a listening socket as result of using SocketServerType which enables listening
        ::Thunder::Core::SocketServerType<WebSocketServer<CustomSecureServerSocketStreamClientValidation, sendBufferSize, receiveBufferSize>> server(localNode /* listening node*/);

        ASSERT_EQ(server.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);

        SleepMs(maxWaitTimeMs);

        // Obtain the endpoint at the server side for each (remotely) connected client
        auto it = server.Clients();

        if (it.Next()) {
            // Unless a client has send an upgrade request we cannot send data out although we might be calling WebSocket functionality
            if (it.Client()->IsOpen()) {
                // No data should be transferred to the remote client
            }
        }

        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(server.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

    TEST(WebSocket, DISABLED_OpeningSecuredClientPortCertificateRequest)
    {
        const std::string webSocketURIPath;     // HTTP URI part, empty path allowed
        const std::string webSocketProtocol;    // Optional HTTP field, WebSocket SubProtocol, ie, Sec-WebSocket-Protocol
        const std::string webSocketURIQuery;    // HTTP URI part, absent query allowe
        const std::string webSocketOrigin;      // Optional, set by browser clients
        constexpr bool binary {false};          // Flag to indicate WebSocket opcode 0x1 (test frame) or 0x2 (binary frame)
        constexpr bool masking {true};          // Flag set by client to enable masking

        const TCHAR remoteHostName[] {"127.0.0.1"};

        constexpr uint16_t tcpServerPort {12345};   // TCP, default 80 or 443 (SSL)
        constexpr uint32_t tcpProtocol {0};         // HTTP or HTTPS but can only be set on raw sockets

        constexpr bool rawSocket {false};

        // The minimum size is determined by the HTTP upgrade process. The limit here is above that threshold.
        constexpr uint16_t sendBufferSize {1024};
        constexpr uint16_t receiveBufferSize {1024};

        constexpr uint32_t maxWaitTimeMs = 4000;

        const ::Thunder::Core::NodeId remoteNode {remoteHostName, tcpServerPort, ::Thunder::Core::NodeId::TYPE_IPV4, tcpProtocol};

        WebSocketClient<CustomSecureSocketStream> client(webSocketURIPath, webSocketProtocol, webSocketURIQuery, webSocketOrigin, false, true, rawSocket, remoteNode.AnyInterface(), remoteNode, sendBufferSize, receiveBufferSize);

//        SleepMs(maxWaitTimeMs);
        EXPECT_EQ(client.Open(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE); // Fails in non-websocket server context

        SleepMs(maxWaitTimeMs);

        EXPECT_EQ(client.Close(maxWaitTimeMs), ::Thunder::Core::ERROR_NONE);
    }

} // Core
} // Tests
} // Thunder

#ifdef VOLATILE_PATH
#undef STR
#undef XSTR
#endif

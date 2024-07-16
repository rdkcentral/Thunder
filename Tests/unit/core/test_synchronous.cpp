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

#include "../IPTestAdministrator.h"

namespace Thunder {
namespace Tests {
namespace Core {

    namespace {
        const string localhost = "127.0.0.1";
        const uint16_t portNumber = 9749;
        const uint16_t bufferSize = 1024;
    }

    class Message : public ::Thunder::Core::IOutbound {
    protected:
        Message(uint8_t buffer[])
        : _buffer(buffer)
        {
        }

    public:
        Message() = delete;
        Message(const Message&) = delete;
        Message& operator=(const Message&) = delete;

        Message(const uint16_t size, uint8_t buffer[])
            : _size(size)
            , _buffer(buffer)
            , _offset(0)
        {
        }

        virtual ~Message()
        {
        }

    private:
        virtual void Reload() const override
        {
            _offset = 0;
        }

        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
        {
            uint16_t result = std::min(static_cast<uint16_t>(sizeof(_buffer) - _offset), length);
            if (result > 0) {

                ::memcpy(stream, &(_buffer[_offset]), result);
                _offset += result;
            }
            return (result);
        }

    private:
        uint16_t _size;
        uint8_t* _buffer;
        mutable uint16_t _offset;
    };

    class InMessage : public ::Thunder::Core::IInbound {
    protected:
        InMessage(uint8_t buffer[])
           : _buffer(buffer)
        {
        }

    public:
        InMessage(const InMessage&) = delete;
        InMessage& operator=(const InMessage&) = delete;

        InMessage()
            : _buffer()
            , _error(~0)
            , _offset(0)
        {
        }

        virtual ~InMessage()
        {
        }

    private:
        virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
        {
            uint16_t result = 0;
            uint16_t toCopy = std::min(static_cast<uint16_t>(sizeof(_buffer)-_offset), length);
            ::memcpy(reinterpret_cast<uint8_t*>(&stream), &(_buffer[_offset]), toCopy);

            _error = ::Thunder::Core::ERROR_NONE;
            result = length;
            return (result);
        }

        virtual state IsCompleted() const override 
        {
            return (_error != static_cast<uint16_t>(~0) ? state::COMPLETED : state::INPROGRESS);
        }

    private:
        uint8_t* _buffer;
        mutable uint16_t _error;
        mutable uint16_t _offset;
    };

    class SynchronousSocket : public ::Thunder::Core::SynchronousChannelType<::Thunder::Core::SocketPort> {
    public:
        SynchronousSocket(const SynchronousSocket&) = delete;
        SynchronousSocket& operator=(const SynchronousSocket&) = delete;
        SynchronousSocket() = delete;

        SynchronousSocket(bool listening)
            : SynchronousChannelType<::Thunder::Core::SocketPort>(
                (listening ? ::Thunder::Core::SocketPort::LISTEN : ::Thunder::Core::SocketPort::STREAM)
              , (listening ? ::Thunder::Core::NodeId(_T(localhost.c_str()), (portNumber), ::Thunder::Core::NodeId::TYPE_IPV4)
                           : ::Thunder::Core::NodeId(_T(localhost.c_str()), (portNumber), ::Thunder::Core::NodeId::TYPE_DOMAIN)
                )
              , (listening ? ::Thunder::Core::NodeId(_T(localhost.c_str()), (portNumber), ::Thunder::Core::NodeId::TYPE_DOMAIN)
                           : ::Thunder::Core::NodeId(_T(localhost.c_str()), (portNumber), ::Thunder::Core::NodeId::TYPE_IPV4)
                )
              , bufferSize, bufferSize
              )
        {
            EXPECT_FALSE(::Thunder::Core::SynchronousChannelType<::Thunder::Core::SocketPort>::Open(::Thunder::Core::infinite) != ::Thunder::Core::ERROR_NONE);
        }

        virtual ~SynchronousSocket()
        {
            ::Thunder::Core::SynchronousChannelType<::Thunder::Core::SocketPort>::Close(::Thunder::Core::infinite);
        }

        virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData)
        {
            return 1;
        }
    };

    TEST(test_synchronous, simple_synchronous)
    {
        constexpr uint32_t initHandshakeValue = 0, maxWaitTime = 4, maxWaitTimeMs = 4000, maxInitTime = 2000;

        IPTestAdministrator::Callback callback_child = [&](IPTestAdministrator& testAdmin) {
            SynchronousSocket synchronousServerSocket(true);

            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Wait(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator::Callback callback_parent = [&](IPTestAdministrator& testAdmin) {
            // a small delay so the child can be set up
            SleepMs(maxInitTime);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            SynchronousSocket synchronousClientSocket(false);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            uint8_t buffer[] = "Hello";
            Message message(sizeof(buffer), buffer);
            // Outbound
            EXPECT_EQ(synchronousClientSocket.Exchange(maxWaitTimeMs, message), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
           
            InMessage response; 
            Message newmessage(sizeof(buffer), buffer);
            // Inbound
            EXPECT_EQ(synchronousClientSocket.Exchange(maxWaitTimeMs, newmessage, response), ::Thunder::Core::ERROR_NONE);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);

            synchronousClientSocket.Revoke(message);

            ASSERT_EQ(testAdmin.Signal(initHandshakeValue), ::Thunder::Core::ERROR_NONE);
        };

        IPTestAdministrator testAdmin(callback_parent, callback_child, initHandshakeValue, maxWaitTime);

        // Code after this line is executed by both parent and child

        ::Thunder::Core::Singleton::Dispose();
    }

} // Core
} // Tests
} // Thunder

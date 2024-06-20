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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace Thunder;
using namespace Thunder::Core;

namespace {
    const string localhost = "127.0.0.1";
    const uint16_t portNumber = 9749;
    const uint16_t bufferSize = 1024;
}

class Message : public Core::IOutbound {
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

class InMessage : public Core::IInbound {
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

        _error = Core::ERROR_NONE;
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

class SynchronousSocket : public Core::SynchronousChannelType<Core::SocketPort> {
public:
    SynchronousSocket(const SynchronousSocket&) = delete;
    SynchronousSocket& operator=(const SynchronousSocket&) = delete;
    SynchronousSocket() = delete;

    SynchronousSocket(bool listening)
        :SynchronousChannelType<SocketPort>((listening ? SocketPort::LISTEN : SocketPort::STREAM),listening ?Core::NodeId(_T(localhost.c_str()),(portNumber),Core::NodeId::TYPE_IPV4):Core::NodeId(_T(localhost.c_str()),(portNumber),Core::NodeId::TYPE_DOMAIN),listening ?Core::NodeId(_T(localhost.c_str()),(portNumber),Core::NodeId::TYPE_DOMAIN):Core::NodeId(_T(localhost.c_str()),(portNumber),Core::NodeId::TYPE_IPV4), bufferSize, bufferSize)
    {
        EXPECT_FALSE(Core::SynchronousChannelType<Core::SocketPort>::Open(Core::infinite) != Core::ERROR_NONE);
    }

    virtual ~SynchronousSocket()
    {
        Core::SynchronousChannelType<Core::SocketPort>::Close(Core::infinite);
    }

    virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData)
    {
        return 1;
    }
};

TEST(test_synchronous, simple_synchronous)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin) {
        SynchronousSocket synchronousServerSocket(true);
        testAdmin.Sync("setup server");

        testAdmin.Sync("connect client");
        testAdmin.Sync("client msg");
        testAdmin.Sync("client newmsg");
        testAdmin.Sync("client revokemsg");
    };

    IPTestAdministrator testAdmin(otherSide);
    {
        testAdmin.Sync("setup server");
        SynchronousSocket synchronousClientSocket(false);

        testAdmin.Sync("connect client");
        uint8_t buffer[] = "Hello";
        Message message(5,buffer);
        EXPECT_EQ(synchronousClientSocket.Exchange(500, message),0u);
        testAdmin.Sync("client msg");
       
        InMessage response; 
        Message newmessage(5,buffer);
        synchronousClientSocket.Exchange(500, newmessage, response);//TODO Output verification is pending.
        testAdmin.Sync("client newmsg");
        synchronousClientSocket.Revoke(message);
        testAdmin.Sync("client revokemsg");
    }
    Core::Singleton::Dispose();
}

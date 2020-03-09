/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "IValueIterator.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    ProxyStub::MethodHandler ValueIteratorStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            uint32_t result = 0;

            // virtual bool Next(string& text) = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = message->Parameters().Implementation<RPC::IValueIterator>()->Next(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Number(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            uint32_t result = 0;

            // virtual bool Previous() = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = message->Parameters().Implementation<RPC::IValueIterator>()->Previous(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Number(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Reset(const uint32_t position)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            uint32_t position(parameters.Number<uint32_t>());

            message->Parameters().Implementation<RPC::IValueIterator>()->Reset(position);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual bool IsValid() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(message->Parameters().Implementation<RPC::IValueIterator>()->IsValid());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Count() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IValueIterator>()->Count());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual string Current() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number(message->Parameters().Implementation<RPC::IValueIterator>()->Current());
        },
        nullptr
    };

    typedef ProxyStub::UnknownStubType<RPC::IValueIterator, ValueIteratorStubMethods> ValueIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class ValueIteratorProxy : public UnknownProxyType<RPC::IValueIterator> {
    public:
        ValueIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
            TRACE_L1("Constructed ValueIteratorProxy: %p", this);
        }
        virtual ~ValueIteratorProxy()
        {
            TRACE_L1("Destructed ValueIteratorProxy: %p", this);
        }

    public:
        virtual bool Next(uint32_t& result) override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(0));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Number<uint32_t>();
                }
            }

            return (valid);
        }
        virtual bool Previous(uint32_t& result) override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Number<uint32_t>();
                }
            }

            return (valid);
        }
        virtual void Reset(const uint32_t position) override
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(position);

            Invoke(newMessage);
        }
        virtual bool IsValid() const override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(3));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                valid = newMessage->Response().Reader().Boolean();
            }
            return (valid);
        }
        virtual uint32_t Count() const override
        {
            uint32_t result = 0;
            IPCMessage newMessage(BaseClass::Message(4));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
        virtual uint32_t Current() const override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(5));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
    };

    // -------------------------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------------------------

    namespace {

        class RPCInstantiation {
        public:
            RPCInstantiation()
            {
                RPC::Administrator::Instance().Announce<RPC::IValueIterator, ValueIteratorProxy, ValueIteratorStub>();
            }
            ~RPCInstantiation()
            {
            }

        } RPCRegistration;
    }
}
}

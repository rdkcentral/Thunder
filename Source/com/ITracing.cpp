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

#include "ITracing.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    ProxyStub::MethodHandler TraceControllerStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Enable(const bool enabled, const string& category, const string& module) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            Trace::ITraceController* implementation(parameters.Implementation<Trace::ITraceController>());

            ASSERT(implementation != nullptr);

            bool enabled(reader.Boolean());
            string module(reader.Text());
            string category(reader.Text());

            implementation->Enable(enabled, module, category);
        },
        nullptr
    };

    ProxyStub::MethodHandler TraceIteratorStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Reset () = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            message->Parameters().Implementation<Trace::ITraceIterator>()->Reset();
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual bool Info(bool& enabled, string& category, string& module) const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool enabled;
            string category;
            string module;

            response.Boolean(message->Parameters().Implementation<Trace::ITraceIterator>()->Info(enabled, module, category));
            response.Boolean(enabled);
            response.Text(module);
            response.Text(category);
        },
        nullptr
    };

    typedef ProxyStub::UnknownStubType<Trace::ITraceController, TraceControllerStubMethods> TraceControllerStub;
    typedef ProxyStub::UnknownStubType<Trace::ITraceIterator, TraceIteratorStubMethods> TraceIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class TraceControllerProxy : public UnknownProxyType<Trace::ITraceController> {
    public:
        TraceControllerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~TraceControllerProxy()
        {
        }

    public:
        virtual void Enable(const bool enabled, const string& module, const string& category)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(enabled);
            writer.Text(module);
            writer.Text(category);

            Invoke(newMessage);
        }
    };

    class TraceIteratorProxy : public UnknownProxyType<Trace::ITraceIterator> {
    public:
        TraceIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~TraceIteratorProxy()
        {
        }

    public:
        virtual void Reset()
        {
            IPCMessage newMessage(BaseClass::Message(0));
            Invoke(newMessage);
        }
        virtual bool Info(bool& enabled, string& module, string& category) const
        {
            bool result = false;

            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

                result = reader.Boolean();
                enabled = reader.Boolean();
                module = reader.Text();
                category = reader.Text();
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
                RPC::Administrator::Instance().Announce<Trace::ITraceController, TraceControllerProxy, TraceControllerStub>();
                RPC::Administrator::Instance().Announce<Trace::ITraceIterator, TraceIteratorProxy, TraceIteratorStub>();
            }
            ~RPCInstantiation()
            {
            }

        } RPCRegistration;
    }
    // Creat a Handler for the Trace Controller:
    class TraceIterator : public Trace::ITraceIterator {
    private:
        TraceIterator(const TraceIterator&);
        TraceIterator& operator=(const TraceIterator&);

    public:
        TraceIterator()
            : _traceIterator(Trace::TraceUnit::Instance().GetCategories())
        {
            TRACE_L1("Created an object for interfaceId <Trace::ITraceIterator>, located @0x%p.\n", this);
        }
        virtual ~TraceIterator()
        {
            TRACE_L1("Destructed an object for interfaceId <Trace::ITraceIterator>, located @0x%p.\n", this);
        }

    public:
        virtual void Reset() override
        {
            _traceIterator.Reset(0);
        }
        virtual bool Info(bool& enabled, string& module, string& category) const override
        {
            bool result = _traceIterator.Next();

            if (result == true) {
                category = _traceIterator->Category();
                module = _traceIterator->Module();
                enabled = _traceIterator->Enabled();
            }

            return (result);
        }

        BEGIN_INTERFACE_MAP(TraceIterator)
        INTERFACE_ENTRY(Trace::ITraceIterator)
        END_INTERFACE_MAP

    private:
        mutable Trace::TraceUnit::Iterator _traceIterator;
    };

    SERVICE_REGISTRATION(TraceIterator, 1, 0);

    class TraceController : public Trace::ITraceController {
    private:
        TraceController(const TraceController&);
        TraceController& operator=(const TraceController&);

    public:
        TraceController()
        {
            TRACE_L1("Constructed an object for interfaceId <Trace::ITraceController>, located @0x%p.\n", this);
        }
        virtual ~TraceController()
        {
            TRACE_L1("Destructed an object for interfaceId <Trace::ITraceController>, located @0x%p.\n", this);
        }

    public:
        virtual void Enable(const bool enabled, const string& module, const string& category) override
        {
            Trace::TraceUnit::Instance().SetCategories(
                enabled,
                (module.empty() == false ? module.c_str() : nullptr),
                (category.empty() == false ? category.c_str() : nullptr));
        }

        BEGIN_INTERFACE_MAP(TraceController)
        INTERFACE_ENTRY(Trace::ITraceController)
        END_INTERFACE_MAP
    };

    SERVICE_REGISTRATION(TraceController, 1, 0);
}
}

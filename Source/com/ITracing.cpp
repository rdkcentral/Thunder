#include "ITracing.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    ProxyStub::MethodHandler RemoteConnectionStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Parent() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IRemoteConnection>()->Parent());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Id() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IRemoteConnection>()->Id());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t RemtoteId() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IRemoteConnection>()->RemoteId());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            uint32_t waitTime(parameters.Number<uint32_t>());
            string className(parameters.Text());
            uint32_t interfaceId(parameters.Number<uint32_t>());
            uint32_t version(parameters.Number<uint32_t>());

            response.Number<void*>(message->Parameters().Implementation<RPC::IRemoteConnection>()->Aquire(waitTime, className, interfaceId, version));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Terminate() = 0;
            message->Parameters().Implementation<RPC::IRemoteConnection>()->Terminate();
        },
        nullptr
    };

    ProxyStub::MethodHandler RemoteConnectionNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Activated(RPC::IRemoteConnection*) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            RPC::IRemoteConnection::INotification* implementation(parameters.Implementation<RPC::IRemoteConnection::INotification>());

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {

                ProxyStub::UnknownProxy* proxy = RPC::Administrator::Instance().ProxyInstance(channel, reader.Number<void*>(), RPC::IRemoteConnection::ID, false, RPC::IRemoteConnection::ID, true);
                RPC::IRemoteConnection* param0_proxy = (proxy != nullptr ? proxy->QueryInterface<RPC::IRemoteConnection>() : nullptr);

                implementation->Activated(param0_proxy);

                if (param0_proxy != nullptr) {
                    RPC::Administrator::Instance().Release(proxy, message->Response());
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Deactivated(RPC::IRemoteConnection*) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            RPC::IRemoteConnection::INotification* implementation(parameters.Implementation<RPC::IRemoteConnection::INotification>());

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {

                ProxyStub::UnknownProxy* proxy = RPC::Administrator::Instance().ProxyInstance(channel, reader.Number<void*>(), RPC::IRemoteConnection::ID, false, RPC::IRemoteConnection::ID, true);
                RPC::IRemoteConnection* param0_proxy = (proxy != nullptr ? proxy->QueryInterface<RPC::IRemoteConnection>() : nullptr);

                implementation->Deactivated(param0_proxy);

                if (param0_proxy != nullptr) {
                    RPC::Administrator::Instance().Release(proxy, message->Response());
                }
            }
        },
        nullptr
    };

    ProxyStub::MethodHandler ProcessStubMethods[] = {
        // virtual void SetCallsign(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            RPC::IProcess* implementation = input.Implementation<RPC::IProcess>();
            ASSERT((implementation != nullptr) && "Null RPC::IProcess implementation pointer");
            implementation->SetCallsign(param0);
        },

        // virtual const string& Callsign() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const RPC::IProcess* implementation = input.Implementation<RPC::IProcess>();
            ASSERT((implementation != nullptr) && "Null RPC::IProcess implementation pointer");
            const string output = implementation->Callsign();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    };

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

    typedef ProxyStub::UnknownStubType<RPC::IRemoteConnection, RemoteConnectionStubMethods> RemoteConnectionStub;
    typedef ProxyStub::UnknownStubType<RPC::IRemoteConnection::INotification, RemoteConnectionNotificationStubMethods> RemoteConnectionNotificationStub;
    typedef ProxyStub::UnknownStubType<RPC::IProcess, ProcessStubMethods> ProcessStub;
    typedef ProxyStub::UnknownStubType<Trace::ITraceController, TraceControllerStubMethods> TraceControllerStub;
    typedef ProxyStub::UnknownStubType<Trace::ITraceIterator, TraceIteratorStubMethods> TraceIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class RemoteConnectionProxy : public UnknownProxyType<RPC::IRemoteConnection> {
    public:
        RemoteConnectionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~RemoteConnectionProxy()
        {
        }

    public:
        virtual uint32_t Parent() const
        {
            uint32_t id = 0;

            IPCMessage newMessage(BaseClass::Message(0));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                id = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (id);
        }
        virtual uint32_t Id() const
        {
            uint32_t id = ~0;

            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                id = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (id);
        }
        virtual uint32_t RemoteId() const
        {
            uint32_t id = 0;

            IPCMessage newMessage(BaseClass::Message(2));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                id = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (id);
        }
        virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
        {
            void* result = nullptr;
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number(waitTime);
            writer.Text(className);
            writer.Number(interfaceId);
            writer.Number(version);

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = Interface(newMessage->Response().Reader().Number<void*>(), interfaceId);
            }
            return (result);
        }
        virtual void Terminate()
        {
            IPCMessage newMessage(BaseClass::Message(4));

            Invoke(newMessage);
        }
    };

    class RemoteConnectionNotificationProxy : public UnknownProxyType<RPC::IRemoteConnection::INotification> {
    public:
        RemoteConnectionNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~RemoteConnectionNotificationProxy()
        {
        }

    public:
        virtual void Activated(RPC::IRemoteConnection* process)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::IRemoteConnection*>(process);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual void Deactivated(RPC::IRemoteConnection* process)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::IRemoteConnection*>(process);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    };

    class ProcessProxy final : public ProxyStub::UnknownProxyType<RPC::IProcess> {
    public:
        ProcessProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void SetCallsign(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        const string& Callsign() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    };

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
                RPC::Administrator::Instance().Announce<RPC::IRemoteConnection, RemoteConnectionProxy, RemoteConnectionStub>();
                RPC::Administrator::Instance().Announce<RPC::IRemoteConnection::INotification, RemoteConnectionNotificationProxy, RemoteConnectionNotificationStub>();
                RPC::Administrator::Instance().Announce<RPC::IProcess, ProcessProxy, ProcessStub>();
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

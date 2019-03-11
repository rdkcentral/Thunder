#include "ITracing.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    ProxyStub::MethodHandler RemoteProcessStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Id() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IRemoteProcess>()->Id());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual enumState State() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<RPC::IRemoteProcess::enumState>(message->Parameters().Implementation<RPC::IRemoteProcess>()->State());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            uint32_t waitTime(parameters.Number<uint32_t>());
            string className(parameters.Text());
            uint32_t interfaceId(parameters.Number<uint32_t>());
            uint32_t version(parameters.Number<uint32_t>());

            response.Number<void*>(message->Parameters().Implementation<RPC::IRemoteProcess>()->Aquire(waitTime, className, interfaceId, version));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Terminate() = 0;
            message->Parameters().Implementation<RPC::IRemoteProcess>()->Terminate();
        },
        nullptr
    };

    ProxyStub::MethodHandler RemoteProcessNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Activated(RPC::IRemoteProcess*) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            RPC::IRemoteProcess::INotification* implementation(parameters.Implementation<RPC::IRemoteProcess::INotification>());

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {

                ProxyStub::UnknownProxy* proxy = RPC::Administrator::Instance().ProxyInstance(channel, reader.Number<void*>(), RPC::IRemoteProcess::ID, false, RPC::IRemoteProcess::ID, true);
                RPC::IRemoteProcess* param0_proxy = (proxy != nullptr ? proxy->QueryInterface<RPC::IRemoteProcess>() : nullptr);

                implementation->Activated(param0_proxy);

                if (param0_proxy != nullptr) {
                    RPC::Administrator::Instance().Release(proxy, message->Response());
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Deactivated(RPC::IRemoteProcess*) = 0;
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            RPC::IRemoteProcess::INotification* implementation(parameters.Implementation<RPC::IRemoteProcess::INotification>());

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {

                ProxyStub::UnknownProxy* proxy = RPC::Administrator::Instance().ProxyInstance(channel, reader.Number<void*>(), RPC::IRemoteProcess::ID, false, RPC::IRemoteProcess::ID, true);
                RPC::IRemoteProcess* param0_proxy = (proxy != nullptr ? proxy->QueryInterface<RPC::IRemoteProcess>() : nullptr);

                implementation->Deactivated(param0_proxy);

                if (param0_proxy != nullptr) {
                    RPC::Administrator::Instance().Release(proxy, message->Response());
                }
            }
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

    typedef ProxyStub::UnknownStubType<RPC::IRemoteProcess, RemoteProcessStubMethods> RemoteProcessStub;
    typedef ProxyStub::UnknownStubType<RPC::IRemoteProcess::INotification, RemoteProcessNotificationStubMethods> RemoteProcessNotificationStub;
    typedef ProxyStub::UnknownStubType<Trace::ITraceController, TraceControllerStubMethods> TraceControllerStub;
    typedef ProxyStub::UnknownStubType<Trace::ITraceIterator, TraceIteratorStubMethods> TraceIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class RemoteProcessProxy : public UnknownProxyType<RPC::IRemoteProcess> {
    public:
        RemoteProcessProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~RemoteProcessProxy()
        {
        }

    public:
        virtual uint32_t Id() const
        {
            uint32_t id = ~0;

            IPCMessage newMessage(BaseClass::Message(0));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                id = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (id);
        }
        virtual RPC::IRemoteProcess::enumState State() const
        {
            RPC::IRemoteProcess::enumState result = CONSTRUCTED;

            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<RPC::IRemoteProcess::enumState>();
            }

            return (result);
        }
        virtual void* Aquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version)
        {
            void* result = nullptr;
            IPCMessage newMessage(BaseClass::Message(2));
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
            IPCMessage newMessage(BaseClass::Message(3));

            Invoke(newMessage);
        }
    };

    class RemoteProcessNotificationProxy : public UnknownProxyType<RPC::IRemoteProcess::INotification> {
    public:
        RemoteProcessNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~RemoteProcessNotificationProxy()
        {
        }

    public:
        virtual void Activated(RPC::IRemoteProcess* process)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::IRemoteProcess*>(process);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual void Deactivated(RPC::IRemoteProcess* process)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::IRemoteProcess*>(process);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
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
                RPC::Administrator::Instance().Announce<RPC::IRemoteProcess, RemoteProcessProxy, RemoteProcessStub>();
                RPC::Administrator::Instance().Announce<RPC::IRemoteProcess::INotification, RemoteProcessNotificationProxy, RemoteProcessNotificationStub>();
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

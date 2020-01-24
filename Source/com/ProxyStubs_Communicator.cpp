//
// generated automatically from "Communicator.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFrameworkIRemoteConnection
//   - class ::WPEFrameworkIRemoteConnection::INotification
//   - class ::WPEFrameworkIRemoteConnection::IProcess
//

#include "Communicator.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace RPC;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IRemoteConnection interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t Id() const = 0
    //  (1) virtual uint32_t RemoteId() const = 0
    //  (2) virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
    //  (3) virtual void Terminate() = 0
    //

    ProxyStub::MethodHandler RemoteConnectionStubMethods[] = {
        // virtual uint32_t Id() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IRemoteConnection* implementation = input.Implementation<IRemoteConnection>();
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            const uint32_t output = implementation->Id();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t RemoteId() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IRemoteConnection* implementation = input.Implementation<IRemoteConnection>();
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            const uint32_t output = implementation->RemoteId();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();
            const string param1 = reader.Text();
            const uint32_t output_interfaceid = reader.Number<uint32_t>();
            const uint32_t param3 = reader.Number<uint32_t>();

            // call implementation
            IRemoteConnection* implementation = input.Implementation<IRemoteConnection>();
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            void* output = implementation->Aquire(param0, param1, output_interfaceid, param3);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<void*>(output);
            RPC::Administrator::Instance().RegisterInterface(channel, output, output_interfaceid);
        },

        // virtual void Terminate() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IRemoteConnection* implementation = input.Implementation<IRemoteConnection>();
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            implementation->Terminate();
        },

        nullptr
    }; // RemoteConnectionStubMethods[]

    //
    // IRemoteConnection::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Activated(IRemoteConnection*) = 0
    //  (1) virtual void Deactivated(IRemoteConnection*) = 0
    //

    ProxyStub::MethodHandler RemoteConnectionNotificationStubMethods[] = {
        // virtual void Activated(IRemoteConnection*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IRemoteConnection* param0 = reader.Number<IRemoteConnection*>();
            IRemoteConnection* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IRemoteConnection::ID, false, IRemoteConnection::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IRemoteConnection>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IRemoteConnection proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IRemoteConnection proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IRemoteConnection::INotification* implementation = input.Implementation<IRemoteConnection::INotification>();
                ASSERT((implementation != nullptr) && "Null IRemoteConnection::INotification implementation pointer");
                implementation->Activated(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Deactivated(IRemoteConnection*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IRemoteConnection* param0 = reader.Number<IRemoteConnection*>();
            IRemoteConnection* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IRemoteConnection::ID, false, IRemoteConnection::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IRemoteConnection>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IRemoteConnection proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IRemoteConnection proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IRemoteConnection::INotification* implementation = input.Implementation<IRemoteConnection::INotification>();
                ASSERT((implementation != nullptr) && "Null IRemoteConnection::INotification implementation pointer");
                implementation->Deactivated(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // RemoteConnectionNotificationStubMethods[]

    //
    // IRemoteConnection::IProcess interface stub definitions
    //
    // Methods:
    //  (0) virtual string Callsign() const = 0
    //

    ProxyStub::MethodHandler RemoteConnectionProcessStubMethods[] = {
        // virtual string Callsign() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IRemoteConnection::IProcess* implementation = input.Implementation<IRemoteConnection::IProcess>();
            ASSERT((implementation != nullptr) && "Null IRemoteConnection::IProcess implementation pointer");
            const string output = implementation->Callsign();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // RemoteConnectionProcessStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IRemoteConnection interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t Id() const = 0
    //  (1) virtual uint32_t RemoteId() const = 0
    //  (2) virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
    //  (3) virtual void Terminate() = 0
    //

    class RemoteConnectionProxy final : public ProxyStub::UnknownProxyType<IRemoteConnection> {
    public:
        RemoteConnectionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t Id() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t RemoteId() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void* Aquire(const uint32_t param0, const string& param1, const uint32_t param2, const uint32_t param3) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);
            writer.Text(param1);
            writer.Number<const uint32_t>(param2);
            writer.Number<const uint32_t>(param3);

            // invoke the method handler
            void* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = Interface(reader.Number<void*>(),param2);
            }

            return output_proxy;
        }

        void Terminate() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }

    }; // class RemoteConnectionProxy

    //
    // IRemoteConnection::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Activated(IRemoteConnection*) = 0
    //  (1) virtual void Deactivated(IRemoteConnection*) = 0
    //

    class RemoteConnectionNotificationProxy final : public ProxyStub::UnknownProxyType<IRemoteConnection::INotification> {
    public:
        RemoteConnectionNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Activated(IRemoteConnection* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRemoteConnection*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Deactivated(IRemoteConnection* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRemoteConnection*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class RemoteConnectionNotificationProxy

    //
    // IRemoteConnection::IProcess interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Callsign() const = 0
    //

    class RemoteConnectionProcessProxy final : public ProxyStub::UnknownProxyType<IRemoteConnection::IProcess> {
    public:
        RemoteConnectionProcessProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Callsign() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class RemoteConnectionProcessProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IRemoteConnection::IProcess, RemoteConnectionProcessStubMethods> RemoteConnectionProcessStub;
        typedef ProxyStub::UnknownStubType<IRemoteConnection::INotification, RemoteConnectionNotificationStubMethods> RemoteConnectionNotificationStub;
        typedef ProxyStub::UnknownStubType<IRemoteConnection, RemoteConnectionStubMethods> RemoteConnectionStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IRemoteConnection::IProcess, RemoteConnectionProcessProxy, RemoteConnectionProcessStub>();
                RPC::Administrator::Instance().Announce<IRemoteConnection::INotification, RemoteConnectionNotificationProxy, RemoteConnectionNotificationStub>();
                RPC::Administrator::Instance().Announce<IRemoteConnection, RemoteConnectionProxy, RemoteConnectionStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

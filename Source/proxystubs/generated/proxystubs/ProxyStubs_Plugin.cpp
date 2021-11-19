//
// generated automatically from "IPlugin.h"
//
// implements RPC proxy stubs for:
//   - class IPlugin
//   - class IPlugin::INotification
//   - class IAuthenticate
//

#include "Module.h"
#include "IPlugin.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace PluginHost;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IPlugin interface stub definitions
    //
    // Methods:
    //  (0) virtual const string Initialize(IShell*) = 0
    //  (1) virtual void Deinitialize(IShell*) = 0
    //  (2) virtual string Information() const = 0
    //

    ProxyStub::MethodHandler PluginStubMethods[] = {
        // virtual const string Initialize(IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IShell* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IShell proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IShell proxy");
                }
            }

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPlugin* implementation = reinterpret_cast<IPlugin*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin implementation pointer");
            const string output = implementation->Initialize(param0_proxy);
            writer.Text(output);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Deinitialize(IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IShell* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IShell proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IShell proxy");
                }
            }

            // call implementation
            IPlugin* implementation = reinterpret_cast<IPlugin*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin implementation pointer");
            implementation->Deinitialize(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual string Information() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IPlugin* implementation = reinterpret_cast<const IPlugin*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin implementation pointer");
            const string output = implementation->Information();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // PluginStubMethods[]

    //
    // IPlugin::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Activated(const string&, IShell*) = 0
    //  (1) virtual void Deactivated(const string&, IShell*) = 0
    //  (2) virtual void Unavailable(const string&, IShell*) = 0
    //

    ProxyStub::MethodHandler PluginNotificationStubMethods[] = {
        // virtual void Activated(const string&, IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            RPC::instance_id param1 = reader.Number<RPC::instance_id>();
            IShell* param1_proxy = nullptr;
            ProxyStub::UnknownProxy* param1_proxy_inst = nullptr;
            if (param1 != 0) {
                param1_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param1, false, param1_proxy);
                ASSERT((param1_proxy_inst != nullptr) && (param1_proxy != nullptr) && "Failed to get instance of IShell proxy");

                if ((param1_proxy_inst == nullptr) || (param1_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IShell proxy");
                }
            }

            // call implementation
            IPlugin::INotification* implementation = reinterpret_cast<IPlugin::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin::INotification implementation pointer");
            implementation->Activated(param0, param1_proxy);

            if (param1_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param1_proxy_inst, message->Response());
            }
        },

        // virtual void Deactivated(const string&, IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            RPC::instance_id param1 = reader.Number<RPC::instance_id>();
            IShell* param1_proxy = nullptr;
            ProxyStub::UnknownProxy* param1_proxy_inst = nullptr;
            if (param1 != 0) {
                param1_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param1, false, param1_proxy);
                ASSERT((param1_proxy_inst != nullptr) && (param1_proxy != nullptr) && "Failed to get instance of IShell proxy");

                if ((param1_proxy_inst == nullptr) || (param1_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IShell proxy");
                }
            }

            // call implementation
            IPlugin::INotification* implementation = reinterpret_cast<IPlugin::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin::INotification implementation pointer");
            implementation->Deactivated(param0, param1_proxy);

            if (param1_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param1_proxy_inst, message->Response());
            }
        },

        // virtual void Unavailable(const string&, IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            RPC::instance_id param1 = reader.Number<RPC::instance_id>();
            IShell* param1_proxy = nullptr;
            ProxyStub::UnknownProxy* param1_proxy_inst = nullptr;
            if (param1 != 0) {
                param1_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param1, false, param1_proxy);
                ASSERT((param1_proxy_inst != nullptr) && (param1_proxy != nullptr) && "Failed to get instance of IShell proxy");

                if ((param1_proxy_inst == nullptr) || (param1_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IShell proxy");
                }
            }

            // call implementation
            IPlugin::INotification* implementation = reinterpret_cast<IPlugin::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IPlugin::INotification implementation pointer");
            implementation->Unavailable(param0, param1_proxy);

            if (param1_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param1_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // PluginNotificationStubMethods[]

    //
    // IAuthenticate interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t CreateToken(const uint16_t, const uint8_t*, string&) = 0
    //  (1) virtual ISecurity* Officer(const string&) = 0
    //

    ProxyStub::MethodHandler AuthenticateStubMethods[] = {
        // virtual uint32_t CreateToken(const uint16_t, const uint8_t*, string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param1 = nullptr;
            uint16_t param1_length = reader.LockBuffer<uint16_t>(param1);
            reader.UnlockBuffer(param1_length);
            string param2{}; // storage

            // call implementation
            IAuthenticate* implementation = reinterpret_cast<IAuthenticate*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IAuthenticate implementation pointer");
            const uint32_t output = implementation->CreateToken(param1_length, param1, param2);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
            writer.Text(param2);
        },

        // virtual ISecurity* Officer(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAuthenticate* implementation = reinterpret_cast<IAuthenticate*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IAuthenticate implementation pointer");
            ISecurity* output = implementation->Officer(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<ISecurity*>(output));
            if (output != nullptr) {
                RPC::Administrator::Instance().RegisterInterface(channel, output);
            }
        },

        nullptr
    }; // AuthenticateStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IPlugin interface proxy definitions
    //
    // Methods:
    //  (0) virtual const string Initialize(IShell*) = 0
    //  (1) virtual void Deinitialize(IShell*) = 0
    //  (2) virtual string Information() const = 0
    //

    class PluginProxy final : public ProxyStub::UnknownProxyType<IPlugin> {
    public:
        PluginProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        const string Initialize(IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param0));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();

                Complete(reader);
            }

            return output;
        }

        void Deinitialize(IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        string Information() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class PluginProxy

    //
    // IPlugin::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Activated(const string&, IShell*) = 0
    //  (1) virtual void Deactivated(const string&, IShell*) = 0
    //  (2) virtual void Unavailable(const string&, IShell*) = 0
    //

    class PluginNotificationProxy final : public ProxyStub::UnknownProxyType<IPlugin::INotification> {
    public:
        PluginNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Activated(const string& param0, IShell* param1) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param1));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Deactivated(const string& param0, IShell* param1) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param1));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unavailable(const string& param0, IShell* param1) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param1));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class PluginNotificationProxy

    //
    // IAuthenticate interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t CreateToken(const uint16_t, const uint8_t*, string&) = 0
    //  (1) virtual ISecurity* Officer(const string&) = 0
    //

    class AuthenticateProxy final : public ProxyStub::UnknownProxyType<IAuthenticate> {
    public:
        AuthenticateProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t CreateToken(const uint16_t param0, const uint8_t* param1, string& /* out */ param2) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint16_t>(param0, param1);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
                param2 = reader.Text();
            }

            return output;
        }

        ISecurity* Officer(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            ISecurity* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ISecurity*>(Interface(reader.Number<RPC::instance_id>(), ISecurity::ID));
            }

            return output_proxy;
        }
    }; // class AuthenticateProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IPlugin, PluginStubMethods> PluginStub;
        typedef ProxyStub::UnknownStubType<IPlugin::INotification, PluginNotificationStubMethods> PluginNotificationStub;
        typedef ProxyStub::UnknownStubType<IAuthenticate, AuthenticateStubMethods> AuthenticateStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IPlugin, PluginProxy, PluginStub>();
                RPC::Administrator::Instance().Announce<IPlugin::INotification, PluginNotificationProxy, PluginNotificationStub>();
                RPC::Administrator::Instance().Announce<IAuthenticate, AuthenticateProxy, AuthenticateStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<IPlugin>();
                RPC::Administrator::Instance().Recall<IPlugin::INotification>();
                RPC::Administrator::Instance().Recall<IAuthenticate>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

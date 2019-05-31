//
// generated automatically from "IDsgccClient.h"
//
// implements RPC proxy stubs for:
//   - class IDsgccClient
//   - class IDsgccClient::INotification
//

#include "IDsgccClient.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IDsgccClient interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (1) virtual string GetChannels() const = 0
    //  (2) virtual string State() const = 0
    //  (3) virtual void Restart() = 0
    //  (4) virtual void Callback(IDsgccClient::INotification*) = 0
    //  (5) virtual void DsgccClientSet(const string&) = 0
    //

    ProxyStub::MethodHandler DsgccClientStubMethods[] = {
        // virtual uint32_t Configure(PluginHost::IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            PluginHost::IShell* param0 = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, PluginHost::IShell::ID, false, PluginHost::IShell::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<PluginHost::IShell>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of PluginHost::IShell proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of PluginHost::IShell proxy");
                }
            }

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IDsgccClient* implementation = input.Implementation<IDsgccClient>();
                ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
                const uint32_t output = implementation->Configure(param0_proxy);
                writer.Number<const uint32_t>(output);
            } else {
                // return error code
                writer.Number<const uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual string GetChannels() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IDsgccClient* implementation = input.Implementation<IDsgccClient>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
            const string output = implementation->GetChannels();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string State() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IDsgccClient* implementation = input.Implementation<IDsgccClient>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
            const string output = implementation->State();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual void Restart() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IDsgccClient* implementation = input.Implementation<IDsgccClient>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
            implementation->Restart();
        },

        // virtual void Callback(IDsgccClient::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IDsgccClient::INotification* param0 = reader.Number<IDsgccClient::INotification*>();
            IDsgccClient::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IDsgccClient::INotification::ID, false, IDsgccClient::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IDsgccClient::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IDsgccClient::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IDsgccClient::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IDsgccClient* implementation = input.Implementation<IDsgccClient>();
                ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
                implementation->Callback(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void DsgccClientSet(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IDsgccClient* implementation = input.Implementation<IDsgccClient>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
            implementation->DsgccClientSet(param0);
        },

        nullptr
    }; // DsgccClientStubMethods[]

    //
    // IDsgccClient::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IDsgccClient::state) = 0
    //

    ProxyStub::MethodHandler DsgccClientNotificationStubMethods[] = {
        // virtual void StateChange(const IDsgccClient::state) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IDsgccClient::state param0 = reader.Number<IDsgccClient::state>();

            // call implementation
            IDsgccClient::INotification* implementation = input.Implementation<IDsgccClient::INotification>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient::INotification implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // DsgccClientNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IDsgccClient interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (1) virtual string GetChannels() const = 0
    //  (2) virtual string State() const = 0
    //  (3) virtual void Restart() = 0
    //  (4) virtual void Callback(IDsgccClient::INotification*) = 0
    //  (5) virtual void DsgccClientSet(const string&) = 0
    //

    class DsgccClientProxy final : public ProxyStub::UnknownProxyType<IDsgccClient> {
    public:
        DsgccClientProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t Configure(PluginHost::IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();

                Complete(reader);
            }

            return output;
        }

        string GetChannels() const override
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

        string State() const override
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

        void Restart() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }

        void Callback(IDsgccClient::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IDsgccClient::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void DsgccClientSet(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class DsgccClientProxy

    //
    // IDsgccClient::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IDsgccClient::state) = 0
    //

    class DsgccClientNotificationProxy final : public ProxyStub::UnknownProxyType<IDsgccClient::INotification> {
    public:
        DsgccClientNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(const IDsgccClient::state param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IDsgccClient::state>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class DsgccClientNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IDsgccClient::INotification, DsgccClientNotificationStubMethods> DsgccClientNotificationStub;
        typedef ProxyStub::UnknownStubType<IDsgccClient, DsgccClientStubMethods> DsgccClientStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IDsgccClient::INotification, DsgccClientNotificationProxy, DsgccClientNotificationStub>();
                RPC::Administrator::Instance().Announce<IDsgccClient, DsgccClientProxy, DsgccClientStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

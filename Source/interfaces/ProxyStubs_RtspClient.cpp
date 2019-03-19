//
// generated automatically from "IRtspClient.h"
//
// implements RPC proxy stubs for:
//   - class IRtspClient
//

#include "IRtspClient.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IRtspClient interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (1) virtual uint32_t Setup(const string&, uint32_t) = 0
    //  (2) virtual uint32_t Play(int32_t, uint32_t) = 0
    //  (3) virtual uint32_t Teardown() = 0
    //  (4) virtual void Set(const string&, const string&) = 0
    //  (5) virtual string Get(const string&) const = 0
    //

    ProxyStub::MethodHandler RtspClientStubMethods[] = {
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

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IRtspClient* implementation = input.Implementation<IRtspClient>();
                ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
                const uint32_t output = implementation->Configure(param0_proxy);

                // write return value
                writer.Number<const uint32_t>(output);
            } else {
                // return error code
                writer.Number<const uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual uint32_t Setup(const string&, uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint32_t param1 = reader.Number<uint32_t>();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRtspClient* implementation = input.Implementation<IRtspClient>();
            ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
            const uint32_t output = implementation->Setup(param0, param1);

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Play(int32_t, uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const int32_t param0 = reader.Number<int32_t>();
            const uint32_t param1 = reader.Number<uint32_t>();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRtspClient* implementation = input.Implementation<IRtspClient>();
            ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
            const uint32_t output = implementation->Play(param0, param1);

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Teardown() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRtspClient* implementation = input.Implementation<IRtspClient>();
            ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
            const uint32_t output = implementation->Teardown();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual void Set(const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();

            // call implementation
            IRtspClient* implementation = input.Implementation<IRtspClient>();
            ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
            implementation->Set(param0, param1);
        },

        // virtual string Get(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IRtspClient* implementation = input.Implementation<IRtspClient>();
            ASSERT((implementation != nullptr) && "Null IRtspClient implementation pointer");
            const string output = implementation->Get(param0);

            // write return value
            writer.Text(output);
        },

        nullptr
    }; // RtspClientStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IRtspClient interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (1) virtual uint32_t Setup(const string&, uint32_t) = 0
    //  (2) virtual uint32_t Play(int32_t, uint32_t) = 0
    //  (3) virtual uint32_t Teardown() = 0
    //  (4) virtual void Set(const string&, const string&) = 0
    //  (5) virtual string Get(const string&) const = 0
    //

    class RtspClientProxy final : public ProxyStub::UnknownProxyType<IRtspClient> {
    public:
        RtspClientProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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

        uint32_t Setup(const string& param0, uint32_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<const uint32_t>(param1);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Play(int32_t param0, uint32_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const int32_t>(param0);
            writer.Number<const uint32_t>(param1);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Teardown() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void Set(const string& param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);

            // invoke the method handler
            Invoke(newMessage);
        }

        string Get(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class RtspClientProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IRtspClient, RtspClientStubMethods> RtspClientStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IRtspClient, RtspClientProxy, RtspClientStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

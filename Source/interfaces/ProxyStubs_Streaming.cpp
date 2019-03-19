//
// generated automatically from "IStreaming.h"
//
// implements RPC proxy stubs for:
//   - class IStreaming
//   - class IStreaming::INotification
//

#include "IStreaming.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IStreaming interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(IStreaming::INotification*) = 0
    //  (1) virtual void Unregister(IStreaming::INotification*) = 0
    //  (2) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (3) virtual void StartScan() = 0
    //  (4) virtual void StopScan() = 0
    //  (5) virtual void SetCurrentChannel(const string&) = 0
    //  (6) virtual const string GetCurrentChannel() = 0
    //  (7) virtual bool IsScanning() = 0
    //  (8) virtual void Test(const string&) = 0
    //

    ProxyStub::MethodHandler StreamingStubMethods[] = {
        // virtual void Register(IStreaming::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IStreaming::INotification* param0 = reader.Number<IStreaming::INotification*>();
            IStreaming::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IStreaming::INotification::ID, false, IStreaming::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IStreaming::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IStreaming::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IStreaming::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IStreaming* implementation = input.Implementation<IStreaming>();
                ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IStreaming::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IStreaming::INotification* param0 = reader.Number<IStreaming::INotification*>();
            IStreaming::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IStreaming::INotification::ID, false, IStreaming::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IStreaming::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IStreaming::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IStreaming::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IStreaming* implementation = input.Implementation<IStreaming>();
                ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

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
                IStreaming* implementation = input.Implementation<IStreaming>();
                ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
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

        // virtual void StartScan() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            implementation->StartScan();
        },

        // virtual void StopScan() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            implementation->StopScan();
        },

        // virtual void SetCurrentChannel(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            implementation->SetCurrentChannel(param0);
        },

        // virtual const string GetCurrentChannel() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            const string output = implementation->GetCurrentChannel();

            // write return value
            writer.Text(output);
        },

        // virtual bool IsScanning() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            const bool output = implementation->IsScanning();

            // write return value
            writer.Boolean(output);
        },

        // virtual void Test(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IStreaming* implementation = input.Implementation<IStreaming>();
            ASSERT((implementation != nullptr) && "Null IStreaming implementation pointer");
            implementation->Test(param0);
        },

        nullptr
    }; // StreamingStubMethods[]

    //
    // IStreaming::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void ScanningStateChanged(const uint32_t) = 0
    //  (1) virtual void CurrentChannelChanged(const string&) = 0
    //  (2) virtual void TestNotification(const string&) = 0
    //

    ProxyStub::MethodHandler StreamingNotificationStubMethods[] = {
        // virtual void ScanningStateChanged(const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IStreaming::INotification* implementation = input.Implementation<IStreaming::INotification>();
            ASSERT((implementation != nullptr) && "Null IStreaming::INotification implementation pointer");
            implementation->ScanningStateChanged(param0);
        },

        // virtual void CurrentChannelChanged(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IStreaming::INotification* implementation = input.Implementation<IStreaming::INotification>();
            ASSERT((implementation != nullptr) && "Null IStreaming::INotification implementation pointer");
            implementation->CurrentChannelChanged(param0);
        },

        // virtual void TestNotification(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IStreaming::INotification* implementation = input.Implementation<IStreaming::INotification>();
            ASSERT((implementation != nullptr) && "Null IStreaming::INotification implementation pointer");
            implementation->TestNotification(param0);
        },

        nullptr
    }; // StreamingNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IStreaming interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(IStreaming::INotification*) = 0
    //  (1) virtual void Unregister(IStreaming::INotification*) = 0
    //  (2) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (3) virtual void StartScan() = 0
    //  (4) virtual void StopScan() = 0
    //  (5) virtual void SetCurrentChannel(const string&) = 0
    //  (6) virtual const string GetCurrentChannel() = 0
    //  (7) virtual bool IsScanning() = 0
    //  (8) virtual void Test(const string&) = 0
    //

    class StreamingProxy final : public ProxyStub::UnknownProxyType<IStreaming> {
    public:
        StreamingProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(IStreaming::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStreaming::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IStreaming::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStreaming::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        uint32_t Configure(PluginHost::IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

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

        void StartScan() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }

        void StopScan() override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            Invoke(newMessage);
        }

        void SetCurrentChannel(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        const string GetCurrentChannel() override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        bool IsScanning() override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        void Test(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(8));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class StreamingProxy

    //
    // IStreaming::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void ScanningStateChanged(const uint32_t) = 0
    //  (1) virtual void CurrentChannelChanged(const string&) = 0
    //  (2) virtual void TestNotification(const string&) = 0
    //

    class StreamingNotificationProxy final : public ProxyStub::UnknownProxyType<IStreaming::INotification> {
    public:
        StreamingNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void ScanningStateChanged(const uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void CurrentChannelChanged(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void TestNotification(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class StreamingNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IStreaming::INotification, StreamingNotificationStubMethods> StreamingNotificationStub;
        typedef ProxyStub::UnknownStubType<IStreaming, StreamingStubMethods> StreamingStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IStreaming::INotification, StreamingNotificationProxy, StreamingNotificationStub>();
                RPC::Administrator::Instance().Announce<IStreaming, StreamingProxy, StreamingStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

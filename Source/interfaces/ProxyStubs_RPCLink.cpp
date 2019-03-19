//
// generated automatically from "IRPCLink.h"
//
// implements RPC proxy stubs for:
//   - class IRPCLink
//   - class IRPCLink::INotification
//

#include "IRPCLink.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IRPCLink interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(IRPCLink::INotification*) = 0
    //  (1) virtual void Unregister(IRPCLink::INotification*) = 0
    //  (2) virtual uint32_t Start(const uint32_t, const string&) = 0
    //  (3) virtual uint32_t Stop() = 0
    //  (4) virtual uint32_t ForceCallback() = 0
    //

    ProxyStub::MethodHandler RPCLinkStubMethods[] = {
        // virtual void Register(IRPCLink::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IRPCLink::INotification* param0 = reader.Number<IRPCLink::INotification*>();
            IRPCLink::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IRPCLink::INotification::ID, false, IRPCLink::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IRPCLink::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IRPCLink::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IRPCLink::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IRPCLink* implementation = input.Implementation<IRPCLink>();
                ASSERT((implementation != nullptr) && "Null IRPCLink implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IRPCLink::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IRPCLink::INotification* param0 = reader.Number<IRPCLink::INotification*>();
            IRPCLink::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IRPCLink::INotification::ID, false, IRPCLink::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IRPCLink::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IRPCLink::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IRPCLink::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IRPCLink* implementation = input.Implementation<IRPCLink>();
                ASSERT((implementation != nullptr) && "Null IRPCLink implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual uint32_t Start(const uint32_t, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();
            const string param1 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRPCLink* implementation = input.Implementation<IRPCLink>();
            ASSERT((implementation != nullptr) && "Null IRPCLink implementation pointer");
            const uint32_t output = implementation->Start(param0, param1);

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Stop() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRPCLink* implementation = input.Implementation<IRPCLink>();
            ASSERT((implementation != nullptr) && "Null IRPCLink implementation pointer");
            const uint32_t output = implementation->Stop();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t ForceCallback() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IRPCLink* implementation = input.Implementation<IRPCLink>();
            ASSERT((implementation != nullptr) && "Null IRPCLink implementation pointer");
            const uint32_t output = implementation->ForceCallback();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // RPCLinkStubMethods[]

    //
    // IRPCLink::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Completed(const uint32_t, const string&) = 0
    //

    ProxyStub::MethodHandler RPCLinkNotificationStubMethods[] = {
        // virtual void Completed(const uint32_t, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();
            const string param1 = reader.Text();

            // call implementation
            IRPCLink::INotification* implementation = input.Implementation<IRPCLink::INotification>();
            ASSERT((implementation != nullptr) && "Null IRPCLink::INotification implementation pointer");
            implementation->Completed(param0, param1);
        },

        nullptr
    }; // RPCLinkNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IRPCLink interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(IRPCLink::INotification*) = 0
    //  (1) virtual void Unregister(IRPCLink::INotification*) = 0
    //  (2) virtual uint32_t Start(const uint32_t, const string&) = 0
    //  (3) virtual uint32_t Stop() = 0
    //  (4) virtual uint32_t ForceCallback() = 0
    //

    class RPCLinkProxy final : public ProxyStub::UnknownProxyType<IRPCLink> {
    public:
        RPCLinkProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(IRPCLink::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRPCLink::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IRPCLink::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IRPCLink::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        uint32_t Start(const uint32_t param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);
            writer.Text(param1);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Stop() override
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

        uint32_t ForceCallback() override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }
    }; // class RPCLinkProxy

    //
    // IRPCLink::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Completed(const uint32_t, const string&) = 0
    //

    class RPCLinkNotificationProxy final : public ProxyStub::UnknownProxyType<IRPCLink::INotification> {
    public:
        RPCLinkNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Completed(const uint32_t param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);
            writer.Text(param1);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class RPCLinkNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IRPCLink::INotification, RPCLinkNotificationStubMethods> RPCLinkNotificationStub;
        typedef ProxyStub::UnknownStubType<IRPCLink, RPCLinkStubMethods> RPCLinkStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IRPCLink::INotification, RPCLinkNotificationProxy, RPCLinkNotificationStub>();
                RPC::Administrator::Instance().Announce<IRPCLink, RPCLinkProxy, RPCLinkStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

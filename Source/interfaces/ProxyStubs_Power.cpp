//
// generated automatically from "IPower.h"
//
// implements RPC proxy stubs for:
//   - class IPower
//   - class IPower::INotification
//

#include "IPower.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IPower interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(IPower::INotification*) = 0
    //  (1) virtual void Unregister(IPower::INotification*) = 0
    //  (2) virtual IPower::PCState GetState() const = 0
    //  (3) virtual IPower::PCStatus SetState(const IPower::PCState, const uint32_t) = 0
    //  (4) virtual void PowerKey() = 0
    //  (5) virtual void Configure(const string&) = 0
    //

    ProxyStub::MethodHandler PowerStubMethods[] = {
        // virtual void Register(IPower::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IPower::INotification* param0 = reader.Number<IPower::INotification*>();
            IPower::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IPower::INotification::ID, false, IPower::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IPower::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPower::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPower::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IPower* implementation = input.Implementation<IPower>();
                ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IPower::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IPower::INotification* param0 = reader.Number<IPower::INotification*>();
            IPower::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IPower::INotification::ID, false, IPower::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IPower::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPower::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPower::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IPower* implementation = input.Implementation<IPower>();
                ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual IPower::PCState GetState() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPower* implementation = input.Implementation<IPower>();
            ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
            const IPower::PCState output = implementation->GetState();

            // write return value
            writer.Number<const IPower::PCState>(output);
        },

        // virtual IPower::PCStatus SetState(const IPower::PCState, const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IPower::PCState param0 = reader.Number<IPower::PCState>();
            const uint32_t param1 = reader.Number<uint32_t>();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPower* implementation = input.Implementation<IPower>();
            ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
            const IPower::PCStatus output = implementation->SetState(param0, param1);

            // write return value
            writer.Number<const IPower::PCStatus>(output);
        },

        // virtual void PowerKey() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IPower* implementation = input.Implementation<IPower>();
            ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
            implementation->PowerKey();
        },

        // virtual void Configure(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IPower* implementation = input.Implementation<IPower>();
            ASSERT((implementation != nullptr) && "Null IPower implementation pointer");
            implementation->Configure(param0);
        },

        nullptr
    }; // PowerStubMethods[]

    //
    // IPower::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IPower::PCState) = 0
    //

    ProxyStub::MethodHandler PowerNotificationStubMethods[] = {
        // virtual void StateChange(const IPower::PCState) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IPower::PCState param0 = reader.Number<IPower::PCState>();

            // call implementation
            IPower::INotification* implementation = input.Implementation<IPower::INotification>();
            ASSERT((implementation != nullptr) && "Null IPower::INotification implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // PowerNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IPower interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(IPower::INotification*) = 0
    //  (1) virtual void Unregister(IPower::INotification*) = 0
    //  (2) virtual IPower::PCState GetState() const = 0
    //  (3) virtual IPower::PCStatus SetState(const IPower::PCState, const uint32_t) = 0
    //  (4) virtual void PowerKey() = 0
    //  (5) virtual void Configure(const string&) = 0
    //

    class PowerProxy final : public ProxyStub::UnknownProxyType<IPower> {
    public:
        PowerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(IPower::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPower::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IPower::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPower::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        IPower::PCState GetState() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            IPower::PCState output = static_cast<IPower::PCState>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IPower::PCState>();
            }

            return output;
        }

        IPower::PCStatus SetState(const IPower::PCState param0, const uint32_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IPower::PCState>(param0);
            writer.Number<const uint32_t>(param1);

            // invoke the method handler
            IPower::PCStatus output = static_cast<IPower::PCStatus>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IPower::PCStatus>();
            }

            return output;
        }

        void PowerKey() override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            Invoke(newMessage);
        }

        void Configure(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class PowerProxy

    //
    // IPower::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IPower::PCState) = 0
    //

    class PowerNotificationProxy final : public ProxyStub::UnknownProxyType<IPower::INotification> {
    public:
        PowerNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(const IPower::PCState param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IPower::PCState>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class PowerNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IPower::INotification, PowerNotificationStubMethods> PowerNotificationStub;
        typedef ProxyStub::UnknownStubType<IPower, PowerStubMethods> PowerStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IPower::INotification, PowerNotificationProxy, PowerNotificationStub>();
                RPC::Administrator::Instance().Announce<IPower, PowerProxy, PowerStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

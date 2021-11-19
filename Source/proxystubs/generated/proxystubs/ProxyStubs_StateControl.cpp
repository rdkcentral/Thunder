//
// generated automatically from "IStateControl.h"
//
// implements RPC proxy stubs for:
//   - class IStateControl
//   - class IStateControl::INotification
//

#include "Module.h"
#include "IStateControl.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace PluginHost;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IStateControl interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(IShell*) = 0
    //  (1) virtual IStateControl::state State() const = 0
    //  (2) virtual uint32_t Request(const IStateControl::command) = 0
    //  (3) virtual void Register(IStateControl::INotification*) = 0
    //  (4) virtual void Unregister(IStateControl::INotification*) = 0
    //

    ProxyStub::MethodHandler StateControlStubMethods[] = {
        // virtual uint32_t Configure(IShell*) = 0
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
            IStateControl* implementation = reinterpret_cast<IStateControl*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl implementation pointer");
            const uint32_t output = implementation->Configure(param0_proxy);
            writer.Number<const uint32_t>(output);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual IStateControl::state State() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IStateControl* implementation = reinterpret_cast<const IStateControl*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl implementation pointer");
            const IStateControl::state output = implementation->State();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const IStateControl::state>(output);
        },

        // virtual uint32_t Request(const IStateControl::command) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IStateControl::command param0 = reader.Number<IStateControl::command>();

            // call implementation
            IStateControl* implementation = reinterpret_cast<IStateControl*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl implementation pointer");
            const uint32_t output = implementation->Request(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual void Register(IStateControl::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IStateControl::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IStateControl::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IStateControl::INotification proxy");
                }
            }

            // call implementation
            IStateControl* implementation = reinterpret_cast<IStateControl*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl implementation pointer");
            implementation->Register(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IStateControl::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IStateControl::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IStateControl::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IStateControl::INotification proxy");
                }
            }

            // call implementation
            IStateControl* implementation = reinterpret_cast<IStateControl*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl implementation pointer");
            implementation->Unregister(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // StateControlStubMethods[]

    //
    // IStateControl::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IStateControl::state) = 0
    //

    ProxyStub::MethodHandler StateControlNotificationStubMethods[] = {
        // virtual void StateChange(const IStateControl::state) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IStateControl::state param0 = reader.Number<IStateControl::state>();

            // call implementation
            IStateControl::INotification* implementation = reinterpret_cast<IStateControl::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IStateControl::INotification implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // StateControlNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IStateControl interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t Configure(IShell*) = 0
    //  (1) virtual IStateControl::state State() const = 0
    //  (2) virtual uint32_t Request(const IStateControl::command) = 0
    //  (3) virtual void Register(IStateControl::INotification*) = 0
    //  (4) virtual void Unregister(IStateControl::INotification*) = 0
    //

    class StateControlProxy final : public ProxyStub::UnknownProxyType<IStateControl> {
    public:
        StateControlProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t Configure(IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IShell*>(param0));

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

        IStateControl::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            IStateControl::state output = static_cast<IStateControl::state>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IStateControl::state>();
            }

            return output;
        }

        uint32_t Request(const IStateControl::command param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IStateControl::command>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void Register(IStateControl::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IStateControl::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IStateControl::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IStateControl::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class StateControlProxy

    //
    // IStateControl::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const IStateControl::state) = 0
    //

    class StateControlNotificationProxy final : public ProxyStub::UnknownProxyType<IStateControl::INotification> {
    public:
        StateControlNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(const IStateControl::state param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IStateControl::state>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class StateControlNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IStateControl, StateControlStubMethods> StateControlStub;
        typedef ProxyStub::UnknownStubType<IStateControl::INotification, StateControlNotificationStubMethods> StateControlNotificationStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IStateControl, StateControlProxy, StateControlStub>();
                RPC::Administrator::Instance().Announce<IStateControl::INotification, StateControlNotificationProxy, StateControlNotificationStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<IStateControl>();
                RPC::Administrator::Instance().Recall<IStateControl::INotification>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

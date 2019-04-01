//
// generated automatically from "ISpark.h"
//
// implements RPC proxy stubs for:
//   - class ISpark
//   - class ISpark::INotification
//

#include "ISpark.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // ISpark interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(ISpark::INotification*) = 0
    //  (1) virtual void Unregister(ISpark::INotification*) = 0
    //

    ProxyStub::MethodHandler SparkStubMethods[] = {
        // virtual void Register(ISpark::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            printf("Ihsan %s:%s:%d\n", __FILE__, __func__, __LINE__);
            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            ISpark::INotification* param0 = reader.Number<ISpark::INotification*>();
            ISpark::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, ISpark::INotification::ID, false, ISpark::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<ISpark::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of ISpark::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ISpark::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                ISpark* implementation = input.Implementation<ISpark>();
                ASSERT((implementation != nullptr) && "Null ISpark implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(ISpark::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            printf("Ihsan %s:%s:%d\n", __FILE__, __func__, __LINE__);
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            ISpark::INotification* param0 = reader.Number<ISpark::INotification*>();
            ISpark::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, ISpark::INotification::ID, false, ISpark::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<ISpark::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of ISpark::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ISpark::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                ISpark* implementation = input.Implementation<ISpark>();
                ASSERT((implementation != nullptr) && "Null ISpark implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // SparkStubMethods[]

    //
    // ISpark::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const ISpark::state) = 0
    //

    ProxyStub::MethodHandler SparkNotificationStubMethods[] = {
        // virtual void StateChange(const ISpark::state) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const ISpark::state param0 = reader.Number<ISpark::state>();

            // call implementation
            ISpark::INotification* implementation = input.Implementation<ISpark::INotification>();
            ASSERT((implementation != nullptr) && "Null ISpark::INotification implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // SparkNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // ISpark interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(ISpark::INotification*) = 0
    //  (1) virtual void Unregister(ISpark::INotification*) = 0
    //

    class SparkProxy final : public ProxyStub::UnknownProxyType<ISpark> {
    public:
        SparkProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
            printf("Ihsan %s:%s:%d\n", __FILE__, __func__, __LINE__);
        }

        void Register(ISpark::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISpark::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(ISpark::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISpark::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class SparkProxy

    //
    // ISpark::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const ISpark::state) = 0
    //

    class SparkNotificationProxy final : public ProxyStub::UnknownProxyType<ISpark::INotification> {
    public:
        SparkNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(const ISpark::state param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const ISpark::state>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class SparkNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<ISpark::INotification, SparkNotificationStubMethods> SparkNotificationStub;
        typedef ProxyStub::UnknownStubType<ISpark, SparkStubMethods> SparkStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<ISpark::INotification, SparkNotificationProxy, SparkNotificationStub>();
                RPC::Administrator::Instance().Announce<ISpark, SparkProxy, SparkStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

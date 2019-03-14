//
// generated automatically from "INetflix.h"
//
// implements RPC proxy stubs for:
//   - class INetflix
//   - class INetflix::INotification
//

#include "INetflix.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // INetflix interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(INetflix::INotification*) = 0
    //  (1) virtual void Unregister(INetflix::INotification*) = 0
    //  (2) virtual string GetESN() const = 0
    //  (3) virtual void FactoryReset() = 0
    //  (4) virtual void SystemCommand(const string&) = 0
    //  (5) virtual void Language(const string&) = 0
    //  (6) virtual void SetVisible(bool) = 0
    //

    ProxyStub::MethodHandler NetflixStubMethods[] = {
        // virtual void Register(INetflix::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            INetflix::INotification* param0 = reader.Number<INetflix::INotification*>();
            INetflix::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, INetflix::INotification::ID, false, INetflix::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<INetflix::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of INetflix::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of INetflix::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                INetflix* implementation = input.Implementation<INetflix>();
                ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(INetflix::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            INetflix::INotification* param0 = reader.Number<INetflix::INotification*>();
            INetflix::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, INetflix::INotification::ID, false, INetflix::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<INetflix::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of INetflix::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of INetflix::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                INetflix* implementation = input.Implementation<INetflix>();
                ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual string GetESN() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const INetflix* implementation = input.Implementation<INetflix>();
            ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
            const string output = implementation->GetESN();

            // write return value
            writer.Text(output);
        },

        // virtual void FactoryReset() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            INetflix* implementation = input.Implementation<INetflix>();
            ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
            implementation->FactoryReset();
        },

        // virtual void SystemCommand(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            INetflix* implementation = input.Implementation<INetflix>();
            ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
            implementation->SystemCommand(param0);
        },

        // virtual void Language(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            INetflix* implementation = input.Implementation<INetflix>();
            ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
            implementation->Language(param0);
        },

        // virtual void SetVisible(bool) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const bool param0 = reader.Boolean();

            // call implementation
            INetflix* implementation = input.Implementation<INetflix>();
            ASSERT((implementation != nullptr) && "Null INetflix implementation pointer");
            implementation->SetVisible(param0);
        },

        nullptr
    }; // NetflixStubMethods[]

    //
    // INetflix::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const INetflix::state) = 0
    //

    ProxyStub::MethodHandler NetflixNotificationStubMethods[] = {
        // virtual void StateChange(const INetflix::state) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const INetflix::state param0 = reader.Number<INetflix::state>();

            // call implementation
            INetflix::INotification* implementation = input.Implementation<INetflix::INotification>();
            ASSERT((implementation != nullptr) && "Null INetflix::INotification implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // NetflixNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // INetflix interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(INetflix::INotification*) = 0
    //  (1) virtual void Unregister(INetflix::INotification*) = 0
    //  (2) virtual string GetESN() const = 0
    //  (3) virtual void FactoryReset() = 0
    //  (4) virtual void SystemCommand(const string&) = 0
    //  (5) virtual void Language(const string&) = 0
    //  (6) virtual void SetVisible(bool) = 0
    //

    class NetflixProxy final : public ProxyStub::UnknownProxyType<INetflix> {
    public:
        NetflixProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(INetflix::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<INetflix::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(INetflix::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<INetflix::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        string GetESN() const override
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

        void FactoryReset() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }

        void SystemCommand(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void Language(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void SetVisible(bool param0) override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class NetflixProxy

    //
    // INetflix::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(const INetflix::state) = 0
    //

    class NetflixNotificationProxy final : public ProxyStub::UnknownProxyType<INetflix::INotification> {
    public:
        NetflixNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(const INetflix::state param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const INetflix::state>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class NetflixNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<INetflix::INotification, NetflixNotificationStubMethods> NetflixNotificationStub;
        typedef ProxyStub::UnknownStubType<INetflix, NetflixStubMethods> NetflixStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<INetflix::INotification, NetflixNotificationProxy, NetflixNotificationStub>();
                RPC::Administrator::Instance().Announce<INetflix, NetflixProxy, NetflixStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

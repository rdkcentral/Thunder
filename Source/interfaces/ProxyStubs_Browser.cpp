//
// generated automatically from "IBrowser.h"
//
// implements RPC proxy stubs for:
//   - class IBrowser
//   - class IBrowser::INotification
//   - class IBrowser::IMetadata
//

#include "IBrowser.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IBrowser interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(IBrowser::INotification*) = 0
    //  (1) virtual void Unregister(IBrowser::INotification*) = 0
    //  (2) virtual void SetURL(const string&) = 0
    //  (3) virtual string GetURL() const = 0
    //  (4) virtual uint32_t GetFPS() const = 0
    //  (5) virtual void Hide(const bool) = 0
    //

    ProxyStub::MethodHandler BrowserStubMethods[] = {
        // virtual void Register(IBrowser::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IBrowser::INotification* param0 = reader.Number<IBrowser::INotification*>();
            IBrowser::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IBrowser::INotification::ID, false, IBrowser::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IBrowser::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IBrowser::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IBrowser::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IBrowser* implementation = input.Implementation<IBrowser>();
                ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IBrowser::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IBrowser::INotification* param0 = reader.Number<IBrowser::INotification*>();
            IBrowser::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IBrowser::INotification::ID, false, IBrowser::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IBrowser::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IBrowser::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IBrowser::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IBrowser* implementation = input.Implementation<IBrowser>();
                ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void SetURL(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IBrowser* implementation = input.Implementation<IBrowser>();
            ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
            implementation->SetURL(param0);
        },

        // virtual string GetURL() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IBrowser* implementation = input.Implementation<IBrowser>();
            ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
            const string output = implementation->GetURL();

            // write return value
            writer.Text(output);
        },

        // virtual uint32_t GetFPS() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IBrowser* implementation = input.Implementation<IBrowser>();
            ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
            const uint32_t output = implementation->GetFPS();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual void Hide(const bool) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const bool param0 = reader.Boolean();

            // call implementation
            IBrowser* implementation = input.Implementation<IBrowser>();
            ASSERT((implementation != nullptr) && "Null IBrowser implementation pointer");
            implementation->Hide(param0);
        },

        nullptr
    }; // BrowserStubMethods[]

    //
    // IBrowser::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void LoadFinished(const string&) = 0
    //  (1) virtual void URLChanged(const string&) = 0
    //  (2) virtual void Hidden(const bool) = 0
    //  (3) virtual void Closure() = 0
    //

    ProxyStub::MethodHandler BrowserNotificationStubMethods[] = {
        // virtual void LoadFinished(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IBrowser::INotification* implementation = input.Implementation<IBrowser::INotification>();
            ASSERT((implementation != nullptr) && "Null IBrowser::INotification implementation pointer");
            implementation->LoadFinished(param0);
        },

        // virtual void URLChanged(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IBrowser::INotification* implementation = input.Implementation<IBrowser::INotification>();
            ASSERT((implementation != nullptr) && "Null IBrowser::INotification implementation pointer");
            implementation->URLChanged(param0);
        },

        // virtual void Hidden(const bool) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const bool param0 = reader.Boolean();

            // call implementation
            IBrowser::INotification* implementation = input.Implementation<IBrowser::INotification>();
            ASSERT((implementation != nullptr) && "Null IBrowser::INotification implementation pointer");
            implementation->Hidden(param0);
        },

        // virtual void Closure() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IBrowser::INotification* implementation = input.Implementation<IBrowser::INotification>();
            ASSERT((implementation != nullptr) && "Null IBrowser::INotification implementation pointer");
            implementation->Closure();
        },

        nullptr
    }; // BrowserNotificationStubMethods[]

    //
    // IBrowser::IMetadata interface stub definitions
    //
    // Methods:
    //  (0) virtual string LocalCache() const = 0
    //  (1) virtual string CookieStore() const = 0
    //  (2) virtual void GarbageCollect() = 0
    //

    ProxyStub::MethodHandler BrowserMetadataStubMethods[] = {
        // virtual string LocalCache() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IBrowser::IMetadata* implementation = input.Implementation<IBrowser::IMetadata>();
            ASSERT((implementation != nullptr) && "Null IBrowser::IMetadata implementation pointer");
            const string output = implementation->LocalCache();

            // write return value
            writer.Text(output);
        },

        // virtual string CookieStore() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IBrowser::IMetadata* implementation = input.Implementation<IBrowser::IMetadata>();
            ASSERT((implementation != nullptr) && "Null IBrowser::IMetadata implementation pointer");
            const string output = implementation->CookieStore();

            // write return value
            writer.Text(output);
        },

        // virtual void GarbageCollect() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IBrowser::IMetadata* implementation = input.Implementation<IBrowser::IMetadata>();
            ASSERT((implementation != nullptr) && "Null IBrowser::IMetadata implementation pointer");
            implementation->GarbageCollect();
        },

        nullptr
    }; // BrowserMetadataStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IBrowser interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(IBrowser::INotification*) = 0
    //  (1) virtual void Unregister(IBrowser::INotification*) = 0
    //  (2) virtual void SetURL(const string&) = 0
    //  (3) virtual string GetURL() const = 0
    //  (4) virtual uint32_t GetFPS() const = 0
    //  (5) virtual void Hide(const bool) = 0
    //

    class BrowserProxy final : public ProxyStub::UnknownProxyType<IBrowser> {
    public:
        BrowserProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(IBrowser::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IBrowser::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IBrowser::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IBrowser::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void SetURL(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        string GetURL() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        uint32_t GetFPS() const override
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

        void Hide(const bool param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class BrowserProxy

    //
    // IBrowser::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void LoadFinished(const string&) = 0
    //  (1) virtual void URLChanged(const string&) = 0
    //  (2) virtual void Hidden(const bool) = 0
    //  (3) virtual void Closure() = 0
    //

    class BrowserNotificationProxy final : public ProxyStub::UnknownProxyType<IBrowser::INotification> {
    public:
        BrowserNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void LoadFinished(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void URLChanged(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void Hidden(const bool param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void Closure() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class BrowserNotificationProxy

    //
    // IBrowser::IMetadata interface proxy definitions
    //
    // Methods:
    //  (0) virtual string LocalCache() const = 0
    //  (1) virtual string CookieStore() const = 0
    //  (2) virtual void GarbageCollect() = 0
    //

    class BrowserMetadataProxy final : public ProxyStub::UnknownProxyType<IBrowser::IMetadata> {
    public:
        BrowserMetadataProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string LocalCache() const override
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

        string CookieStore() const override
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

        void GarbageCollect() override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class BrowserMetadataProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IBrowser::INotification, BrowserNotificationStubMethods> BrowserNotificationStub;
        typedef ProxyStub::UnknownStubType<IBrowser, BrowserStubMethods> BrowserStub;
        typedef ProxyStub::UnknownStubType<IBrowser::IMetadata, BrowserMetadataStubMethods> BrowserMetadataStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IBrowser::INotification, BrowserNotificationProxy, BrowserNotificationStub>();
                RPC::Administrator::Instance().Announce<IBrowser, BrowserProxy, BrowserStub>();
                RPC::Administrator::Instance().Announce<IBrowser::IMetadata, BrowserMetadataProxy, BrowserMetadataStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

//
// generated automatically from "IPackager.h"
//
// implements RPC proxy stubs for:
//   - class IPackager
//   - class IPackager::IInstallationInfo
//   - class IPackager::IPackageInfo
//   - class IPackager::INotification
//

#include "IPackager.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IPackager interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(IPackager::INotification*) = 0
    //  (1) virtual void Unregister(const IPackager::INotification*) = 0
    //  (2) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (3) virtual uint32_t Install(const string&, const string&, const string&) = 0
    //  (4) virtual uint32_t SynchronizeRepository() = 0
    //

    ProxyStub::MethodHandler PackagerStubMethods[] = {
        // virtual void Register(IPackager::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IPackager::INotification* param0 = reader.Number<IPackager::INotification*>();
            IPackager::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IPackager::INotification::ID, false, IPackager::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IPackager::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPackager::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPackager::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IPackager* implementation = input.Implementation<IPackager>();
                ASSERT((implementation != nullptr) && "Null IPackager implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(const IPackager::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IPackager::INotification* param0 = reader.Number<IPackager::INotification*>();
            IPackager::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IPackager::INotification::ID, false, IPackager::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IPackager::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPackager::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPackager::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IPackager* implementation = input.Implementation<IPackager>();
                ASSERT((implementation != nullptr) && "Null IPackager implementation pointer");
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
                IPackager* implementation = input.Implementation<IPackager>();
                ASSERT((implementation != nullptr) && "Null IPackager implementation pointer");
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

        // virtual uint32_t Install(const string&, const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();
            const string param2 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPackager* implementation = input.Implementation<IPackager>();
            ASSERT((implementation != nullptr) && "Null IPackager implementation pointer");
            const uint32_t output = implementation->Install(param0, param1, param2);

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t SynchronizeRepository() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPackager* implementation = input.Implementation<IPackager>();
            ASSERT((implementation != nullptr) && "Null IPackager implementation pointer");
            const uint32_t output = implementation->SynchronizeRepository();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // PackagerStubMethods[]

    //
    // IPackager::IInstallationInfo interface stub definitions
    //
    // Methods:
    //  (0) virtual IPackager::state State() const = 0
    //  (1) virtual uint8_t Progress() const = 0
    //  (2) virtual uint32_t ErrorCode() const = 0
    //  (3) virtual uint32_t Abort() = 0
    //

    ProxyStub::MethodHandler PackagerInstallationInfoStubMethods[] = {
        // virtual IPackager::state State() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IInstallationInfo* implementation = input.Implementation<IPackager::IInstallationInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer");
            const IPackager::state output = implementation->State();

            // write return value
            writer.Number<const IPackager::state>(output);
        },

        // virtual uint8_t Progress() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IInstallationInfo* implementation = input.Implementation<IPackager::IInstallationInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer");
            const uint8_t output = implementation->Progress();

            // write return value
            writer.Number<const uint8_t>(output);
        },

        // virtual uint32_t ErrorCode() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IInstallationInfo* implementation = input.Implementation<IPackager::IInstallationInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer");
            const uint32_t output = implementation->ErrorCode();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Abort() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPackager::IInstallationInfo* implementation = input.Implementation<IPackager::IInstallationInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer");
            const uint32_t output = implementation->Abort();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // PackagerInstallationInfoStubMethods[]

    //
    // IPackager::IPackageInfo interface stub definitions
    //
    // Methods:
    //  (0) virtual string Name() const = 0
    //  (1) virtual string Version() const = 0
    //  (2) virtual string Architecture() const = 0
    //

    ProxyStub::MethodHandler PackagerPackageInfoStubMethods[] = {
        // virtual string Name() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IPackageInfo* implementation = input.Implementation<IPackager::IPackageInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer");
            const string output = implementation->Name();

            // write return value
            writer.Text(output);
        },

        // virtual string Version() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IPackageInfo* implementation = input.Implementation<IPackager::IPackageInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer");
            const string output = implementation->Version();

            // write return value
            writer.Text(output);
        },

        // virtual string Architecture() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IPackager::IPackageInfo* implementation = input.Implementation<IPackager::IPackageInfo>();
            ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer");
            const string output = implementation->Architecture();

            // write return value
            writer.Text(output);
        },

        nullptr
    }; // PackagerPackageInfoStubMethods[]

    //
    // IPackager::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void StateChange(IPackager::IPackageInfo*, IPackager::IInstallationInfo*) = 0
    //  (1) virtual void RepositorySynchronize(uint32_t) = 0
    //

    ProxyStub::MethodHandler PackagerNotificationStubMethods[] = {
        // virtual void StateChange(IPackager::IPackageInfo*, IPackager::IInstallationInfo*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IPackager::IPackageInfo* param0 = reader.Number<IPackager::IPackageInfo*>();
            IPackager::IInstallationInfo* param1 = reader.Number<IPackager::IInstallationInfo*>();
            IPackager::IPackageInfo* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IPackager::IPackageInfo::ID, false, IPackager::IPackageInfo::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IPackager::IPackageInfo>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPackager::IPackageInfo proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPackager::IPackageInfo proxy");
                }
            }
            IPackager::IInstallationInfo* param1_proxy = nullptr;
            ProxyStub::UnknownProxy* param1_proxy_inst = nullptr;
            if (param1 != nullptr) {
                param1_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param1, IPackager::IInstallationInfo::ID, false, IPackager::IInstallationInfo::ID, true);
                if (param1_proxy_inst != nullptr) {
                    param1_proxy = param1_proxy_inst->QueryInterface<IPackager::IInstallationInfo>();
                }

                ASSERT((param1_proxy != nullptr) && "Failed to get instance of IPackager::IInstallationInfo proxy");
                if (param1_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IPackager::IInstallationInfo proxy");
                }
            }

            if (((param0 == nullptr) || (param0_proxy != nullptr)) && ((param1 == nullptr) || (param1_proxy != nullptr))) {
                // call implementation
                IPackager::INotification* implementation = input.Implementation<IPackager::INotification>();
                ASSERT((implementation != nullptr) && "Null IPackager::INotification implementation pointer");
                implementation->StateChange(param0_proxy, param1_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
            if (param1_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param1_proxy_inst, message->Response());
            }
        },

        // virtual void RepositorySynchronize(uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IPackager::INotification* implementation = input.Implementation<IPackager::INotification>();
            ASSERT((implementation != nullptr) && "Null IPackager::INotification implementation pointer");
            implementation->RepositorySynchronize(param0);
        },

        nullptr
    }; // PackagerNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IPackager interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(IPackager::INotification*) = 0
    //  (1) virtual void Unregister(const IPackager::INotification*) = 0
    //  (2) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //  (3) virtual uint32_t Install(const string&, const string&, const string&) = 0
    //  (4) virtual uint32_t SynchronizeRepository() = 0
    //

    class PackagerProxy final : public ProxyStub::UnknownProxyType<IPackager> {
    public:
        PackagerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(IPackager::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPackager::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(const IPackager::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IPackager::INotification*>(param0);

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

        uint32_t Install(const string& param0, const string& param1, const string& param2) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);
            writer.Text(param2);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t SynchronizeRepository() override
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
    }; // class PackagerProxy

    //
    // IPackager::IInstallationInfo interface proxy definitions
    //
    // Methods:
    //  (0) virtual IPackager::state State() const = 0
    //  (1) virtual uint8_t Progress() const = 0
    //  (2) virtual uint32_t ErrorCode() const = 0
    //  (3) virtual uint32_t Abort() = 0
    //

    class PackagerInstallationInfoProxy final : public ProxyStub::UnknownProxyType<IPackager::IInstallationInfo> {
    public:
        PackagerInstallationInfoProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        IPackager::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            IPackager::state output = static_cast<IPackager::state>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IPackager::state>();
            }

            return output;
        }

        uint8_t Progress() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            uint8_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint8_t>();
            }

            return output;
        }

        uint32_t ErrorCode() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Abort() override
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
    }; // class PackagerInstallationInfoProxy

    //
    // IPackager::IPackageInfo interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Name() const = 0
    //  (1) virtual string Version() const = 0
    //  (2) virtual string Architecture() const = 0
    //

    class PackagerPackageInfoProxy final : public ProxyStub::UnknownProxyType<IPackager::IPackageInfo> {
    public:
        PackagerPackageInfoProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Name() const override
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

        string Version() const override
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

        string Architecture() const override
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
    }; // class PackagerPackageInfoProxy

    //
    // IPackager::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void StateChange(IPackager::IPackageInfo*, IPackager::IInstallationInfo*) = 0
    //  (1) virtual void RepositorySynchronize(uint32_t) = 0
    //

    class PackagerNotificationProxy final : public ProxyStub::UnknownProxyType<IPackager::INotification> {
    public:
        PackagerNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void StateChange(IPackager::IPackageInfo* param0, IPackager::IInstallationInfo* param1) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPackager::IPackageInfo*>(param0);
            writer.Number<IPackager::IInstallationInfo*>(param1);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void RepositorySynchronize(uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class PackagerNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IPackager::IInstallationInfo, PackagerInstallationInfoStubMethods> PackagerInstallationInfoStub;
        typedef ProxyStub::UnknownStubType<IPackager, PackagerStubMethods> PackagerStub;
        typedef ProxyStub::UnknownStubType<IPackager::IPackageInfo, PackagerPackageInfoStubMethods> PackagerPackageInfoStub;
        typedef ProxyStub::UnknownStubType<IPackager::INotification, PackagerNotificationStubMethods> PackagerNotificationStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IPackager::IInstallationInfo, PackagerInstallationInfoProxy, PackagerInstallationInfoStub>();
                RPC::Administrator::Instance().Announce<IPackager, PackagerProxy, PackagerStub>();
                RPC::Administrator::Instance().Announce<IPackager::IPackageInfo, PackagerPackageInfoProxy, PackagerPackageInfoStub>();
                RPC::Administrator::Instance().Announce<IPackager::INotification, PackagerNotificationProxy, PackagerNotificationStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

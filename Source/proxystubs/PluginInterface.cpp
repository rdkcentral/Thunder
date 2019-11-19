#include "Module.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace PluginHost;

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    //
    // IPlugin interface stub definitions (plugins/IPlugin.h)
    //
    ProxyStub::MethodHandler PluginStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual const string Initialize(PluginHost::IShell* shell) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Output& response(message->Response());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(response.Writer());

            IPlugin* implementation(parameters.Implementation<IPlugin>());
            string result(_T("Implementation of ProxyStub NOT supplied."));

            ASSERT(implementation != nullptr);

            if (implementation != nullptr) {
                ProxyStub::UnknownProxy* proxy = nullptr;
                IShell* param0_proxy = reader.Number<IShell*>();

                if (param0_proxy != nullptr) {
                    proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IShell::ID, false, IShell::ID, true);
                    param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IShell>() : nullptr);

                    ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
                }

                result = (implementation->Initialize(param0_proxy));

                if (param0_proxy != nullptr) {
                    RPC::Administrator::Instance().Release(proxy, message->Response());
                }
            }

            writer.Text(result);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Deinitialize(PluginHost::IShell* shell) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            ProxyStub::UnknownProxy* proxy = nullptr;
            IShell* param0_proxy = message->Parameters().Reader().Number<IShell*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IShell::ID, false, IShell::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IShell>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            message->Parameters().Implementation<IPlugin>()->Deinitialize(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Information() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IPlugin>()->Information());
        },
        nullptr
    };
    // IPlugin stub definitions

    //
    // IShell interface stub definitions (plugins/IShell.h)
    //
    ProxyStub::MethodHandler ShellStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void EnableWebServer(const string& URLPath, const string& fileSystemPath) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            string URLPath(reader.Text());
            string fileSystemPath(reader.Text());

            message->Parameters().Implementation<IShell>()->EnableWebServer(URLPath, fileSystemPath);
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void DisableWebServer() = 0;
            //
            message->Parameters().Implementation<IShell>()->DisableWebServer();
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, this interface method is in a different order from the actual interface, which means all methods
            // following this one are also out of order.
            //
            // virtual string ConfigLine() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->ConfigLine());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order due to ConfigLine() in incorrect position.
            //
            // virtual string Versions () const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Versions());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Back in order due to Model() missing before this.
            //
            // virtual bool Background() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(message->Parameters().Implementation<IShell>()->Background());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Accessor() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Accessor());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string WebPrefix() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->WebPrefix());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Locator() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Locator());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string ClassName() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->ClassName());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string Callsign() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Callsign());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string PersistentPath() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->PersistentPath());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string VolatilePath() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->VolatilePath());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string DataPath() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->DataPath());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, this interface method is in a different order from the actual interface, which means all methods
            // following this one are also out of order.
            //
            // virtual string HashKey() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->HashKey());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual bool AutoStart() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(message->Parameters().Implementation<IShell>()->AutoStart());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order due to Model() missing.
            //
            // virtual void Notify(const string& message) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());

            message->Parameters().Implementation<IShell>()->Notify(reader.Text());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            uint32_t id(parameters.Number<uint32_t>());
            string name(parameters.Text());

            response.Number<void*>(message->Parameters().Implementation<IShell>()->QueryInterfaceByCallsign(id, name));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual void Register(IPlugin::INotification* sink) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            IPlugin::INotification* param0_proxy = reader.Number<IPlugin::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IPlugin::INotification::ID, false, IPlugin::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IPlugin::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<IShell>()->Register(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual void Unregister(IPlugin::INotification* sink) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            IPlugin::INotification* param0_proxy = reader.Number<IPlugin::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IPlugin::INotification::ID, false, IPlugin::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IPlugin::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }
            parameters.Implementation<IShell>()->Unregister(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual uint32_t Activate(const reason) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            IShell::reason reason(reader.Number<IShell::reason>());

            response.Number<uint32_t>(message->Parameters().Implementation<IShell>()->Activate(reason));
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual uint32_t Deactivate(const reason) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            IShell::reason reason(reader.Number<IShell::reason>());

            response.Number<uint32_t>(message->Parameters().Implementation<IShell>()->Deactivate(reason));
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual state State() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number<IShell::state>(message->Parameters().Implementation<IShell>()->State());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual reason Reason() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number<IShell::reason>(message->Parameters().Implementation<IShell>()->Reason());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual string Model() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Model());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual ISubSystem* SubSystems() = 0;
            //
            ISubSystem* output = message->Parameters().Implementation<IShell>()->SubSystems();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<ISubSystem*>(output);
            RPC::Administrator::Instance().RegisterInterface(channel, output);
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string VolatilePath() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->ProxyStubPath());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual bool IsSupported(const uint8_t reason) const = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            uint8_t version(reader.Number<uint8_t>());

            response.Boolean(message->Parameters().Implementation<IShell>()->IsSupported(version));
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order due to ConfigLine() in incorrect position.
            //
            // virtual string Version () const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->Version());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual bool Resumed() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(message->Parameters().Implementation<IShell>()->Resumed());
        },
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string ConfigSubstitution(const string& input) const = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<IShell>()->ConfigSubstitution(reader.Text()));
        },
        nullptr
    };
    // IShell stub definitions

    //
    // IStateControl interface stub definitions (plugins/IStateControl.h)
    //
    ProxyStub::MethodHandler StateControlStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            ProxyStub::UnknownProxy* proxy = nullptr;
            IShell* param0_proxy = reader.Number<IShell*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IShell::ID, false, IShell::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IShell>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            writer.Number<uint32_t>(message->Parameters().Implementation<IStateControl>()->Configure(param0_proxy));

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual IStateControl::state State() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Number<IStateControl::state>(message->Parameters().Implementation<IStateControl>()->State());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Request(const IStateControl::Command command) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
            IStateControl::command command = parameters.Number<IStateControl::command>();
            response.Number(message->Parameters().Implementation<IStateControl>()->Request(command));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(IStateControl::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            IStateControl::INotification* param0_proxy = reader.Number<IStateControl::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IStateControl::INotification::ID, false, IStateControl::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IStateControl::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<IStateControl>()->Register(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IStateControl::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            IStateControl::INotification* param0_proxy = reader.Number<IStateControl::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, IStateControl::INotification::ID, false, IStateControl::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<IStateControl::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<IStateControl>()->Unregister(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        nullptr
    };
    // IStateControl stub definitions

    //
    // IStateControl::INotification interface stub definitions (plugins/IStateControl.h)
    //
    ProxyStub::MethodHandler StateControlNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void StateChange(const IStateControl::state state) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            IStateControl::state newState(parameters.Number<IStateControl::state>());

            message->Parameters().Implementation<IStateControl::INotification>()->StateChange(newState);
        },
        nullptr
    };
    // IStateControl::INotification stub definitions

    //
    // ISubSystem interface stub definitions (plugins/ISubSystem.h)
    //
    ProxyStub::MethodHandler SubSystemStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Register(ISubSystem::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            ISubSystem::INotification* param0_proxy = reader.Number<ISubSystem::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, ISubSystem::INotification::ID, false, ISubSystem::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<ISubSystem::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<ISubSystem>()->Register(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(ISubSystem::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ProxyStub::UnknownProxy* proxy = nullptr;
            ISubSystem::INotification* param0_proxy = reader.Number<ISubSystem::INotification*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, ISubSystem::INotification::ID, false, ISubSystem::INotification::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<ISubSystem::INotification>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<ISubSystem>()->Unregister(param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual string BuildTreeHash() const = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());
            response.Text(message->Parameters().Implementation<ISubSystem>()->BuildTreeHash());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Set(const ISubSystem::event type, Core::IUnknown* information) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            ISubSystem::subsystem eventType = reader.Number<ISubSystem::subsystem>();

            ProxyStub::UnknownProxy* proxy = nullptr;
            Core::IUnknown* param0_proxy = reader.Number<Core::IUnknown*>();

            if (param0_proxy != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance(channel, param0_proxy, Core::IUnknown::ID, false, Core::IUnknown::ID, true);
                param0_proxy = (proxy != nullptr ? proxy->QueryInterface<Core::IUnknown>() : nullptr);

                ASSERT((param0_proxy != nullptr) && "Failed to create proxy");
            }

            parameters.Implementation<ISubSystem>()->Set(eventType, param0_proxy);

            if (param0_proxy != nullptr) {
                RPC::Administrator::Instance().Release(proxy, message->Response());
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual const Core::IUnknown* Get (const subsystem type) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            ISubSystem::subsystem eventType = reader.Number<ISubSystem::subsystem>();
            response.Number<const Core::IUnknown*>(parameters.Implementation<ISubSystem>()->Get(eventType));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual bool IsActive(const subsystem type) const = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            ISubSystem::subsystem eventType = reader.Number<ISubSystem::subsystem>();
            response.Boolean(parameters.Implementation<ISubSystem>()->IsActive(eventType));
        },
        nullptr

    };
    // ISubSystem stub definitions

    //
    // ISubSystem::INotification interface stub definitions (plugins/ISubSystem.h)
    //
    ProxyStub::MethodHandler SubSystemNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Updated() = 0;
            //
            message->Parameters().Implementation<ISubSystem::INotification>()->Updated();
        },
        nullptr
    };
    // ISubSystem::INotification stub definitions

    typedef ProxyStub::UnknownStubType<IPlugin, PluginStubMethods> PluginStub;
    typedef ProxyStub::UnknownStubType<IShell, ShellStubMethods> ShellStub;
    typedef ProxyStub::UnknownStubType<IStateControl, StateControlStubMethods> StateControlStub;
    typedef ProxyStub::UnknownStubType<IStateControl::INotification, StateControlNotificationStubMethods> StateControlNotificationStub;
    typedef ProxyStub::UnknownStubType<ISubSystem, SubSystemStubMethods> SubSystemStub;
    typedef ProxyStub::UnknownStubType<ISubSystem::INotification, SubSystemNotificationStubMethods> SubSystemNotificationStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class PluginProxy : public ProxyStub::UnknownProxyType<IPlugin> {
    public:
        PluginProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~PluginProxy()
        {
        }

    public:
        // Stub order:
        // virtual const string Initialize(PluginHost::IShell* shell) = 0;
        // virtual void Deinitialize(PluginHost::IShell* shell) = 0;
        // virtual string Information() const = 0;

        virtual const string Initialize(IShell* service) override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                result = reader.Text();
                Complete(reader);
            }

            return (result);
        }
        virtual void Deinitialize(IShell* service) override
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual string Information() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(2));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                result = reader.Text();
            }

            return (result);
        }
    };

    class ShellProxy : public ProxyStub::UnknownProxyType<IShell> {
    public:
        ShellProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~ShellProxy()
        {
        }

    public:
        // Stub order:
        // virtual void EnableWebServer(const string& URLPath, const string& fileSystemPath) = 0;
        // virtual void DisableWebServer() = 0;
        // virtual string ConfigLine() const = 0;
        // virtual string Versions () const = 0;
        // virtual bool Background() const = 0;
        // virtual string Accessor() const = 0;
        // virtual string WebPrefix() const = 0;
        // virtual string Locator() const = 0;
        // virtual string ClassName() const = 0;
        // virtual string Callsign() const = 0;
        // virtual string PersistentPath() const = 0;
        // virtual string VolatilePath() const = 0;
        // virtual string DataPath() const = 0;
        // virtual string HashKey() const = 0;
        // virtual bool AutoStart() const = 0;
        // virtual bool Resumed() const = 0;
        // virtual void Notify(const string& message) = 0;
        // virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;
        // virtual void Register(IPlugin::INotification* sink) = 0;
        // virtual void Unregister(IPlugin::INotification* sink) = 0;
        // virtual uint32_t Activate(const reason) = 0;
        // virtual uint32_t Deactivate(const reason) = 0;
        // virtual state State() const = 0;
        // virtual reason Reason() const = 0;
        // virtual string Model() const = 0;
        // virtual bool IsSupported(const uint8_t version) const = 0;
        virtual void EnableWebServer(const string& URLPath, const string& fileSystemPath) override
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Text(URLPath);
            writer.Text(fileSystemPath);

            Invoke(newMessage);
        }
        virtual void DisableWebServer() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            Invoke(newMessage);
        }
        virtual string ConfigLine() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(2));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string Versions() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(3));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual bool Background() const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(4));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }

            return (result);
        }
        virtual string Accessor() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(5));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string WebPrefix() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(6));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string Locator() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(7));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string ClassName() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(8));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string Callsign() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(9));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string PersistentPath() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(10));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string VolatilePath() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(11));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string DataPath() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(12));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual string HashKey() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(13));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual bool AutoStart() const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(14));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }

            return (result);
        }
        virtual void Notify(const string& message) override
        {
            IPCMessage newMessage(BaseClass::Message(15));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Text(message);

            Invoke(newMessage);
        }
        virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override
        {
            void* result = nullptr;
            IPCMessage newMessage(BaseClass::Message(16));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number(id);
            writer.Text(name);

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = Interface(newMessage->Response().Reader().Number<void*>(), id);
            }

            return (result);
        }
        virtual void Register(IPlugin::INotification* sink) override
        {
            IPCMessage newMessage(BaseClass::Message(17));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPlugin::INotification*>(sink);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual void Unregister(IPlugin::INotification* sink) override
        {
            IPCMessage newMessage(BaseClass::Message(18));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPlugin::INotification*>(sink);

            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual uint32_t Activate(const IShell::reason theReason) override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(19));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<IShell::reason>(theReason);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
        virtual uint32_t Deactivate(const IShell::reason theReason) override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(20));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<IShell::reason>(theReason);

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
        virtual IShell::state State() const override
        {
            IShell::state result = IShell::state::DESTROYED;
            IPCMessage newMessage(BaseClass::Message(21));
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<IShell::state>();
            }

            return (result);
        }
        virtual IShell::reason Reason() const override
        {
            IShell::reason result = IShell::reason::FAILURE;

            IPCMessage newMessage(BaseClass::Message(22));
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<IShell::reason>();
            }

            return (result);
        }
        virtual string Model() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(23));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual ISubSystem* SubSystems() override
        {
            ISubSystem* result = nullptr;
            IPCMessage newMessage(BaseClass::Message(24));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = reinterpret_cast<ISubSystem*>(Interface(newMessage->Response().Reader().Number<void*>(), ISubSystem::ID));
            }
            return (result);
        }
        virtual string ProxyStubPath() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(25));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual bool IsSupported(const uint8_t version) const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(26));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<uint8_t>(version);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }

            return (result);
        }
        virtual string Version() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(27));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual bool Resumed() const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(28));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }

            return (result);
        }
        virtual string ConfigSubstitution(const string& input) const override
        {
            IPCMessage newMessage(BaseClass::Message(29));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Text(input);
            string result;

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
        virtual ICOMLink* COMLink() override
        {
            return (nullptr);
        }
        virtual uint32_t Submit(const uint32_t, const Core::ProxyType<Core::JSON::IElement>&) override
        {
            return (Core::ERROR_UNAVAILABLE);
        }
    };

    class StateControlProxy : public ProxyStub::UnknownProxyType<IStateControl> {
    public:
        StateControlProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~StateControlProxy()
        {
        }

    public:
        // Stub order:
        // virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
        // virtual IStateControl::state State() const = 0;
        // virtual void Request(const IStateControl::Command command) = 0;
        // virtual void Register(IStateControl::INotification* notification) = 0;
        // virtual void Unregister(IStateControl::INotification* notification) = 0;
        virtual uint32_t Configure(IShell* service) override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }
            return (result);
        }
        virtual IStateControl::state State() const override
        {
            IStateControl::state result = IStateControl::state::UNINITIALIZED;

            IPCMessage newMessage(BaseClass::Message(1));
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<IStateControl::state>();
            }

            return (result);
        }
        virtual uint32_t Request(const IStateControl::command command) override
        {
            uint32_t result = ~0;
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::command>(command);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }
            return (result);
        }
        virtual void Register(IStateControl::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::INotification*>(notification);
            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual void Unregister(IStateControl::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::INotification*>(notification);
            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    };

    class StateControlNotificationProxy : public ProxyStub::UnknownProxyType<IStateControl::INotification> {
    public:
        StateControlNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~StateControlNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void StateChange(const IStateControl::state state) = 0;
        virtual void StateChange(const IStateControl::state newState) override
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::state>(newState);
            Invoke(newMessage);
        }
    };

    class SubSystemProxy : public ProxyStub::UnknownProxyType<ISubSystem> {
    public:
        SubSystemProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~SubSystemProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Register(ISubSystem::INotification* notification) = 0;
        // virtual void Unregister(ISubSystem::INotification* notification) = 0;
        // virtual string BuildTreeHash() const = 0;
        // virtual void Event(const ISubSystem::event type, Core::IUnknown* information) = 0;
        // virtual const Core::IUnknown* Event(const subsystem type) const = 0;
        // virtual bool IsSet(const subsystem type) const = 0;
        virtual void Register(ISubSystem::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::INotification*>(notification);
            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual void Unregister(ISubSystem::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::INotification*>(notification);
            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual string BuildTreeHash() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(2));
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }
            return (result);
        }
        virtual void Set(const ISubSystem::subsystem type, Core::IUnknown* information) override
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            writer.Number<Core::IUnknown*>(information);
            if ((Invoke(newMessage) == Core::ERROR_NONE) && (newMessage->Response().Length() > 0)) {
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
        virtual const Core::IUnknown* Get(const subsystem type) const override
        {
            Core::IUnknown* result = nullptr;
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = reinterpret_cast<Core::IUnknown*>(Interface(newMessage->Response().Reader().Number<void*>(), Core::IUnknown::ID));
            }
            return (result);
        }
        virtual bool IsActive(const subsystem type) const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }
            return (result);
        }
    };

    class SubSystemNotificationProxy : public ProxyStub::UnknownProxyType<ISubSystem::INotification> {
    public:
        SubSystemNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~SubSystemNotificationProxy()
        {
        }

    public:
        // Stub order:
        // virtual void Updated() = 0;
        virtual void Updated() override
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            Invoke(newMessage);
        }
    };

    // -------------------------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------------------------
    static class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<IPlugin, PluginProxy, PluginStub>();
            RPC::Administrator::Instance().Announce<IShell, ShellProxy, ShellStub>();
            RPC::Administrator::Instance().Announce<IStateControl, StateControlProxy, StateControlStub>();
            RPC::Administrator::Instance().Announce<IStateControl::INotification, StateControlNotificationProxy, StateControlNotificationStub>();
            RPC::Administrator::Instance().Announce<ISubSystem, SubSystemProxy, SubSystemStub>();
            RPC::Administrator::Instance().Announce<ISubSystem::INotification, SubSystemNotificationProxy, SubSystemNotificationStub>();
        }
        ~Instantiation()
        {
        }

    } ProxyStubRegistration;
}
}

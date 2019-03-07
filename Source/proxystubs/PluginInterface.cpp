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
                IShell* proxy = RPC::Administrator::Instance().ProxyInstance<IShell>(channel, reader.Number<void*>());

                result = (implementation->Initialize(proxy));

                if (proxy != nullptr) {
                    proxy->Release();
                }
            }

            writer.Text(result);

        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Deinitialize(PluginHost::IShell* shell) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            IShell* proxy = RPC::Administrator::Instance().ProxyInstance<IShell>(channel, parameters.Number<void*>());

            message->Parameters().Implementation<IPlugin>()->Deinitialize(proxy);

            if (proxy != nullptr) {
                proxy->Release();
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
            // Careful, out of order due to HashKey() in incorrect position.
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

            IPlugin::INotification* proxy = RPC::Administrator::Instance().ProxyInstance<IPlugin::INotification>(channel, reader.Number<void*>());

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for Plugin::IPluginNotification: %d"), IPlugin::INotification::ID);
            } else {
                parameters.Implementation<IShell>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
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

            // Need to find the proxy that goes with the given implementation..
            IPlugin::INotification* stub = reader.Number<IPlugin::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IPlugin::INotification* proxy = RPC::Administrator::Instance().ProxyFind<IPlugin::INotification>(channel, stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Coud not find stub for Plugin::IPluginNotification: %p"), stub);
            } else {
                parameters.Implementation<IShell>()->Unregister(proxy);
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
        [](Core::ProxyType<Core::IPCChannel>& /* channel */, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Careful, out of order.
            //
            // virtual ISubSystem* SubSystems() = 0;
            //
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<ISubSystem*>(message->Parameters().Implementation<IShell>()->SubSystems());
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

            IShell* proxy = RPC::Administrator::Instance().ProxyInstance<IShell>(channel, reader.Number<void*>());

            writer.Number<uint32_t>(message->Parameters().Implementation<IStateControl>()->Configure(proxy));

            if (proxy != nullptr) {
                proxy->Release();
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

            IStateControl::INotification* implementation = reader.Number<IStateControl::INotification*>();
            IStateControl::INotification* proxy = RPC::Administrator::Instance().ProxyInstance<IStateControl::INotification>(channel, implementation);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for IStateControlNotification: %p"), implementation);
            } else {
                parameters.Implementation<IStateControl>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(IStateControl::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            IStateControl::INotification* stub = reader.Number<IStateControl::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            IStateControl::INotification* proxy = RPC::Administrator::Instance().ProxyFind<IStateControl::INotification>(channel, stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IStateControl::INotification: %p"), stub);
            } else {
                parameters.Implementation<IStateControl>()->Unregister(proxy);
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

            ISubSystem::INotification* implementation = reader.Number<ISubSystem::INotification*>();
            ISubSystem::INotification* proxy = RPC::Administrator::Instance().ProxyInstance<ISubSystem::INotification>(channel, implementation);

            ASSERT((proxy != nullptr) && "Failed to create proxy");

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not create a stub for ISubSystem::INotification: %p"), implementation);
            } else {
                parameters.Implementation<ISubSystem>()->Register(proxy);
                if (proxy->Release() != Core::ERROR_NONE) {
                    TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
                }
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Unregister(ISubSystem::INotification* notification) = 0;
            //
            RPC::Data::Input& parameters(message->Parameters());
            RPC::Data::Frame::Reader reader(parameters.Reader());

            // Need to find the proxy that goes with the given implementation..
            ISubSystem::INotification* stub = reader.Number<ISubSystem::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            ISubSystem::INotification* proxy = RPC::Administrator::Instance().ProxyFind<ISubSystem::INotification>(channel, stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Could not find stub for IStateControl::INotification: %p"), stub);
            } else {
                parameters.Implementation<ISubSystem>()->Unregister(proxy);
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
            Core::IUnknown* information = reader.Number<Core::IUnknown*>();
            Core::IUnknown* proxy = nullptr;
            if (information != nullptr)
            {
                proxy = RPC::Administrator::Instance().ProxyInstance<Core::IUnknown>(channel, information);

                ASSERT((proxy != nullptr) && "Failed to create proxy");
                if (proxy == nullptr) {
                    TRACE_L1(_T("Could not create a stub for Core::IUnknown: %p"), information);
                }
            }

            parameters.Implementation<ISubSystem>()->Set(eventType, proxy);
            if ((proxy != nullptr) && (proxy->Release() != Core::ERROR_NONE)) {
                TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
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
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual void Deinitialize(IShell* service) override
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual string Information() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
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

            Complete(newMessage->Response());
        }
        virtual void DisableWebServer() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual string ConfigLine() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string Versions() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual bool Background() const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            return (reader.Boolean());
        }
        virtual string Accessor() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string WebPrefix() const override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string Locator() const override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string ClassName() const override
        {
            IPCMessage newMessage(BaseClass::Message(8));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string Callsign() const override
        {
            IPCMessage newMessage(BaseClass::Message(9));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string PersistentPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(10));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string VolatilePath() const override
        {
            IPCMessage newMessage(BaseClass::Message(11));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string DataPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(12));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual string HashKey() const override
        {
            IPCMessage newMessage(BaseClass::Message(13));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual bool AutoStart() const override
        {
            IPCMessage newMessage(BaseClass::Message(14));

            Invoke(newMessage);

            return (Boolean(newMessage->Response()));
        }
        virtual void Notify(const string& message) override
        {
            IPCMessage newMessage(BaseClass::Message(15));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Text(message);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override
        {
            IPCMessage newMessage(BaseClass::Message(16));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number(id);
            writer.Text(name);

            Invoke(newMessage);

            return (Interface(newMessage->Response(), id));
        }
        virtual void Register(IPlugin::INotification* sink) override
        {
            IPCMessage newMessage(BaseClass::Message(17));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPlugin::INotification*>(sink);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual void Unregister(IPlugin::INotification* sink) override
        {
            IPCMessage newMessage(BaseClass::Message(18));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IPlugin::INotification*>(sink);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual uint32_t Activate(const IShell::reason theReason) override
        {
            IPCMessage newMessage(BaseClass::Message(19));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<IShell::reason>(theReason);
            Invoke(newMessage);

            return (Number<uint32_t>(newMessage->Response()));
        }
        virtual uint32_t Deactivate(const IShell::reason theReason) override
        {
            IPCMessage newMessage(BaseClass::Message(20));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<IShell::reason>(theReason);
            Invoke(newMessage);

            return (Number<uint32_t>(newMessage->Response()));
        }
        virtual IShell::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(21));
            Invoke(newMessage);

            return (Number<IShell::state>(newMessage->Response()));
        }
        virtual IShell::reason Reason() const override
        {
            IPCMessage newMessage(BaseClass::Message(22));
            Invoke(newMessage);

            return (Number<IShell::reason>(newMessage->Response()));
        }
        virtual string Model() const override
        {
            IPCMessage newMessage(BaseClass::Message(23));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual ISubSystem* SubSystems() override
        {
            IPCMessage newMessage(BaseClass::Message(24));

            Invoke(newMessage);

            return (reinterpret_cast<ISubSystem*>(Interface(newMessage->Response(), ISubSystem::ID)));
        }
        virtual string ProxyStubPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(25));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual bool IsSupported(const uint8_t version) const override
        {
            IPCMessage newMessage(BaseClass::Message(26));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

            writer.Number<uint8_t>(version);
            Invoke(newMessage);

            return (Boolean(newMessage->Response()));
        }
        virtual string Version() const override
        {
            IPCMessage newMessage(BaseClass::Message(27));

            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
        virtual IProcess* Process() override {
            return (nullptr);
        }
        virtual uint32_t Submit(const uint32_t, const Core::ProxyType<Core::JSON::IElement>&) override {
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
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IShell*>(service);
            Invoke(newMessage);
            return (Number<uint32_t>(newMessage->Response()));
        }
        virtual IStateControl::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);
            return (Number<IStateControl::state>(newMessage->Response()));
        }
        virtual uint32_t Request(const IStateControl::command command) override
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::command>(command);
            Invoke(newMessage);
            return (Number<uint32_t>(newMessage->Response()));
        }
        virtual void Register(IStateControl::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::INotification*>(notification);
            Invoke(newMessage);
            Complete(newMessage->Response());
        }
        virtual void Unregister(IStateControl::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStateControl::INotification*>(notification);
            Invoke(newMessage);
            Complete(newMessage->Response());
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
            Complete(newMessage->Response());
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
            Invoke(newMessage);
            Complete(newMessage->Response());
        }
        virtual void Unregister(ISubSystem::INotification* notification) override
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::INotification*>(notification);
            Invoke(newMessage);
            Complete(newMessage->Response());
        }
        virtual string BuildTreeHash() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));
            Invoke(newMessage);
            return Text(newMessage->Response());
        }
        virtual void Set(const ISubSystem::subsystem type, Core::IUnknown* information) override
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            writer.Number<Core::IUnknown*>(information);
            Invoke(newMessage);
            Complete(newMessage->Response());
        }
        virtual const Core::IUnknown* Get(const subsystem type) const override
        {
            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            Invoke(newMessage);
            return (reinterpret_cast<const Core::IUnknown*>(Interface(newMessage->Response(), Core::IUnknown::ID)));
        }
        virtual bool IsActive(const subsystem type) const override
        {
            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISubSystem::subsystem>(type);
            Invoke(newMessage);
            return Boolean(newMessage->Response());
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
            Complete(newMessage->Response());
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

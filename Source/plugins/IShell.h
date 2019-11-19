#ifndef __ISHELL_H
#define __ISHELL_H

#include "IPlugin.h"
#include "ISubSystem.h"

namespace WPEFramework {
namespace PluginHost {

    struct EXTERNAL IShell
        : virtual public Core::IUnknown {
        enum {
            ID = RPC::ID_SHELL
        };

        // This interface is only returned if the IShell is accessed in the main process. The interface can
        // be used to instantiate new objects (COM objects) in a new process, or monitor the state of such a process.
        // If this interface is requested outside of the main process, it will return a nullptr.
        struct EXTERNAL ICOMLink {
            virtual ~ICOMLink() {}
            virtual void Register(RPC::IRemoteConnection::INotification* sink) = 0;
            virtual void Unregister(RPC::IRemoteConnection::INotification* sink) = 0;
            virtual RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId) = 0;
            virtual void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId, const string& className, const string& callsign) = 0;
        };

        virtual ~IShell()
        {
        }

        // State of the IPlugin interface associated with this shell.
        enum state {
            DEACTIVATED,
            DEACTIVATION,
            ACTIVATED,
            ACTIVATION,
            PRECONDITION,
            DESTROYED
        };

        enum reason {
            REQUESTED,
            AUTOMATIC,
            FAILURE,
            MEMORY_EXCEEDED,
            STARTUP,
            SHUTDOWN,
            CONDITIONS
        };

        class EXTERNAL Job
            : public Core::IDispatchType<void> {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
            Job(IShell* shell, IShell::state toState, IShell::reason why)
                : _shell(shell)
                , _state(toState)
                , _reason(why)
            {
                if (shell != nullptr) {
                    shell->AddRef();
                }
            }

            virtual ~Job()
            {
                if (_shell != nullptr) {
                    _shell->Release();
                }
            }

        public:
            static Core::ProxyType<Core::IDispatchType<void>>
            Create(IShell* shell, IShell::state toState, IShell::reason why);

            virtual void Dispatch()
            {
                ASSERT(_shell != nullptr);

                if (_shell != nullptr) {

                    switch (_state) {
                    case ACTIVATED:
                        _shell->Activate(_reason);
                        break;
                    case DEACTIVATED:
                        _shell->Deactivate(_reason);
                        break;
                    default:
                        ASSERT(false);
                        break;
                    }
                }
            }

        private:
            IShell* _shell;
            state _state;
            reason _reason;
        };
        //! @{
        //! =========================== ACCESSOR TO THE SHELL AROUND THE PLUGIN ===============================
        //! This interface allows access to the shell that scontrolls the lifetimeof the Plugin. It's access
        //! and responses is based on configuration applicable to the plugin e.g. enabling a WebServer is
        //! enabling a webserver on a path, applicable to the plugin being initialized..
        //! The full network URL looks like: http://<accessor>/<webprefix>/<callsign> for accessing the plugin.
        //!
        //! Methods to enable/disable a webserver. URLPath is "added" to the end of the path that reaches this
        //! plugin, based on the DataPath.
        //! URL: http://<accessor>/<webprefix>/<callsign>/<URLPath> for accessing the webpages on
        //! <DataPath>/<fileSystemPath>.
        //! @}
        virtual void EnableWebServer(const string& URLPath, const string& fileSystemPath) = 0;
        virtual void DisableWebServer() = 0;

        //! Versions: Returns the version of the application hosting the plugin
        virtual string Version() const = 0;

        //! Model: Returns a Human Readable name for the platform it is running on.
        virtual string Model() const = 0;

        //! Background: If enabled, the PluginHost is running in daemon mode
        virtual bool Background() const = 0;

        //! Accessor: Identifier that can be used for Core:NodeId to connect to the webbridge.
        virtual string Accessor() const = 0;

        //! WebPrefix: First part of the pathname in the HTTP request to select the webbridge components.
        virtual string WebPrefix() const = 0;

        //! Locator: The name of the binary (so) that holds the given ClassName code.
        virtual string Locator() const = 0;

        //! ClassName: Name of the class to be instantiated for this IShell
        virtual string ClassName() const = 0;

        //! Versions: Returns a JSON Array of versions supported by this plugin.
        virtual string Versions() const = 0;

        //! Callsign: Instantiation name of this specific plugin. It is the name given in the config for the classname.
        virtual string Callsign() const = 0;

        //! PersistentPath: <config:persistentpath>/<plugin:callsign>/
        virtual string PersistentPath() const = 0;

        //! VolatilePath: <config:volatilepath>/<plugin:callsign>/
        virtual string VolatilePath() const = 0;

        //! DataPath: <config:datapath>/<plugin:classname>/
        virtual string DataPath() const = 0;

        //! VolatilePath: <config:volatilepath>/<plugin:callsign>/
        virtual string ProxyStubPath() const = 0;

        //! Substituted Config value
        virtual string ConfigSubstitution(const string& input) const = 0;

        //! AutoStart: boolean to inidcate wheter we need to start up this plugin at start
        virtual bool AutoStart() const = 0;

        //! Resumed: boolean to inidcate wheter we need to start a plugin in a Resumed state, i.s.o. the Suspended state
        virtual bool Resumed() const = 0;

        virtual string HashKey() const = 0;
        virtual string ConfigLine() const = 0;

        //! Return whether the given version is supported by this IShell instance.
        virtual bool IsSupported(const uint8_t version) const = 0;

        // Get access to the SubSystems and their corrresponding information. Information can be set or get to see what the
        // status of the sub systems is.
        virtual ISubSystem* SubSystems() = 0;

        // Notify all subscribers of this service with the given string.
        // It is expected to be JSON formatted strings as it is assumed that this is for reaching websockets clients living in
        // the web world that have build in functionality to parse JSON structs.
        virtual void Notify(const string& message) = 0;

        // Allow access to the Shells, configured for the different Plugins found in the configuration.
        // Calling the QueryInterfaceByCallsign with an empty callsign will query for interfaces located
        // on the controller.
        virtual void Register(IPlugin::INotification* sink) = 0;
        virtual void Unregister(IPlugin::INotification* sink) = 0;
        virtual state State() const = 0;
        virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;

        // Methods to Activate and Deactivate the aggregated Plugin to this shell.
        // NOTE: These are Blocking calls!!!!!
        virtual uint32_t Activate(const reason) = 0;
        virtual uint32_t Deactivate(const reason) = 0;
        virtual reason Reason() const = 0;

        // Method to access, in the main process space, the channel factory to submit JSON objects to be send.
        // This method will return a error if it is NOT in the main process.
        virtual uint32_t Submit(const uint32_t Id, const Core::ProxyType<Core::JSON::IElement>& response) = 0;

        // Method to access, in the main space, a COM factory to instantiate objects out-of-process.
        // This method will return a nullptr if it is NOT in the main process.
        virtual ICOMLink* COMLink() = 0;

        inline void Register(RPC::IRemoteConnection::INotification* sink)
        {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                handler->Register(sink);
            }
        }
        inline void Unregister(RPC::IRemoteConnection::INotification* sink)
        {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                handler->Unregister(sink);
            }
        }
        inline RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId)
        {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            return (handler == nullptr ? nullptr : handler->RemoteConnection(connectionId));
        }

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Root(uint32_t& pid, const uint32_t waitTime, const string className, const uint32_t version = ~0)
        {
            void* baseInterface (Root(pid, waitTime, className, REQUESTEDINTERFACE::ID, version));

            if (baseInterface != nullptr) {

                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }
        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* QueryInterfaceByCallsign(const string& name)
        {
            void* baseInterface(QueryInterfaceByCallsign(REQUESTEDINTERFACE::ID, name));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }
        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Instantiate(const uint32_t waitTime, const string className, const uint32_t version, uint32_t& connecionId, const string& locator)
        {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                RPC::Object definition(Callsign(), locator, className, REQUESTEDINTERFACE::ID, version, string(), string(), 1, RPC::Object::HostType::DISTRIBUTED, string());

                void* baseptr = handler->Instantiate(definition, waitTime, connecionId, ClassName(), Callsign());

                REQUESTEDINTERFACE* result = reinterpret_cast<REQUESTEDINTERFACE*>(baseptr);

                return (result);
            }

            return (nullptr);
        }

    private:
        void* Root(uint32_t& pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version = ~0);
    };
} // namespace PluginHost

namespace Core {

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IShell::state>*
    EnumerateType<PluginHost::IShell::state>::Table(const uint16_t);

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IShell::reason>*
    EnumerateType<PluginHost::IShell::reason>::Table(const uint16_t);

} // namespace Core
} // namespace WPEFramework

#endif // __ISHELL_H

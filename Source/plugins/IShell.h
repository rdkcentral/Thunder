/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __ISHELL_H__
#define __ISHELL_H__

#include "IPlugin.h"
#include "ISubSystem.h"

#include <com/ICOM.h>

namespace WPEFramework {

    namespace RPC {
        class Object;
    }

namespace PluginHost {

    struct EXTERNAL IShell : virtual public Core::IUnknown {
        enum { ID = RPC::ID_SHELL };

        // This interface is only returned if the IShell is accessed in the main process. The interface can
        // be used to instantiate new objects (COM objects) in a new process, or monitor the state of such a process.
        // If this interface is requested outside of the main process, it will return a nullptr.
        /* @stubgen:omit */
        struct EXTERNAL ICOMLink {

            struct INotification : virtual public Core::IUnknown {
                virtual ~INotification() = default;
                virtual void Dangling(const Core::IUnknown* source, const uint32_t interfaceId) = 0;
                virtual void Revoked(const Core::IUnknown* remote, const uint32_t interfaceId) = 0;
            };

            virtual ~ICOMLink() = default;
            virtual void Register(RPC::IRemoteConnection::INotification* sink) = 0;
            virtual void Unregister(const RPC::IRemoteConnection::INotification* sink) = 0;

            virtual void Register(INotification* sink) = 0;
            virtual void Unregister(INotification* sink) = 0;

            virtual RPC::IRemoteConnection* RemoteConnection(const uint32_t connectionId) = 0;
            virtual void* Instantiate(const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) = 0;
        };

        enum class startup : uint8_t {
            UNAVAILABLE,
            DEACTIVATED,
            ACTIVATED
        };

        // State of the IPlugin interface associated with this shell.
        enum state : uint8_t {
            UNAVAILABLE,
            DEACTIVATED,
            DEACTIVATION,
            ACTIVATED,
            ACTIVATION,
            PRECONDITION,
            HIBERNATED,
            DESTROYED
        };

        enum reason : uint8_t {
            REQUESTED,
            AUTOMATIC,
            FAILURE,
            MEMORY_EXCEEDED,
            STARTUP,
            SHUTDOWN,
            CONDITIONS,
            WATCHDOG_EXPIRED,
            INITIALIZATION_FAILED
        };

        /* @stubgen:omit */
        class EXTERNAL Job : public Core::IDispatch {
        protected:
             Job(IShell* shell, IShell::state toState, IShell::reason why)
                : _shell(shell)
                , _state(toState)
                , _reason(why)
            {
                if (shell != nullptr) {
                    shell->AddRef();
                }
            }

       public:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

            ~Job() override
            {
                if (_shell != nullptr) {
                    _shell->Release();
                }
            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(IShell* shell, IShell::state toState, IShell::reason why);

            //
            // Core::IDispatch implementation
            // -------------------------------------------------------------------------------
            void Dispatch() override
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
            IShell* const _shell;
            const state _state;
            const reason _reason;
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

        //! Versions: Returns a JSON Array of versions (JSONRPC interfaces) supported by this plugin.
        virtual string Versions() const = 0;

        //! Callsign: Instantiation name of this specific plugin. It is the name given in the config for the classname.
        virtual string Callsign() const = 0;

        //! PersistentPath: <config:persistentpath>/<plugin:callsign>/
        virtual string PersistentPath() const = 0;

        //! VolatilePath: <config:volatilepath>/<plugin:callsign>/
        virtual string VolatilePath() const = 0;

        //! DataPath: <config:datapath>/<plugin:classname>/
        virtual string DataPath() const = 0;

        //! ProxyStubPath: <config:proxystubpath>/
        virtual string ProxyStubPath() const = 0;

        //! SystemPath: <config:systempath>/
        virtual string SystemPath() const = 0;

        //! SystemPath: <config:apppath>/Plugins/
        virtual string PluginPath() const = 0;

        //! SystemPath: <config:systemrootpath>/
        virtual string SystemRootPath() const = 0;

        //! SystemRootPath: Set <config:systemrootpath>/
        virtual Core::hresult SystemRootPath(const string& systemRootPath) = 0;

        //! Startup: <config:startup>/
        virtual PluginHost::IShell::startup Startup() const = 0;

        //! Startup: Set<startup,autostart,resumed states>/
        virtual Core::hresult Startup(const startup value) = 0;

        //! Substituted Config value
        virtual string Substitute(const string& input) const = 0;

        virtual bool Resumed() const = 0;
        virtual Core::hresult Resumed(const bool value) = 0;

        virtual string HashKey() const = 0;
        
        virtual string ConfigLine() const = 0;
        virtual Core::hresult ConfigLine(const string& config) = 0;
        virtual Core::hresult Metadata(string& info /* @out */) const = 0;

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
        virtual void* /* @interface:id */ QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;

        // Methods to Activate/Deactivate and Unavailable the aggregated Plugin to this shell.
        // NOTE: These are Blocking calls!!!!!
        virtual Core::hresult Activate(const reason) = 0;
        virtual Core::hresult Deactivate(const reason) = 0;
        virtual Core::hresult Unavailable(const reason) = 0;
        virtual Core::hresult Hibernate(const uint32_t timeout) = 0;
        virtual reason Reason() const = 0;

        // Method to access, in the main process space, the channel factory to submit JSON objects to be send.
        // This method will return a error if it is NOT in the main process.
        /* @stubgen:stub */
        virtual uint32_t Submit(const uint32_t Id, const Core::ProxyType<Core::JSON::IElement>& response) = 0;

        // Method to access, in the main space, a COM factory to instantiate objects out-of-process.
        // This method will return a nullptr if it is NOT in the main process.
        /* @stubgen:stub */
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
        inline void Unregister(const RPC::IRemoteConnection::INotification* sink)
        {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                handler->Unregister(sink);
            }
        }
        inline void Register(ICOMLink::INotification* sink)
        {
            ICOMLink* handler(COMLink());

            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                handler->Register(sink);
            }
        }
        inline void Unregister(ICOMLink::INotification* sink)
        {
            ICOMLink* handler(COMLink());

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
        inline uint32_t EnablePersistentStorage(uint32_t permission = 0, const string& user = {}, const string& group = {})
        {
            return (EnableStoragePath(PersistentPath(), permission, user, group));
        }
        inline uint32_t EnableVolatileStorage(uint32_t permission = 0, const string& user = {}, const string& group = {})
        {
            return (EnableStoragePath(VolatilePath(), permission, user, group));
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

    private:
        inline uint32_t EnableStoragePath(const string& storagePath, uint32_t permission, const string& user, const string& group)
        {
            uint32_t result = Core::ERROR_NONE;

            // Make sure there is a path to the persitent infmration
            Core::File path(storagePath);

            if (path.IsDirectory() == false) {
                if (Core::Directory(storagePath.c_str()).Create() != true) {
                    result = Core::ERROR_BAD_REQUEST;
                }
            }
            if (result == Core::ERROR_NONE) {
                if (permission) {
                    path.Permission(permission);
                }
                if (user.empty() != true) {
                    path.User(user);
                }
                if (group.empty() != true) {
                    path.Group(group);
                }
            }

            return (result);
        }

        void* Root(uint32_t& pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version = ~0);

        /* @stubgen:omit */
        virtual std::vector<string> GetLibrarySearchPaths(const string&) const
        {
            return std::vector<string> {};
        }
    };

} // namespace PluginHost
}

#endif //__ISHELL_H__

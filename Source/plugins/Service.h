/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#ifndef __WEBBRIDGESUPPORT_SERVICE__
#define __WEBBRIDGESUPPORT_SERVICE__

#include "Module.h"
#include "Channel.h"
#include "Configuration.h"
#include "MetaData.h"
#include "System.h"
#include "IPlugin.h"
#include "IShell.h"

namespace WPEFramework {
namespace PluginHost {

    class EXTERNAL Service : public IShell {
        // This object is created by the instance that instantiates the plugins. As the lifetime
        // of this object is controlled by the server, instantiating this object, do not allow
        // this obnject to be copied or created by any other instance.
    private:
        Service() = delete;
        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        class EXTERNAL Config {
        private:
            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config(const Plugin::Config& plugin, const string& webPrefix, const string& persistentPath, const string& dataPath, const string& volatilePath)
            {
                const string& callSign(plugin.Callsign.Value());

                _webPrefix = webPrefix + '/' + callSign;
                _persistentPath = plugin.PersistentPath(persistentPath);
                _dataPath = plugin.DataPath(dataPath);
                _volatilePath = plugin.VolatilePath(volatilePath);

                // Volatile means that the path could not have been created, create it for now.
                Core::Directory(_volatilePath.c_str()).CreatePath();

                Update(plugin);
            }
            ~Config()
            {
            }

        public:
            inline bool IsSupported(const uint8_t number) const
            {
                return (std::find(_versions.begin(), _versions.end(), number) != _versions.end());
            }
            inline void Configuration(const string& value)
            {
                _config.Configuration = value;
            }
            inline void AutoStart(const bool value)
            {
                _config.AutoStart = value;
            }
            inline const Plugin::Config& Configuration() const
            {
                return (_config);
            }
            // WebPrefix is the Fully qualified name, indicating the endpoint for this plugin.
            inline const string& WebPrefix() const
            {
                return (_webPrefix);
            }

            // PersistentPath is a path to a location where the plugin instance can store data needed
            // by the plugin instance, hence why the callSign is included. .
            // This path is build up from: PersistentPath / callSign /
            inline const string& PersistentPath() const
            {
                return (_persistentPath);
            }

            // VolatilePath is a path to a location where the plugin instance can store data needed
            // by the plugin instance, hence why the callSign is included. .
            // This path is build up from: VolatilePath / callSign /
            inline const string& VolatilePath() const
            {
                return (_volatilePath);
            }

            // DataPath is a path, to a location (read-only to be used to store
            // This path is build up from: DataPath / callSign /
            inline const string& DataPath() const
            {
                return (_dataPath);
            }

            inline void Update(const Plugin::Config& config)
            {
                _config = config;

                _versions.clear();

                // Extract the version list from the config
                Core::JSON::ArrayType<Core::JSON::DecUInt8> versions;
                Core::OptionalType<Core::JSON::Error> error;
                versions.FromString(config.Versions.Value(), error);
                if (error.IsSet() == true) {
                    SYSLOG(Logging::ParsingError, (_T("Parsing failed with %s"), ErrorDisplayMessage(error.Value()).c_str()));
                }
                Core::JSON::ArrayType<Core::JSON::DecUInt8>::Iterator index(versions.Elements());

                while (index.Next() == true) {
                    _versions.push_back(index.Current().Value());
                }

                // If no versions are give, lets assume this is version 1, and we support it :-)
                if (_versions.empty() == true) {
                    _versions.push_back(1);
                }
            }

        private:
            Plugin::Config _config;

            string _webPrefix;
            string _persistentPath;
            string _volatilePath;
            string _dataPath;
            string _accessor;
            std::list<uint8_t> _versions;
        };

    public:
        Service(const Plugin::Config& plugin, const string& webPrefix, const string& persistentPath, const string& dataPath, const string& volatilePath)
            : _adminLock()
#ifdef RUNTIME_STATISTICS
            , _processedRequests(0)
            , _processedObjects(0)
#endif
            , _state(DEACTIVATED)
            , _config(plugin, webPrefix, persistentPath, dataPath, volatilePath)
#ifdef RESTFULL_API
            , _notifiers()
#endif
        {
        }
        ~Service()
        {
        }

    public:
        bool IsWebServerRequest(const string& segment) const;

#ifdef RESTFULL_API
        void Notification(const string& message);
#endif
        virtual string Versions() const
        {
            return (_config.Configuration().Versions.Value());
        }
        virtual string Locator() const
        {
            return (_config.Configuration().Locator.Value());
        }
        virtual string ClassName() const
        {
            return (_config.Configuration().ClassName.Value());
        }
        virtual string Callsign() const
        {
            return (_config.Configuration().Callsign.Value());
        }
        virtual string WebPrefix() const
        {
            return (_config.WebPrefix());
        }
        virtual string ConfigLine() const
        {
            return (_config.Configuration().Configuration.Value());
        }
        virtual string PersistentPath() const
        {
            return (_config.PersistentPath());
        }
        virtual string VolatilePath() const
        {
            return (_config.VolatilePath());
        }
        virtual string DataPath() const
        {
            return (_config.DataPath());
        }
        virtual state State() const
        {
            return (_state);
        }
        virtual bool AutoStart() const
        {
            return (_config.Configuration().AutoStart.Value());
        }
        virtual bool Resumed() const
        {
            return (_config.Configuration().Resumed.Value());
        }
        virtual bool IsSupported(const uint8_t number) const
        {
            return (_config.IsSupported(number));
        }
        inline const Plugin::Config& Configuration() const
        {
            return (_config.Configuration());
        }
        inline bool IsActive() const
        {
            return (_state == ACTIVATED);
        }
        inline bool HasError() const
        {
            return (_errorMessage.empty() == false);
        }
        inline const string& ErrorMessage() const
        {
            return (_errorMessage);
        }
        inline void GetMetaData(MetaData::Service& metaData) const
        {
            metaData = _config.Configuration();
#ifdef RESTFULL_API
            metaData.Observers = static_cast<uint32_t>(_notifiers.size());
#endif
            // When we do this, we need to make sure that the Service does not change state, otherwise it might
            // be that the the plugin is deinitializing and the IStateControl becomes invalid during our run.
            // Now we can first check if
            Lock();

            metaData.JSONState = this;

            Unlock();

#ifdef RUNTIME_STATISTICS
            metaData.ProcessedRequests = _processedRequests;
            metaData.ProcessedObjects = _processedObjects;
#endif
        }

        // As a service, the plugin could act like a WebService. The Webservice hosts files from a location over the
        // HTTP protocol. This service is hosting files at:
        // http://<bridge host ip>:<bridge port>/Service/<Callsign>/<PostFixURL>/....
        // Root directory of the files to be services by the URL are in case the passed in value is empty:
        // <DataPath>/<PostFixYRL>
        virtual void EnableWebServer(const string& postFixURL, const string& fileRootPath)
        {
            // The postFixURL should *NOT* contain a starting or trailing slash!!!
            ASSERT((postFixURL.length() > 0) && (postFixURL[0] != '/') && (postFixURL[postFixURL.length() - 1] != '/'));

            // The postFixURL should *NOT* contain a starting or trailing slash!!!
            // We signal the request to service web files via a non-empty _webServerFilePath.
            _webURLPath = postFixURL;

            if (fileRootPath.empty() == true) {
                _webServerFilePath = _config.DataPath() + postFixURL + '/';
            } else {
                // File path needs to end in a slash to indicate it is a directory and not a file.
                ASSERT(fileRootPath[fileRootPath.length() - 1] == '/');

                _webServerFilePath = fileRootPath;
            }
        }
        virtual void DisableWebServer()
        {
            // We signal the request to ervice web files via a non-empty _webServerFilePath.
            _webServerFilePath.clear();
            _webURLPath.clear();
        }
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) = 0;

        virtual void Notify(const string& message) = 0;
        virtual void* QueryInterface(const uint32_t id) = 0;
        virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;
        virtual void Register(IPlugin::INotification* sink) = 0;
        virtual void Unregister(IPlugin::INotification* sink) = 0;

        // Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
        virtual ICOMLink* COMLink() = 0;

        // Methods to Activate and Deactivate the aggregated Plugin to this shell.
        // These are Blocking calls!!!!!
        virtual uint32_t Activate(const PluginHost::IShell::reason) = 0;
        virtual uint32_t Deactivate(const PluginHost::IShell::reason) = 0;

        inline uint32_t ConfigLine(const string& newConfiguration)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            Lock();

            if (State() == PluginHost::IShell::DEACTIVATED) {

                // Time to update the config line...
                _config.Configuration(newConfiguration);

                result = Core::ERROR_NONE;
            }

            Unlock();

            return (result);
        }
        inline uint32_t AutoStart(const bool autoStart)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            Lock();

            if (State() == PluginHost::IShell::DEACTIVATED) {

                // Time to update the config line...
                _config.AutoStart(autoStart);

                result = Core::ERROR_NONE;
            }

            Unlock();

            return (result);
        }

    protected:
        inline void Lock() const
        {
            _adminLock.Lock();
        }
        inline void Unlock() const
        {
            _adminLock.Unlock();
        }
        inline void State(const state value)
        {
            _state = value;
        }
        inline void ErrorMessage(const string& message)
        {
            _errorMessage = message;
        }
#ifdef RESTFULL_API
        inline bool Subscribe(Channel& channel)
        {
            _notifierLock.Lock();

            bool result = std::find(_notifiers.begin(), _notifiers.end(), &channel) == _notifiers.end();

            if (result == true) {
                if (channel.IsNotified() == true) {
                    _notifiers.push_back(&channel);
                }
            }

            _notifierLock.Unlock();

            return (result);
        }
        inline void Unsubscribe(Channel& channel)
        {

            _notifierLock.Lock();

            std::list<Channel*>::iterator index(std::find(_notifiers.begin(), _notifiers.end(), &channel));

            if (index != _notifiers.end()) {
                _notifiers.erase(index);
            }

            _notifierLock.Unlock();
        }
#endif
#ifdef RUNTIME_STATISTICS
        inline void IncrementProcessedRequests()
        {
            _processedRequests++;
        }
        inline void IncrementProcessedObjects()
        {
            _processedObjects++;
        }
#endif
        void FileToServe(const string& webServiceRequest, Web::Response& response);

    private:
        mutable Core::CriticalSection _adminLock;
#ifdef RESTFULL_API
        Core::CriticalSection _notifierLock;
#endif

#ifdef RUNTIME_STATISTICS
        uint32_t _processedRequests;
        uint32_t _processedObjects;
#endif

        state _state;
        Config _config;
        string _errorMessage;

        // In case the Service also has WebPage availability, these variables
        // contain the URL path and the start of the path location on disk.
        string _webURLPath;
        string _webServerFilePath;

#ifdef RESTFULL_API
        // Keep track of people who want to be notified of changes.
        std::list<Channel*> _notifiers;
#endif
    };
}
}

#endif // __WEBBRIDGESUPPORT_SERVICE__

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

#pragma once

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
    private:
        class EXTERNAL Config {
        public:
            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

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
            inline void Startup(const PluginHost::IShell::startup value)
            {
                _config.Startup = value;
            }
            inline void AutoStart(const bool value)
            {
                _config.AutoStart = value;
            }
            inline void Resumed(const bool value)
            {
                _config.Resumed = value;
            }
            inline void SystemRootPath(const string& value)
            {
                _config.SystemRootPath = value;
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

                _config.Startup = ((_config.AutoStart.Value() == true) ?
                                   PluginHost::IShell::startup::ACTIVATED :
                                   PluginHost::IShell::startup::DEACTIVATED);
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
        // This object is created by the instance that instantiates the plugins. As the lifetime
        // of this object is controlled by the server, instantiating this object, do not allow
        // this obnject to be copied or created by any other instance.
        Service() = delete;
        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        Service(const Plugin::Config& plugin, const string& webPrefix, const string& persistentPath, const string& dataPath, const string& volatilePath)
            : _adminLock()
            #if THUNDER_RUNTIME_STATISTICS
            , _processedRequests(0)
            , _processedObjects(0)
            #endif
            , _state(DEACTIVATED)
            , _config(plugin, webPrefix, persistentPath, dataPath, volatilePath)
            #if THUNDER_RESTFULL_API
            , _notifiers()
            #endif
        {
            if ( (plugin.Startup.IsSet() == true) && (plugin.Startup.Value() == PluginHost::IShell::startup::UNAVAILABLE) ) {
                _state = UNAVAILABLE;
            }
        }
        ~Service() override = default;

    public:
        string Versions() const override
        {
            return (_config.Configuration().Versions.Value());
        }
        string Locator() const override
        {
            return (_config.Configuration().Locator.Value());
        }
        string ClassName() const override
        {
            return (_config.Configuration().ClassName.Value());
        }
        string Callsign() const override
        {
            return (_config.Configuration().Callsign.Value());
        }
        string WebPrefix() const override
        {
            return (_config.WebPrefix());
        }
        string ConfigLine() const override
        {
            Core::SafeSyncType<Core::CriticalSection> sync(_adminLock);
            return (_config.Configuration().Configuration.Value());
        }
        Core::hresult ConfigLine(const string& newConfiguration) override
        {
            Core::hresult result = Core::ERROR_ILLEGAL_STATE;

            Lock();

            if (State() == PluginHost::IShell::DEACTIVATED || 
                State() == PluginHost::IShell::DEACTIVATION ||
                State() == PluginHost::IShell::PRECONDITION ||
                State() == PluginHost::IShell::UNAVAILABLE ) {

                // Time to update the config line...
                _config.Configuration(newConfiguration);

                result = Core::ERROR_NONE;
            }

            Unlock();

            return (result);
        }
        string PersistentPath() const override
        {
            return (_config.PersistentPath());
        }
        string VolatilePath() const override
        {
            return (_config.VolatilePath());
        }
        string DataPath() const override
        {
            return (_config.DataPath());
        }
        string SystemRootPath() const override
        {
            return (_config.Configuration().SystemRootPath.Value());
        }
        Core::hresult SystemRootPath(const string& systemRootPath) override
        {
            _config.SystemRootPath(systemRootPath);
            return (Core::ERROR_NONE);
        }
        state State() const override
        {
            return (_state);
        }
        bool Resumed() const override
        {
            return ((_config.Configuration().Resumed.IsSet() ? _config.Configuration().Resumed.Value() : (_config.Configuration().Startup.Value() == PluginHost::IShell::startup::ACTIVATED)));
        }
        Core::hresult Resumed(const bool resumed) override
        {
            _config.Resumed(resumed);
            return (Core::ERROR_NONE);
        }
        PluginHost::IShell::startup Startup() const override
        {
            return _config.Configuration().Startup.Value();
        }
        Core::hresult Startup(const PluginHost::IShell::startup value) override
        {
            _config.Startup(value);
            _config.AutoStart(value == PluginHost::IShell::startup::ACTIVATED);

            return (Core::ERROR_NONE);
        }
        bool IsSupported(const uint8_t number) const override
        {
            return (_config.IsSupported(number));
        }

        // As a service, the plugin could act like a WebService. The Webservice hosts files from a location over the
        // HTTP protocol. This service is hosting files at:
        // http://<bridge host ip>:<bridge port>/Service/<Callsign>/<PostFixURL>/....
        // Root directory of the files to be services by the URL are in case the passed in value is empty:
        // <DataPath>/<PostFixYRL>
        void EnableWebServer(const string& postFixURL, const string& fileRootPath) override
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
        void DisableWebServer() override
        {
            // We signal the request to ervice web files via a non-empty _webServerFilePath.
            _webServerFilePath.clear();
            _webURLPath.clear();
        }

        inline const Plugin::Config& Configuration() const
        {
            return (_config.Configuration());
        }
        uint32_t StartupOrder() const
        {
            return (_config.Configuration().StartupOrder.Value());
        }
        inline bool IsActive() const
        {
            return (_state == ACTIVATED);
        }
        inline bool IsHibernated() const
        {
            return (_state == HIBERNATED);
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
            #if THUNDER_RESTFULL_API
            metaData.Observers = static_cast<uint32_t>(_notifiers.size());
            #endif

            // When we do this, we need to make sure that the Service does not change state, otherwise it might
            // be that the the plugin is deinitializing and the IStateControl becomes invalid during our run.
            // Now we can first check if
            Lock();

            metaData.JSONState = this;

            Unlock();

            #if THUNDER_RUNTIME_STATISTICS
            metaData.ProcessedRequests = _processedRequests;
            metaData.ProcessedObjects = _processedObjects;
            #endif
        }

        bool IsWebServerRequest(const string& segment) const;

        #if THUNDER_RESTFULL_API
        void Notification(const string& message);
        #endif
 
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) = 0;

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
        #if THUNDER_RESTFULL_API
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
        #if THUNDER_RUNTIME_STATISTICS
        inline void IncrementProcessedRequests()
        {
            _processedRequests++;
        }
        inline void IncrementProcessedObjects()
        {
            _processedObjects++;
        }
        #endif

        void FileToServe(const string& webServiceRequest, Web::Response& response, bool allowUnsafePath);

    private:
        mutable Core::CriticalSection _adminLock;

        #if THUNDER_RESTFULL_API
        Core::CriticalSection _notifierLock;
        #endif

        #if THUNDER_RUNTIME_STATISTICS
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

        #if THUNDER_RESTFULL_API
        // Keep track of people who want to be notified of changes.
        std::list<Channel*> _notifiers;
        #endif
    };
}
}

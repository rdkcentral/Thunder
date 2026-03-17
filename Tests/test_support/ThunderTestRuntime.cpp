/*
 * Copyright 2024 RDK Management
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

#include "ThunderTestRuntime.h"

#include <PluginServer.h>
#include <core/core.h>
#include <fstream>
#include <sstream>

// ==========================================================================
// ThunderTestRuntime implementation
//
// Lifecycle: Initialize() -> [run tests] -> Shutdown()
//
// Initialize creates a unique /tmp/thunder_test_XXXXXX/ directory tree,
// writes a minimal Thunder config.json, parses it into PluginHost::Config,
// constructs PluginHost::Server, and calls Server::Open() which boots
// the controller and activates auto-start plugins.
//
// Shutdown reverses the process: Server::Close(), cleanup temp files.
// ==========================================================================

namespace Thunder {
namespace TestCore {

    ThunderTestRuntime::~ThunderTestRuntime()
    {
        Shutdown();
    }

    void ThunderTestRuntime::CreateDirectories() const
    {
        Core::Directory(_tempDir.c_str()).Create();
        Core::Directory((_tempDir + "persistent/").c_str()).Create();
        Core::Directory((_tempDir + "volatile/").c_str()).Create();
        Core::Directory((_tempDir + "data/").c_str()).Create();
    }

    void ThunderTestRuntime::CleanupDirectories() const
    {
        if (!_tempDir.empty()) {
            Core::Directory(_tempDir.c_str()).Destroy();
        }
    }

    // Build a minimal Thunder JSON config from the plugin list.
    // Uses port 0 (OS-assigned) and binds to localhost only.
    string ThunderTestRuntime::BuildConfigJSON(const std::vector<PluginConfig>& plugins,
                                                const string& systemPath,
                                                const string& proxyStubPath) const
    {
        std::ostringstream json;
        json << "{"
             << "\"port\":0,"
             << "\"binding\":\"127.0.0.1\","
             << "\"idletime\":180,"
             << "\"persistentpath\":\"" << _tempDir << "persistent/\","
             << "\"volatilepath\":\"" << _tempDir << "volatile/\","
             << "\"datapath\":\"" << _tempDir << "data/\","
             << "\"systempath\":\"" << systemPath << "\","
             << "\"proxystubpath\":\"" << proxyStubPath << "\","
             << "\"communicator\":\"" << _tempDir << "communicator\","
             << "\"plugins\":[";

        for (size_t i = 0; i < plugins.size(); ++i) {
            const auto& p = plugins[i];
            if (i > 0) json << ",";
            json << "{"
                 << "\"callsign\":\"" << p.callsign << "\","
                 << "\"locator\":\"" << p.locator << "\","
                 << "\"classname\":\"" << p.classname << "\","
                 << "\"startuporder\":" << p.startuporder << ","
                 << "\"autostart\":" << (p.autostart ? "true" : "false");

            if (!p.configuration.empty()) {
                json << ",\"configuration\":" << p.configuration;
            }

            json << "}";
        }

        json << "]}";
        return json.str();
    }

    uint32_t ThunderTestRuntime::Initialize(const std::vector<PluginConfig>& plugins,
                                             const string& systemPath,
                                             const string& proxyStubPath)
    {
        if (_server != nullptr) {
            return Core::ERROR_ALREADY_CONNECTED;
        }

        // Create unique temp directory for this test run
        char tempTemplate[] = "/tmp/thunder_test_XXXXXX";
        char* tempResult = mkdtemp(tempTemplate);
        if (tempResult == nullptr) {
            return Core::ERROR_GENERAL;
        }
        _tempDir = string(tempResult) + "/";

        CreateDirectories();

        // Determine system path for plugin .so files
        string sysPath = systemPath;
        if (sysPath.empty()) {
            sysPath = "/usr/lib/wpeframework/plugins/";
        }
        if (sysPath.back() != '/') {
            sysPath += '/';
        }

        // Determine proxy stub path
        _proxyStubPath = proxyStubPath;
        if (_proxyStubPath.empty()) {
            // Default: look next to system path
            _proxyStubPath = sysPath + "../proxystubs/";
        }
        if (_proxyStubPath.back() != '/') {
            _proxyStubPath += '/';
        }

        // Build and write config to temp file
        string configJSON = BuildConfigJSON(plugins, sysPath, _proxyStubPath);
        _configFilePath = _tempDir + "config.json";

        {
            std::ofstream configFile(_configFilePath);
            if (!configFile.is_open()) {
                CleanupDirectories();
                return Core::ERROR_OPENING_FAILED;
            }
            configFile << configJSON;
        }

        // Parse config
        Core::File configFile(_configFilePath);
        if (configFile.Open(true) == false) {
            CleanupDirectories();
            return Core::ERROR_OPENING_FAILED;
        }

        Core::OptionalType<Core::JSON::Error> error;
        _config = new PluginHost::Config(configFile, false, error);
        configFile.Close();

        if (error.IsSet()) {
            delete _config;
            _config = nullptr;
            CleanupDirectories();
            return Core::ERROR_INCOMPLETE_CONFIG;
        }

        // Create and start the server
        _server = new PluginHost::Server(*_config, false);
        _server->Open();

        return Core::ERROR_NONE;
    }

    // Invoke a JSON-RPC method synchronously via the in-process dispatcher.
    // Bypasses HTTP/WebSocket — calls IDispatcher::Invoke() directly.
    uint32_t ThunderTestRuntime::InvokeJSONRPC(const string& callsign, const string& method,
                                                const string& params, string& response)
    {
        if (_server == nullptr) {
            return Core::ERROR_ILLEGAL_STATE;
        }

        Core::ProxyType<PluginHost::IShell> shell;
        uint32_t result = _server->Services().FromIdentifier(callsign, shell);
        if (result != Core::ERROR_NONE) {
            return result;
        }

        PluginHost::IDispatcher* dispatcher = shell->QueryInterface<PluginHost::IDispatcher>();
        if (dispatcher == nullptr) {
            return Core::ERROR_UNAVAILABLE;
        }

        result = dispatcher->Invoke(0, 0, string(), method, params, response);
        dispatcher->Release();

        return result;
    }

    Core::ProxyType<PluginHost::IShell> ThunderTestRuntime::GetShell(const string& callsign)
    {
        Core::ProxyType<PluginHost::IShell> shell;
        if (_server != nullptr) {
            _server->Services().FromIdentifier(callsign, shell);
        }
        return shell;
    }

    PluginHost::Server& ThunderTestRuntime::Server()
    {
        ASSERT(_server != nullptr);
        return *_server;
    }

    string ThunderTestRuntime::CommunicatorPath() const
    {
        if (_config != nullptr) {
            return _config->Communicator().HostName();
        }
        return string();
    }

    void ThunderTestRuntime::Shutdown()
    {
        if (_server != nullptr) {
            _server->Close();
            delete _server;
            _server = nullptr;
        }

        if (_config != nullptr) {
            delete _config;
            _config = nullptr;
        }

        if (!_configFilePath.empty()) {
            Core::File(_configFilePath).Destroy();
            _configFilePath.clear();
        }

        CleanupDirectories();
        _tempDir.clear();
    }

} // namespace TestCore
} // namespace Thunder

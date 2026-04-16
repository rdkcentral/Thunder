#include "ThunderTestRuntime.h"

#include <PluginServer.h>
#include <core/core.h>
#include <fstream>
#include <plugins/Configuration.h>

// ==========================================================================
// ThunderTestRuntime implementation
//
// Lifecycle: Initialize() -> [run tests] -> Deinitialize()
//
// Initialize creates a unique temp directory tree under the platform temp root,
// writes a minimal Thunder config.json, parses it into PluginHost::Config,
// constructs PluginHost::Server, and calls Server::Open() which boots
// the controller and activates configured plugins.
//
// Deinitialize reverses the process: Server::Close(), cleanup temp files.
// ==========================================================================

namespace Thunder {
namespace TestCore {

    ThunderTestRuntime::~ThunderTestRuntime()
    {
        Deinitialize();
    }

    bool ThunderTestRuntime::CreateDirectories() const
    {
        const string persistentPath = _tempDir + "persistent/";
        const string volatilePath = _tempDir + "volatile/";
        const string dataPath = _tempDir + "data/";

        const bool created =
            Core::Directory(persistentPath.c_str()).Create() &&
            Core::Directory(volatilePath.c_str()).Create() &&
            Core::Directory(dataPath.c_str()).Create();

        if (created == false) {
            TRACE_L1("ThunderTestRuntime: Failed to create temp directory tree at '%s'", _tempDir.c_str());
        }

        return created;
    }

    void ThunderTestRuntime::CleanupDirectories() const
    {
        if (!_tempDir.empty()) {
            Core::Directory(_tempDir.c_str()).Destroy();

            // Core::Directory::Destroy() does not remove the directory if the path
            // ends with a trailing separator. Normalize the path before calling it.
            string path = _tempDir;
            while (!path.empty() && (path.back() == '/' || path.back() == '\\')) {
                path.pop_back();
            }
            if (!path.empty()) {
                Core::Directory(path.c_str()).Destroy();
            }
        }
    }

    static string TemporaryRootPath()
    {
        string path;
        static const char* variables[] = { "TMPDIR", "TEMP", "TMP" };

        for (const char* variable : variables) {
            if ((Core::SystemInfo::GetEnvironment(variable, path) == true) && (path.empty() == false)) {
                return Core::Directory::Normalize(path);
            }
        }

#ifdef __WINDOWS__
        return Core::Directory::Normalize("c:/temp");
#else
        return Core::Directory::Normalize("/tmp");
#endif
    }

    static bool CreateUniqueTemporaryDirectory(string& path)
    {
        const string root = TemporaryRootPath();

        if (root.empty() == true) {
            return false;
        }

        Core::Directory rootDirectory(root.c_str());
        if ((rootDirectory.Exists() == false) && (rootDirectory.CreatePath() == false)) {
            TRACE_L1("ThunderTestRuntime: Failed to create temporary root '%s'", root.c_str());
            return false;
        }

        const string processId = Core::NumberType<uint32_t>(static_cast<uint32_t>(Core::ProcessInfo().Id())).Text();
        const string ticks = Core::NumberType<uint64_t>(Core::Time::Now().Ticks()).Text();

        for (uint8_t attempt = 0; attempt < 32; ++attempt) {
            const string candidate = root + "thunder_test_" + processId + '_' + ticks + '_' + Core::NumberType<uint8_t>(attempt).Text();
            Core::Directory directory(candidate.c_str());

            if ((directory.Exists() == false) && (directory.Create() == true)) {
                path = Core::Directory::Normalize(candidate);
                return true;
            }
        }

        TRACE_L1("ThunderTestRuntime: Failed to create unique temporary directory under '%s'", root.c_str());
        return false;
    }

    string ThunderTestRuntime::BuildConfigJSON(const std::vector<PluginConfig>& plugins,
                                                const string& systemPath,
                                                const string& proxyStubPath) const
    {
        const string communicatorPath = _tempDir + "communicator|0777";

        JsonObject config;
        JsonArray pluginList;

        config["port"] = 0;
        config["binding"] = "127.0.0.1";
        config["idletime"] = 180;
        config["persistentpath"] = _tempDir + "persistent/";
        config["volatilepath"] = _tempDir + "volatile/";
        config["datapath"] = _tempDir + "data/";
        config["systempath"] = systemPath;
        config["proxystubpath"] = proxyStubPath;
        config["communicator"] = communicatorPath;

        for (const auto& plugin : plugins) {

            string serializedPluginConfig;
            JsonValue pluginValue;
            Core::OptionalType<Core::JSON::Error> error;

            plugin.ToString(serializedPluginConfig);

            if ((pluginValue.FromString(serializedPluginConfig, error) == false) || (pluginValue.IsValid() == false)) {
                TRACE_L1("ThunderTestRuntime: Failed to serialize configuration for plugin '%s'", plugin.Callsign.Value().c_str());
                return string();
            }

            pluginList.Add(pluginValue);
        }

        config["plugins"] = pluginList;

        string json;
        config.ToString(json);

        return json;
    }

    uint32_t ThunderTestRuntime::Initialize(const std::vector<PluginConfig>& plugins,
                                             const string& systemPath,
                                             const string& proxyStubPath)
    {
        if (_server != nullptr) {
            return Core::ERROR_ALREADY_CONNECTED;
        }

        // Create a unique temp directory for this test run using Thunder Core helpers.
        if (CreateUniqueTemporaryDirectory(_tempDir) == false) {
            return Core::ERROR_GENERAL;
        }

        if (CreateDirectories() == false) {
            CleanupDirectories();
            _tempDir.clear();
            return Core::ERROR_OPENING_FAILED;
        }

        // Determine system path for plugin .so files
        string sysPath = systemPath.empty()
            ? Core::Directory::Normalize(DEFAULT_SYSTEM_PATH)
            : Core::Directory::Normalize(systemPath);

        // Determine proxy stub path
        _proxyStubPath = proxyStubPath.empty()
            ? Core::Directory::Normalize(DEFAULT_PROXYSTUB_PATH)
            : Core::Directory::Normalize(proxyStubPath);

        // Build and write config to temp file
        string configJSON = BuildConfigJSON(plugins, sysPath, _proxyStubPath);
        if (configJSON.empty()) {
            CleanupDirectories();
            _tempDir.clear();
            return Core::ERROR_INCOMPLETE_CONFIG;
        }
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

        // Initialize the messaging subsystem (must happen before Server creation,
        // mirrors the MessagingInitialization() call in the real PluginHost main()).
        Messaging::MessageUnit::Settings::Config messagingConfig;
        uint32_t messagingResult = Messaging::MessageUnit::Instance().Open(
            _config->VolatilePath(), messagingConfig, false, Messaging::MessageUnit::flush::OFF);

        if (messagingResult != Core::ERROR_NONE) {
            TRACE_L1("ThunderTestRuntime: Failed to initialize messaging subsystem (0x%08X)", messagingResult);
        }

        // Create and start the server
        _server = new PluginHost::Server(*_config, false);
        _server->Open();

        return Core::ERROR_NONE;
    }

    // Invoke a JSON-RPC method synchronously via the in-process dispatcher.
    // Bypasses HTTP/WebSocket — calls IDispatcher::Invoke() directly.
    // Derives the callsign from the method string (text before the first '.').
    uint32_t ThunderTestRuntime::Invoke(const string& method,
                                         const string& params, string& response)
    {
        uint32_t result = Core::ERROR_ILLEGAL_STATE;

        if (_server != nullptr) {

            string callsign = Core::JSONRPC::Message::Callsign(method);
            string methodName = Core::JSONRPC::Message::Method(method);

            if (callsign.empty() == true) {
                result = Core::ERROR_INVALID_SIGNATURE;
            } else {

                Core::ProxyType<PluginHost::IShell> shell;
                result = _server->Services().FromIdentifier(callsign, shell);

                if (result == Core::ERROR_NONE) {

                    PluginHost::IDispatcher* dispatcher = shell->QueryInterface<PluginHost::IDispatcher>();

                    if (dispatcher == nullptr) {
                        result = Core::ERROR_UNAVAILABLE;
                    } else {

                        if (MethodExists(dispatcher, callsign, methodName) == false) {
                            result = Core::ERROR_UNKNOWN_KEY;
                        } else {
                            result = dispatcher->Invoke(0, 0, string(), method, params, response);
                        }

                        dispatcher->Release();
                    }
                }
            }
        }

        return result;
    }

    bool ThunderTestRuntime::MethodExists(PluginHost::IDispatcher* dispatcher,
                                          const string& callsign,
                                          const string& methodName) const
    {
        bool found = false;

        JsonObject existsParams;
        existsParams["method"] = methodName;

        string serializedParams;
        existsParams.ToString(serializedParams);

        string existsResponse;
        dispatcher->Invoke(0, 0, string(), callsign + ".exists", serializedParams, existsResponse);

        Core::JSON::Boolean available;
        available.FromString(existsResponse);
        found = available.Value();

        return found;
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

    void ThunderTestRuntime::Deinitialize()
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

        Messaging::MessageUnit::Instance().Close();

        if (!_configFilePath.empty()) {
            Core::File(_configFilePath).Destroy();
            _configFilePath.clear();
        }

        CleanupDirectories();
        _tempDir.clear();
    }

} // namespace TestCore
} // namespace Thunder

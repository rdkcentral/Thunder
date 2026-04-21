#include "Module.h"
#include "ThunderTestRuntime.h"

#include <PluginServer.h>
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

    // ==================================================================
    // JSONRPCLink implementation
    // ==================================================================

    ThunderTestRuntime::JSONRPCLink::JSONRPCLink(ThunderTestRuntime& runtime, const string& callsign)
        : _runtime(runtime)
        , _callsign(callsign)
        , _dispatcher(nullptr)
    {
        Core::ProxyType<PluginHost::IShell> shell = _runtime.GetShell(_callsign);
        if (shell.IsValid()) {
            _dispatcher = shell->QueryInterface<PluginHost::IDispatcher>();
        }
    }

    ThunderTestRuntime::JSONRPCLink::~JSONRPCLink()
    {
        // Unsubscribe from all active events
        if (_dispatcher != nullptr) {
            std::lock_guard<std::mutex> guard(_lock);
            for (const auto& entry : _handlers) {
                _dispatcher->Unsubscribe(this, entry.first, _callsign, string());
            }
            _handlers.clear();
            _dispatcher->Release();
        }
    }

    uint32_t ThunderTestRuntime::JSONRPCLink::Invoke(const string& method,
                                                      const string& params,
                                                      string& response)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_dispatcher != nullptr) {

            string fullMethod = _callsign + '.' + method;

            if (_runtime.MethodExists(_dispatcher, _callsign, method) == false) {
                result = Core::ERROR_UNKNOWN_KEY;
            } else {
                result = _dispatcher->Invoke(0, 0, string(), fullMethod, params, response);
            }
        }

        return result;
    }

    uint32_t ThunderTestRuntime::JSONRPCLink::Subscribe(const string& event, EventHandler handler)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_dispatcher != nullptr) {

            result = _dispatcher->Subscribe(this, event, _callsign, string());

            if (result == Core::ERROR_NONE) {
                std::lock_guard<std::mutex> guard(_lock);
                _handlers[event] = std::move(handler);
            }
        }

        return result;
    }

    uint32_t ThunderTestRuntime::JSONRPCLink::Unsubscribe(const string& event)
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        if (_dispatcher != nullptr) {

            result = _dispatcher->Unsubscribe(this, event, _callsign, string());

            std::lock_guard<std::mutex> guard(_lock);
            _handlers.erase(event);
        }

        return result;
    }

    Core::hresult ThunderTestRuntime::JSONRPCLink::Event(const string& event,
                                                          const string& /* designator */,
                                                          const string& /* index */,
                                                          const string& parameters)
    {
        std::lock_guard<std::mutex> guard(_lock);

        auto it = _handlers.find(event);
        if (it != _handlers.end()) {
            it->second(parameters);
        }

        return Core::ERROR_NONE;
    }

    // ==================================================================
    // ThunderTestRuntime implementation
    // ==================================================================

    ThunderTestRuntime::~ThunderTestRuntime()
    {
        fprintf(stderr, "[TestRuntime] ~ThunderTestRuntime() - begin\n"); fflush(stderr);
        Deinitialize();
        fprintf(stderr, "[TestRuntime] ~ThunderTestRuntime() - done\n"); fflush(stderr);
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
        if (_tempDir.empty() == false) {
            Core::Directory(_tempDir.c_str()).Destroy();

            // Core::Directory::Destroy() does not remove the directory if the path
            // ends with a trailing separator. Strip it before the final call.
            string path = _tempDir;
            while ((path.empty() == false) && (path.back() == '/' || path.back() == '\\')) {
                path.pop_back();
            }
            if (path.empty() == false) {
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

        _initialized = true;

        return Core::ERROR_NONE;
    }

    ThunderTestRuntime::JSONRPCLink* ThunderTestRuntime::CreateJSONRPCLink(const string& callsign)
    {
        return new class JSONRPCLink(*this, callsign);
    }

    // Invoke a JSON-RPC method synchronously via the in-process dispatcher.
    // Bypasses HTTP/WebSocket — calls IDispatcher::Invoke() directly.
    // Callsign and method are parsed using Core::JSONRPC::Message helpers.
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
        fprintf(stderr, "[TestRuntime] Deinitialize - begin\n"); fflush(stderr);

        if (_server != nullptr) {
            fprintf(stderr, "[TestRuntime] Server::Close() - begin\n"); fflush(stderr);
            _server->Close();
            fprintf(stderr, "[TestRuntime] Server::Close() - done\n"); fflush(stderr);
            delete _server;
            fprintf(stderr, "[TestRuntime] delete _server - done\n"); fflush(stderr);
            _server = nullptr;
        }

        if (_config != nullptr) {
            delete _config;
            fprintf(stderr, "[TestRuntime] delete _config - done\n"); fflush(stderr);
            _config = nullptr;
        }

        if (_initialized == true) {
            fprintf(stderr, "[TestRuntime] MessageUnit::Close() - begin\n"); fflush(stderr);
            Messaging::MessageUnit::Instance().Close();
            fprintf(stderr, "[TestRuntime] Singleton::Dispose() - begin\n"); fflush(stderr);
            Core::Singleton::Dispose();
            fprintf(stderr, "[TestRuntime] Singleton::Dispose() - done\n"); fflush(stderr);
            _initialized = false;
        }

        if (_configFilePath.empty() == false) {
            Core::File(_configFilePath).Destroy();
            _configFilePath.clear();
        }

        CleanupDirectories();
        _tempDir.clear();
        fprintf(stderr, "[TestRuntime] Deinitialize - done\n"); fflush(stderr);
    }

} // namespace TestCore
} // namespace Thunder

#include "ThunderTestRuntime.h"

#include <PluginServer.h>
#include <core/core.h>
#include <fstream>
#include <sstream>

// ==========================================================================
// ThunderTestRuntime implementation
//
// Lifecycle: Initialize() -> [run tests] -> Deinitialize()
//
// Initialize creates a unique /tmp/thunder_test_XXXXXX/ directory tree,
// writes a minimal Thunder config.json, parses it into PluginHost::Config,
// constructs PluginHost::Server, and calls Server::Open() which boots
// the controller and activates auto-start plugins.
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

    // Helper to escape a string for safe inclusion as a JSON string value.
    // It escapes quotes, backslashes, and control characters (< 0x20).
    static std::string JsonEscape(const std::string& input)
    {
        std::string output;
        output.reserve(input.size());
        const char* hex = "0123456789abcdef";

        for (unsigned char c : input) {
            switch (c) {
            case '"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            default:
                if (c < 0x20) {
                    output += "\\u00";
                    output += hex[(c >> 4) & 0x0F];
                    output += hex[c & 0x0F];
                } else {
                    output += static_cast<char>(c);
                }
                break;
            }
        }

        return output;
    }

    static const char* ToStartModeString(const ThunderTestRuntime::PluginConfig::StartMode startMode)
    {
        switch (startMode) {
        case ThunderTestRuntime::PluginConfig::StartMode::Activated:
            return "Activated";
        case ThunderTestRuntime::PluginConfig::StartMode::Deactivated:
            return "Deactivated";
        case ThunderTestRuntime::PluginConfig::StartMode::Unavailable:
            return "Unavailable";
        default:
            return "Activated";
        }
    }

    string ThunderTestRuntime::BuildConfigJSON(const std::vector<PluginConfig>& plugins,
                                                const string& systemPath,
                                                const string& proxyStubPath) const
    {
                const string communicatorPath = _tempDir + "communicator|0777";

        std::ostringstream json;
        json << "{"
             << "\"port\":0,"
             << "\"binding\":\"127.0.0.1\","
             << "\"idletime\":180,"
               << "\"persistentpath\":\"" << JsonEscape(_tempDir + "persistent/") << "\","
               << "\"volatilepath\":\"" << JsonEscape(_tempDir + "volatile/") << "\","
               << "\"datapath\":\"" << JsonEscape(_tempDir + "data/") << "\","
               << "\"systempath\":\"" << JsonEscape(systemPath) << "\","
               << "\"proxystubpath\":\"" << JsonEscape(proxyStubPath) << "\","
                             << "\"communicator\":\"" << JsonEscape(communicatorPath) << "\","
             << "\"plugins\":[";

        for (size_t i = 0; i < plugins.size(); ++i) {
            const auto& p = plugins[i];
            if (i > 0) json << ",";
            json << "{"
                  << "\"callsign\":\"" << JsonEscape(p.callsign) << "\","
                  << "\"locator\":\"" << JsonEscape(p.locator) << "\","
                  << "\"classname\":\"" << JsonEscape(p.classname) << "\","
                 << "\"startuporder\":" << p.startuporder << ","
                 << "\"startmode\":\"" << ToStartModeString(p.startmode) << "\"";

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

        if (CreateDirectories() == false) {
            CleanupDirectories();
            _tempDir.clear();
            return Core::ERROR_OPENING_FAILED;
        }

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
    // Derives the callsign from the method string (text before the first '.').
    uint32_t ThunderTestRuntime::InvokeJSONRPC(const string& method,
                                                const string& params, string& response)
    {
        if (_server == nullptr) {
            return Core::ERROR_ILLEGAL_STATE;
        }

        size_t dot = method.find('.');
        if (dot == string::npos) {
            return Core::ERROR_INVALID_SIGNATURE;
        }
        string callsign = method.substr(0, dot);

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

        if (!_configFilePath.empty()) {
            Core::File(_configFilePath).Destroy();
            _configFilePath.clear();
        }

        CleanupDirectories();
        _tempDir.clear();
    }

} // namespace TestCore
} // namespace Thunder

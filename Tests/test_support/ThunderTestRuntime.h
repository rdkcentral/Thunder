#pragma once

// ==========================================================================
// ThunderTestRuntime — Public API for in-process Thunder plugin testing
//
// Provides a lightweight wrapper around PluginHost::Server that:
//   - Creates an isolated temp directory per test run
//   - Generates a minimal Thunder config (JSON) on the fly
//   - Boots the embedded server and activates the controller
//   - Exposes helpers for JSON-RPC invocation and COM-RPC queries
//   - Tears everything down cleanly on Deinitialize()
//
// Typical usage in a GTest fixture:
//
//   static ThunderTestRuntime _runtime;
//
//   static void SetUpTestSuite() {
//       std::vector<PluginConfig> plugins = { ... };
//       _runtime.Initialize(plugins, pluginPath, proxyStubPath);
//   }
//   static void TearDownTestSuite() { _runtime.Deinitialize(); }
//
//   TEST_F(MyTest, JsonRpc) {
//       string resp;
//       _runtime.Invoke("Callsign.method", params, resp);
//   }
//
//   TEST_F(MyTest, ComRpc) {
//       auto* iface = _runtime.GetInterface<IMyInterface>("Callsign");
//   }
// ==========================================================================

#include <core/Portability.h>
#include <core/Proxy.h>
#include <plugins/IShell.h>
#include <plugins/Configuration.h>
#include <plugins/IDispatcher.h>
#include <string>
#include <vector>

namespace Thunder {

namespace PluginHost {
    class Server;
    class Config;
}

namespace TestCore {

    class ThunderTestRuntime {
    public:
        // Reuse the real Thunder plugin configuration type.
        // Key fields: Callsign, Locator, ClassName, StartupOrder, StartMode, Configuration.
        using PluginConfig = Plugin::Config;

        ThunderTestRuntime() = default;
        ~ThunderTestRuntime();

        ThunderTestRuntime(const ThunderTestRuntime&) = delete;
        ThunderTestRuntime& operator=(const ThunderTestRuntime&) = delete;

        // Boot the embedded Thunder server with the given plugins.
        // Creates temp directories, generates config, parses it, and calls Server::Open().
        // systemPath  — directory containing plugin .so files
        // proxyStubPath — directory containing proxy stub .so files
        // Returns Core::ERROR_NONE on success.
        uint32_t Initialize(const std::vector<PluginConfig>& plugins, const string& systemPath = "", const string& proxyStubPath = "");

        // Invoke a JSON-RPC method on a loaded plugin.
        // The callsign is derived from the method string (text before the first '.').
        // Method format: "Callsign.method" (e.g. "MyPlugin.doSomething")
        // Returns Core::ERROR_UNAVAILABLE if the JSON-RPC endpoint is not available.
        uint32_t Invoke(const string& method, const string& params, string& response);

        // Obtain a COM-RPC interface from a loaded plugin.
        // Caller must Release() the returned pointer when done.
        template <typename INTERFACE>
        INTERFACE* GetInterface(const string& callsign)
        {
            INTERFACE* result = nullptr;
            Core::ProxyType<PluginHost::IShell> shell = GetShell(callsign);
            if (shell.IsValid()) {
                result = shell->QueryInterface<INTERFACE>();
            }
            return result;
        }

        // Get the IShell proxy for a plugin (for activation/deactivation control).
        Core::ProxyType<PluginHost::IShell> GetShell(const string& callsign);

        // Direct access to the underlying PluginHost::Server.
        PluginHost::Server& Server();

        // Returns the UNIX domain socket path used by the communicator.
        string CommunicatorPath() const;

        // Stop the server, release config, and clean up temp directories.
        void Deinitialize();

    private:
        string BuildConfigJSON(const std::vector<PluginConfig>& plugins, const string& systemPath, const string& proxyStubPath) const;
        bool MethodExists(PluginHost::IDispatcher* dispatcher, const string& callsign, const string& methodName) const;
        bool CreateDirectories() const;
        void CleanupDirectories() const;

        PluginHost::Config* _config = nullptr;
        PluginHost::Server* _server = nullptr;
        string _tempDir;
        string _configFilePath;
        string _proxyStubPath;
    };

} // namespace TestCore
} // namespace Thunder

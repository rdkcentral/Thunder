#pragma once

// ==========================================================================
// ThunderTestRuntime — Public API for in-process Thunder plugin testing
//
// Provides a lightweight wrapper around PluginHost::Server that:
//   - Creates an isolated temp directory per test run
//   - Generates a minimal Thunder config (JSON) on the fly
//   - Boots the embedded server and activates the controller
//   - Exposes helpers for JSON-RPC invocation, event handling,
//     and COM-RPC queries
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
//       auto link = _runtime.JSONRPCLink("MyPlugin");
//       link.Invoke("someMethod", R"({"param":1})", resp);
//   }
//
//   TEST_F(MyTest, Events) {
//       auto link = _runtime.JSONRPCLink("MyPlugin");
//       link.Subscribe("onSomethingChanged",
//           [](const string& params) { /* handle event */ });
//       // ... trigger event ...
//       link.Unsubscribe("onSomethingChanged");
//   }
//
//   TEST_F(MyTest, ComRpc) {
//       auto* iface = _runtime.GetInterface<IMyInterface>("MyPlugin");
//   }
// ==========================================================================

#include <core/Portability.h>
#include <core/Proxy.h>
#include <plugins/IShell.h>
#include <plugins/IDispatcher.h>
#include <plugins/Configuration.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <mutex>

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

        // ------------------------------------------------------------------
        // JSONRPCLink — callsign-bound helper for JSON-RPC calls and events
        // ------------------------------------------------------------------
        class JSONRPCLink : public PluginHost::IDispatcher::ICallback {
        public:
            using EventHandler = std::function<void(const string& params)>;

            JSONRPCLink(ThunderTestRuntime& runtime, const string& callsign);
            ~JSONRPCLink() override;

            JSONRPCLink(const JSONRPCLink&) = delete;
            JSONRPCLink& operator=(const JSONRPCLink&) = delete;

            // JSON-RPC method invocation (callsign is implicit).
            // Method is the bare method name (e.g. "getLogLevel"), not "Callsign.getLogLevel".
            uint32_t Invoke(const string& method, const string& params, string& response);

            // Subscribe to a JSON-RPC event with a callback.
            uint32_t Subscribe(const string& event, EventHandler handler);

            // Unsubscribe from a previously subscribed event.
            uint32_t Unsubscribe(const string& event);

            const string& Callsign() const { return _callsign; }

            // IDispatcher::ICallback
            void AddRef() const override {}
            uint32_t Release() const override { return Core::ERROR_NONE; }

        private:
            // IDispatcher::ICallback
            Core::hresult Event(const string& event, const string& designator,
                                const string& index, const string& parameters) override;

            BEGIN_INTERFACE_MAP(JSONRPCLink)
                INTERFACE_ENTRY(PluginHost::IDispatcher::ICallback)
            END_INTERFACE_MAP

            ThunderTestRuntime& _runtime;
            string _callsign;
            PluginHost::IDispatcher* _dispatcher;

            mutable std::mutex _lock;
            std::unordered_map<string, EventHandler> _handlers;
        };

        ThunderTestRuntime() = default;
        ~ThunderTestRuntime();

        ThunderTestRuntime(const ThunderTestRuntime&) = delete;
        ThunderTestRuntime& operator=(const ThunderTestRuntime&) = delete;

        // Boot the embedded Thunder server with the given plugins.
        // Creates temp directories, generates config, parses it, and calls Server::Open().
        // systemPath  — directory containing plugin .so files
        // proxyStubPath — directory containing proxy stub .so files
        // Returns Core::ERROR_NONE on success.
        uint32_t Initialize(const std::vector<PluginConfig>& plugins,
                            const string& systemPath = "",
                            const string& proxyStubPath = "");

        // Create a callsign-bound JSON-RPC link for invoke and event operations.
        // Caller owns the returned object.
        JSONRPCLink* JSONRPCLink(const string& callsign);

        // Invoke a JSON-RPC method on a loaded plugin (full designator form).
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

        // Returns the domain socket path used by the communicator.
        string CommunicatorPath() const;

        // Stop the server, release config, and clean up temp directories.
        void Deinitialize();

    private:
        string BuildConfigJSON(const std::vector<PluginConfig>& plugins,
                               const string& systemPath,
                               const string& proxyStubPath) const;
        bool MethodExists(PluginHost::IDispatcher* dispatcher,
                          const string& callsign,
                          const string& methodName) const;
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

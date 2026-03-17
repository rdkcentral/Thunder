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

#pragma once

// ==========================================================================
// ThunderTestRuntime — Public API for in-process Thunder plugin testing
//
// Provides a lightweight wrapper around PluginHost::Server that:
//   - Creates an isolated temp directory per test run
//   - Generates a minimal Thunder config (JSON) on the fly
//   - Boots the embedded server and activates the controller
//   - Exposes helpers for JSON-RPC invocation and COM-RPC queries
//   - Tears everything down cleanly on Shutdown()
//
// Typical usage in a GTest fixture:
//
//   static ThunderTestRuntime _runtime;
//
//   static void SetUpTestSuite() {
//       std::vector<PluginConfig> plugins = { ... };
//       _runtime.Initialize(plugins, pluginPath, proxyStubPath);
//   }
//   static void TearDownTestSuite() { _runtime.Shutdown(); }
//
//   TEST_F(MyTest, JsonRpc) {
//       string resp;
//       _runtime.InvokeJSONRPC("Callsign", "Method", params, resp);
//   }
//
//   TEST_F(MyTest, ComRpc) {
//       auto* iface = _runtime.GetInterface<IMyInterface>("Callsign");
//   }
// ==========================================================================

#ifndef MODULE_NAME
#define MODULE_NAME Application
#endif

#include <plugins/plugins.h>
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
        // Describes a plugin to be loaded by the test runtime.
        // Maps directly to a Thunder plugin JSON config entry.
        struct PluginConfig {
            string callsign;        // Plugin callsign (e.g. "Counter")
            string locator;         // Shared library name (e.g. "libThunderCounterImplementation.so")
            string classname;       // Class name registered via SERVICE_REGISTRATION
            bool autostart = true;  // Whether to auto-activate on startup
            int startuporder = 50;  // Activation priority (lower = earlier)
            string configuration;   // Optional JSON object for plugin-specific config
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
        uint32_t Initialize(const std::vector<PluginConfig>& plugins, const string& systemPath = "", const string& proxyStubPath = "");

        // Invoke a JSON-RPC method on a loaded plugin.
        // Method format: "Callsign.Version.method" (e.g. "Counter.1.increment")
        uint32_t InvokeJSONRPC(const string& callsign, const string& method,
                               const string& params, string& response);

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
        void Shutdown();

    private:
        string BuildConfigJSON(const std::vector<PluginConfig>& plugins, const string& systemPath, const string& proxyStubPath) const;
        void CreateDirectories() const;
        void CleanupDirectories() const;

        PluginHost::Config* _config = nullptr;
        PluginHost::Server* _server = nullptr;
        string _tempDir;
        string _configFilePath;
        string _proxyStubPath;
    };

} // namespace TestCore
} // namespace Thunder

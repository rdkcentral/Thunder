#pragma once

#include <core/Portability.h>
#include <core/Proxy.h>
#include <plugins/IShell.h>
#include <plugins/IDispatcher.h>
#include <plugins/Configuration.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <atomic>
#include <mutex>

namespace Thunder {

namespace PluginHost {
    class Server;
    class Config;
}

namespace TestCore {

    class ThunderTestRuntime {
    public:
        using PluginConfig = Plugin::Config;

        class JSONRPCLink : public PluginHost::IDispatcher::ICallback {
        public:
            using EventHandler = std::function<void(const string& params)>;

            JSONRPCLink(ThunderTestRuntime& runtime, const string& callsign);
            ~JSONRPCLink() override;

            JSONRPCLink(const JSONRPCLink&) = delete;
            JSONRPCLink& operator=(const JSONRPCLink&) = delete;

            uint32_t Invoke(const string& method, const string& params, string& response);
            uint32_t Subscribe(const string& event, EventHandler handler);
            uint32_t Unsubscribe(const string& event);

            const string& Callsign() const { return _callsign; }

            uint32_t AddRef() const override
            {
                return ++_refCount;
            }
            uint32_t Release() const override
            {
                const uint32_t result = --_refCount;
                if (result == 0) {
                    delete const_cast<JSONRPCLink*>(this);
                }
                return result;
            }

        private:
            Core::hresult Event(const string& event, const string& designator,
                const string& index, const string& parameters) override;

            BEGIN_INTERFACE_MAP(JSONRPCLink)
                INTERFACE_ENTRY(PluginHost::IDispatcher::ICallback)
            END_INTERFACE_MAP

            ThunderTestRuntime& _runtime;
            string _callsign;
            PluginHost::IDispatcher* _dispatcher;
            mutable std::atomic<uint32_t> _refCount { 1 };

            mutable std::mutex _lock;
            std::unordered_map<string, EventHandler> _handlers;
        };

        ThunderTestRuntime() = default;
        ~ThunderTestRuntime();

        ThunderTestRuntime(const ThunderTestRuntime&) = delete;
        ThunderTestRuntime& operator=(const ThunderTestRuntime&) = delete;

        uint32_t Initialize(const std::vector<PluginConfig>& plugins,
            const string& systemPath = "",
            const string& proxyStubPath = "");

        class JSONRPCLink* CreateJSONRPCLink(const string& callsign);
        uint32_t Invoke(const string& method, const string& params, string& response);

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

        Core::ProxyType<PluginHost::IShell> GetShell(const string& callsign);
        PluginHost::Server& Server();
        string CommunicatorPath() const;
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
        bool _initialized = false;
        string _tempDir;
        string _configFilePath;
        string _proxyStubPath;
    };

} // namespace TestCore
} // namespace Thunder
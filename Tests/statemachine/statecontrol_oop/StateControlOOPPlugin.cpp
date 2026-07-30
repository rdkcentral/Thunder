#include "Module.h"

namespace Thunder {
namespace Plugin {

    // OOP variant: the shell spawns an out-of-process implementation that owns
    // IStateControl and aggregates it so the framework's
    // _handler->QueryInterface<IStateControl>() resolves across the process
    // boundary.
    //
    // CAVEAT (read before relying on this): the implementation runs in a child
    // process, so the test process CANNOT read its request log. Only State()
    // (marshalled) and the outbound StateControlStateChange notification are
    // observable in the parent. The in-process tests that assert on
    // g_stateControlRequests / g_stateControlSinkCount therefore do NOT transfer.
    // For the "framework never drives SUSPEND" question, in-process is the
    // working harness; this skeleton is here for the spawn/connection path only.
    //
    // BUILD ASSUMPTIONS to confirm against your tree:
    //  - IStateControl proxy-stubs are available on the proxyStubPath the runtime
    //    is initialized with.
    //  - HOSTING_COMPROCESS points at a usable child host (ThunderPlugin).
    //  - "StateControlImplementation" matches the SERVICE_REGISTRATION name below.
    class StateControlOOPPlugin : public PluginHost::IPlugin {
    public:
        StateControlOOPPlugin()
            : _connectionId(0)
            , _impl(nullptr)
        {
        }
        ~StateControlOOPPlugin() override = default;
        StateControlOOPPlugin(const StateControlOOPPlugin&) = delete;
        StateControlOOPPlugin& operator=(const StateControlOOPPlugin&) = delete;

        BEGIN_INTERFACE_MAP(StateControlOOPPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_AGGREGATE(PluginHost::IStateControl, _impl)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* shell) override
        {
            _impl = shell->Root<PluginHost::IStateControl>(_connectionId, Core::infinite, _T("StateControlImplementation"));
            return (_impl != nullptr) ? string() : _T("could not spawn StateControlImplementation");
        }
        void Deinitialize(PluginHost::IShell* shell) override
        {
            if (_impl != nullptr) {
                RPC::IRemoteConnection* connection = shell->RemoteConnection(_connectionId);
                _impl->Release();
                _impl = nullptr;
                if (connection != nullptr) {
                    connection->Terminate();
                    connection->Release();
                }
            }
        }
        string Information() const override { return string(); }

    private:
        uint32_t _connectionId;
        PluginHost::IStateControl* _impl;
    };

    SERVICE_REGISTRATION(StateControlOOPPlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

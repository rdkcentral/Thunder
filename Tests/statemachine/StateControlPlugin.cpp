#include "Module.h"
#include "StateControlControl.h"

#include <mutex>

namespace Thunder {

namespace TestSupport {
    std::vector<int> g_stateControlRequests;
    int g_stateControlSinkCount = 0;
    int g_stateControlNotifyCount = 0;
}

namespace Plugin {

    // In-process plugin that implements IStateControl so the Service's live
    // state-control wiring (AcquireInterfaces -> Register(&_composit) -> auto
    // RESUME if configured; Detach -> Unregister) can be characterized.
    //
    // It records every Request it receives and every sink registration, so the
    // test can prove what the framework does and, crucially, does NOT do (no
    // framework-driven SUSPEND).
    //
    // NOTE: the IStateControl signature below matches the canonical Thunder
    // interface. Sanity-check command/state enum members and Configure()'s
    // return type against your ThunderInterfaces version; older trees differ.
    class StateControlPlugin : public PluginHost::IPlugin, public PluginHost::IStateControl {
    public:
        StateControlPlugin()
            : _adminLock()
            , _state(PluginHost::IStateControl::SUSPENDED) // start SUSPENDED so an auto-resume is observable
            , _sinks()
        {
        }
        ~StateControlPlugin() override = default;
        StateControlPlugin(const StateControlPlugin&) = delete;
        StateControlPlugin& operator=(const StateControlPlugin&) = delete;

        BEGIN_INTERFACE_MAP(StateControlPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

        // IPlugin
        const string Initialize(PluginHost::IShell* /* shell */) override { return string(); }
        void Deinitialize(PluginHost::IShell* /* shell */) override {}
        string Information() const override { return string(); }

        // IStateControl
        uint32_t Configure(PluginHost::IShell* /* framework */) override
        {
            return Core::ERROR_NONE;
        }
        PluginHost::IStateControl::state State() const override
        {
            return _state;
        }
        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            std::lock_guard<std::mutex> guard(_adminLock);
            TestSupport::g_stateControlRequests.push_back(static_cast<int>(command));

            _state = (command == PluginHost::IStateControl::SUSPEND)
                ? PluginHost::IStateControl::SUSPENDED
                : PluginHost::IStateControl::RESUMED;

            // Always notify so the test sees that the framework's sink is wired
            // and the outbound StateChange path fires.
            for (auto* sink : _sinks) {
                TestSupport::g_stateControlNotifyCount++;
                sink->StateChange(_state);
            }
            return Core::ERROR_NONE;
        }
        void Register(PluginHost::IStateControl::INotification* notification) override
        {
            std::lock_guard<std::mutex> guard(_adminLock);
            notification->AddRef();
            _sinks.push_back(notification);
            TestSupport::g_stateControlSinkCount = static_cast<int>(_sinks.size());
        }
        void Unregister(PluginHost::IStateControl::INotification* notification) override
        {
            std::lock_guard<std::mutex> guard(_adminLock);
            for (auto it = _sinks.begin(); it != _sinks.end(); ++it) {
                if (*it == notification) {
                    (*it)->Release();
                    _sinks.erase(it);
                    break;
                }
            }
            TestSupport::g_stateControlSinkCount = static_cast<int>(_sinks.size());
        }

    private:
        mutable std::mutex _adminLock;
        PluginHost::IStateControl::state _state;
        std::vector<PluginHost::IStateControl::INotification*> _sinks;
    };

    SERVICE_REGISTRATION(StateControlPlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

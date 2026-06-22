#include "Module.h"
#include <mutex>
#include <vector>

namespace Thunder {
namespace Plugin {

    // Out-of-process implementation of IStateControl. Lives in the child process,
    // so its state is only observable from the parent via State() and via the
    // StateChange notifications it emits to the (proxied) registered sink.
    class StateControlImplementation : public PluginHost::IStateControl {
    public:
        StateControlImplementation()
            : _adminLock()
            , _state(PluginHost::IStateControl::SUSPENDED)
            , _sinks()
        {
        }
        ~StateControlImplementation() override = default;
        StateControlImplementation(const StateControlImplementation&) = delete;
        StateControlImplementation& operator=(const StateControlImplementation&) = delete;

        BEGIN_INTERFACE_MAP(StateControlImplementation)
            INTERFACE_ENTRY(PluginHost::IStateControl)
        END_INTERFACE_MAP

        uint32_t Configure(PluginHost::IShell* /* framework */) override { return Core::ERROR_NONE; }
        PluginHost::IStateControl::state State() const override { return _state; }

        uint32_t Request(const PluginHost::IStateControl::command command) override
        {
            std::lock_guard<std::mutex> guard(_adminLock);
            _state = (command == PluginHost::IStateControl::SUSPEND)
                ? PluginHost::IStateControl::SUSPENDED
                : PluginHost::IStateControl::RESUMED;
            for (auto* sink : _sinks) {
                sink->StateChange(_state);
            }
            return Core::ERROR_NONE;
        }
        void Register(PluginHost::IStateControl::INotification* notification) override
        {
            std::lock_guard<std::mutex> guard(_adminLock);
            notification->AddRef();
            _sinks.push_back(notification);
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
        }

    private:
        mutable std::mutex _adminLock;
        PluginHost::IStateControl::state _state;
        std::vector<PluginHost::IStateControl::INotification*> _sinks;
    };

    SERVICE_REGISTRATION(StateControlImplementation, 1, 0)

} // namespace Plugin
} // namespace Thunder

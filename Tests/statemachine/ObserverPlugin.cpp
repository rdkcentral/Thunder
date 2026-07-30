#include "Module.h"
#include "ObserverControl.h"

namespace Thunder {

namespace TestSupport {
    std::string g_observedCallsign;
    bool g_deactivatedFired = false;
    bool g_shellQiWorked = false;
    bool g_handlerQiResolved = false;
}

namespace Plugin {

    // Observes plugin lifecycle notifications. On the observed plugin's
    // Deactivated() it probes QueryInterface to pin the notification-ordering
    // contract: fired in DEACTIVATION state, with _handler still alive, so both
    // IShell and the handler interface resolve.
    class ObserverPlugin : public PluginHost::IPlugin, public PluginHost::IPlugin::INotification {
    public:
        ObserverPlugin() = default;
        ~ObserverPlugin() override = default;
        ObserverPlugin(const ObserverPlugin&) = delete;
        ObserverPlugin& operator=(const ObserverPlugin&) = delete;

        BEGIN_INTERFACE_MAP(ObserverPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IPlugin::INotification)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* shell) override
        {
            shell->Register(static_cast<PluginHost::IPlugin::INotification*>(this));
            return string();
        }
        void Deinitialize(PluginHost::IShell* shell) override
        {
            shell->Unregister(static_cast<PluginHost::IPlugin::INotification*>(this));
        }
        string Information() const override { return string(); }

        void Activated(const string&, PluginHost::IShell*) override {}
        void Unavailable(const string&, PluginHost::IShell*) override {}
        void Deactivated(const string& callsign, PluginHost::IShell* plugin) override
        {
            if (callsign != TestSupport::g_observedCallsign) {
                return;
            }
            TestSupport::g_deactivatedFired = true;

            PluginHost::IShell* asShell = plugin->QueryInterface<PluginHost::IShell>();
            TestSupport::g_shellQiWorked = (asShell != nullptr);
            if (asShell != nullptr) { asShell->Release(); }

            PluginHost::IPlugin* asPlugin = plugin->QueryInterface<PluginHost::IPlugin>();
            TestSupport::g_handlerQiResolved = (asPlugin != nullptr);
            if (asPlugin != nullptr) { asPlugin->Release(); }
        }
    };

    SERVICE_REGISTRATION(ObserverPlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

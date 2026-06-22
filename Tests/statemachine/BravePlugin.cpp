#include "Module.h"

namespace Thunder {
namespace Plugin {

    // Minimal in-process plugin: loads, initializes and tears down cleanly.
    // Empty Locator in the config routes instantiation through the in-process
    // service chain, matched on the bare class name "BravePlugin".
    class BravePlugin : public PluginHost::IPlugin {
    public:
        BravePlugin() = default;
        ~BravePlugin() override = default;

        BravePlugin(const BravePlugin&) = delete;
        BravePlugin& operator=(const BravePlugin&) = delete;

        BEGIN_INTERFACE_MAP(BravePlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* /* shell */) override
        {
            return string(); // empty == success
        }
        void Deinitialize(PluginHost::IShell* /* shell */) override
        {
        }
        string Information() const override
        {
            return string();
        }
    };

    SERVICE_REGISTRATION(BravePlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

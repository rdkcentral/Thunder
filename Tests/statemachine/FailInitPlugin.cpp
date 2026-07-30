#include "Module.h"

namespace Thunder {
namespace Plugin {

    // In-process plugin whose Initialize() reports failure, to drive the
    // INITIALIZATION_FAILED branch (ends back in DEACTIVATED).
    class FailInitPlugin : public PluginHost::IPlugin {
    public:
        FailInitPlugin() = default;
        ~FailInitPlugin() override = default;

        FailInitPlugin(const FailInitPlugin&) = delete;
        FailInitPlugin& operator=(const FailInitPlugin&) = delete;

        BEGIN_INTERFACE_MAP(FailInitPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* /* shell */) override
        {
            return _T("init refused"); // non-empty == failure
        }
        void Deinitialize(PluginHost::IShell* /* shell */) override
        {
        }
        string Information() const override
        {
            return string();
        }
    };

    SERVICE_REGISTRATION(FailInitPlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

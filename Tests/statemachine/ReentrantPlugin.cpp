#include "Module.h"
#include "ReentrantControl.h"

namespace Thunder {

namespace TestSupport {
    ReentrantTrigger g_reentrantTrigger = ReentrantTrigger::Deactivate;
    uint32_t g_reentrantResult = Core::ERROR_NONE;
}

namespace Plugin {

    // Calls a lifecycle trigger on its OWN shell from within Initialize, i.e. on
    // the transition thread. The re-entrancy guard must reject every such call
    // with ERROR_ILLEGAL_STATE without deadlocking or corrupting state.
    class ReentrantPlugin : public PluginHost::IPlugin {
    public:
        ReentrantPlugin() = default;
        ~ReentrantPlugin() override = default;
        ReentrantPlugin(const ReentrantPlugin&) = delete;
        ReentrantPlugin& operator=(const ReentrantPlugin&) = delete;

        BEGIN_INTERFACE_MAP(ReentrantPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* shell) override
        {
            using Trigger = TestSupport::ReentrantTrigger;
            const auto why = PluginHost::IShell::reason::REQUESTED;

            switch (TestSupport::g_reentrantTrigger) {
            case Trigger::Activate:
                TestSupport::g_reentrantResult = shell->Activate(why);
                break;
            case Trigger::Deactivate:
                TestSupport::g_reentrantResult = shell->Deactivate(why);
                break;
            case Trigger::Unavailable:
                TestSupport::g_reentrantResult = shell->Unavailable(why);
                break;
            case Trigger::Hibernate:
                TestSupport::g_reentrantResult = shell->Hibernate(0);
                break;
            }

            return string(); // Initialize itself succeeds; the re-entry was rejected.
        }
        void Deinitialize(PluginHost::IShell* /* shell */) override {}
        string Information() const override { return string(); }
    };

    SERVICE_REGISTRATION(ReentrantPlugin, 1, 0)

} // namespace Plugin
} // namespace Thunder

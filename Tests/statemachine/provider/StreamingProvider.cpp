#include "Module.h"

namespace Thunder {
namespace Plugin {

    // Real .so plugin that CONTROLS the STREAMING subsystem. Because it has a
    // non-empty locator, LoadMetadata fills _control, so STREAMING is treated as
    // externally controlled and is unset at startup.
    class StreamingProvider : public PluginHost::IPlugin {
    public:
        StreamingProvider() = default;
        ~StreamingProvider() override = default;
        StreamingProvider(const StreamingProvider&) = delete;
        StreamingProvider& operator=(const StreamingProvider&) = delete;

        BEGIN_INTERFACE_MAP(StreamingProvider)
            INTERFACE_ENTRY(PluginHost::IPlugin)
        END_INTERFACE_MAP

        const string Initialize(PluginHost::IShell* shell) override
        {
            PluginHost::ISubSystem* subSystem = shell->SubSystems();
            if (subSystem != nullptr) {
                subSystem->Set(PluginHost::ISubSystem::STREAMING, nullptr); // controlled -> allowed
                subSystem->Release();
            }
            return string();
        }
        void Deinitialize(PluginHost::IShell* /* shell */) override
        {
            // The framework unsets controlled subsystems on deactivation
            // (PluginServer.cpp, SubSystemControl loop), so nothing to do here.
        }
        string Information() const override
        {
            return string();
        }
    };

    namespace {
        static Metadata<StreamingProvider> metadata(
            1, 0, 0,
            {},                                    // preconditions
            {},                                    // terminations
            { PluginHost::ISubSystem::STREAMING }  // controls
        );
    }

} // namespace Plugin
} // namespace Thunder

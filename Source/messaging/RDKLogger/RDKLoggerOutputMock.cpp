#include "ExternalOutput.h"

#include "Module.h"

extern "C" {
    uint32_t ThunderExternalOutput_Initialize(void)
    {
        TRACE_L1("RDKLogger mock backend initialized");
        return (0);
    }

    uint32_t ThunderExternalOutput_IsEnabled(const char* module, const ThunderExternalLogLevel level)
    {
        TRACE_L1("RDKLogger mock enablement query: module=%s level=%u", module, static_cast<unsigned>(level));
        return (1);
    }

    uint32_t ThunderExternalOutput_Submit(const char* module, const ThunderExternalLogLevel level, const char* payload)
    {
        TRACE_L1("RDKLogger mock message: module=%s level=%u payload=%s", module, static_cast<unsigned>(level), payload);
        return (0);
    }

    uint32_t ThunderExternalOutput_Deinitialize(void)
    {
        TRACE_L1("RDKLogger mock backend deinitialized");
        return (0);
    }
}
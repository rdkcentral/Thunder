#include "Module.h"

// only to make sure the OCDM proxystub code is loaded

extern void* OCDM_ProxyStubRegistration;

extern "C" {
void* opencdm_announce_proxy_stubs()
{
    return &OCDM_ProxyStubRegistration;
}
}

#include "../IDriver.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t construct_bluetooth_driver(const char* /* config */) {
    return (Core::ERROR_NONE);
}

void destruct_bluetooth_driver() {
}


#ifdef __cplusplus
}
#endif


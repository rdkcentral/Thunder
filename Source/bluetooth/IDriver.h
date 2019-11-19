#pragma once

#ifdef __cplusplus
extern "C" {
#endif

const char* construct_bluetooth_driver(const char* config);
void destruct_bluetooth_driver();

#ifdef __cplusplus
}
#endif

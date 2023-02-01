#ifndef __HIBERNATE_H
#define __HIBERNATE_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t Hibernate(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage);
uint32_t Wakeup(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage);
                 
#ifdef __cplusplus
}
#endif

#endif

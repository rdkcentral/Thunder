#ifndef __HIBERNATE_H
#define __HIBERNATE_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t HibernateProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], struct state_store** storage);
uint32_t WakeupProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], struct state_store** storage);
                 
#ifdef __cplusplus
}
#endif

#endif

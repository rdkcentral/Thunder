#ifndef __HIBERNATE_H
#define __HIBERNATE_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


struct vm_skip_addr {
    void* addr;
    char desc;
} __attribute__((packed));

struct state_store {
    int                 nr_threads;
    pid_t               parasite_pid;
    pid_t               tids[1024];
    struct vm_skip_addr skip_addr[1024];
};

uint32_t Hibernate(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], struct state_store** storage);
uint32_t Restore(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], struct state_store* storage);

#ifdef __cplusplus
}
#endif

#endif

#include "process_containers.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct DummyProcessContainer {
    struct ProcessContainer cContainer;

    char logPath[256];
    uint8_t running;
    struct ProcessContainerMemory memory;

    char networkInterface[16];
    char address[256];
};

void strcpy_safe(char* dest, const char* src, size_t bufferLength) 
{
    strncpy(dest, src, bufferLength - 1);
    dest[bufferLength - 1] = '\0';
}

ContainerError process_container_logging(const char* logPath, const char* loggingOptions)
{
    printf("Global logging initialized to directory: %s\n", logPath);

    return ERROR_NONE;
}

ContainerError process_container_initialize() 
{
    printf("Container framework is initialized!\n");

    return ERROR_NONE;
}

ContainerError process_container_deinitialize() 
{
    printf("Container framework is deinitialized!\n");

    return ERROR_NONE;
}

ContainerError process_container_create(struct ProcessContainer** container, const char* id, const char** searchPaths, const char* logPath, const char* configuration)
{
    struct DummyProcessContainer* output = (struct DummyProcessContainer*)malloc(sizeof(struct DummyProcessContainer));

    output->cContainer.id = malloc(strlen(id));

    strcpy(output->cContainer.id, id);
    strcpy_safe(output->logPath, logPath, sizeof(output->logPath));

    output->running = 1;
    output->memory.allocated = rand() % (1024 * 1024);
    output->memory.resident = rand() % (1024 * 1024);
    output->memory.shared = rand() % (1024 * 1024);

    strcpy_safe(output->networkInterface, "container0", sizeof(output->networkInterface));
    strcpy_safe(output->address, "127.0.0.23", sizeof(output->address));

    *container = (struct ProcessContainer*)output;

    printf("Container %s created!\n", output->cContainer.id);

    return ERROR_NONE;    
}

ContainerError process_container_destroy(struct ProcessContainer* container)
{
    printf("Container %s released!\n", container->id);

    free(container);

    return ERROR_NONE;
}

ContainerError process_container_start(struct ProcessContainer* container, const char* command, const char** params)
{
    printf("Executed %s command! with arguments:\n", command);
    for (int i = 0; params[i] != NULL; i++) {
        printf("%s\n", params[i]);
    }

    printf("\n\n");

    return ERROR_NONE;
}

ContainerError process_container_stop(struct ProcessContainer* container)
{
    struct DummyProcessContainer* dummyContainer = (struct DummyProcessContainer*)container;

    printf("Container %s stopped!\n", dummyContainer->cContainer.id);
    dummyContainer->running = 0;

    return ERROR_NONE;
}

uint8_t process_container_running(struct ProcessContainer* container)
{
    return ((struct DummyProcessContainer*)container)->running;
}

ContainerError process_container_memory_status(struct ProcessContainer* container, struct ProcessContainerMemory* memory)
{
    memcpy(memory, &(((struct DummyProcessContainer*)container)->memory), sizeof(struct ProcessContainerMemory));

    return ERROR_NONE;
}

ContainerError process_container_cpu_usage(struct ProcessContainer* container, int32_t threadNum, uint64_t* usage)
{
    ContainerError result = ERROR_NONE;

    if (threadNum > sysconf(_SC_NPROCESSORS_ONLN)) {
        result = ERROR_OUT_OF_BOUNDS;
    } else {
        *usage = rand() % (1024 * 1024 * 256);
    }

    return result;
}

ContainerError process_container_pid(struct ProcessContainer* container, uint32_t* pid) 
{
    // Dumm random pid
    pid = (*((uint32_t*)container)) % 100 + 10;
}


ContainerError process_container_network_status_create(struct ProcessContainer* container, struct ProcessContainerNetworkStatus* networkStatus) 
{
    networkStatus->numInterfaces = 2;
    networkStatus->interfaces = malloc(2 * sizeof(struct ProcessContainerNetworkStatus));

    networkStatus->interfaces[0].interfaceName = "eth0";
    networkStatus->interfaces[0].numIp = 1;
    networkStatus->interfaces[0].ips = malloc(2 * sizeof(char*));
    networkStatus->interfaces[0].ips[0] = "10.11.12.13";


    networkStatus->interfaces[0].interfaceName = "eth1";
    networkStatus->interfaces[0].numIp = 2;
    networkStatus->interfaces[0].ips = malloc(2 * sizeof(char*));
    networkStatus->interfaces[0].ips[0] = "1.2.3.4";
    networkStatus->interfaces[0].ips[0] = "1.2.3.5";
    
    return ERROR_NONE;
}

ContainerError process_container_network_status_destroy(struct ProcessContainerNetworkStatus* networkStatus) 
{
    for (int interfaceId = 0; interfaceId < networkStatus->numInterfaces; ++interfaceId) {
        
        free(networkStatus->interfaces[interfaceId].ips);
    }

    free(networkStatus);

    return ERROR_NONE;
}


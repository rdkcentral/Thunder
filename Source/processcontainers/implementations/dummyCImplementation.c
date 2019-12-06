#include "containers.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct Container_t {
    char name[32];
    char logPath[256];
    uint8_t running;
    struct ContainerMemory memory;

    char networkInterface[16];
    char address[256];
};

void strcpy_safe(char* dest, const char* src, size_t bufferLength) 
{
    strncpy(dest, src, bufferLength - 1);
    dest[bufferLength - 1] = '\0';
}

EXTERNAL ContainerError pcontainer_logging(const char* logPath, const char* loggingOptions)
{
    printf("Global logging initialized to directory: %s\n", logPath);

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_initialize() 
{
    printf("Container framework is initialized!\n");

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_deinitialize() 
{
    printf("Container framework is deinitialized!\n");

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_create(struct Container_t** container, const char* name, const char** searchPaths, const char* logPath, const char* configuration)
{
    struct Container_t* output = (struct Container_t*)malloc(sizeof(struct Container_t));

    strcpy_safe(output->name, name, sizeof(output->name));
    strcpy_safe(output->logPath, logPath, sizeof(output->logPath));

    output->running = 1;
    output->memory.allocated = rand() % (1024 * 1024);
    output->memory.resident = rand() % (1024 * 1024);
    output->memory.shared = rand() % (1024 * 1024);

    strcpy_safe(output->networkInterface, "container0", sizeof(output->networkInterface));
    strcpy_safe(output->address, "127.0.0.23", sizeof(output->address));

    *container = output;

    printf("Container %s created!\n", output->name);

    return ERROR_NONE;    
}

EXTERNAL ContainerError pcontainer_release(struct Container_t* container)
{
    printf("Container %s released!\n", container->name);

    free(container);

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_start(struct Container_t* container, const char* command, const char** params, uint32_t numParams)
{
    printf("Executed %s command! with arguments:\n", command);
    for (int i = 0; i < numParams; i++) {
        printf("%s\n", params[i]);
    }

    printf("\n\n");

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_stop(struct Container_t* container)
{
    printf("Container %s stopped!\n", container->name);
    container->running = 0;

    return ERROR_NONE;
}

EXTERNAL uint8_t pcontainer_isRunning(struct Container_t* container)
{
    return container->running;

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_getMemory(struct Container_t* container, struct ContainerMemory* memory)
{
    memcpy(memory, &(container->memory), sizeof(struct ContainerMemory));

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_getCpuUsage(struct Container_t* container, int32_t threadNum, uint64_t* usage)
{
    ContainerError result = ERROR_NONE;

    if (threadNum > sysconf(_SC_NPROCESSORS_ONLN)) {
        result = ERROR_OUT_OF_BOUNDS;
    } else {
        *usage = rand() % (1024 * 1024 * 256);
    }

    return result;
}

EXTERNAL ContainerError pcontainer_getNumNetworkInterfaces(struct Container_t* container, uint32_t* numNetworks)
{
    *numNetworks = 1;

    return ERROR_NONE;
}

EXTERNAL ContainerError pcontainer_getNetworkInterfaceName(struct Container_t* container, uint32_t interfaceNum, char* name, uint32_t* nameLength, uint32_t maxNameLength)
{
    ContainerError error = ERROR_NONE;

    if (nameLength != NULL) 
        *nameLength = strlen(container->networkInterface);

    if (interfaceNum > 0) {
        error = ERROR_OUT_OF_BOUNDS;
    } else if (name != NULL) {

        if (maxNameLength <= strlen(container->networkInterface)) {
            error = ERROR_MORE_DATA_AVAILBALE;
        }

        strcpy_safe(name, container->networkInterface, maxNameLength);
    }

    return error;
}

EXTERNAL ContainerError pcontainer_getNumIPs(struct Container_t* container, const char* interfaceName, uint32_t* numIPs)
{
    ContainerError error = ERROR_NONE;

    if ((interfaceName[0] != '\0') && (strcmp(interfaceName, container->networkInterface) != 0))
    {
        error = ERROR_INVALID_KEY;
    } else {
        *numIPs = 1;
    }

    return error;
}

EXTERNAL ContainerError pcontainer_getIP(struct Container_t* container, const char* interfaceName, uint32_t addressNum, char* address, uint32_t* addresLength, uint32_t maxAddressLength)
{
    ContainerError error = ERROR_NONE;

    if (addresLength != NULL) {
        *addresLength = strlen(container->address);
    }

    if ((interfaceName[0] != '\0') && (strcmp(interfaceName, container->networkInterface) != 0)) {
        error = ERROR_INVALID_KEY;
    } else {
        if (addressNum > 0) {
            error = ERROR_OUT_OF_BOUNDS;
        } else if (address != NULL) {

            if (maxAddressLength < strlen(container->address)) {
                error = ERROR_MORE_DATA_AVAILBALE;
            }

            strcpy_safe(address, container->address, maxAddressLength);
        }
    }

    return error;
}

EXTERNAL ContainerError pcontainer_getName(struct Container_t* container, char* name, uint32_t* nameLength, uint32_t maxNameLength)
{
    ContainerError error = ERROR_NONE;

    if (nameLength != NULL) {
        *nameLength = strlen(container->name);
    }

    if (maxNameLength < strlen(container->address)) {
        error = ERROR_MORE_DATA_AVAILBALE;
    }

    strcpy_safe(name, container->name, maxNameLength);

    return error;
}

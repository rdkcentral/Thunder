#include "containers.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

struct Container_t {
    char name[32];
    char configPath[256];
    char logPath[256];
    uint8_t running;
    ContainerMemory memory;

    char networkInterface[16];
    char address[256];
};

/**
 * \brief Initializes a container.
 * This function initializes a container and prepares it to be started
 * 
 * \param container - (output) Container that is to be initialized
 * \param name - Name of started container
 * \param searchpaths - Null-terminated list of locations where container can be located. 
 *                      List is processed in the order it is provided, and the first 
 *                      container matching provided name will be selected & initialized
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError container_create(struct Container_t** container, const char* name, const char** searchpaths, const char* logPath, const char* configuration)
{
    Container_t* output = (Container_t*)malloc(sizeof(Container_t));

    strncpy(output->name, name, 32);
    strncpy(output->logPath, logPath, 256);

    output->running = 1;
    output->memory.allocated = rand() % (1024 * 1024);
    output->memory.resident = rand() % (1024 * 1024);
    output->memory.shared = rand() % (1024 * 1024);

    strncpy(output->networkInterface, "container0", sizeof(output->networkInterface);
    strncpy(output->address, "127.0.0.23", sizeof(output->address));

    *container = output;

    return ContainerError::ERROR_NONE;    
}

/**
 * \brief Enables logging.
 * This functions setups logging of containers
 * 
 * \param logpath - Path to folder where the logging will be stored
 * \param logId - Null-terminated list of locations where container can be located. 
 *                      List is processed in the order it is provided, and the first 
 *                      container matching provided name will be selected & initialized
 * \param loggingOptions - Logging configuration
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError container_enableLogging(const char* logpath, const char* logId, const char* loggingOptions)
{
    return ContainerError::ERROR_NONE;
}

/**
 * \brief Releases a container.
 * This function releases all resources taken by a container
 * 
 * all resources taken by it
 * \param container - Container that will be released
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError container_release(struct Container_t* container)
{
    free(container);
}

/**
 * \brief Starts a container.
 * This function starts a given command in the container
 * 
 * \param container - Container that is to be initialized
 * \param command - Command that will be started in container's shell
 * \param params - List of parameters provied to command
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError container_start(struct Container_t* container, const char* command, const char** params, uint32_t numParams)
{
    printf("Executed %s command!\n", command);
}

/**
 * \brief Stops a container.
 * This function stops a container
 * 
 * \param container - Container that is to be initialized
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError container_stop(struct Container_t* container)
{
    container->running = false;
}

/**
 * \brief Gives information if the container is running.
 *
 * This function can be used to check if container is in stopped state
 * or in running state.
 * 
 * \param container - Container that is checked
 * \return 1 if is running, 0 otherwise.
 */
EXTERNAL uint8_t container_isRunning(struct Container_t* container)
{
    return container->running;
}

/**
 * \brief Information about memory usage.
 *
 * Function gives information about memory allocated to 
 * runnning the container
 * 
 * \param container - Container that is checked
 * \param memory - Pointer to structure that will be filled with memory 
 * information
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getMemory(struct Container_t* container, ContainerMemory* memory)
{
    memcpy(memory, &(container->memory), sizeof(ContainerMemory);

    return ContainerError::ERROR_NONE;
}

/**
 * \brief Information about cpu usage.
 * Function gives information about how much of CPU time was 
 * used for the container
 * 
 * \param container - Container that is checked
 * \param threadNum - Ordinal number of thread of which usage will be returned. 
 *                    If -1 is provided, total CPU usage will be reported. 
 * \param usage - Usage of cpu time in nanoseconds
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getCpuUsage(struct Container_t* container, int32_t threadNum, uint64_t* usage)
{
    ContainerError result = ContainerError::ERROR_NONE;

    if (threadNum > sysconf(_SC_NPROCESSORS_ONLN)) {
        result = ContainerError::ERROR_OUT_OF_BOUNDS;
    } else {
        *usage = rand() % (1024 * 1024 * 256);
    }

    return result;
}


/**
 * \brief Number of network interfaces assigned to container.
 * Function gives information about how many network interface
 * the container have
 * 
 * \param container - Container that is checked
 * \param numNetworks - (output) Number of network interface assigned to a container.
 *  
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getNumNetworkInterfaces(struct Container_t* container, uint32_t* numNetworks)
{
    *numNetworks = 1;

    return ContainerError::ERROR_NONE;
}

/**
 * \brief Containers's network interfaces.
 * Function gives information about memory allocated to 
 * runnning the container
 * 
 * \param container - Container that is checked
 * \param interfaceNum - Ordinal number of interface. Can take value from 0 to value returned by  
 *                       container_getNumNetworkInterfaces - 1. Otherwise ERROR_OUT_OF_BOUNDS is returned
 * \param name - (output) Name of network interface
 * \param maxNameLength - Maximum length of name buffer.
 * 
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getNetworkInterfaceName(struct Container_t* container, uint32_t interfaceNum, char* name, uint32_t maxNameLength = 16)
{
    ContainerError error = ContainerError::ERROR_NONE;

    if (interfaceNum > 0) {
        error = ContainerError::ERROR_OUT_OF_BOUNDS;
    } else {
        size_t maxLength = maxNameLength < sizeof(container->networkInterface) ? maxNameLength : sizeof(container->networkInterface);

        strncpy(name, container->networkInterface, maxLength);
    }

    return error;
}

/**
 * \brief Number of IP addresses asssigend to interface.
 * Function gives information about how many ip addresses are assigned to a  
 * given network interface
 * 
 * \param container - Container that is checked
 * \param interfaceName - Name of network interface on which we are checking ip. 
 *                        If set to NULL, function will return number of all ip addresses of container
 * \param numIPs - (output) Number of ips asssigned to network interface
 * 
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getNumIPs(struct Container_t* container, const char* interfaceName, uint32_t* numIPs)
{
    ContainerError error = ContainerError::ERROR_NONE;

    if ((interfaceName != nullptr) && (strcmp(interfaceName, container->networkInterface) != 0))
    {
        error = ContainerError::ERROR_INVALID_KEY;
    } else {
        *numIPs = 1;
    }

    return error;
}

/**
 * \brief List of IP addresses given to a container.
 * Function gives information about memory allocated to 
 * runnning the container
 * 
 * \param container - Container that is checked
 * \param interfaceName - Name of network interface on which we are checking ip. 
 *                        If set to NULL, function will return addresses from of 
 *                        all container's addresses
 * \param addressNum - Ordinal number of ip assigned to interface. Can range from 0 to value obtained from
 *                     container_getNumIPs() for a given interface. If other address is given, 
 *                     ERROR_OUT_OF_BOUNDS is returned
 * \param address - (output) IP Address of container
 * \param maxAddressLength - Maximum length of address. If address length is higher than maxAddressLength,
 *                           ERROR_MORE_DATA_AVAILABLE is returned
 * 
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getIP(struct Container_t* container, const char* interfaceName, uint32_t addressNum, char* address, uint32_t maxAddressLength)
{
    ContainerError error = ContainerError::ERROR_NONE;

    if ((interfaceName != nullptr) && (strcmp(interfaceName, container->networkInterface) != 0))
    {
        error = ContainerError::ERROR_INVALID_KEY;
    } else {
        if (addressNum > 1) {
            error = ContainerError::ERROR_OUT_OF_BOUNDS;
        } else {
            size_t maxLength = maxAddressLength < sizeof(container->address) ? maxAddressLength : sizeof(container->address);

            strncpy(address, container->address, maxLength);
        }
    }

    return error;
}

/**
 * \brief Container's config path.
 * Function gives path of configuration that was used to  
 * initialize container
 * 
 * \param container - Container that is checked
 * \param path - Buffer where the containers config path will be placed
 * \param maxPathLength - Maximum length of path. If path is longer than 
 *                        this, ERROR_MORE_DATA will be returned
 * 
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getConfigPath(struct Container_t* container, char* path, uint32_t maxPathLength)
{
    size_t maxLength = maxPathLength < sizeof(container->configPath) ? maxPathLength : sizeof(container->configPath);

    strncpy(path, container->configPath, maxLength);

    return ContainerError::ERROR_NONE;
}

/**
 * \brief Containers name.
 * Function gives name of the container
 * 
 * \param container - Container that is checked
 * \param id - Name of the container
 * \param maxIdLength - Maximum length of containers name. If name is longer than 
 *                      this, ERROR_MORE_DATA will be returned
 * 
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError container_getName(struct Container_t* container, char* name, uint32_t maxNameLength)
{
    size_t maxLength = maxNameLength < sizeof(container->name) ? maxNameLength : sizeof(container->configPath);

    strncpy(name, container->name, maxLength);

    return ContainerError::ERROR_NONE;
}

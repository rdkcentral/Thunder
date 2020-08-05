/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CONTAINERS_H
#define __CONTAINERS_H

#include <stdint.h>
#include <string.h>

#define EXTERNAL EXTERNAL_EXPORT

#ifdef __cplusplus

extern "C" {

#endif

struct ProcessContainer {
    char* id;
};

struct ProcessContainerNetworkInterface {
    char* interfaceName;
    uint32_t numIp;
    char** ips;
};

struct ProcessContainerNetworkStatus {
    uint32_t numInterfaces;
    struct ProcessContainerNetworkInterface* interfaces;
};

/**
 * Container error code. Zero always means success.
 */
typedef enum {
    PC_ERROR_NONE = 0,
    PC_ERROR_UNKNOWN = 1,
    PC_ERROR_MORE_DATA_AVAILBALE=2,
    PC_ERROR_OUT_OF_BOUNDS = 3,
    PC_ERROR_INVALID_KEY = 4
} ContainerError;

struct ProcessContainerMemory {
    uint64_t allocated; // in bytes
    uint64_t resident; // in bytes
    uint64_t shared; // in bytes
};

/**
 * \brief Initializes container framework.
 * This functions setups container logging. Bear in mind that it might be called 
 * before process_container_initialize() or after process_container_deinitialize()
 * 
 * \param logpath - Path to folder where the global container logs will be stored
 * \param loggingOptions - Logging configuration
 * 
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_logging(char* logPath, char* loggingOptions);


/**
 * \brief Initializes container framework.
 * This function initializes container framework
 * 
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_initialize();

/**
 * \brief Frees resources held by containers api.
 * This functions releases resources held by containers framework
 * 
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_deinitialize();

/**
 * \brief Initializes a container.
 * This function initializes a container and prepares it to be started
 * 
 * \param container - (output) Container that is to be initialized
 * \param id - Id of started container
 * \param searchPaths - Null-terminated list of locations where container can be located. 
 *                      List is processed in the order it is provided, and the first 
 *                      container matching provided id will be selected & initialized
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_create(struct ProcessContainer** container, char* id, char** searchPaths, char* logPath, char* configuration);

/**
 * \brief Destroys a container.
 * This function releases all resources taken by a container
 * 
 * all resources taken by it
 * \param container - Container that will be released
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_destroy(struct ProcessContainer* container);

/**
 * \brief Starts a process container.
 * This function starts a given command in the container
 * 
 * \param container - Container that is to be initialized
 * \param command - Command that will be started in container's shell
 * \param params - Null-terminated list of parameters provided to command
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_start(struct ProcessContainer* container, char* command, char** params);

/**
 * \brief Stops a process container.
 * This function stops a container
 * 
 * \param container - Container that is to be initialized
 * \return Zero on success, non-zero on error.
 */
EXTERNAL ContainerError process_container_stop(struct ProcessContainer* container);

/**
 * \brief Gives information if the container is running.
 *
 * This function can be used to check if container is in stopped state
 * or in running state.
 * 
 * \param container - Container that is checked
 * \return 1 if is running, 0 otherwise.
 */
EXTERNAL uint8_t process_container_running(struct ProcessContainer* container);

/**
 * \brief Information about memory usage.
 *
 * Function gives information about memory allocated to 
 * runnning the container
 * 
 * \param container - Container that is checked
 * \param memory - (output) Pointer to structure that will be filled with memory 
 * information
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError process_container_memory_status(struct ProcessContainer* container, struct ProcessContainerMemory* memory);

/**
 * \brief Information about cpu usage.
 * Function gives information about how much of CPU time was 
 * used for the container
 * 
 * \param container - Container that is checked
 * \param threadNum - id core of (0,1,2...) which usage will be returned. 
 *                    If -1 is provided, total CPU usage will be reported. 
 * \param usage - (output) Usage of cpu in nanoseconds
 * \return Zero on success, error-code otherwise (specificly PC_ERROR_OUT_OF_BOUNDS when threadNum is out of bounds)
 */
EXTERNAL ContainerError process_container_cpu_usage(struct ProcessContainer* container, int32_t threadNum, uint64_t* usage);

/**
 * \brief Returns PID.
 * Funtion returns PID of a process in host namespace
 * 
 * \param container - Container that is checked
 * \param pid - (output) pid of process in host namespaces
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError process_container_pid(struct ProcessContainer* container, uint32_t* pid);

/**
 * \brief Information about network configuration.
 * Functions return informations about network of the provided
 * container
 * 
 * \param container - Container that is checked
 * \param networkStatus - (output) Structure in which current networking informations will be stored.
 *  
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError process_container_network_status_create(struct ProcessContainer* container, struct ProcessContainerNetworkStatus* networkStatus);

/**
 * \brief Releases resources held by ProcessContainerNetworkInterface
 * 
 * \param networkStatus - (output) networkstatus to be destroyed
 *  
 * \return Zero on success, error-code otherwise
 */
EXTERNAL ContainerError process_container_network_status_destroy(struct ProcessContainerNetworkStatus* networkStatus);

#ifdef __cplusplus
}
#endif

#endif // __CONTAINERS_H

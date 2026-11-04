/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
#define MODULE "CheckpointLib"

#include "../hibernate.h"
#include "../common/Log.h"

//TODO: #include <memcr.h>
#include <assert.h>
#include <stdlib.h>

typedef struct {
    pid_t pid;
} CheckpointMetaData;

uint32_t HibernateProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    assert(*storage == NULL);
    CheckpointMetaData* metaData = (CheckpointMetaData*) malloc(sizeof(CheckpointMetaData));
    assert(metaData);

    metaData->pid = pid;

    *storage = (void*)(metaData);

    //TODO: MEMCR_Checkpoint(timeoute, pid, data_dir, volatile_dir);

    return HIBERNATE_ERROR_NONE;
}

uint32_t WakeupProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    assert(*storage != NULL);
    CheckpointMetaData* metaData = (CheckpointMetaData*)(*storage);
    assert(metaData->pid == pid);

    //TODO: MEMCR_Resume(timeoute, pid, data_dir, volatile_dir);
    free(metaData);
    *storage = NULL;

    return HIBERNATE_ERROR_NONE;
}

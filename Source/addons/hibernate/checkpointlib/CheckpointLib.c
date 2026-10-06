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

// PUBLIC_INTERFACE
/**
 * Attempt to checkpoint pid using the selected library backend.
 * timeout and directory arguments describe the requested operation; storage
 * belongs to the caller. Returns GENERAL without changing storage because
 * this backend has no checkpoint implementation.
 */
uint32_t HibernateProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    // Bookkeeping is not a checkpoint. Never let callers infer it is safe
    // to suspend a process while the actual backend is unavailable.
    (void)timeout;
    (void)pid;
    (void)data_dir;
    (void)volatile_dir;
    (void)storage;
    LOGERR("Checkpoint library backend is not implemented");
    return HIBERNATE_ERROR_GENERAL;
}

// PUBLIC_INTERFACE
/**
 * Attempt to resume pid with the supplied timeout, directories and storage.
 * Returns GENERAL because resume is unavailable; opaque caller storage is
 * preserved and may be NULL.
 */
uint32_t WakeupProcess(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
{
    // Do not destroy metadata for an operation that was never performed.
    (void)timeout;
    (void)pid;
    (void)data_dir;
    (void)volatile_dir;
    (void)storage;
    LOGERR("Checkpoint library resume backend is not implemented");
    return HIBERNATE_ERROR_GENERAL;
}

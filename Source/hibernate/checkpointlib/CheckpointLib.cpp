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
#include "hibernate/Hibernate.h"

namespace WPEFramework {
namespace Hibernate {

    struct CheckpointMetaData {
        pid_t pid;
    };

    uint32_t Hibernate(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
    {
        ASSERT (*storage == nullptr);
        CheckpointMetaData *metaData = new CheckpointMetaData;
        if(metaData)
        {
            metaData->pid = pid;    
        }

        *storage = static_cast<void*>(metaData);

        return 0;
    }
    uint32_t Wakeup(const uint32_t timeout, const pid_t pid, const char data_dir[], const char volatile_dir[], void** storage)
    {
        ASSERT (*storage != nullptr);
        CheckpointMetaData *metaData = static_cast<CheckpointMetaData*>(*storage);
        ASSERT (metaData->pid == pid);
        delete metaData;
        *storage = nullptr;

        return 0;
    }

}
}
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
 
#include "WorkerPool.h"

namespace Thunder {

    namespace Core {

	IWorkerPool* _workerPoolInstance = nullptr;

        /* static */ void IWorkerPool::Assign(IWorkerPool* instance) {
            ASSERT ( (_workerPoolInstance == nullptr) ^ (instance == nullptr) );
            _workerPoolInstance = instance;
        }
        /* static */ IWorkerPool& IWorkerPool::Instance() {
            ASSERT(_workerPoolInstance != nullptr);
            return (*_workerPoolInstance);
        }
        /* static */ bool IWorkerPool::IsAvailable() {
            return (_workerPoolInstance != nullptr);
        }
    }
}

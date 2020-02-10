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
 
#include "WorkerPool.h"

namespace WPEFramework {

	namespace Core {

	    /* static */ WorkerPool* WorkerPool::_instance = nullptr;

		WorkerPool::WorkerPool(const uint8_t threadCount, uint32_t* counters)
			: _handleQueue(16)
			, _occupation(0)
			, _timer(1024 * 1024, _T("WorkerPool::Timer"))
		{
			ASSERT(_instance == nullptr);

			_metadata.Slots = threadCount;
			_metadata.Slot = counters;
			_instance = this;
		}

		WorkerPool ::~WorkerPool()
		{
			_handleQueue.Disable();
			_instance = nullptr;
		}
	}
}

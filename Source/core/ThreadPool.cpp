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

#include "ThreadPool.h"

#ifdef __CORE_WARNING_REPORTING__
namespace WPEFramework {
namespace Core {

    ThreadPool::IJob* ThreadPool::MeasurableJob::Process(IDispatcher* dispatcher, const char moduleName[])
    {
        ASSERT(dispatcher != nullptr);
        ASSERT(_job.IsValid());
        ASSERT(_time != NumberType<uint64_t>::Max());

        IDispatch* request = &(*_job);
        REPORT_PRIORITY_OUTOFBOUNDS_WARNING_CONTEXT(WarningReporting::JobTooLongWaitingInQueue, moduleName, CallsignTLS::CallsignForModule(moduleName),
            static_cast<uint32_t>((Time::Now().Ticks() - _time) / Time::TicksPerMillisecond));
        REPORT_PRIORITY_DURATION_WARNING_CONTEXT({ dispatcher->Dispatch(request); }, WarningReporting::JobTooLongToFinish,
            moduleName, CallsignTLS::CallsignForModule(moduleName));

        return (dynamic_cast<IJob*>(request));
    }
}
}
#endif

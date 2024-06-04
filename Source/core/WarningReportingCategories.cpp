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

#include "Module.h"
#include "WarningReportingControl.h"
#include "WarningReportingCategories.h"

namespace Thunder {

namespace WarningReporting {

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        static class Instantiation {
        public:
            Instantiation()
            {
                ANNOUNCE_WARNING(TooLongWaitingForLock);
                ANNOUNCE_WARNING(SinkStillHasReference);
                ANNOUNCE_WARNING(TooLongInvokeRPC);
                ANNOUNCE_WARNING(JobTooLongToFinish);
                ANNOUNCE_WARNING(JobTooLongWaitingInQueue);
                ANNOUNCE_WARNING(TooLongDecrypt);
                ANNOUNCE_WARNING(JobActiveForTooLong);
            }
        } ControlsRegistration;

    }
}
}

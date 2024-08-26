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

#include "TunerAdministrator.h"

namespace Thunder {

namespace Broadcast {

    /* static */ TunerAdministrator& TunerAdministrator::Instance()
    {
        static TunerAdministrator _instance;
        return (_instance);
    }

    // Accessor to metadata on the tuners.
    /* static */ void ITuner::Register(ITuner::INotification* notify)
    {
        TunerAdministrator::Instance().Register(notify);
    }

    /* static */ void ITuner::Unregister(ITuner::INotification* notify)
    {
        TunerAdministrator::Instance().Unregister(notify);
    }

}
} // namespace Thunder::Broadcast

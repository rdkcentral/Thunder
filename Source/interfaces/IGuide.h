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

#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a SIControl to change
    struct EXTERNAL IGuide : virtual public Core::IUnknown {
        enum { ID = ID_GUIDE };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_GUIDE_NOTIFICATION };

            virtual void EITBroadcast() = 0;
            virtual void EmergencyAlert() = 0;
            virtual void ParentalControlChanged() = 0;
            virtual void ParentalLockChanged(const string&) = 0;
            virtual void TestNotification(const string&) = 0; // XXX: Just for test
        };

        virtual uint32_t StartParser(PluginHost::IShell*) = 0;
        virtual const string GetChannels() = 0;
        virtual const string GetPrograms() = 0;
        virtual const string GetCurrentProgram(const string&) = 0;
        virtual const string GetAudioLanguages(const uint32_t) = 0;
        virtual const string GetSubtitleLanguages(const uint32_t) = 0;
        virtual bool SetParentalControlPin(const string&, const string&) = 0;
        virtual bool SetParentalControl(const string&, const bool) = 0;
        virtual bool IsParentalControlled() = 0;
        virtual bool SetParentalLock(const string&, const bool, const string&) = 0;
        virtual bool IsParentalLocked(const string&) = 0;

        virtual void Register(IGuide::INotification*) = 0;
        virtual void Unregister(IGuide::INotification*) = 0;
    };
}
}


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

    namespace Dolby {

        struct EXTERNAL IOutput : virtual public Core::IUnknown {

            enum { ID = ID_DOLBY_OUTPUT };

            ~IOutput() override = default;

            enum Type : uint8_t {
                DIGITAL_PCM = 0,
                DIGITAL_PASS_THROUGH = 3,
                ATMOS_PASS_THROUGH = 4,
                AUTO = 5
            };

            virtual void Mode(const Dolby::IOutput::Type value) = 0;
            
            virtual Dolby::IOutput::Type Mode() const = 0;
        };
    }
}
}

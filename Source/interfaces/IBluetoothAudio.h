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

    // @json
    struct IBluetoothAudioSink : virtual public Core::IUnknown {
        enum { ID = ID_BLUETOOTHAUDIOSINK };

        enum status {
            UNASSIGNED,
            DISCONNECTED,
            IDLE,
            CONFIGURED,
            OPEN,
            STREAMING
        };

        virtual ~IBluetoothAudioSink() {}

        virtual uint32_t Assign(const string& address) = 0;
        virtual uint32_t Revoke(const string& address) = 0;

        virtual uint32_t Status(status& sinkStatus /* @out */) const = 0;
    };

}

}

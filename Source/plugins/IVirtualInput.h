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

#pragma once

#include <core/core.h>

namespace Thunder {

namespace IVirtualInput {

    enum inputtypes : uint8_t {
        INPUT_KEY   = 0x01,
        INPUT_MOUSE = 0x02,
        INPUT_TOUCH = 0x04
    };
 
    struct LinkInfo {
        uint8_t Mode; /* input types activated */
        char Name[20];
    };

    struct KeyData {
        typedef enum : uint8_t {
            RELEASED = 0,
            PRESSED = 1,
            REPEAT = 2,
            COMPLETED = 3
        } type;

        type Action;
        uint32_t Code;
    };

    struct MouseData {
        typedef enum : uint8_t {
            RELEASED = 0,
            PRESSED = 1,
            MOTION = 2,
            SCROLL = 3
        } type;
 
        type Action;
        uint16_t Button;
        int16_t Horizontal;
        int16_t Vertical;
    };

    struct TouchData {
        typedef enum : uint8_t {
            RELEASED = 0,
            PRESSED = 1,
            MOTION = 2
        } type;

        type Action;
        uint16_t Index;
        uint16_t X;
        uint16_t Y;
    };

    typedef Core::IPCMessageType<0, Core::Void, LinkInfo>   NameMessage;
    typedef Core::IPCMessageType<1, KeyData,    Core::Void> KeyMessage;
    typedef Core::IPCMessageType<2, MouseData,  Core::Void> MouseMessage;
    typedef Core::IPCMessageType<3, TouchData,  Core::Void> TouchMessage;

} // namespace IVirtualInput

} // namespace Thunder

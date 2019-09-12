#pragma once

#include "Module.h"

namespace WPEFramework {

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

} // namespace WPEFramework

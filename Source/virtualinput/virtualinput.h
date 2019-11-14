#pragma once

#include <stdbool.h>

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef VIRTUALINPUT_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "virtualinput.lib")
#endif
#else
#ifndef EXTERNAL
#define EXTERNAL
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// ================================================================================================================

enum keyactiontype {
    KEY_RELEASED = 0,
    KEY_PRESSED = 1,
    KEY_REPEAT = 2,
    KEY_COMPLETED = 3
};

typedef void (*FNKeyEvent)(enum keyactiontype type, unsigned int code);

// ================================================================================================================

enum mouseactiontype {
    MOUSE_RELEASED = 0,
    MOUSE_PRESSED = 1,
    MOUSE_MOTION = 2,
    MOUSE_SCROLL = 3,
};

typedef void (*FNMouseEvent)(enum mouseactiontype type, unsigned short button, short horizontal, short vertical);

// ================================================================================================================

enum touchactiontype {
    TOUCH_RELEASED = 0,
    TOUCH_PRESSED = 1,
    TOUCH_MOTION = 2
};

typedef void (*FNTouchEvent)(enum touchactiontype type, unsigned short index,  unsigned short x, unsigned short y);

// ================================================================================================================

EXTERNAL void* virtualinput_open(const char listenerName[], const char connector[], FNKeyEvent keyCallback, FNMouseEvent mouseCallback, FNTouchEvent touchCallback);
EXTERNAL void  virtualinput_close(void* handle);

#ifdef __cplusplus
}
#endif

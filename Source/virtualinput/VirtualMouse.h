#ifndef __VIRTUAL_MOUSE_H__
#define __VIRTUAL_MOUSE_H__

#include "Module.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mouseactiontype {
    MOUSE_RELEASED = 0,
    MOUSE_PRESSED = 1,
    MOUSE_MOTION = 2,
    MOUSE_SCROLL = 3,
};

typedef void (*FNMouseEvent)(enum mouseactiontype type, unsigned short button, short horizontal, short vertical);

EXTERNAL void* ConstructMouse(const char listenerName[], const char connector[], FNMouseEvent callback);
EXTERNAL void DestructMouse(void* handle);

#ifdef __cplusplus
}
#endif

#endif // __VIRTUAL_MOUSE_H__

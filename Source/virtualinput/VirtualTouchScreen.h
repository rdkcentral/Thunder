#ifndef __VIRTUAL_TOUCHSCREEN_H__
#define __VIRTUAL_TOUCHSCREEN_H__

#include "Module.h"

#ifdef __cplusplus
extern "C" {
#endif

enum touchactiontype {
    TOUCH_RELEASED = 0,
    TOUCH_PRESSED = 1,
    TOUCH_MOTION = 2
};

typedef void (*FNTouchEvent)(enum touchactiontype type, unsigned short index,  unsigned short x, unsigned short y);

EXTERNAL void* ConstructTouchScreen(const char listenerName[], const char connector[], FNTouchEvent callback);
EXTERNAL void DestructTouchScreen(void* handle);

#ifdef __cplusplus
}
#endif

#endif // __VIRTUAL_TOUCHSCREEN_H__

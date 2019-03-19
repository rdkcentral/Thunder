#ifndef __VIRTUAL_KEYBOARD_H__
#define __VIRTUAL_KEYBOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

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
#define EXTERNAL
#endif

enum actiontype {
    RELEASED = 0,
    PRESSED = 1,
    REPEAT = 2,
    COMPLETED = 3
};

typedef void (*FNKeyEvent)(enum actiontype type, unsigned int code);

// Producer, Consumer, We produce the virtual keyboard, the receiver needs
// to destruct it once the done with the virtual keyboard.
// Use the Destruct, to destruct it.
EXTERNAL void* Construct(const char listenerName[], const char connector[], FNKeyEvent callback);
EXTERNAL void Destruct(void* handle);

#ifdef __cplusplus
}
#endif

#endif // __VIRTUAL_KEYBOARD_H__

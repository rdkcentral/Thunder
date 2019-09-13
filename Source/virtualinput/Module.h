#ifndef __MODULE_VIRTUALINPUT_H
#define __MODULE_VIRTUALINPUT_H

#ifndef MODULE_NAME
#define MODULE_NAME VirtualInput
#endif

#include <stdbool.h>
#include "../core/core.h"

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

#endif // __MODULE_VIRTUALINPUT_H

#ifndef __MODULE_VIRTUALINPUT_H
#define __MODULE_VIRTUALINPUT_H

#ifndef MODULE_NAME
#define MODULE_NAME VirtualInput
#endif

#include "../core/core.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef VIRTUALINPUT_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_VIRTUALINPUT_H

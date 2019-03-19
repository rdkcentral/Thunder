#ifndef __MODULE_BROADCAST_H
#define __MODULE_BROADCAST_H

#ifndef MODULE_NAME
#define MODULE_NAME Broadcast
#endif

#include "../core/core.h"

#undef EXTERNAL

#ifdef __WIN32__
#ifdef __MODULE_BROADCAST__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_BROADCAST_H

#ifndef __MODULE_CRYPTALGO_H
#define __MODULE_CRYPTALGO_H

#ifndef MODULE_NAME
#define MODULE_NAME Crypto
#endif

#include "../core/core.h"

#undef EXTERNAL

#ifdef __WIN32__
#ifdef __MODULE_CRYPTALGO__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_CRYPTALGO_H


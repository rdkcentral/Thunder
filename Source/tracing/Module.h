#ifndef __MODULE_TRACING_H
#define __MODULE_TRACING_H

#ifndef MODULE_NAME
#define MODULE_NAME Tracing
#endif

#include "../core/core.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef __MODULE_TRACING__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_TRACING_H

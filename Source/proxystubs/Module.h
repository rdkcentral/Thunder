#ifndef __MODULE_PROXYSTUBS_H
#define __MODULE_PROXYSTUBS_H

#ifndef MODULE_NAME
#define MODULE_NAME ProxyStubs
#endif

#include "../core/core.h"
#include "../com/com.h"
#include "../plugins/plugins.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef __MODULE_PROXYSTUBS__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_PROXYSTUB_H


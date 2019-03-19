#ifndef __MODULE_PLUGINS_H
#define __MODULE_PLUGINS_H

#ifndef MODULE_NAME
#define MODULE_NAME Plugins
#endif

#include "../com/com.h"
#include "../core/core.h"
#include "../cryptalgo/cryptalgo.h"
#include "../tracing/tracing.h"
#include "../websocket/websocket.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef PLUGINS_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_PLUGINS_H

#ifndef __MODULE_PROTOCOLS_H__
#define __MODULE_PROTOCOLS_H__

#ifndef MODULE_NAME
#define MODULE_NAME WPEFramework_Protocols
#endif

#include "../core/core.h"
#include "../tracing/tracing.h"
#include "../cryptalgo/cryptalgo.h"

#ifdef __WIN32__
#include "../websocket/windows/include/zlib.h"
#else
#include "zlib.h"
#endif

#endif // __MODULE_PROTOCOLS_H__

#ifndef __MODULE_PROTOCOLS_H__
#define __MODULE_PROTOCOLS_H__

#ifndef MODULE_NAME
#define MODULE_NAME Protocols
#endif

#include "../com/Ids.h"
#include "../core/core.h"
#include "../cryptalgo/cryptalgo.h"
#include "../tracing/tracing.h"

#ifdef __WIN32__
#include "../websocket/windows/include/zlib.h"
#else
#include "zlib.h"
#endif

#endif // __MODULE_PROTOCOLS_H__

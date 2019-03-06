#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME JSONRPC
#endif

#include "../core/core.h"
#include "../websocket/websocket.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef __MODULE_JSONRPC__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

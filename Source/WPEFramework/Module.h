#ifndef __MODULE_FRAMEWORK_APPLICATION_H
#define __MODULE_FRAMEWORK_APPLICATION_H

#ifndef MODULE_NAME
#define MODULE_NAME Application
#endif

#include "../core/core.h"
#include "../cryptalgo/cryptalgo.h"
#include "../plugins/plugins.h"
#include "../tracing/tracing.h"
#include "../websocket/websocket.h"

#undef EXTERNAL
#define EXTERNAL

#ifndef TREE_REFERENCE
#define TREE_REFERENCE engineering_build_for_debug_purpose_only
#endif

#endif // __MODULE_FRAMEWORK_APPLICATION_H

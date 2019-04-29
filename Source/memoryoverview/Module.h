#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME MemoryOverview
#endif

#include "../core/core.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef MEMORYOVERVIEW_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME ProcessContainers
#endif

#include "../core/core.h"

#ifdef __WINDOWS__
#undef EXTERNAL
#ifdef CONTAINERS_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

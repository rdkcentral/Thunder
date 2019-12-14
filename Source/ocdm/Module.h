#ifndef __MODULE_OCDMINTERFACE_H
#define __MODULE_OCDMINTERFACE_H

#ifndef MODULE_NAME
#define MODULE_NAME OpenCDM
#endif

#include "../core/core.h"
#include "../com/com.h"

#ifdef __WINDOWS__
#undef EXTERNAL
#ifdef OCDM_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_OCDMINTERFACE_H

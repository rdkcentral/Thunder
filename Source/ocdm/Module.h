#ifndef __MODULE_OCDMINTERFACE_H
#define __MODULE_OCDMINTERFACE_H

#ifndef MODULE_NAME
#define MODULE_NAME OpenCDM
#endif

#include "../core/core.h"
#include "../com/com.h"

#ifdef __WIN32__
#undef EXTERNAL
#ifdef __MODULE_OCDM__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_OCDMINTERFACE_H

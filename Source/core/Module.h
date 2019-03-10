#ifndef __MODULE_CORE_H
#define __MODULE_CORE_H

#ifndef MODULE_NAME
#define MODULE_NAME Core
#endif

#ifdef _WINDOWS
#ifdef CORE_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#endif
#else
#define EXTERNAL
#endif

#endif // __MODULE_CORE_H

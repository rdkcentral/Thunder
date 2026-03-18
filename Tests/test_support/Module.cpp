// Module definition for the thunder_test_support static library.
// MODULE_NAME is set to ThunderTestRuntime via -D in CMakeLists.txt,
// which overrides Source/Thunder/Module.h's default of "Application".
// This ensures all server sources and the MODULE_NAME_DECLARATION below
// use the same symbol, and trace output shows "ThunderTestRuntime".

#ifndef MODULE_NAME
#define MODULE_NAME ThunderTestRuntime
#endif

#include <core/core.h>

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

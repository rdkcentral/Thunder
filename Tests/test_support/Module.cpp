// Module definition for the thunder_test_support static library.
//
// Uses MODULE_NAME_ARCHIVE_DECLARATION instead of MODULE_NAME_DECLARATION
// because this is a static archive, not a standalone binary. The archive
// macro only defines the MODULE_NAME string symbol. The full declaration
// (ModuleBuildRef, GetModuleServices, SetModuleServices) is left to the
// consumer's own MODULE_NAME_DECLARATION, avoiding duplicate definitions.

#define MODULE_NAME ThunderTestRuntime
#include <core/core.h>

MODULE_NAME_ARCHIVE_DECLARATION

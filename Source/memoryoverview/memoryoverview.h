#pragma once

#include "Module.h"

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef MEMORYOVERVIEW_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "memoryoverview.lib")
#endif
#else
#define EXTERNAL
#endif

namespace WPEFramework {
namespace MemoryOverview {

class MemoryOverview
{
public:

static void ClearOSCaches();

static std::string GetMeminfo()  {
    return Core::SystemInfo::Instance().TakeMemorySnapshot().AsJSON();
}

static std::string GetDependencies();

};

}
}
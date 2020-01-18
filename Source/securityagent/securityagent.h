#pragma once

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef SECURITYAGENT_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "securityagent.lib")
#endif
#endif

#include "SecurityToken.h"
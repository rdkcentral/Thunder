#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Definitions
#endif

#include <core/Enumerate.h>

#include "IComposition.h"
#include "IStream.h"
#include "IVoiceHandler.h"

#undef EXTERNAL

#ifdef __WIN32__
#ifdef DEFINITIONS_EXPORTS
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL EXTERNAL_IMPORT
#pragma comment(lib, "definitions.lib")
#endif
#else
#define EXTERNAL
#endif

namespace WPEFramework {

	ENUM_CONVERSION_HANDLER(Exchange::IComposition::ScreenResolution);
	ENUM_CONVERSION_HANDLER(Exchange::IStream::streamtype);
	ENUM_CONVERSION_HANDLER(Exchange::IStream::state);
}

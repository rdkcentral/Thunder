#pragma once

#ifndef MODULE_NAME
#define MODULE_NAME Definitions
#endif

#include <core/Enumerate.h>

#include "IComposition.h"
#include "IStream.h"

#include "json/JsonData_NetworkControl.h"
#include "json/JsonData_RemoteControl.h"
#include "json/JsonData_TestUtility.h"
#include "json/JsonData_WifiControl.h"
#include "json/JsonData_Messenger.h"
#include "json/JsonData_Streamer.h"
#include "json/JsonData_StateControl.h"
#include "json/JsonData_Browser.h"

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

	ENUM_CONVERSION_HANDLER(JsonData::NetworkControl::NetworkResultData::ModeType);
	ENUM_CONVERSION_HANDLER(JsonData::TestUtility::InputInfo::TypeType);
	ENUM_CONVERSION_HANDLER(JsonData::RemoteControl::ModifiersType);
	ENUM_CONVERSION_HANDLER(JsonData::WifiControl::TypeType);
	ENUM_CONVERSION_HANDLER(JsonData::Messenger::RoomupdateParamsData::ActionType);
	ENUM_CONVERSION_HANDLER(JsonData::Messenger::UserupdateParamsData::ActionType);
	ENUM_CONVERSION_HANDLER(JsonData::StateControl::StateType);
	ENUM_CONVERSION_HANDLER(JsonData::Browser::VisibilityType);

	ENUM_CONVERSION_HANDLER(JsonData::Streamer::TypeType);
	ENUM_CONVERSION_HANDLER(JsonData::Streamer::StateType);
	ENUM_CONVERSION_HANDLER(JsonData::Streamer::DrmType);
}

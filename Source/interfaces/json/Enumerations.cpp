#include"../definitions.h"

namespace WPEFramework {

// NetworkControl

ENUM_CONVERSION_BEGIN(JsonData::NetworkControl::NetworkResultData::ModeType)
    { JsonData::NetworkControl::NetworkResultData::ModeType::MANUAL, _TXT("Manual") },
    { JsonData::NetworkControl::NetworkResultData::ModeType::STATIC, _TXT("Static") },
    { JsonData::NetworkControl::NetworkResultData::ModeType::DYNAMIC, _TXT("Dynamic") },
ENUM_CONVERSION_END(JsonData::NetworkControl::NetworkResultData::ModeType);

// RemoteControl

ENUM_CONVERSION_BEGIN(JsonData::RemoteControl::ModifiersType)
    { JsonData::RemoteControl::ModifiersType::LEFTSHIFT, _TXT("leftshift") },
    { JsonData::RemoteControl::ModifiersType::RIGHTSHIFT, _TXT("rightshift") },
    { JsonData::RemoteControl::ModifiersType::LEFTALT, _TXT("leftalt") },
    { JsonData::RemoteControl::ModifiersType::RIGHTALT, _TXT("rightalt") },
    { JsonData::RemoteControl::ModifiersType::LEFTCTRL, _TXT("leftctrl") },
    { JsonData::RemoteControl::ModifiersType::RIGHTCTRL, _TXT("rightctrl") },
ENUM_CONVERSION_END(JsonData::RemoteControl::ModifiersType);

// TestUtility

ENUM_CONVERSION_BEGIN(JsonData::TestUtility::InputInfo::TypeType)
    { JsonData::TestUtility::InputInfo::TypeType::NUMBER, _TXT("Number") },
    { JsonData::TestUtility::InputInfo::TypeType::STRING, _TXT("String") },
    { JsonData::TestUtility::InputInfo::TypeType::BOOLEAN, _TXT("Boolean") },
    { JsonData::TestUtility::InputInfo::TypeType::OBJECT, _TXT("Object") },
    { JsonData::TestUtility::InputInfo::TypeType::SYMBOL, _TXT("Symbol") },
ENUM_CONVERSION_END(JsonData::TestUtility::InputInfo::TypeType);

// WifiControl

ENUM_CONVERSION_BEGIN(JsonData::WifiControl::TypeType)
    { JsonData::WifiControl::TypeType::UNKNOWN, _TXT("Unknown") },
    { JsonData::WifiControl::TypeType::UNSECURE, _TXT("Unsecure") },
    { JsonData::WifiControl::TypeType::WPA, _TXT("WPA") },
    { JsonData::WifiControl::TypeType::ENTERPRISE, _TXT("Enterprise") },
ENUM_CONVERSION_END(JsonData::WifiControl::TypeType);

// Messenger

ENUM_CONVERSION_BEGIN(JsonData::Messenger::RoomupdateParamsData::ActionType)
    { JsonData::Messenger::RoomupdateParamsData::ActionType::CREATED, _TXT("created") },
    { JsonData::Messenger::RoomupdateParamsData::ActionType::DESTROYED, _TXT("destroyed") },
ENUM_CONVERSION_END(JsonData::Messenger::RoomupdateParamsData::ActionType);

ENUM_CONVERSION_BEGIN(JsonData::Messenger::UserupdateParamsData::ActionType)
    { JsonData::Messenger::UserupdateParamsData::ActionType::JOINED, _TXT("joined") },
    { JsonData::Messenger::UserupdateParamsData::ActionType::LEFT, _TXT("left") },
ENUM_CONVERSION_END(JsonData::Messenger::UserupdateParamsData::ActionType);

// Streamer

ENUM_CONVERSION_BEGIN(JsonData::Streamer::TypeType)
    { JsonData::Streamer::TypeType::STUBBED, _TXT("stubbed") },
    { JsonData::Streamer::TypeType::DVB, _TXT("dvb") },
    { JsonData::Streamer::TypeType::ATSC, _TXT("atsc") },
    { JsonData::Streamer::TypeType::VOD, _TXT("vod") },
ENUM_CONVERSION_END(JsonData::Streamer::TypeType);

ENUM_CONVERSION_BEGIN(JsonData::Streamer::DrmType)
    { JsonData::Streamer::DrmType::UNKNOWN, _TXT("unknown") },
    { JsonData::Streamer::DrmType::CLEARKEY, _TXT("clearkey") },
    { JsonData::Streamer::DrmType::PLAYREADY, _TXT("playready") },
    { JsonData::Streamer::DrmType::WIDEVINE, _TXT("widevine") },
ENUM_CONVERSION_END(JsonData::Streamer::DrmType);

ENUM_CONVERSION_BEGIN(JsonData::Streamer::StateType)
    { JsonData::Streamer::StateType::IDLE, _TXT("idle") },
    { JsonData::Streamer::StateType::LOADING, _TXT("loading") },
    { JsonData::Streamer::StateType::PREPARED, _TXT("prepared") },
    { JsonData::Streamer::StateType::PAUSED, _TXT("paused") },
    { JsonData::Streamer::StateType::PLAYING, _TXT("playing") },
    { JsonData::Streamer::StateType::ERROR, _TXT("error") },
ENUM_CONVERSION_END(JsonData::Streamer::StateType);

// StateControl

ENUM_CONVERSION_BEGIN(JsonData::StateControl::StateType)
    { JsonData::StateControl::StateType::RESUMED, _TXT("resumed") },
    { JsonData::StateControl::StateType::SUSPENDED, _TXT("suspended") },
ENUM_CONVERSION_END(JsonData::StateControl::StateType);

// Browser

ENUM_CONVERSION_BEGIN(JsonData::Browser::VisibilityType)
    { JsonData::Browser::VisibilityType::VISIBLE, _TXT("visible") },
    { JsonData::Browser::VisibilityType::HIDDEN, _TXT("hidden") },
ENUM_CONVERSION_END(JsonData::Browser::VisibilityType);

}

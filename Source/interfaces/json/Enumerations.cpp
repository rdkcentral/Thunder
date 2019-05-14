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

ENUM_CONVERSION_BEGIN(JsonData::TestUtility::InputInfo::ParamType)
    { JsonData::TestUtility::InputInfo::ParamType::NUMBER, _TXT("Number") },
    { JsonData::TestUtility::InputInfo::ParamType::STRING, _TXT("String") },
    { JsonData::TestUtility::InputInfo::ParamType::BOOLEAN, _TXT("Boolean") },
    { JsonData::TestUtility::InputInfo::ParamType::OBJECT, _TXT("Object") },
    { JsonData::TestUtility::InputInfo::ParamType::SYMBOL, _TXT("Symbol") },
ENUM_CONVERSION_END(JsonData::TestUtility::InputInfo::ParamType);

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
ENUM_CONVERSION_BEGIN(JsonData::Streamer::TypeResultData::StreamType)
    { JsonData::Streamer::TypeResultData::StreamType::STUBBED, _TXT("Stubbed") },
    { JsonData::Streamer::TypeResultData::StreamType::DVB, _TXT("DVB") },
    { JsonData::Streamer::TypeResultData::StreamType::VOD, _TXT("VOD") },
ENUM_CONVERSION_END(JsonData::Streamer::TypeResultData::StreamType);

ENUM_CONVERSION_BEGIN(JsonData::Streamer::DRMResultData::DrmType)
    { JsonData::Streamer::DRMResultData::DrmType::UNKNOWN, _TXT("UnKnown") },
    { JsonData::Streamer::DRMResultData::DrmType::CLEARKEY, _TXT("ClearKey") },
    { JsonData::Streamer::DRMResultData::DrmType::PLAYREADY, _TXT("PlayReady") },
    { JsonData::Streamer::DRMResultData::DrmType::WIDEVINE, _TXT("Widevine") },
ENUM_CONVERSION_END(JsonData::Streamer::DRMResultData::DrmType);

ENUM_CONVERSION_BEGIN(JsonData::Streamer::StateResultData::StateType)
    { JsonData::Streamer::StateResultData::StateType::IDLE, _TXT("Idle") },
    { JsonData::Streamer::StateResultData::StateType::LOADING, _TXT("Loading") },
    { JsonData::Streamer::StateResultData::StateType::PREPARED, _TXT("Prepared") },
    { JsonData::Streamer::StateResultData::StateType::PAUSED, _TXT("Paused") },
    { JsonData::Streamer::StateResultData::StateType::PLAYING, _TXT("Playing") },
    { JsonData::Streamer::StateResultData::StateType::ERROR, _TXT("Error") },
ENUM_CONVERSION_END(JsonData::Streamer::StateResultData::StateType);

}

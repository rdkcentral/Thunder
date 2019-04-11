
// C++ classes for Remote Control JSON-RPC API.
// Generated automatically from 'RemoteControlAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace RemoteControl {

        // Common enums
        //

        // Key modifiers
        enum ModifiersType {
            LEFTSHIFT = 1,
            RIGHTSHIFT = 2,
            LEFTALT = 4,
            RIGHTALT = 8,
            LEFTCTRL = 16,
            RIGHTCTRL = 32
        };

        // Common classes
        //

        class DeviceParamsInfo : public Core::JSON::Container {
        public:
            DeviceParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
            }

            DeviceParamsInfo(const DeviceParamsInfo&) = delete;
            DeviceParamsInfo& operator=(const DeviceParamsInfo&) = delete;

        public:
            Core::JSON::String Device; // Device name
        }; // class DeviceParamsInfo

        class KeyParamsInfo : public Core::JSON::Container {
        public:
            KeyParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
                Add(_T("code"), &Code);
            }

            KeyParamsInfo(const KeyParamsInfo&) = delete;
            KeyParamsInfo& operator=(const KeyParamsInfo&) = delete;

        public:
            Core::JSON::String Device; // Device name
            Core::JSON::DecUInt32 Code; // Key code
        }; // class KeyParamsInfo

        class RcinfoInfo : public Core::JSON::Container {
        public:
            RcinfoInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
                Add(_T("code"), &Code);
                Add(_T("key"), &Key);
                Add(_T("modifiers"), &Modifiers);
            }

            RcinfoInfo(const RcinfoInfo&) = delete;
            RcinfoInfo& operator=(const RcinfoInfo&) = delete;

        public:
            Core::JSON::String Device; // Device name
            Core::JSON::DecUInt32 Code; // Key code
            Core::JSON::DecUInt16 Key; // Key ingest code
            Core::JSON::ArrayType<Core::JSON::EnumType<ModifiersType>> Modifiers;
        }; // class RcinfoInfo

        // Method params/result classes
        //

        class DeviceResultData : public Core::JSON::Container {
        public:
            DeviceResultData()
                : Core::JSON::Container()
            {
                Add(_T("name"), &Name);
                Add(_T("metadata"), &Metadata);
            }

            DeviceResultData(const DeviceResultData&) = delete;
            DeviceResultData& operator=(const DeviceResultData&) = delete;

        public:
            Core::JSON::String Name; // Device name
            Core::JSON::String Metadata; // Device metadata
        }; // class DeviceResultData

        class KeyResultData : public Core::JSON::Container {
        public:
            KeyResultData()
                : Core::JSON::Container()
            {
                Add(_T("code"), &Code);
                Add(_T("key"), &Key);
                Add(_T("modifiers"), &Modifiers);
            }

            KeyResultData(const KeyResultData&) = delete;
            KeyResultData& operator=(const KeyResultData&) = delete;

        public:
            Core::JSON::DecUInt32 Code; // Key code
            Core::JSON::DecUInt16 Key; // Key ingest code
            Core::JSON::ArrayType<Core::JSON::EnumType<ModifiersType>> Modifiers;
        }; // class KeyResultData

        class UnpairParamsData : public Core::JSON::Container {
        public:
            UnpairParamsData()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
                Add(_T("bindid"), &Bindid);
            }

            UnpairParamsData(const UnpairParamsData&) = delete;
            UnpairParamsData& operator=(const UnpairParamsData&) = delete;

        public:
            Core::JSON::String Device; // Device name
            Core::JSON::String Bindid; // Binding id
        }; // class UnpairParamsData

    } // namespace RemoteControl

} // namespace JsonData

}


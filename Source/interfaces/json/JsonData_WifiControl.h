
// C++ classes for WiFi Control JSON-RPC API.
// Generated automatically from 'WifiControlAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace WifiControl {

        // Common enums
        //

        // Level of security
        enum class TypeType {
            UNKNOWN,
            UNSECURE,
            WPA,
            ENTERPRISE
        };

        // Common classes
        //

        class ConfigInfo : public Core::JSON::Container {
        public:
            ConfigInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            ConfigInfo(const ConfigInfo& other)
                : Core::JSON::Container()
                , Ssid(other.Ssid)
                , Type(other.Type)
                , Hidden(other.Hidden)
                , Accesspoint(other.Accesspoint)
                , Psk(other.Psk)
                , Hash(other.Hash)
                , Identity(other.Identity)
                , Password(other.Password)
            {
                Init();
            }

            ConfigInfo& operator=(const ConfigInfo& rhs)
            {
                Ssid = rhs.Ssid;
                Type = rhs.Type;
                Hidden = rhs.Hidden;
                Accesspoint = rhs.Accesspoint;
                Psk = rhs.Psk;
                Hash = rhs.Hash;
                Identity = rhs.Identity;
                Password = rhs.Password;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("ssid"), &Ssid);
                Add(_T("type"), &Type);
                Add(_T("hidden"), &Hidden);
                Add(_T("accesspoint"), &Accesspoint);
                Add(_T("psk"), &Psk);
                Add(_T("hash"), &Hash);
                Add(_T("identity"), &Identity);
                Add(_T("password"), &Password);
            }

        public:
            Core::JSON::String Ssid; // Identifier of a network (32-bytes long)
            Core::JSON::EnumType<TypeType> Type; // Level of security
            Core::JSON::Boolean Hidden; // Indicates whether a network is hidden
            Core::JSON::Boolean Accesspoint; // Indicates if the network operates in AP mode
            Core::JSON::String Psk; // Network's PSK in plaintext (irrelevant if hash is provided)
            Core::JSON::String Hash; // Network's PSK as a hash
            Core::JSON::String Identity; // User credentials (username part) for EAP
            Core::JSON::String Password; // User credentials (password part) for EAP
        }; // class ConfigInfo

        class ConfigParamsInfo : public Core::JSON::Container {
        public:
            ConfigParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("ssid"), &Ssid);
            }

            ConfigParamsInfo(const ConfigParamsInfo&) = delete;
            ConfigParamsInfo& operator=(const ConfigParamsInfo&) = delete;

        public:
            Core::JSON::String Ssid; // Identifier of a network (32-bytes long)
        }; // class ConfigParamsInfo

        class PairsInfo : public Core::JSON::Container {
        public:
            PairsInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            PairsInfo(const PairsInfo& other)
                : Core::JSON::Container()
                , Method(other.Method)
                , Keys(other.Keys)
            {
                Init();
            }

            PairsInfo& operator=(const PairsInfo& rhs)
            {
                Method = rhs.Method;
                Keys = rhs.Keys;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("method"), &Method);
                Add(_T("keys"), &Keys);
            }

        public:
            Core::JSON::String Method; // Encryption method used by a network
            Core::JSON::ArrayType<Core::JSON::String> Keys;
        }; // class PairsInfo

        class NetworkInfo : public Core::JSON::Container {
        public:
            NetworkInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            NetworkInfo(const NetworkInfo& other)
                : Core::JSON::Container()
                , Ssid(other.Ssid)
                , Pairs(other.Pairs)
                , Bssid(other.Bssid)
                , Frequency(other.Frequency)
                , Signal(other.Signal)
            {
                Init();
            }

            NetworkInfo& operator=(const NetworkInfo& rhs)
            {
                Ssid = rhs.Ssid;
                Pairs = rhs.Pairs;
                Bssid = rhs.Bssid;
                Frequency = rhs.Frequency;
                Signal = rhs.Signal;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("ssid"), &Ssid);
                Add(_T("pairs"), &Pairs);
                Add(_T("bssid"), &Bssid);
                Add(_T("frequency"), &Frequency);
                Add(_T("signal"), &Signal);
            }

        public:
            Core::JSON::String Ssid; // Identifier of a network (32-bytes long)
            Core::JSON::ArrayType<PairsInfo> Pairs;
            Core::JSON::String Bssid; // 48-bits long BSS' identifier (might be MAC format)
            Core::JSON::DecUInt32 Frequency; // Network's frequency in MHz
            Core::JSON::DecUInt32 Signal; // Network's signal level in dBm
        }; // class NetworkInfo

        // Method params/result classes
        //

        class DebugParamsData : public Core::JSON::Container {
        public:
            DebugParamsData()
                : Core::JSON::Container()
            {
                Add(_T("level"), &Level);
            }

            DebugParamsData(const DebugParamsData&) = delete;
            DebugParamsData& operator=(const DebugParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Level; // Debul level
        }; // class DebugParamsData

        class SetconfigParamsData : public Core::JSON::Container {
        public:
            SetconfigParamsData()
                : Core::JSON::Container()
            {
                Add(_T("ssid"), &Ssid);
                Add(_T("config"), &Config);
            }

            SetconfigParamsData(const SetconfigParamsData&) = delete;
            SetconfigParamsData& operator=(const SetconfigParamsData&) = delete;

        public:
            Core::JSON::String Ssid; // Identifier of a network (32-bytes long)
            ConfigInfo Config;
        }; // class SetconfigParamsData

        class StatusResultData : public Core::JSON::Container {
        public:
            StatusResultData()
                : Core::JSON::Container()
            {
                Add(_T("connected"), &Connected);
                Add(_T("scanning"), &Scanning);
            }

            StatusResultData(const StatusResultData&) = delete;
            StatusResultData& operator=(const StatusResultData&) = delete;

        public:
            Core::JSON::String Connected; // Identifier of the connected network (32-bytes long)
            Core::JSON::Boolean Scanning; // Indicates whether a scanning for available network is in progress
        }; // class StatusResultData

    } // namespace WifiControl

} // namespace JsonData

}


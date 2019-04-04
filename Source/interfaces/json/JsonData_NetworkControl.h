
// C++ classes for Network Control JSON-RPC API.
// Generated automatically from 'NetworkControlAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace NetworkControl {

        // Common classes
        //

        class NetworkParamsInfo : public Core::JSON::Container {
        public:
            NetworkParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("device"), &Device);
            }

            NetworkParamsInfo(const NetworkParamsInfo&) = delete;
            NetworkParamsInfo& operator=(const NetworkParamsInfo&) = delete;

        public:
            Core::JSON::String Device; // Network interface
        }; // class NetworkParamsInfo

        // Method params/result classes
        //

        class NetworkResultData : public Core::JSON::Container {
        public:
            // Mode type
            enum ModeType {
                MANUAL,
                STATIC,
                DYNAMIC
            };

            NetworkResultData()
                : Core::JSON::Container()
                , Interface()
                , Mode()
                , Address()
                , Mask()
                , Gateway()
                , Broadcast()
            {
                Init();
            }

            NetworkResultData(const NetworkResultData& other)
                : Core::JSON::Container()
                , Interface(other.Interface)
                , Mode(other.Mode)
                , Address(other.Address)
                , Mask(other.Mask)
                , Gateway(other.Gateway)
                , Broadcast(other.Broadcast)
            {
                Init();
            }

            NetworkResultData& operator=(const NetworkResultData& rhs)
            {
                Interface = rhs.Interface;
                Mode = rhs.Mode;
                Address = rhs.Address;
                Mask = rhs.Mask;
                Gateway = rhs.Gateway;
                Broadcast = rhs.Broadcast;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("interface"), &Interface);
                Add(_T("mode"), &Mode);
                Add(_T("address"), &Address);
                Add(_T("mask"), &Mask);
                Add(_T("gateway"), &Gateway);
                Add(_T("broadcast"), &Broadcast);
            }

        public:
            Core::JSON::String Interface; // Interface name
            Core::JSON::EnumType<ModeType> Mode; // Mode type
            Core::JSON::String Address; // IP address
            Core::JSON::DecUInt8 Mask; // Network inteface mask
            Core::JSON::String Gateway; // Gateway address
            Core::JSON::String Broadcast; // Broadcast IP
        }; // class NetworkResultData

    } // namespace NetworkControl

} // namespace JsonData

} // namespace WPEFramework


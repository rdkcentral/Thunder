
// C++ classes for SecureShellServer JSON-RPC API.
// Generated automatically from 'SecureShellServerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace SecureShellServer {

        // Method params/result classes
        //

        class CloseclientsessionParamsData : public Core::JSON::Container {
        public:
            CloseclientsessionParamsData()
                : Core::JSON::Container()
            {
                Add(_T("clientpid"), &ClientPid);
            }

            CloseclientsessionParamsData(const CloseclientsessionParamsData&) = delete;
            CloseclientsessionParamsData& operator=(const CloseclientsessionParamsData&) = delete;

        public:
            Core::JSON::String ClientPid;
        }; // class CloseclientsessionParamsData

        class SessioninfoResultData : public Core::JSON::Container {
        public:
            SessioninfoResultData()
                : Core::JSON::Container()
            {
		Init();
            }

            SessioninfoResultData(const SessioninfoResultData& other)
                : Core::JSON::Container()
                , Pid(other.Pid)
                , IpAddress(other.IpAddress)
                , TimeStamp(other.TimeStamp)
            {
                Init();
            }

            SessioninfoResultData& operator=(const SessioninfoResultData& rhs)
            {
                Pid = rhs.Pid;
                IpAddress = rhs.IpAddress;
                TimeStamp = rhs.TimeStamp;
                return (*this);
            }

            void Init()
            {
                Add(_T("pid"), &Pid);
                Add(_T("ipaddress"), &IpAddress);
                Add(_T("timestamp"), &TimeStamp);
            }

        public:
            Core::JSON::String Pid; // Port on which DHCP service shall run
            Core::JSON::String IpAddress; // Port on which DHCP service shall run
            Core::JSON::String TimeStamp; // Port on which DHCP service shall run
        }; // class SessioninfoResultData

    } // namespace SecureShellServer

} // namespace JsonData

}


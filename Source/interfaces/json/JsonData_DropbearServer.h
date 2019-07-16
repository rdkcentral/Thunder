
// C++ classes for Dropbear Server JSON-RPC API.
// Generated automatically from 'DropbearServerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace DropbearServer {

        // Method params/result classes
        //

        class StartserviceParamsData : public Core::JSON::Container {
        public:
            StartserviceParamsData()
                : Core::JSON::Container()
            {
                Add(_T("hostkeys"), &HostKeys);
                Add(_T("portflag"), &PortFlag);
                Add(_T("port"), &Port);
            }

            StartserviceParamsData(const StartserviceParamsData&) = delete;
            StartserviceParamsData& operator=(const StartserviceParamsData&) = delete;

        public:
            Core::JSON::String Port; // Port on which DHCP service shall run
            Core::JSON::String PortFlag; // Flag to enable user defined port number to use
            Core::JSON::String HostKeys; // Falg to run the DHCP service in background mode
        }; // class StartserviceParamsData

        class CloseclientsessionParamsData : public Core::JSON::Container {
        public:
            CloseclientsessionParamsData()
                : Core::JSON::Container()
            {
                Add(_T("pid"), &Pid);
            }

            CloseclientsessionParamsData(const CloseclientsessionParamsData&) = delete;
            CloseclientsessionParamsData& operator=(const CloseclientsessionParamsData&) = delete;

        public:
            Core::JSON::String Pid; // Port on which DHCP service shall run
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
                IpAdress = rhs.IpAddress;
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

    } // namespace DropbearServer

} // namespace JsonData

}


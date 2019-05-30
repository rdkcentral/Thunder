
// C++ classes for Device Info JSON-RPC API.
// Generated automatically from 'DeviceInfoAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace DeviceInfo {

        // Method params/result classes
        //

        class AddressesParamsData : public Core::JSON::Container {
        public:
            AddressesParamsData()
                : Core::JSON::Container()
            {
                Init();
            }

            AddressesParamsData(const AddressesParamsData& other)
                : Core::JSON::Container()
                , Name(other.Name)
                , Mac(other.Mac)
                , Ip(other.Ip)
            {
                Init();
            }

            AddressesParamsData& operator=(const AddressesParamsData& rhs)
            {
                Name = rhs.Name;
                Mac = rhs.Mac;
                Ip = rhs.Ip;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("name"), &Name);
                Add(_T("mac"), &Mac);
                Add(_T("ip"), &Ip);
            }

        public:
            Core::JSON::String Name; // Interface name
            Core::JSON::String Mac; // Interface MAC address
            Core::JSON::ArrayType<Core::JSON::String> Ip;
        }; // class AddressesParamsData

        class SocketinfoParamsData : public Core::JSON::Container {
        public:
            SocketinfoParamsData()
                : Core::JSON::Container()
            {
                Add(_T("total"), &Total);
                Add(_T("open"), &Open);
                Add(_T("link"), &Link);
                Add(_T("exception"), &Exception);
                Add(_T("shutdown"), &Shutdown);
                Add(_T("runs"), &Runs);
            }

            SocketinfoParamsData(const SocketinfoParamsData&) = delete;
            SocketinfoParamsData& operator=(const SocketinfoParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Total;
            Core::JSON::DecUInt32 Open;
            Core::JSON::DecUInt32 Link;
            Core::JSON::DecUInt32 Exception;
            Core::JSON::DecUInt32 Shutdown;
            Core::JSON::DecUInt32 Runs; // Number of runs
        }; // class SocketinfoParamsData

        class SysteminfoParamsData : public Core::JSON::Container {
        public:
            SysteminfoParamsData()
                : Core::JSON::Container()
            {
                Add(_T("version"), &Version);
                Add(_T("uptime"), &Uptime);
                Add(_T("totalram"), &Totalram);
                Add(_T("freeram"), &Freeram);
                Add(_T("devicename"), &Devicename);
                Add(_T("cpuload"), &Cpuload);
                Add(_T("totalgpuram"), &Totalgpuram);
                Add(_T("freegpuram"), &Freegpuram);
                Add(_T("serialnumber"), &Serialnumber);
                Add(_T("deviceid"), &Deviceid);
                Add(_T("time"), &Time);
            }

            SysteminfoParamsData(const SysteminfoParamsData&) = delete;
            SysteminfoParamsData& operator=(const SysteminfoParamsData&) = delete;

        public:
            Core::JSON::String Version; // Software version (in form "version#hashtag")
            Core::JSON::DecUInt64 Uptime; // System uptime (in seconds)
            Core::JSON::DecUInt64 Totalram; // Total installed system RAM memory (in bytes)
            Core::JSON::DecUInt64 Freeram; // Free system RAM memory (in bytes)
            Core::JSON::String Devicename; // Host name
            Core::JSON::String Cpuload; // Current CPU load (percentage)
            Core::JSON::DecUInt64 Totalgpuram; // Total GPU DRAM memory (in bytes)
            Core::JSON::DecUInt64 Freegpuram; // Free GPU DRAM memory (in bytes)
            Core::JSON::String Serialnumber; // Device serial number
            Core::JSON::String Deviceid; // Device ID
            Core::JSON::String Time; // Current system date and time
        }; // class SysteminfoParamsData

    } // namespace DeviceInfo

} // namespace JsonData

}


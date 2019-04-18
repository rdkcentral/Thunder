
// C++ classes for Controller JSON-RPC API.
// Generated automatically from 'ControllerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>
#include "../PluginServer.h"
#include "../Probe.h"

namespace WPEFramework {

namespace JsonData {

    namespace Controller {

        // Common classes
        //

        class ActivateParamsInfo : public Core::JSON::Container {
        public:
            ActivateParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
            }

            ActivateParamsInfo(const ActivateParamsInfo&) = delete;
            ActivateParamsInfo& operator=(const ActivateParamsInfo&) = delete;

        public:
            Core::JSON::String Callsign; // Plugin callsign
        }; // class ActivateParamsInfo

        // Method params/result classes
        //

        class AllParamsData : public Core::JSON::Container {
        public:
            AllParamsData()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("data"), &Data);
            }

            AllParamsData(const AllParamsData&) = delete;
            AllParamsData& operator=(const AllParamsData&) = delete;

        public:
            Core::JSON::String Callsign; // Callsign of the originator plugin of the event
            Core::JSON::Variant Data; // Object that was broadcasted as an event by the originator plugin
        }; // class AllParamsData

        class DeleteParamsData : public Core::JSON::Container {
        public:
            DeleteParamsData()
                : Core::JSON::Container()
            {
                Add(_T("path"), &Path);
            }

            DeleteParamsData(const DeleteParamsData&) = delete;
            DeleteParamsData& operator=(const DeleteParamsData&) = delete;

        public:
            Core::JSON::String Path; // Path to the file or directory (within the persistent storage) to delete
        }; // class DeleteParamsData

        class DownloadParamsData : public Core::JSON::Container {
        public:
            DownloadParamsData()
                : Core::JSON::Container()
            {
                Add(_T("source"), &Source);
                Add(_T("destination"), &Destination);
                Add(_T("hash"), &Hash);
            }

            DownloadParamsData(const DownloadParamsData&) = delete;
            DownloadParamsData& operator=(const DownloadParamsData&) = delete;

        public:
            Core::JSON::String Source; // Source URL pointing to the file to download
            Core::JSON::String Destination; // Path within the persistent storage where to save the file
            Core::JSON::String Hash; // Base64-encoded binary SHA256 signature for authenticity verification
        }; // class DownloadParamsData

        class DownloadcompletedParamsData : public Core::JSON::Container {
        public:
            DownloadcompletedParamsData()
                : Core::JSON::Container()
            {
                Add(_T("source"), &Source);
            }

            DownloadcompletedParamsData(const DownloadcompletedParamsData&) = delete;
            DownloadcompletedParamsData& operator=(const DownloadcompletedParamsData&) = delete;

        public:
            Core::JSON::String Source; // Source URL identifying the downloaded file
        }; // class DownloadcompletedParamsData

        class ExistsParamsData : public Core::JSON::Container {
        public:
            ExistsParamsData()
                : Core::JSON::Container()
            {
                Add(_T("designator"), &Designator);
            }

            ExistsParamsData(const ExistsParamsData&) = delete;
            ExistsParamsData& operator=(const ExistsParamsData&) = delete;

        public:
            Core::JSON::String Designator; // Method designator; if callsign is omitted then the Controller itself will be queried
        }; // class ExistsParamsData

        class GetconfigParamsData : public Core::JSON::Container {
        public:
            GetconfigParamsData()
                : Core::JSON::Container()
            {
                Add(_T("service"), &Service);
            }

            GetconfigParamsData(const GetconfigParamsData&) = delete;
            GetconfigParamsData& operator=(const GetconfigParamsData&) = delete;

        public:
            Core::JSON::String Service; // Name of the service to get the configuration of
        }; // class GetconfigParamsData

        class GetenvParamsData : public Core::JSON::Container {
        public:
            GetenvParamsData()
                : Core::JSON::Container()
            {
                Add(_T("variable"), &Variable);
            }

            GetenvParamsData(const GetenvParamsData&) = delete;
            GetenvParamsData& operator=(const GetenvParamsData&) = delete;

        public:
            Core::JSON::String Variable; // Name of the environment variable to get the value of
        }; // class GetenvParamsData

        class SetconfigParamsData : public Core::JSON::Container {
        public:
            SetconfigParamsData()
                : Core::JSON::Container()
            {
                Add(_T("service"), &Service);
                Add(_T("configuration"), &Configuration);
            }

            SetconfigParamsData(const SetconfigParamsData&) = delete;
            SetconfigParamsData& operator=(const SetconfigParamsData&) = delete;

        public:
            Core::JSON::String Service; // Name of the service to set the configuration of
            Core::JSON::String Configuration; // Configuration object to set
        }; // class SetconfigParamsData

       class StartdiscoveryParamsData : public Core::JSON::Container {
        public:
            StartdiscoveryParamsData()
                : Core::JSON::Container()
            {
                Add(_T("ttl"), &Ttl);
            }

            StartdiscoveryParamsData(const StartdiscoveryParamsData&) = delete;
            StartdiscoveryParamsData& operator=(const StartdiscoveryParamsData&) = delete;

        public:
            Core::JSON::DecUInt8 Ttl; // TTL (time to live) parameter for SSDP discovery
        }; // class StartdiscoveryParamsData

        class StatechangeParamsData : public Core::JSON::Container {
        public:
            StatechangeParamsData()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("state"), &State);
                Add(_T("reason"), &Reason);
            }

            StatechangeParamsData(const StatechangeParamsData&) = delete;
            StatechangeParamsData& operator=(const StatechangeParamsData&) = delete;

        public:
            Core::JSON::String Callsign; // Callsign of the plugin that changed state
            Core::JSON::EnumType<PluginHost::IShell::state> State; // State of the plugin
            Core::JSON::EnumType<PluginHost::IShell::reason> Reason; // Cause of the state change
        }; // class StatechangeParamsData

        class SubsystemsResultData : public Core::JSON::Container {
        public:
            SubsystemsResultData()
                : Core::JSON::Container()
            {
                Init();
            }

            SubsystemsResultData(const SubsystemsResultData& other)
                : Core::JSON::Container()
                , Subsystem(other.Subsystem)
                , Active(other.Active)
            {
                Init();
            }

            SubsystemsResultData& operator=(const SubsystemsResultData& rhs)
            {
                Subsystem = rhs.Subsystem;
                Active = rhs.Active;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("subsystem"), &Subsystem);
                Add(_T("active"), &Active);
            }

        public:
            Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> Subsystem;
            Core::JSON::Boolean Active;
        }; // class SubsystemsResultData

    } // namespace Controller

} // namespace JsonData

}


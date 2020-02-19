 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
            Core::JSON::String Data; // Object that was broadcasted as an event by the originator plugin
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
            Core::JSON::String Path; // Path to directory (within the persistent storage) to delete contents of
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
                Add(_T("result"), &Result);
                Add(_T("source"), &Source);
                Add(_T("destination"), &Destination);
            }

            DownloadcompletedParamsData(const DownloadcompletedParamsData&) = delete;
            DownloadcompletedParamsData& operator=(const DownloadcompletedParamsData&) = delete;

        public:
            Core::JSON::DecUInt32 Result; // Download operation result (0: success)
            Core::JSON::String Source; // Source URL identifying the downloaded file
            Core::JSON::String Destination; // Path to the downloaded file in the persistent storage
        }; // class DownloadcompletedParamsData

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

        class SubsystemsParamsData : public Core::JSON::Container {
        public:
            SubsystemsParamsData()
                : Core::JSON::Container()
            {
                Init();
            }

            SubsystemsParamsData(const SubsystemsParamsData& other)
                : Core::JSON::Container()
                , Subsystem(other.Subsystem)
                , Active(other.Active)
            {
                Init();
            }

            SubsystemsParamsData& operator=(const SubsystemsParamsData& rhs)
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
            Core::JSON::EnumType<PluginHost::ISubSystem::subsystem> Subsystem; // Subsystem name
            Core::JSON::Boolean Active; // Denotes whether the subsystem is active (true)
        }; // class SubsystemsParamsData

    } // namespace Controller

} // namespace JsonData

}


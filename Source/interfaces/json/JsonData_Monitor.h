
// C++ classes for Monitor JSON-RPC API.
// Generated automatically from 'MonitorAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace Monitor {

        // Common classes
        //

        class MeasurementInfo : public Core::JSON::Container {
        public:
            MeasurementInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            MeasurementInfo(const MeasurementInfo& other)
                : Core::JSON::Container()
                , Min(other.Min)
                , Max(other.Max)
                , Average(other.Average)
                , Last(other.Last)
            {
                Init();
            }

            MeasurementInfo& operator=(const MeasurementInfo& rhs)
            {
                Min = rhs.Min;
                Max = rhs.Max;
                Average = rhs.Average;
                Last = rhs.Last;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("min"), &Min);
                Add(_T("max"), &Max);
                Add(_T("average"), &Average);
                Add(_T("last"), &Last);
            }

        public:
            Core::JSON::DecUInt32 Min; // Minimal value measured
            Core::JSON::DecUInt32 Max; // Maximal value measured
            Core::JSON::DecUInt32 Average; // Average of all measurements
            Core::JSON::DecUInt32 Last; // Last measured value
        }; // class MeasurementInfo

        class MeasurementsInfo : public Core::JSON::Container {
        public:
            MeasurementsInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            MeasurementsInfo(const MeasurementsInfo& other)
                : Core::JSON::Container()
                , Resident(other.Resident)
                , Allocated(other.Allocated)
                , Shared(other.Shared)
                , Process(other.Process)
                , Operational(other.Operational)
                , Count(other.Count)
            {
                Init();
            }

            MeasurementsInfo& operator=(const MeasurementsInfo& rhs)
            {
                Resident = rhs.Resident;
                Allocated = rhs.Allocated;
                Shared = rhs.Shared;
                Process = rhs.Process;
                Operational = rhs.Operational;
                Count = rhs.Count;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("resident"), &Resident);
                Add(_T("allocated"), &Allocated);
                Add(_T("shared"), &Shared);
                Add(_T("process"), &Process);
                Add(_T("operational"), &Operational);
                Add(_T("count"), &Count);
            }

        public:
            MeasurementInfo Resident; // Resident memory measurement
            MeasurementInfo Allocated; // Allocated memory measurement
            MeasurementInfo Shared; // Shared memory measurement
            MeasurementInfo Process; // Processes measurement
            Core::JSON::Boolean Operational; // Whether the plugin is up and running
            Core::JSON::DecUInt32 Count; // Number of measurements
        }; // class MeasurementsInfo

        class RestartsettingsInfo : public Core::JSON::Container {
        public:
            RestartsettingsInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            RestartsettingsInfo(const RestartsettingsInfo& other)
                : Core::JSON::Container()
                , Limit(other.Limit)
                , Windowseconds(other.Windowseconds)
            {
                Init();
            }

            RestartsettingsInfo& operator=(const RestartsettingsInfo& rhs)
            {
                Limit = rhs.Limit;
                Windowseconds = rhs.Windowseconds;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("limit"), &Limit);
                Add(_T("windowseconds"), &Windowseconds);
            }

        public:
            Core::JSON::DecSInt32 Limit; // Maximum number or restarts to be attempted
            Core::JSON::DecSInt32 Windowseconds; // Time period within which failures must happen for the limit to be considered crossed
        }; // class RestartsettingsInfo

        class InfoInfo : public Core::JSON::Container {
        public:
            InfoInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            InfoInfo(const InfoInfo& other)
                : Core::JSON::Container()
                , Measurements(other.Measurements)
                , Observable(other.Observable)
                , Memoryrestartsettings(other.Memoryrestartsettings)
                , Operationalrestartsettings(other.Operationalrestartsettings)
            {
                Init();
            }

            InfoInfo& operator=(const InfoInfo& rhs)
            {
                Measurements = rhs.Measurements;
                Observable = rhs.Observable;
                Memoryrestartsettings = rhs.Memoryrestartsettings;
                Operationalrestartsettings = rhs.Operationalrestartsettings;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("measurements"), &Measurements);
                Add(_T("observable"), &Observable);
                Add(_T("memoryrestartsettings"), &Memoryrestartsettings);
                Add(_T("operationalrestartsettings"), &Operationalrestartsettings);
            }

        public:
            MeasurementsInfo Measurements; // Measurements for the plugin
            Core::JSON::String Observable; // A callsign of the watched plugin
            RestartsettingsInfo Memoryrestartsettings; // Restart limits for memory consumption related failures applying to the plugin
            RestartsettingsInfo Operationalrestartsettings; // Restart limits for stability failures applying to the plugin
        }; // class InfoInfo

        class StatusParamsInfo : public Core::JSON::Container {
        public:
            StatusParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
            }

            StatusParamsInfo(const StatusParamsInfo&) = delete;
            StatusParamsInfo& operator=(const StatusParamsInfo&) = delete;

        public:
            Core::JSON::String Callsign; // The callsing of a plugin to get measurements snapshot of, if set empty then all observed objects will be returned
        }; // class StatusParamsInfo

        // Method params/result classes
        //

        class ActionParamsData : public Core::JSON::Container {
        public:
            ActionParamsData()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("action"), &Action);
                Add(_T("reason"), &Reason);
            }

            ActionParamsData(const ActionParamsData&) = delete;
            ActionParamsData& operator=(const ActionParamsData&) = delete;

        public:
            Core::JSON::String Callsign; // Callsign of the plugin the monitor acted upon
            Core::JSON::String Action; // The action executed by the monitor on a plugin. One of: "Activate", "Deactivate", "StoppedRestarting"
            Core::JSON::String Reason; // A message describing the reason the action was taken of
        }; // class ActionParamsData

        class RestartlimitsParamsData : public Core::JSON::Container {
        public:
            RestartlimitsParamsData()
                : Core::JSON::Container()
            {
                Add(_T("callsign"), &Callsign);
                Add(_T("operationalrestartsettings"), &Operationalrestartsettings);
                Add(_T("memoryrestartsettings"), &Memoryrestartsettings);
            }

            RestartlimitsParamsData(const RestartlimitsParamsData&) = delete;
            RestartlimitsParamsData& operator=(const RestartlimitsParamsData&) = delete;

        public:
            Core::JSON::String Callsign; // The callsign of a plugin to reset measurements snapshot for
            RestartsettingsInfo Operationalrestartsettings; // Restart setting for memory consumption type of failures
            RestartsettingsInfo Memoryrestartsettings; // Restart setting for stability type of failures
        }; // class RestartlimitsParamsData

    } // namespace Monitor

} // namespace JsonData

}


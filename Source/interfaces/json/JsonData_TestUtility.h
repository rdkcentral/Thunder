
// C++ classes for Test Utility JSON-RPC API.
// Generated automatically from 'TestUtilityAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace TestUtility {

        // Common classes
        //

        class DescriptionParamsInfo : public Core::JSON::Container {
        public:
            DescriptionParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("command"), &Command);
            }

            DescriptionParamsInfo(const DescriptionParamsInfo&) = delete;
            DescriptionParamsInfo& operator=(const DescriptionParamsInfo&) = delete;

        public:
            Core::JSON::String Command; // The test command name
        }; // class DescriptionParamsInfo

        class InputInfo : public Core::JSON::Container {
        public:
            // The test command input parameter type
            enum class ParamType {
                NUMBER,
                STRING,
                BOOLEAN,
                OBJECT,
                SYMBOL
            };

            InputInfo()
                : Core::JSON::Container()
            {
                Init();
            }

            InputInfo(const string& name, const ParamType& type, const string& comment)
                : Core::JSON::Container()
                , Name()
                , Type()
                , Comment()
            {
                this->Name = name;
                this->Type = type;
                this->Comment = comment;

                Init();
            }

            InputInfo(const InputInfo& other)
                : Core::JSON::Container()
                , Name(other.Name)
                , Type(other.Type)
                , Comment(other.Comment)
            {
                Init();
            }

            InputInfo& operator=(const InputInfo& rhs)
            {
                Name = rhs.Name;
                Type = rhs.Type;
                Comment = rhs.Comment;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("name"), &Name);
                Add(_T("type"), &Type);
                Add(_T("comment"), &Comment);
            }

        public:
            Core::JSON::String Name; // The test command input parameter
            Core::JSON::EnumType<ParamType> Type; // The test command input parameter type
            Core::JSON::String Comment; // The test command input parameter description
        }; // class InputInfo

        // Method params/result classes
        //

        class DescriptionResultData : public Core::JSON::Container {
        public:
            DescriptionResultData()
                : Core::JSON::Container()
            {
                Add(_T("description"), &Description);
            }

            DescriptionResultData(const DescriptionResultData&) = delete;
            DescriptionResultData& operator=(const DescriptionResultData&) = delete;

        public:
            Core::JSON::String Description; // Test command description
        }; // class DescriptionResultData

        class ParametersResultData : public Core::JSON::Container {
        public:
            ParametersResultData()
                : Core::JSON::Container()
            {
                Add(_T("input"), &Input);
                Add(_T("output"), &Output);
            }

            ParametersResultData(const ParametersResultData&) = delete;
            ParametersResultData& operator=(const ParametersResultData&) = delete;

        public:
            Core::JSON::ArrayType<InputInfo> Input;
            InputInfo Output;
        }; // class ParametersResultData

        class RuncrashParamsData : public Core::JSON::Container {
        public:
            RuncrashParamsData()
                : Core::JSON::Container()
            {
                Add(_T("command"), &Command);
                Add(_T("delay"), &Delay);
                Add(_T("count"), &Count);
            }

            RuncrashParamsData(const RuncrashParamsData&) = delete;
            RuncrashParamsData& operator=(const RuncrashParamsData&) = delete;

        public:
            Core::JSON::String Command; // The test command name
            Core::JSON::DecUInt8 Delay; // Delay timeout
            Core::JSON::DecUInt8 Count; // How many times a Crash command will be executed consecutively (applicable for "CrashNTimes" command)
        }; // class RuncrashParamsData

        class RunmemoryParamsData : public Core::JSON::Container {
        public:
            RunmemoryParamsData()
                : Core::JSON::Container()
            {
                Add(_T("command"), &Command);
                Add(_T("size"), &Size);
            }

            RunmemoryParamsData(const RunmemoryParamsData&) = delete;
            RunmemoryParamsData& operator=(const RunmemoryParamsData&) = delete;

        public:
            Core::JSON::String Command; // The test command name
            Core::JSON::DecUInt32 Size; // The amount of memory in KB for allocation (applicable for "Malloc" commands)
        }; // class RunmemoryParamsData

        class RunmemoryResultData : public Core::JSON::Container {
        public:
            RunmemoryResultData()
                : Core::JSON::Container()
            {
                Add(_T("allocated"), &Allocated);
                Add(_T("size"), &Size);
                Add(_T("resident"), &Resident);
            }

            RunmemoryResultData(const RunmemoryResultData&) = delete;
            RunmemoryResultData& operator=(const RunmemoryResultData&) = delete;

        public:
            Core::JSON::DecUInt32 Allocated; // Already allocated memory in KB
            Core::JSON::DecUInt32 Size; // Current allocation in KB
            Core::JSON::DecUInt32 Resident; // Resident memory in KB
        }; // class RunmemoryResultData

    } // namespace TestUtility

} // namespace JsonData

}


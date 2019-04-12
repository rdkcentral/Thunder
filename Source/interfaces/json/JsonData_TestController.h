
// C++ classes for Test Controller JSON-RPC API.
// Generated automatically from 'TestControllerAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace TestController {

        // Common classes
        //

        class DescriptionParamsInfo : public Core::JSON::Container {
        public:
            DescriptionParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("category"), &Category);
                Add(_T("test"), &Test);
            }

            DescriptionParamsInfo(const DescriptionParamsInfo&) = delete;
            DescriptionParamsInfo& operator=(const DescriptionParamsInfo&) = delete;

        public:
            Core::JSON::String Category; // The test category name
            Core::JSON::String Test; // The test name
        }; // class DescriptionParamsInfo

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
            Core::JSON::String Description; // The test description
        }; // class DescriptionResultData

        class RunResultData : public Core::JSON::Container {
        public:
            RunResultData()
                : Core::JSON::Container()
            {
                Init();
            }

            RunResultData(const RunResultData& other)
                : Core::JSON::Container()
                , Test(other.Test)
                , Status(other.Status)
            {
                Init();
            }

            RunResultData& operator=(const RunResultData& rhs)
            {
                Test = rhs.Test;
                Status = rhs.Status;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("test"), &Test);
                Add(_T("status"), &Status);
            }

        public:
            Core::JSON::String Test; // The test name
            Core::JSON::String Status; // The test status
        }; // class RunResultData

        class TestsParamsData : public Core::JSON::Container {
        public:
            TestsParamsData()
                : Core::JSON::Container()
            {
                Add(_T("category"), &Category);
            }

            TestsParamsData(const TestsParamsData&) = delete;
            TestsParamsData& operator=(const TestsParamsData&) = delete;

        public:
            Core::JSON::String Category; // The test category name
        }; // class TestsParamsData

    } // namespace TestController

} // namespace JsonData

}


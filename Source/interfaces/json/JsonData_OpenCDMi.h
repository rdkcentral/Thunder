
// C++ classes for OpenCDMi JSON-RPC API.
// Generated automatically from 'OpenCDMiAPI.json'.

// Note: This code is inherently not thread safe. If required, proper synchronisation must be added.

#pragma once

#include <core/JSON.h>

namespace WPEFramework {

namespace JsonData {

    namespace OCDM {

        // Method params/result classes
        //

        class DrmData : public Core::JSON::Container {
        public:
            DrmData()
                : Core::JSON::Container()
            {
                Init();
            }

            DrmData(const DrmData& other)
                : Core::JSON::Container()
                , Name(other.Name)
                , Keysystems(other.Keysystems)
            {
                Init();
            }

            DrmData& operator=(const DrmData& rhs)
            {
                Name = rhs.Name;
                Keysystems = rhs.Keysystems;
                return (*this);
            }

        private:
            void Init()
            {
                Add(_T("name"), &Name);
                Add(_T("keysystems"), &Keysystems);
            }

        public:
            Core::JSON::String Name; // Name of the DRM
            Core::JSON::ArrayType<Core::JSON::String> Keysystems;
        }; // class DrmData

    } // namespace OCDM

} // namespace JsonData

}


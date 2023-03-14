 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifndef __DATAELEMENTFILE_H
#define __DATAELEMENTFILE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "DataElement.h"
#include "FileSystem.h"
#include "Portability.h"

// ---- Referenced classes and types ----
#ifdef __LINUX__
#include <sys/stat.h>
#endif

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----
namespace WPEFramework {
namespace Core {
    // The datapackage is the abstract of a package that needs to be send over the line.
    class EXTERNAL DataElementFile : public DataElement {
    public:
        DataElementFile() = delete;
        DataElementFile& operator=(const DataElementFile&) = delete;

        DataElementFile(File& fileName, const uint32_t type);
        DataElementFile(const string& fileName, const uint32_t mode, const uint32_t requiredSize = 0);
        DataElementFile(const DataElementFile&);
        ~DataElementFile() override {
            Close();
        }

    public:
        inline const string& Name() const
        {
            return (m_File.Name());
        }
        inline bool IsValid() const
        {
            return (m_File.IsOpen());
        }
        inline const File& Storage() const
        {
            return (m_File);
        }
        inline uint32_t ErrorCode() const
        {
            return (m_File.ErrorCode());
        }
        inline void ReloadFileInfo() const
        {
            m_File.LoadFileInfo();
        }
        inline uint32_t User(const string& userName) const
        {
            return (m_File.User(userName));
        }
        inline uint32_t Group(const string& groupName) const
        {
            return (m_File.Group(groupName));
        }
        inline uint32_t Permission(uint32_t mode) const
        {
            return (m_File.Permission(mode));
        }
        bool Destroy()
        {
            bool closed = IsValid();

            if (closed == true) {
                Close();
                closed = m_File.Destroy();
            }
            return (closed);
        }
        bool Load();
        void Sync();

    protected:
        void Close();
        virtual void Reallocation(const uint64_t size);

        void ReopenMemoryMappedFile();

    private:
        void OpenMemoryMappedFile(uint32_t requiredSize);

    private:
#ifdef __WINDOWS__
        typedef HANDLE Handle;
#else
        typedef void* Handle;
#endif

        mutable File m_File;
        Handle m_MemoryMappedFile;
        uint32_t m_Flags;
    };
}
} // namespace Core

#endif // __DATAELEMENTFILE_H

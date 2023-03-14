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

#include "DataElementFile.h"

#ifdef __LINUX__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#undef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE nullptr
#endif

#ifdef __WINDOWS__
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace WPEFramework {
namespace Core {
    DataElementFile::DataElementFile(File& file, const uint32_t type)
        : DataElement()
        , m_File(file)
        , m_MemoryMappedFile(INVALID_HANDLE_VALUE)
        , m_Flags(type)
    {
        // What is the use of a file that is not readable nor writable ?
        ASSERT(m_Flags != 0);

        // The file needs to be prepared in the same way as we request the Memorymapped file...
        ASSERT(m_File.IsOpen() == true);

        if (IsValid()) {
            OpenMemoryMappedFile(static_cast<uint32_t>(m_File.Size()));
        }
    }

    DataElementFile::DataElementFile(const string& fileName, const uint32_t type, const uint32_t requestedSize)
        : DataElement()
        , m_File(fileName)
        , m_MemoryMappedFile(INVALID_HANDLE_VALUE)
        , m_Flags(type)
    {
        // What is the use of a file that is not readable nor writable ?
        ASSERT(m_Flags != 0);

        if ((type & File::CREATE) != 0) {
            m_File.Create(type);
            m_File.Permission(type & ~(File::CREATE | Core::File::SHAREABLE));
        } else {
            m_File.Open((type & File::USER_WRITE) == 0);
        }

        if (IsValid()) {
            if ((requestedSize != 0) && (requestedSize > m_File.Size())) {
                m_File.SetSize(requestedSize);
                OpenMemoryMappedFile(requestedSize);
            } else {
                OpenMemoryMappedFile(static_cast<uint32_t>(m_File.Size()));
            }
        }
    }

    DataElementFile::DataElementFile(const DataElementFile& copy)
        : DataElement(copy)
        , m_File(copy.m_File)
        , m_MemoryMappedFile(copy.m_MemoryMappedFile)
        , m_Flags(copy.m_Flags) {
    }

    bool DataElementFile::Load() {
        bool available = (m_File.IsOpen() == true);
        if (available == false) {
            m_File.Open((m_Flags & File::USER_WRITE) == 0);
            if (m_File.IsOpen() == true) {
                available = true;
                OpenMemoryMappedFile(static_cast<uint32_t>(m_File.Size()));
            }
        }
        return (available);
    }

#ifdef __WINDOWS__
    void DataElementFile::OpenMemoryMappedFile(uint32_t requiredSize)
    {
        if (requiredSize > 0) {
            DWORD flags = ((m_Flags & File::USER_WRITE) != 0 ? PAGE_READWRITE : PAGE_READONLY);
            SYSTEM_INFO systemInfo;
            ::GetSystemInfo(&systemInfo);
            uint32_t mapSize = ((((requiredSize - 1) / systemInfo.dwPageSize) + 1) * systemInfo.dwPageSize);

            // Open the file in MM mode as one element.
            m_MemoryMappedFile = ::CreateFileMapping(m_File, nullptr, flags, 0, mapSize, nullptr);

            if (m_MemoryMappedFile == nullptr) {
                DWORD value = GetLastError();
                m_File.Close();
            } else {
                flags = ((m_Flags & File::USER_READ) != 0 ? FILE_MAP_READ : 0) | ((m_Flags & File::USER_WRITE) != 0 ? FILE_MAP_WRITE : 0);

                void* newBuffer = (::MapViewOfFile(m_MemoryMappedFile, flags, 0, 0, mapSize));

                // Seems like everything succeeded. Lets map it.
                UpdateCache(0, static_cast<uint8_t*>(newBuffer), requiredSize, mapSize);
            }
        }
    }

    void DataElementFile::Close()
    {
        if ((IsValid()) && (m_MemoryMappedFile != INVALID_HANDLE_VALUE)) {
            DWORD flags = ((m_Flags & File::USER_READ) != 0 ? FILE_MAP_READ : 0) | ((m_Flags & File::USER_WRITE) != 0 ? FILE_MAP_WRITE : 0);
            // Set the last size...
            ::UnmapViewOfFile(Buffer());
            ::CloseHandle(m_MemoryMappedFile);

            m_MemoryMappedFile = INVALID_HANDLE_VALUE;
        }
    }

    /* virtual */ void DataElementFile::Reallocation(const uint64_t size)
    {
        if (IsValid()) {
            SYSTEM_INFO systemInfo;
            ::GetSystemInfo(&systemInfo);

            // Allocated a new page.
            uint64_t requestedSize = ((size / systemInfo.dwPageSize) * systemInfo.dwPageSize) + systemInfo.dwPageSize;

            if (m_MemoryMappedFile == INVALID_HANDLE_VALUE) {
                DWORD flags = ((m_Flags & File::USER_WRITE) != 0 ? PAGE_READWRITE : PAGE_READONLY);

                // Open the file in MM mode as one element.
                m_MemoryMappedFile = ::CreateFileMapping(m_File, nullptr, flags, 0, static_cast<DWORD>(requestedSize), nullptr);

                if (m_MemoryMappedFile == nullptr) {
                    DWORD value = GetLastError();
                    m_File.Close();
                    m_MemoryMappedFile = INVALID_HANDLE_VALUE;
                }
            }

            if (m_MemoryMappedFile == INVALID_HANDLE_VALUE) {
                DWORD flags = ((m_Flags & File::USER_READ) != 0 ? FILE_MAP_READ : 0) | ((m_Flags & File::USER_WRITE) != 0 ? FILE_MAP_WRITE : 0);

                void* newBuffer = ::MapViewOfFileEx(m_MemoryMappedFile, flags, 0, 0, static_cast<SIZE_T>(requestedSize), Buffer());

                // Seems like everything succeeded. Lets map it.
                UpdateCache(0, static_cast<uint8_t*>(newBuffer), size, requestedSize);

                if (newBuffer == nullptr) {
                    m_File.Close();

                    ::CloseHandle(m_MemoryMappedFile);
                    m_MemoryMappedFile = INVALID_HANDLE_VALUE;

                    UpdateCache(0, nullptr, 0, 0);
                } else {
                    // Seems we upgraded, set the caches
                    UpdateCache(0, static_cast<uint8_t*>(newBuffer), Size(), requestedSize);
                }
            }
        }
    }

    void DataElementFile::Sync()
    {
        if ((m_Flags & File::SHAREABLE) != 0) {
            m_File.SetSize(Size());
            ::FlushViewOfFile(Buffer(), static_cast<SIZE_T>(Size()));
        }
    }

#endif

#ifdef __POSIX__
    void DataElementFile::OpenMemoryMappedFile(uint32_t requiredSize)
    {
        if (requiredSize > 0) {
            int pageSize = getpagesize();
            uint64_t mapSize = ((((requiredSize - 1) / pageSize) + 1) * pageSize);
            int flags = (((m_Flags & File::USER_READ) != 0 ? PROT_READ : 0) | ((m_Flags & File::USER_WRITE) != 0 ? PROT_WRITE : 0));

            // Open the file in MM mode as one element.
            m_MemoryMappedFile = mmap(nullptr, mapSize, flags, ((m_Flags & File::SHAREABLE) != 0 ? MAP_SHARED : MAP_PRIVATE), m_File, 0);

            if (m_MemoryMappedFile == MAP_FAILED) {
                m_File.Close();
                m_MemoryMappedFile = nullptr;
            } else {
                // Seems like everything succeeded. Lets map it.
                UpdateCache(0, static_cast<uint8_t*>(m_MemoryMappedFile), requiredSize, mapSize);
            }
        }
    }

    void DataElementFile::ReopenMemoryMappedFile()
    {
        m_File.LoadFileInfo();

        if (IsValid())
            munmap(m_MemoryMappedFile, AllocatedSize());

        OpenMemoryMappedFile(m_File.Size());
    }

    void DataElementFile::Close()
    {
        if ((IsValid()) && (m_MemoryMappedFile != INVALID_HANDLE_VALUE)) {

            munmap(m_MemoryMappedFile, AllocatedSize());

            m_MemoryMappedFile = INVALID_HANDLE_VALUE;

            m_File.Close();
        }
    }

    /* virtual */ void DataElementFile::Reallocation(const uint64_t size)
    {
        if (IsValid()) {
            int pageSize = getpagesize();
            uint64_t requestedSize = ((size / pageSize) * pageSize) + pageSize;

            m_File.SetSize(requestedSize);

            if (m_MemoryMappedFile == INVALID_HANDLE_VALUE) {
                int flags = (((m_Flags & File::USER_READ) != 0 ? PROT_READ : 0) | ((m_Flags & File::USER_WRITE) != 0 ? PROT_WRITE : 0));

                // Open the file in MM mode as one element.
                m_MemoryMappedFile = mmap(nullptr, requestedSize, flags, ((m_Flags & File::SHAREABLE) != 0 ? MAP_SHARED : MAP_PRIVATE), m_File, 0);
            } else {

                // TODO: no need for memcpy, is possible?
                m_MemoryMappedFile = mremap(m_MemoryMappedFile, AllocatedSize(), requestedSize, MREMAP_MAYMOVE);
            }

            if (m_MemoryMappedFile == MAP_FAILED) {

                m_File.Close();
                m_MemoryMappedFile = INVALID_HANDLE_VALUE;

                UpdateCache(0, nullptr, 0, 0);
            } else {
                // Seems we upgraded, set the caches
                UpdateCache(0, static_cast<uint8_t*>(m_MemoryMappedFile), size, requestedSize);
            }
        }
    }

    void DataElementFile::Sync()
    {
        if ((m_Flags & File::SHAREABLE) != 0) {
            m_File.SetSize(Size());
            msync(Buffer(), Size(), MS_INVALIDATE | MS_SYNC);
        }
    }

#endif
}
} // namespace Core

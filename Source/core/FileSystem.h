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

#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#include "Portability.h"
#include "Number.h"
#include "Time.h"

#ifdef __POSIX__
#include <sys/statvfs.h>
#define INVALID_HANDLE_VALUE -1
#if defined(_LARGEFILE64_SOURCE) && defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#define LSEEK lseek64
#else
#define LSEEK lseek
#endif
#endif

namespace Thunder {
namespace Core {

    PUSH_WARNING(DISABLE_WARNING_UNUSED_FUNCTIONS)

    static void ParsePathInfo(const string& pathInfo, string& path, uint16_t& permission)
    {
        size_t position = pathInfo.find("|");
        if (position != string::npos) {
            Core::NumberType<uint16_t> number(pathInfo.substr(position + 1).c_str(), static_cast<uint32_t>(pathInfo.length() - position));
            permission = number.Value();
        }
        path = pathInfo.substr(0, position);
    }

    POP_WARNING()

    class EXTERNAL File {
    public:
#ifdef __WINDOWS__
        typedef enum {
            // A file that is read-only. Applications can read the file, but cannot write to it or delete it. This attribute is not honored on directories. For more information, see You cannot view or change the Read-only or the System attributes of folders in Windows Server 2003, in Windows XP, in Windows Vista or in Windows 7.
            FILE_READONLY = FILE_ATTRIBUTE_READONLY,
            // The file or directory is hidden. It is not included in an ordinary directory listing.
            FILE_HIDDEN = FILE_ATTRIBUTE_HIDDEN,
            // A file or directory that the operating system uses a part of, or uses exclusively.
            FILE_SYSTEM = FILE_ATTRIBUTE_SYSTEM,
            // The handle that identifies a directory.
            FILE_DIRECTORY = FILE_ATTRIBUTE_DIRECTORY,
            // A file or directory that is an archive file or directory. Applications typically use this attribute to mark files for backup or removal .
            FILE_ARCHIVE = FILE_ATTRIBUTE_ARCHIVE,
            // This value is reserved for system use.
            FILE_NORMAL = FILE_ATTRIBUTE_NORMAL,
            // A file or directory that has an associated reparse point, or a file that is a symbolic link.
            FILE_LINK = FILE_ATTRIBUTE_REPARSE_POINT,
            // A file or directory that is compressed. For a file, all of the data in the file is compressed. For a directory, compression is the default for newly created files and subdirectories.
            FILE_COMPRESSED = FILE_ATTRIBUTE_COMPRESSED,
            // A file or directory that is encrypted. For a file, all data streams in the file are encrypted. For a directory, encryption is the default for newly created files and subdirectories.
            FILE_ENCRYPTED = FILE_ATTRIBUTE_ENCRYPTED

        } Atrributes;

        typedef enum : uint32_t {
            USER_READ      = 0x00000001,
            USER_WRITE     = 0x00000002,
            USER_EXECUTE   = 0x00000004,
            GROUP_READ     = 0x00000008,
            GROUP_WRITE    = 0x00000010,
            GROUP_EXECUTE  = 0x00000020,
            OTHERS_READ    = 0x00000040,
            OTHERS_WRITE   = 0x00000080,
            OTHERS_EXECUTE = 0x00000100,
            SHAREABLE      = 0x10000000,
            CREATE         = 0x20000000
       } Mode;

#endif

#ifdef __POSIX__
        typedef enum {
            // A file or directory that the operating system uses a part of, or uses exclusively.
            FILE_SYSTEM = 0x0001,
            // The handle that identifies a directory.
            FILE_DIRECTORY = S_IFDIR,
            // A file that is read-only. Applications can read the file, but cannot write to it or delete it. This attribute is not honored on directories. For more information, see You cannot view or change the Read-only or the System attributes of folders in Windows Server 2003, in Windows XP, in Windows Vista or in Windows 7.
            FILE_READONLY = 0x0002,
            // The file or directory is hidden. It is not included in an ordinary directory listing.
            FILE_HIDDEN = 0x0004,
            // A file or directory that is an archive file or directory. Applications typically use this attribute to mark files for backup or removal .
            FILE_ARCHIVE = 0x0008,
            // A file that does not have other attributes set. This attribute is valid only when used alone.
            FILE_NORMAL = S_IFREG,
            // A file or directory that has an associated reparse point, or a file that is a symbolic link.
            FILE_LINK = S_IFLNK,
            // A device
            FILE_DEVICE = 0x0010
        } Atrributes;

        typedef enum : uint32_t {
            USER_READ      = S_IRUSR,
            USER_WRITE     = S_IWUSR,
            USER_EXECUTE   = S_IXUSR,
            GROUP_READ     = S_IRGRP,
            GROUP_WRITE    = S_IWGRP,
            GROUP_EXECUTE  = S_IXGRP,
            OTHERS_READ    = S_IROTH,
            OTHERS_WRITE   = S_IWOTH,
            OTHERS_EXECUTE = S_IXOTH,
            SHAREABLE      = 0x10000000,
            CREATE         = 0x20000000
       } Mode;


#endif

    public:
#ifdef __POSIX__
        typedef int Handle;
#endif
#ifdef __WINDOWS__
        typedef HANDLE Handle;
#endif

    public:
        explicit File();
        explicit File(const string& fileName);
        explicit File(const File& copy);
        explicit File(File&& move);
        ~File();

        File& operator=(const string& location)
        {
            _name = location;
            LoadFileInfo();

            return (*this);
        }
        File& operator=(const File& RHS)
        {
            _name = RHS._name;
            _size = RHS._size;
            _attributes = RHS._attributes;
            _creation = RHS._creation;
            _modification = RHS._modification;
            _access = RHS._access;

            if (_handle != INVALID_HANDLE_VALUE) {
#ifdef __POSIX__
                close(_handle);
#else
                ::CloseHandle(_handle);
#endif
            }

            _handle = RHS.DuplicateHandle();

            return (*this);
        }

        File& operator=(File&& move)
        {
            if (this != &move) {
                _name = std::move(move._name);
                _size = move._size;
                _attributes = move._attributes;
                _creation = std::move(move._creation);
                _modification = std::move(move._modification);
                _access = std::move(move._access);

                if (_handle != INVALID_HANDLE_VALUE) {
#ifdef __POSIX__
                    close(_handle);
#else
                    ::CloseHandle(_handle);
#endif
                }
                _handle = move._handle;

                move._handle = INVALID_HANDLE_VALUE;
                move._size = 0;
                move._attributes = 0;
            }
            return (*this);
	}

        static string Normalize(const string& input, bool& valid);

    public:
        inline static string FileName(const string& name)
        {
            uint32_t filenameOffset = FileNameOffset(name);
            uint32_t offset = ExtensionOffset(name);
            int value = (offset >= (filenameOffset + 2) ? (offset - 1 - filenameOffset) : -1);

            return (name.substr(filenameOffset, value));
        }
        inline static string FileNameExtended(const string& name)
        {
            return (name.substr(FileNameOffset(name), -1));
        }
        inline static string PathName(const string& name)
        {
            string result;
            uint32_t offset = FileNameOffset(name);

            if (offset != 0) {
                result = name.substr(0, offset);
            }

            return (result);
        }
        inline static bool IsPathAbsolute(const string& path)
        {
#ifdef __WINDOWS__
            return ((path.size() > 0 && (path[0] == '\\' || path[0] == '/')) || (path.size() > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/')));
#else
            return (path.size() > 0 && path[0] == '/');
#endif
        }
        inline static string Extension(const string& name)
        {
            string result;
            uint32_t offset = ExtensionOffset(name);

            if (offset != 0) {
                result = name.substr(offset, -1);
            }

            return (result);
        }
        inline uint32_t ErrorCode() const
        {
#ifdef __WINDOWS__
            return (static_cast<uint32_t>(::GetLastError()));
#endif
#ifdef __POSIX__
            return (static_cast<uint32_t>(errno));
#endif
        }
        inline bool IsOpen() const
        {
            return (_handle != INVALID_HANDLE_VALUE);
        }
        inline bool Exists() const
        {
            return (_attributes != 0);
        }
        inline bool IsReadOnly() const
        {
            return ((_attributes & FILE_READONLY) != 0);
        }
        inline bool IsHidden() const
        {
            return ((_attributes & FILE_HIDDEN) != 0);
        }
        inline bool IsSystem() const
        {
            return ((_attributes & FILE_SYSTEM) != 0);
        }
        inline bool IsArchive() const
        {
            return ((_attributes & FILE_ARCHIVE) != 0);
        }
        inline bool IsDirectory() const
        {
            return ((_attributes & FILE_DIRECTORY) == FILE_DIRECTORY);
        }
        inline bool IsLink() const
        {
            return ((_attributes & FILE_LINK) == FILE_LINK);
        }
        inline bool IsCompressed() const
        {
#ifdef __POSIX__
            return (false);
#endif
#ifdef __WINDOWS__
            return ((_attributes & FILE_COMPRESSED) != 0);
#endif
        }
        inline bool IsEncrypted() const
        {
#ifdef __POSIX__
            return (false);
#endif
#ifdef __WINDOWS__
            return ((_attributes & FILE_ENCRYPTED) != 0);
#endif
        }
        inline const string& Name() const
        {
            return (_name);
        }
        inline string PathName() const
        {
            return (PathName(_name));
        }
        inline string FileName() const
        {
            return (FileName(_name));
        }
        inline string FileNameExtended() const
        {
            return (FileNameExtended(_name));
        }
        inline string Extension() const
        {
            return (Extension(_name));
        }
        inline Core::Time ModificationTime() const
        {
            return (_modification);
        }
        inline Core::Time AccessTime() const
        {
            return (_access);
        }
        inline Core::Time CreationTime() const
        {
            return (_creation);
        }
        inline uint64_t Size() const
        {
            return (_size);
        }
        bool SetSize(const uint64_t size)
        {
            bool result;
#ifdef __WINDOWS__
            LARGE_INTEGER position;
            position.HighPart = (size >> 32);
            position.LowPart = (size & 0xFFFFFFFF);
            result = (SetFilePointerEx(_handle, position, nullptr, FILE_BEGIN) != 0);
#else
            result = (ftruncate(_handle, size) != -1);
#endif

            LoadFileInfo();

            return (result);
        }

        bool Link(const string& symlinkName)
        {
#ifdef __WINDOWS__
            return (::CreateSymbolicLink(symlinkName.c_str(), _name.c_str(), (IsDirectory() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0)) != FALSE);
#else
            return (symlink(_name.c_str(), symlinkName.c_str()) >= 0);
#endif
        }

        bool Create(const bool exclusive = false)
        {
            return (Create((USER_READ|USER_WRITE|USER_EXECUTE|GROUP_READ|GROUP_EXECUTE), exclusive));
        }

        bool Create(const uint32_t mode, const bool exclusive = false)
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_CLOEXEC | O_RDWR | O_CREAT | O_TRUNC | (exclusive ? O_EXCL : 0), mode);
#endif
#ifdef __WINDOWS__
            _handle = ::CreateFile(_name.c_str(), (GENERIC_READ | GENERIC_WRITE), (exclusive ? 0 : (FILE_SHARE_READ | FILE_SHARE_WRITE)), nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
            LoadFileInfo();
            return (IsOpen());
        }

        bool Open() const
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_RDONLY | O_CLOEXEC);
#endif
#ifdef __WINDOWS__
            _handle = ::CreateFile(_name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, nullptr);
#endif
            const_cast<File*>(this)->LoadFileInfo();
            return (IsOpen());
        }
        bool Open(bool readOnly)
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_CLOEXEC | (readOnly ? O_RDONLY : O_RDWR));
#endif
#ifdef __WINDOWS__
            _handle = ::CreateFile(_name.c_str(), GENERIC_READ | (readOnly ? 0 : GENERIC_WRITE), FILE_SHARE_READ | (readOnly ? 0 : FILE_SHARE_WRITE), nullptr, OPEN_EXISTING, (readOnly ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL), nullptr);
#endif
            LoadFileInfo();
            return (IsOpen());
        }
        bool Append()
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_CLOEXEC| O_RDWR | O_APPEND);

            if ((_handle == -1) && (errno == ENOENT)) {
                return (Create());
            }
#endif
#ifdef __WINDOWS__
            _handle = ::CreateFile(_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (_handle != INVALID_HANDLE_VALUE) {
                ::SetFilePointer(_handle, 0, nullptr, FILE_END);
            }
#endif
            return (IsOpen());
        }
        bool Unlink()
        {
            bool result = true;

            if (Exists() == true) {
                Close();
#ifdef __POSIX__
                result = (unlink(_name.c_str()) == 0);
#endif
#ifdef __WINDOWS__
                result = (::DeleteFile(_name.c_str()) != FALSE);
#endif
            }
            return (result);
        }
        bool Destroy()
        {
            bool result = true;

            if (Exists() == true) {
                Close();

#ifdef __POSIX__
                result = (remove(_name.c_str()) == 0);
#endif
#ifdef __WINDOWS__
                result = (::DeleteFile(_name.c_str()) != FALSE);
#endif
            }
            return (result);
        }
        bool Move(const string& newLocation)
        {
            bool result = false;

            if (Exists() == true) {
                Close();

#ifdef __POSIX__
                result = (rename(_name.c_str(), newLocation.c_str()) == 0);
#endif
#ifdef __WINDOWS__
                result = (::MoveFile(_name.c_str(), newLocation.c_str()) != FALSE);
#endif

                if (result == true) {
                    _name = newLocation;
                    LoadFileInfo();
                }
            }

            return (result);
        }
        void Close() const
        {
            if (IsOpen()) {
#ifdef __POSIX__
                close(_handle);
                _handle = -1;
#endif
#ifdef __WINDOWS__
                ::CloseHandle(_handle);
                _handle = INVALID_HANDLE_VALUE;
#endif
            }
        }
        uint32_t Write(const uint8_t buffer[], uint32_t size)
        {
            // Only call these methods if the file is open.
            ASSERT(IsOpen());

#ifdef __POSIX__
            int value = write(_handle, buffer, size);

            if (value == -1) {
                return (0);
            } else {
                return (static_cast<uint32_t>(value));
            }
#endif

#ifdef __WINDOWS__
            DWORD writtenBytes;

            ::WriteFile(_handle, buffer, size, &writtenBytes, nullptr);
            return static_cast<uint32_t>(writtenBytes);
#endif
        }
        uint32_t Read(uint8_t buffer[], uint32_t size) const
        {
            // Only call these methods if the file is open.
            ASSERT(IsOpen());

#ifdef __POSIX__
            int value = read(_handle, buffer, size);

            if (value == -1) {
                return (0);
            } else {
                return (static_cast<uint32_t>(value));
            }
#endif
#ifdef __WINDOWS__
            DWORD readBytes = 0;
            ::ReadFile(_handle, buffer, size, &readBytes, nullptr);
            return static_cast<uint32_t>(readBytes);
#endif
        }

        bool Position(const bool relative, int64_t offset)
        {
            // Only call these methods if the file is open.
            ASSERT(IsOpen());

#ifdef __POSIX__
            return (LSEEK(_handle, offset, (relative ? SEEK_CUR : SEEK_SET)) != -1);
#endif
#ifdef __WINDOWS__
            LARGE_INTEGER value; value.QuadPart = offset;
            return (::SetFilePointerEx(_handle, value, nullptr, (relative ? FILE_CURRENT : FILE_BEGIN)));
#endif
        }
        int64_t Position() const
        {
            int64_t result = 0;

            // If the file is not open, we are at the beginning :-)
            if (IsOpen()) {
#ifdef __POSIX__
                result = LSEEK(_handle, 0, SEEK_CUR);
                ASSERT(result >= 0);

                if (result == -1) {
                    result = 0;
                }

#endif
#ifdef __WINDOWS__
                LARGE_INTEGER newPos;
                LARGE_INTEGER startPos; startPos.QuadPart = 0;

                if (::SetFilePointerEx(_handle, startPos, &newPos, FILE_CURRENT)) {
                    ASSERT(newPos.QuadPart >= 0);

                    result = newPos.QuadPart;
                }
#endif
            }

            return (result);
        }

        Handle DuplicateHandle() const
        {
            Handle result = INVALID_HANDLE_VALUE;

            if (_handle != INVALID_HANDLE_VALUE) {
#ifdef __POSIX__
                result = ::dup(_handle);
#else
                ::DuplicateHandle(GetCurrentProcess(), _handle, GetCurrentProcess(), &result, 0, FALSE, DUPLICATE_SAME_ACCESS);
#endif
            }

            return (result);
        }

        operator Handle()
        {
            return (_handle);
        }

        operator FILE*()
        {
            if (_handle != INVALID_HANDLE_VALUE) {
#ifdef __POSIX__
                return (fdopen(DuplicateHandle(), (IsReadOnly() ? "r" : "r+")));
#endif
#ifdef __WINDOWS__
                Handle fd = DuplicateHandle();
PUSH_WARNING(DISABLE_WARNING_POINTER_TRUNCATION, DISABLE_WARNING_CONVERSION_TRUNCATION)
                return ((fd != 0) ? ::_fdopen(reinterpret_cast<int>(fd), (IsReadOnly() ? "r" : "r+")) : nullptr);
POP_WARNING()
#endif
            } else {
                return nullptr;
            }
        }

        void LoadFileInfo();

        uint32_t User(const string& userName) const;
        uint32_t Group(const string& groupName) const;
        uint32_t Permission(uint16_t flags) const;

    private:
        inline static uint32_t ExtensionOffset(const string& name)
        {
            int offset;
            return (((offset = static_cast<int>(name.find_last_of('.'))) == -1) ? 0 : offset + 1);
        }
        inline static uint32_t FileNameOffset(const string& name)
        {
            int offset;
            if ((offset = static_cast<int>(name.find_last_of('/'))) != -1) {
                // under linux/posix this is enough..
                offset++;
            } else {
                offset = 0;
            }
#ifdef __WINDOWS__
            int intermediate;
            if ((intermediate = static_cast<int>(name.find_last_of('\\'))) != -1) {
                if (intermediate > offset) {
                    offset = intermediate + 1;
                }
            }
            if ((offset == 0) && ((intermediate = static_cast<int>(name.find_last_of(':'))) != -1)) {
                offset = intermediate + 1;
            }
#endif

            return (static_cast<uint32_t>(offset));
        }

    private:
        string _name;
        uint64_t _size;
        uint32_t _attributes;
        Core::Time _creation;
        Core::Time _modification;
        Core::Time _access;
        mutable Handle _handle;
    };

    class EXTERNAL Directory {
    public:
        Directory();
        explicit Directory(const TCHAR location[]);
        Directory(const TCHAR location[], const TCHAR filter[]);
        Directory(const Directory& copy);
        Directory(Directory&& move);
        ~Directory();

    public:
        static string Normalize(const string& location)
        {
            string result;

            // First see if we are not empy.
            if (location.empty() == false) {

                bool valid;
                result = File::Normalize(location, valid);

                if ((valid == true) && ((result.empty() == true) || (result[result.length() - 1] != '/'))) {
                    result += '/';
                }
            }
            return (result);
        }

        bool Create();
        bool CreatePath();

        Directory& operator=(const TCHAR location[])
        {
            _name = location;
            return (*this);
        }

        bool Exists() const; 

#ifdef __LINUX__
        bool IsValid() const
        {
            return ((_dirFD != nullptr) && (_entry != nullptr));
        }
        void Reset()
        {
            if (_dirFD != nullptr) {
                closedir(_dirFD);
            }
            _dirFD = nullptr;
            _entry = nullptr;
        }
        bool Next()
        {
            bool valid = false;

            if ((_dirFD == nullptr) && (_entry == nullptr)) {
                _dirFD = opendir(_name.c_str());
            }

            while ((_dirFD != nullptr) && (valid == false)) {
                _entry = readdir(_dirFD);

                if (_entry != nullptr) {
                    valid = (fnmatch(_filter.c_str(), _entry->d_name, FNM_CASEFOLD) == 0);
                } else {
                    // We are at the end..
                    if (_dirFD != nullptr) {
                        closedir(_dirFD);
                        _dirFD = nullptr;
                    }
                    _entry = reinterpret_cast<dirent*>(~0);
                }
            }

            return (valid);
        }
        inline string Current() const
        {
            return (_name + _entry->d_name);
        }
        inline string Name() const
        {
            return (_entry->d_name);
        }
        inline bool IsDirectory() const
        {
            return (_entry != nullptr ? ((_entry->d_type & DT_DIR) != 0) : false);
        }
#endif

#ifdef __WINDOWS__
        bool IsValid() const
        {
            return ((_dirFD != INVALID_HANDLE_VALUE) && (noMoreFiles == false));
        }
        void Reset()
        {
            _dirFD = INVALID_HANDLE_VALUE;

            if (_dirFD != INVALID_HANDLE_VALUE) {
                ::FindClose(_dirFD);
                _dirFD = INVALID_HANDLE_VALUE;
            }
        }
        bool Next()
        {
            if (_dirFD == INVALID_HANDLE_VALUE) {
                _dirFD = ::FindFirstFile((_name + _filter).c_str(), &_data);
            } else if (::FindNextFile(_dirFD, &_data) == FALSE) {
                // We are at the end
                noMoreFiles = true;
            }

            return (IsValid());
        }
        inline string Current() const
        {
            return (_name + _data.cFileName);
        }
        inline string Name() const
        {
            return (_data.cFileName);
        }
        inline bool IsDirectory() const
        {
            return ((_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
        }
#endif

        Directory& operator=(const Directory& RHS)
        {
            _name = RHS._name;
            _filter = "";

#ifdef __LINUX__
            _dirFD = nullptr;
            _entry = nullptr;
#endif

#ifdef __WINDOWS__
            _dirFD = INVALID_HANDLE_VALUE;
            noMoreFiles = false;
#endif

            return (*this);
        }

        Directory& operator=(Directory&& move)
        {
            if (this != &move) {
                _name = std::move(move._name);
                _filter = std::move(move._filter);
#ifdef __LINUX__
                _dirFD = move._dirFD;
                _entry = move._entry;
                move._dirFD = nullptr;
                move._entry = nullptr;
#endif
#ifdef __WINDOWS__
                _dirFD = move._dirFD;
                noMoreFiles = move.noMoreFiles;
                move._dirFD = INVALID_HANDLE_VALUE;
                move.noMoreFiles = false;
#endif
            }
            return (*this);
        }

        bool Destroy () {

            // Allow only if the path does not contain ".." entries
            Reset();

            while (Next() == true) {
                Core::File file(Current());

                if (file.IsDirectory() == true) {
                    string name(file.FileName());

                    // We can not delete the "." or  ".." entries....
                    if ( (name.length() > 2) || 
                            ((name.length() == 1) && (name[0] != '.')) ||
                            ((name.length() == 2) && !((name[0] == '.') && (name[1] == '.'))) ) {
                        Directory deleteIt(Current().c_str());
                        deleteIt.Destroy();
                        file.Destroy();
                    }
                } else {
                    file.Destroy();
                }
            }

            if (_name.back() != '/') {
                Core::File(_name).Destroy();
            }

            return (true);
        }

        uint32_t User(const string& userName) const;
        uint32_t Group(const string& groupName) const;
        uint32_t Permission(uint16_t flags) const;

    private:
        string _name;
        string _filter;

#ifdef __LINUX__
        DIR* _dirFD;
        struct dirent* _entry;
#endif

#ifdef __WINDOWS__
        HANDLE _dirFD;
        WIN32_FIND_DATA _data;
        bool noMoreFiles;
#endif
    };

    class EXTERNAL Partition {
    private:
        static constexpr const TCHAR LineSeparator = _T('\n');
        static constexpr const TCHAR WordSeparator = _T(' ');
        static constexpr const TCHAR PathSeparator = _T('/');

#ifdef __POSIX__
        typedef struct statvfs StatFS;
#endif
#ifdef __LINUX__
#define PARTITION_BUFFER_SIZE 1024
        static constexpr const TCHAR* MountKey = _T("mounted on ");
        static constexpr const TCHAR* DeviceKey = _T("device ");
        static constexpr const TCHAR* MountStatsFileName = _T("/proc/self/mountstats");
#endif

    public:
        Partition() = delete;
        Partition(const Partition& copy) = delete;
        explicit Partition(const TCHAR path[])
            : _size(0)
            , _free(0)
            , _path(path)
        {
            LoadPartitionInfo();
        }
        ~Partition()
        {
        }

    private:
        void LoadPartitionInfo();
        string RemoveRepeatedPathSeparator(const string& path);
        bool ReadPartitionName(const string& fileName, string& device);

    public:
        uint64_t Size() const
        {
            return _size;
        }
        uint64_t Free() const
        {
            return _free;
        }
        bool IsValid() const
        {
            return (_size != 0);
        }
        string Name() const
        {
            return _name;
        }

    private:
        uint64_t _size;
        uint64_t _free;

        string _name;
        string _path;
    };
}
} // namespace Core

#endif

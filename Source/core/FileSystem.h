#ifndef __FILESYSTEM_H
#define __FILESYSTEM_H

#include "Portability.h"
#include "Time.h"

#ifdef __POSIX__
#define INVALID_HANDLE_VALUE -1
#endif

namespace WPEFramework {
namespace Core {
    class EXTERNAL File {
    public:
#ifdef __WIN32__
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
#endif

    public:
#ifdef __POSIX__
        typedef int Handle;
#endif
#ifdef __WIN32__
        typedef HANDLE Handle;
#endif

    public:
        explicit File(const bool sharable = false);
        explicit File(const string& fileName, const bool sharable = false);
        explicit File(const File& copy);
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
#ifdef __WIN32__
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
        inline bool IsShared() const
        {
            return (_sharable);
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
#ifdef __WIN32__
            return ((_attributes & FILE_COMPRESSED) != 0);
#endif
        }
        inline bool IsEncrypted() const
        {
#ifdef __POSIX__
            return (false);
#endif
#ifdef __WIN32__
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
#ifdef __WIN32__
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
#ifdef __WIN32__
			return (::CreateSymbolicLink(symlinkName.c_str(), _name.c_str(), (IsDirectory() ? SYMBOLIC_LINK_FLAG_DIRECTORY : 0)) != FALSE);
#else
			return (symlink(_name.c_str(), symlinkName.c_str()) >= 0);
#endif

		}

        bool Create()
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_RDWR | O_CREAT | (_sharable ? 0 : O_EXCL), S_IRWXU | S_IRGRP | S_IWGRP);
#endif
#ifdef __WIN32__
            _handle = ::CreateFile(_name.c_str(), (GENERIC_READ | GENERIC_WRITE), (_sharable ? (FILE_SHARE_READ | FILE_SHARE_WRITE) : 0), nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
            LoadFileInfo();
            return (IsOpen());
        }
        bool Open() const
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_RDONLY);
#endif
#ifdef __WIN32__
            _handle = ::CreateFile(_name.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
            const_cast<File*>(this)->LoadFileInfo();
            return (IsOpen());
        }
        bool Open(bool readOnly)
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), (readOnly ? O_RDONLY : O_RDWR));
#endif
#ifdef __WIN32__
            _handle = ::CreateFile(_name.c_str(), (readOnly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE), (_sharable ? (FILE_SHARE_READ | (readOnly ? 0 : FILE_SHARE_WRITE)) : 0), nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
            LoadFileInfo();
            return (IsOpen());
        }
        bool Append()
        {
#ifdef __POSIX__
            _handle = open(_name.c_str(), O_RDWR | O_APPEND);

            if ((_handle == -1) && (errno == ENOENT)) {
                return (Create());
            }
#endif
#ifdef __WIN32__
            _handle = ::CreateFile(_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (_handle != INVALID_HANDLE_VALUE) {
                ::SetFilePointer(_handle, 0, nullptr, FILE_END);
            }
#endif
            return (IsOpen());
        }
        bool Destroy()
        {
            bool result = true;

            if (Exists() == true) {
                Close();

#ifdef __POSIX__
                result = (remove(_name.c_str()) == 0);
#endif
#ifdef __WIN32__
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
#ifdef __WIN32__
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
#ifdef __WIN32__
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
            }
            else {
                return (static_cast<uint32_t>(value));
            }
#endif

#ifdef __WIN32__
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
            }
            else {
                return (static_cast<uint32_t>(value));
            }
#endif
#ifdef __WIN32__
            DWORD readBytes = 0;
            ::ReadFile(_handle, buffer, size, &readBytes, nullptr);
            return static_cast<uint32_t>(readBytes);
#endif
        }

        bool Position(const bool relative, int32_t offset)
        {
            // Only call these methods if the file is open.
            ASSERT(IsOpen());

#ifdef __POSIX__
            return (lseek(_handle, offset, (relative ? SEEK_CUR : SEEK_SET)) != -1);
#endif
#ifdef __WIN32__
            return (::SetFilePointer(_handle, offset, nullptr, (relative ? FILE_CURRENT : FILE_BEGIN)) != INVALID_SET_FILE_POINTER);
#endif
        }
        int32_t Position() const
        {
            int32_t result = 0;

            // If the file is not open, we are at the beginning :-)
            if (IsOpen()) {
#ifdef __POSIX__
                result = lseek(_handle, 0, SEEK_CUR);
                if (result == -1) {
                    result = 0;
                }

                ASSERT(result > 0);
#endif
#ifdef __WIN32__
                DWORD newPos = ::SetFilePointer(_handle, 0, nullptr, FILE_CURRENT);

                if (newPos != INVALID_SET_FILE_POINTER) {
                    ASSERT(newPos >= 0);

                    result = (static_cast<int32_t>(newPos));
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
#ifdef __WIN32__
                Handle fd = DuplicateHandle();
                return ((fd > 0) ? ::_fdopen(reinterpret_cast<int>(fd),  (IsReadOnly() ? "r" : "r+")) : nullptr);
#endif
            }
            else {
                return nullptr;
            }
        }

        void LoadFileInfo();

    private:
        inline static uint32_t ExtensionOffset(const string& name)
        {
            int offset;
            return (((offset = name.find_last_of('.')) == -1) ? 0 : offset + 1);
        }
        inline static uint32_t FileNameOffset(const string& name)
        {
            int offset;
            if ((offset = name.find_last_of('/')) != -1) {
                // under linux/posix this is enough..
                offset++;
            }
            else {
                offset = 0;
            }
#ifdef __WIN32__
            int intermediate;
            if ((intermediate = name.find_last_of('\\')) != -1) {
                if (intermediate > offset) {
                    offset = intermediate + 1;
                }
            }
            if ((offset == 0) && ((intermediate = name.find_last_of(':')) != -1)) {
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
        bool _sharable;
        mutable Handle _handle;
    };

    class EXTERNAL Directory {
    public:
        Directory();
        explicit Directory(const TCHAR location[]);
        Directory(const TCHAR location[], const TCHAR filter[]);
        Directory(const Directory& copy);
        ~Directory();

    public:
        static string Normalize(const string& input);

		bool Create();
        bool CreatePath();

        Directory& operator=(const TCHAR location[])
        {
            _name = location;
            return (*this);
        }

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
                }
                else {
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
        inline string Current()
        {
            return (_name + _entry->d_name);
        }
#endif

#ifdef __WIN32__
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
            }
            else if (::FindNextFile(_dirFD, &_data) == FALSE) {
                // We are at the end
                noMoreFiles = true;
            }

            return (IsValid());
        }
        inline string Current() const
        {
            return (_name + _data.cFileName);
        }
#endif

        Directory& operator=(const Directory& RHS)
        {
            _name = RHS._name;
            return (*this);
        }

    private:
        string _name;
        string _filter;

#ifdef __LINUX__
        DIR* _dirFD;
        struct dirent* _entry;
#endif

#ifdef __WIN32__
        HANDLE _dirFD;
        WIN32_FIND_DATA _data;
        bool noMoreFiles;
#endif
    };
}
} // namespace Core

#endif

#include "FileSystem.h"

namespace WPEFramework {
namespace Core {
    File::File(const bool sharable)
        : _name()
        , _size(0)
        , _attributes(0)
        , _creation()
        , _modification()
        , _access()
        , _sharable(sharable)
        , _handle(INVALID_HANDLE_VALUE)
    {
    }
    File::File(const string& fileName, const bool sharable)
        : _name(fileName)
        , _size(0)
        , _attributes(0)
        , _creation()
        , _modification()
        , _access()
        , _sharable(sharable)
        , _handle(INVALID_HANDLE_VALUE)
    {
        LoadFileInfo();
    }
    File::File(const File& copy)
        : _name(copy._name)
        , _size(copy._size)
        , _attributes(copy._attributes)
        , _creation(copy._creation)
        , _modification(copy._modification)
        , _access(copy._access)
        , _sharable(copy._sharable)
        , _handle(copy.DuplicateHandle())
    {
    }
    File::~File()
    {
        Close();
    }
    void File::LoadFileInfo()
    {
#ifdef __WIN32__
        WIN32_FILE_ATTRIBUTE_DATA data;
        GET_FILEEX_INFO_LEVELS infoLevelId = GetFileExInfoStandard;

        if (GetFileAttributesEx(_name.c_str(), infoLevelId, &data) == FALSE) {
            _attributes = 0;
            _size = 0;
        }
        else {
            _size = data.nFileSizeHigh;
            _size = ((_size << 32) | data.nFileSizeLow);
            _creation = Core::Time(data.ftCreationTime);
            _access = Core::Time(data.ftLastAccessTime);
            _modification = Core::Time(data.ftLastWriteTime);
            _attributes = data.dwFileAttributes;
        }
#endif

#ifdef __POSIX__
        struct stat data;
        if (stat(_name.c_str(), &data) == 0) {
            _size = data.st_size;
#ifdef __APPLE__
            _creation = Core::Time(data.st_ctimespec);
            _access = Core::Time(data.st_atimespec);
            _modification = Core::Time(data.st_mtimespec);
#else
            _creation = Core::Time(data.st_ctim);
            _access = Core::Time(data.st_atim);
            _modification = Core::Time(data.st_mtim);
#endif
            _attributes = ((data.st_mode & (S_IFCHR | S_IFIFO)) == (S_IFCHR | S_IFIFO) ? FILE_SYSTEM : 0);
            _attributes |= (data.st_mode & FILE_DIRECTORY);
            _attributes |= (data.st_mode & FILE_NORMAL);
            _attributes |= (data.st_mode & FILE_LINK);
            _attributes |= (access(_name.c_str(), W_OK) == 0 ? 0 : FILE_READONLY);
            _attributes |= (_name[0] == '.' ? FILE_HIDDEN : 0);
            _attributes |= ((data.st_mode & (S_IFCHR | S_IFBLK)) != 0 ? FILE_DEVICE : 0);
        }
        else {
            _attributes = 0;
        }
#endif
    }
    Directory::Directory()
        : _name()
        , _filter()
#ifdef __LINUX__
        , _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WIN32__
        , _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::Directory(const TCHAR location[])
        : _name(Normalize(location))
        , _filter(_T("*"))
        ,
#ifdef __LINUX__
        _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WIN32__
              _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::Directory(const TCHAR location[], const TCHAR filter[])
        : _name(Normalize(location))
        , _filter(filter)
        ,
#ifdef __LINUX__
        _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WIN32__
              _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::Directory(const Directory& copy)
        : _name(copy._name)
        ,
#ifdef __LINUX__
        _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WIN32__
              _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::~Directory()
    {
#ifdef __LINUX__
        if (_dirFD != nullptr) {
            closedir(_dirFD);
            _dirFD = nullptr;
        }
#endif

#ifdef __WIN32__
        if (_dirFD != INVALID_HANDLE_VALUE) {
            ::FindClose(_dirFD);
            _dirFD = INVALID_HANDLE_VALUE;
        }
#endif
    }

    /* static */ string Directory::Normalize(const string& input)
    {
        string result(input);

        // First see if we are not empy.
        if (result.empty() == false) {
            uint32_t length = result.length();

#ifdef __WIN32__
            for (uint32_t teller = 0; teller < length; teller++) {
                if (result[teller] == '\\') {
                    result[teller] = '/';
                }
            }
#endif

#ifdef __WIN32__
            if ((result[length - 1] != '/') && (result[length - 1] != '\\'))
#else
            if (result[length - 1] != '/')
#endif
            {
                result += '/';
            }
        }
        return (result);
    }

    bool Directory::Create()
    {
#ifdef __WIN32__
        return (CreateDirectory(_name.c_str(), nullptr) != FALSE);
#else
        return (mkdir(_name.c_str(), 0775) >= 0);
#endif
    }

    bool Directory::CreatePath()
    {
        /* recursive mkdir based on http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html */

        char tmp[256];
        char* p = nullptr;
        struct stat sb;
        size_t len;

        /* copy path */
        strncpy(tmp, _name.c_str(), sizeof(tmp));
        len = strlen(tmp);
        if (len >= sizeof(tmp)) {
            return false;
        }

        /* remove trailing slash */
        if (tmp[len - 1] == '/') {
            tmp[len - 1] = 0;
        }

        /* recursive mkdir */
        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                /* test path */
                if (stat(tmp, &sb) != 0) {
/* path does not exist - create directory */
#ifdef __WIN32__
                    if (CreateDirectory(tmp, nullptr) == FALSE) {
                        return false;
                    };
#else
                    if (mkdir(tmp, 0775) < 0) {
                        return false;
                    }
#endif
                }
                else if ((sb.st_mode & File::FILE_DIRECTORY) != File::FILE_DIRECTORY) {
                    /* not a directory */
                    return false;
                }
                *p = '/';
            }
        }
        /* test path */
        if (stat(tmp, &sb) != 0) {
/* path does not exist - create directory */
#ifdef __WIN32__
            if (CreateDirectory(tmp, nullptr) == FALSE) {
                return false;
            };
#else
            if (mkdir(tmp, 0775) < 0) {
                return false;
            }
#endif
        }
        else if ((sb.st_mode & File::FILE_DIRECTORY) != File::FILE_DIRECTORY) {
            /* not a directory */
            return false;
        }
        return true;
    }
}
} // namespace Core

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

#include "AccessControl.h"
#include "FileSystem.h"

namespace Thunder {
namespace Core {

    File::File()
        : _name()
        , _size(0)
        , _attributes(0)
        , _creation()
        , _modification()
        , _access()
        , _handle(INVALID_HANDLE_VALUE)
    {
    }
    File::File(const string& fileName)
        : _name(fileName)
        , _size(0)
        , _attributes(0)
        , _creation()
        , _modification()
        , _access()
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
        , _handle(copy.DuplicateHandle())
    {
    }
    File::File(File&& move)
        : _name(std::move(move._name))
        , _size(move._size)
        , _attributes(move._attributes)
        , _creation(std::move(move._creation))
        , _modification(std::move(move._modification))
        , _access(std::move(move._access))
        , _handle(move._handle)
    {
        move._size = 0;
        move._attributes = 0;
        move._handle = INVALID_HANDLE_VALUE;
    }
    File::~File()
    {
        Close();
    }

    /* static */ string File::Normalize(const string& location, bool& valid)
    {
        string result(location);

        valid = true;

        // First see if we are not empy.
        if (result.empty() == false) {
            uint32_t index = 0;

           while (index < result.length()) {

#ifdef __WINDOWS__
                if (result[index] == '\\') {
                    result[index] = '/';
                }
#endif

                if ((result[index] == '/') && (index >= 1) ) {
                    if (result[index - 1] == '/') {
                        if (index >= 2) {
                            // We have a double slash, clear all till the beginning
                           result = result.substr(index - 1);
                           index = 1;
                        }
                    }
                    else if (result[index - 1] == '.') {
                        if ((index == 1) || (result[index - 2] == '/')) {
                            // It is a dot, remove it...
                            uint32_t offset = (index == 1 ? 0 : index - 2);
                            result.erase(offset, 2);
                            index = offset;
                        }
                        else if ((result[index - 2] == '.') && ((index == 2) || (result[index - 3] == '/'))) {
                            if (index <= 3) {
                                valid = false;
                                result.clear();
                            }
                            else {
                                // Seems like we are moving up a directory... execute that on the result... if we can...
                                // there is data we can drop, drop it, drop it till the '/' is found
                                uint32_t offset = index - 4;
                                while ((offset > 0) && (result[offset] != '/')) {
                                    offset--;
                                }
                                result.erase(offset, index - offset);
                                index = offset;
                            }
                        }
                    }
                }
                index++;
           }

           // It could be that the last slash is not part of the full line, check the last part, assuming there is such a slash, 
           // normalization rules applyt than as well....
           if ((result.length() >= 1) && (result[result.length() - 1] == '.')) {

               if (result.length() == 1) {
                   // We have only a dot...
                   result.clear();
               }
               else if ( (result.length() == 2) && (result[0] == '.') ) {
                   // We have a ".." and nothing more
                   valid = false;
                   result.clear();
               }
               else if ((result.length() >= 2) && (result[result.length() - 2] == '/')) {
                   result = result.substr(0, result.length() - 2);
               }
               else if ((result.length() >= 3) && (result[result.length() - 2] == '.') && (result[result.length() - 3] == '/')) {
                   // How about ThisFile/.., it is valid, but /.. would not be, both end up at an empty string... but the difference
                   // is the fact that the first, had a length > 3 and the second was exactly 3, so a length of 3 is invalid and empty..
                   if (result.length() == 3) {
                       valid = false;
                       result.clear();
                   }
                   else {
                       // there is data we can drop, drop it, drop it till the '/' is found
                       uint32_t offset = static_cast<uint32_t>(result.length() - 4);

                        while ((offset > 0) && (result[offset] != '/')) {
                            offset--;
                        }
                        result = result.substr(0, offset);
                   }
               }
           }
        }
        return (result);
    }

    void File::LoadFileInfo()
    {
#ifdef __WINDOWS__
        WIN32_FILE_ATTRIBUTE_DATA data;
        GET_FILEEX_INFO_LEVELS infoLevelId = GetFileExInfoStandard;

        if (GetFileAttributesEx(_name.c_str(), infoLevelId, &data) == FALSE) {
            _attributes = 0;
            _size = 0;
        } else {
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

            if (lstat(_name.c_str(), &data) == 0) {
                _attributes |= (data.st_mode & FILE_LINK);
            }

        } else {
            _attributes = 0;
        }
#endif
    }

    uint32_t File::User(const string& userName) const
    {
        return AccessControl::OwnerShip(_name, userName, "");
    }

    uint32_t File::Group(const string& groupName) const
    {
        return AccessControl::OwnerShip(_name, "", groupName);
    }

    uint32_t File::Permission(uint16_t flags) const
    {
        return AccessControl::Permission(_name, flags);
    }


    Directory::Directory()
        : _name()
        , _filter()
#ifdef __LINUX__
        , _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WINDOWS__
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
#ifdef __WINDOWS__
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
#ifdef __WINDOWS__
              _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::Directory(const Directory& copy)
        : _name(copy._name)
        , _filter(copy._filter)
#ifdef __LINUX__
        , _dirFD(nullptr)
        , _entry(nullptr)
#endif
#ifdef __WINDOWS__
        , _dirFD(INVALID_HANDLE_VALUE)
        , noMoreFiles(false)
#endif
    {
    }
    Directory::Directory(Directory&& move)
        : _name(std::move(move._name))
        , _filter(std::move(move._filter))
    {
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
    Directory::~Directory()
    {
#ifdef __LINUX__
        if (_dirFD != nullptr) {
            closedir(_dirFD);
            _dirFD = nullptr;
        }
#endif

#ifdef __WINDOWS__
        if (_dirFD != INVALID_HANDLE_VALUE) {
            ::FindClose(_dirFD);
            _dirFD = INVALID_HANDLE_VALUE;
        }
#endif
    }

    bool Directory::Create()
    {
#ifdef __WINDOWS__
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
        strncpy(tmp, _name.c_str(), sizeof(tmp) - 1);
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
#ifdef __WINDOWS__
                    if (!((tmp[1] == ':') && (tmp[2] == '\0'))) {
                        if (CreateDirectory(tmp, nullptr) == FALSE) {
                            return false;
                        }
                    }
#else
                    if (mkdir(tmp, 0775) < 0) {
                        return false;
                    }
#endif
                } else if ((sb.st_mode & File::FILE_DIRECTORY) != File::FILE_DIRECTORY) {
                    /* not a directory */
                    return false;
                }
                *p = '/';
            }
        }
        /* test path */
        if (stat(tmp, &sb) != 0) {
/* path does not exist - create directory */
#ifdef __WINDOWS__
            if (CreateDirectory(tmp, nullptr) == FALSE) {
                return false;
            };
#else
            if (mkdir(tmp, 0775) < 0) {
                return false;
            }
#endif
        } else if ((sb.st_mode & File::FILE_DIRECTORY) != File::FILE_DIRECTORY) {
            /* not a directory */
            return false;
        }
        return true;
    }

    bool Directory::Exists() const {
        bool result = false;

#ifdef __WINDOWS__
        WIN32_FILE_ATTRIBUTE_DATA data;
        GET_FILEEX_INFO_LEVELS infoLevelId = GetFileExInfoStandard;

        if (GetFileAttributesEx(_name.c_str(), infoLevelId, &data) != FALSE) {
            result = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
#endif

#ifdef __POSIX__
        struct stat data;
        if (stat(_name.c_str(), &data) == 0) {
            result = ( (data.st_mode & S_IFDIR) != 0);
        }
#endif

        return (result);

    }

    uint32_t Directory::User(const string& userName) const
    {
        return AccessControl::OwnerShip(_name, userName, "");
    }

    uint32_t Directory::Group(const string& groupName) const
    {
        return AccessControl::OwnerShip(_name, "", groupName);
    }

    uint32_t Directory::Permission(uint16_t flags) const
    {
        return AccessControl::Permission(_name, flags);
    }


    string Partition::RemoveRepeatedPathSeparator(const string& path)
    {
        string trimmedPath;
        if (path.empty() != true) {
            uint32_t index = 0;
            while (index < path.size()) {
                if ((index + 1 < path.size()) && (path[index] == PathSeparator) && (path[index] == path[index + 1])) {
                    index++;
                    continue;
                }
                trimmedPath += path[index];
                index++;
            }
        }
        return trimmedPath;
    }

    bool Partition::ReadPartitionName(const string& fileName, string& deviceName)
    {
#ifdef __LINUX__
        Core::File path(fileName);
        if ((path.Open() == true)) {
            string mountStatsFileName(MountStatsFileName);
            Core::File mountStatsFile(mountStatsFileName);

            if (mountStatsFile.Open() == true) {
                string mountStats;
                uint32_t size = 0;
                do {
                    uint8_t buffer[PARTITION_BUFFER_SIZE];
                    if ((size = mountStatsFile.Read(buffer, PARTITION_BUFFER_SIZE)) > 0) {
                        string buf(reinterpret_cast<char*>(buffer), size);
                        mountStats += buf;
                    }
                } while (size > 0);

                if (mountStats.empty() != true) {
                    size_t found = 0;
                    size_t current = 0;
                    while ((found = mountStats.find(LineSeparator, current)) != std::string::npos) {
                        string entry = mountStats.substr(current, found - current);
                        if (entry.size()) {
                            string mounted;
                            size_t mountPointStart = entry.find(MountKey, 0);
                            if (mountPointStart != std::string::npos) {
                                mountPointStart = mountPointStart + strlen(MountKey);
                                size_t mountPointEnd = entry.find(" ", mountPointStart);
                                mounted = entry.substr(mountPointStart, (mountPointEnd - mountPointStart));
                            }

                            string mount = fileName;
                            if (mounted.size()) {
                                do {
                                    mount = ((mount.size() > 1) && (mount.at(mount.size() - 1) == PathSeparator)) ? mount.substr(0, mount.size() - 1): mount;
                                    if (mounted == mount) {
                                        size_t deviceNameStart = strlen(DeviceKey);
                                        deviceName = entry.substr(deviceNameStart, (entry.find(WordSeparator, deviceNameStart) - deviceNameStart));
                                        break;
                                    } else {
                                        if (mount.size() > 1) {
                                            size_t delimEnd = mount.find_last_of(PathSeparator);
                                           if (delimEnd != std::string::npos) {
                                               mount = mount.substr(0, mount.size() - (mount.size() - delimEnd) + 1);
                                           } else {
                                               mount.clear();
                                           }
                                        } else {
                                            mount.clear();
                                        }
                                    }
                                } while (mount.size());
                            }
                        }
                        current = found + 1;
                    }
                }
            }
        }

        return (deviceName.empty() != true);
#else
        return false;
#endif
    }

    void Partition::LoadPartitionInfo()
    {
#ifdef __POSIX__
        StatFS statFSData;

        if (statvfs (_path.c_str(), &statFSData) == 0) {
            uint64_t blockSize = (statFSData.f_frsize != 0) ? statFSData.f_frsize: statFSData.f_bsize;
            _free = statFSData.f_bavail * blockSize;
            _size = statFSData.f_blocks * blockSize;
        }
        ReadPartitionName(_path, _name);
#endif

#ifdef __WINDOWS__
        string dirName = _path;
        File path(_path);
        if (path.IsDirectory() != true) {
            // size_t position = str.find_last_of(ch);
            // if (position) {
            //    dirName = s.substr(0, position);
            //}
        }
        if (dirName.empty() != true) {
            // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdiskfreespaceexa
            // BOOL GetDiskFreeSpaceExA(
            //    LPCSTR          lpDirectoryName,
            //    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
            //    PULARGE_INTEGER lpTotalNumberOfBytes,
            //    PULARGE_INTEGER lpTotalNumberOfFreeBytes
            // );
        }
#endif
    }
}
} // namespace Core

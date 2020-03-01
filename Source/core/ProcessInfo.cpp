 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#include "ProcessInfo.h"
#include "FileSystem.h"

#ifdef __WINDOWS__
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <libproc.h>
#endif

namespace WPEFramework {
namespace Core {
#ifndef __WINDOWS__
    const uint32_t PageSize = getpagesize();
#endif

#ifdef __WINDOWS__
    static string ExecutableName(HANDLE handle)
    {
        string result;

        if (handle) {
            TCHAR Buffer[MAX_PATH];
            if (::GetModuleFileNameEx(handle, 0, Buffer, MAX_PATH)) {
                result = Buffer;
            }
        }

        return (result);
    }
#elif defined(__APPLE__)
    void ProcessName(const uint32_t pid, TCHAR buffer[], const uint32_t maxLength)
    {
        proc_pidpath(pid, buffer, maxLength);
    }
    static string ExecutableName(HANDLE handle)
    {
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];

        ProcessName(handle, pathbuf, sizeof(pathbuf));
        return pathbuf;
    }

#else
    void ProcessName(const uint32_t pid, TCHAR buffer[], const uint32_t maxLength)
    {
        ASSERT(maxLength > 0);

        if (maxLength > 0) {
            char procpath[48];
            snprintf(procpath, sizeof(procpath), "/proc/%u/exe", pid);
            ssize_t length = readlink(procpath, buffer, maxLength - 1);

            if (length > 0) {
                buffer[length] = '\0';
            } else {
                int fd;

                snprintf(procpath, sizeof(procpath), "/proc/%u/comm", pid);

                if ((fd = open(procpath, O_RDONLY)) > 0) {
                    ssize_t size;
                    if ((size = read(fd, buffer, maxLength - 1)) > 0) {
                        if(buffer[size - 1] == '\n') {
                            buffer[size - 1] = '\0';
                        } else {
                            buffer[size] = '\0';
                        }
                    } else {
                        buffer[0] = '\0';
                    }

                    close(fd);
                } else {
                    buffer[0] = '\0';
                }
            }
        }
    }

    static string ExecutableName(const uint32_t PID)
    {
        char fullname[PATH_MAX];

        ProcessName(PID, fullname, sizeof(fullname));

        return (string(fullname));
    }

    // Iterate over Processes
    static void FindChildren(const uint32_t parent, std::list<uint32_t>& children)
    {
        DIR* dp;
        struct dirent* ep;

        children.clear();

        dp = opendir("/proc");
        if (dp != nullptr) {
            while (nullptr != (ep = readdir(dp))) {
                int pid;
                char* endptr;

                pid = strtol(ep->d_name, &endptr, 10);

                if ('\0' == endptr[0]) {
                    // We have a valid PID, Find, the parent of this process..
                    TCHAR buffer[512];
                    int fd;

                    snprintf(buffer, sizeof(buffer), "/proc/%d/stat", pid);
                    if ((fd = open(buffer, O_RDONLY)) > 0) {
                        if (read(fd, buffer, sizeof(buffer)) > 0) {
                            int ppid = 0;
                            sscanf(buffer, "%*d (%*[^)]) %*c %d", &ppid);

                            if (static_cast<uint32_t>(ppid) == parent) {
                                children.push_back(pid);
                            }
                        }

                        close(fd);
                    }
                }
            }

            (void)closedir(dp);
        }
    }

    // Iterate over Processes
    static void FindPid(const string& item, const bool exact, std::list<uint32_t>& pids)
    {
        DIR* dp;
        struct dirent* ep;

        pids.clear();

        string fileName(Core::File::FileNameExtended(item));
        bool fullMatch(exact && (Core::File::PathName(item).empty() == false));

        dp = opendir("/proc");
        if (dp != nullptr) {
            while (nullptr != (ep = readdir(dp))) {
                int pid;
                char* endptr;

                pid = strtol(ep->d_name, &endptr, 10);

                if ('\0' == endptr[0]) {
                    // We have a valid PID, Find, the parent of this process..
                    TCHAR buffer[512];
                    ProcessName(pid, buffer, sizeof(buffer));

                    if (fullMatch == true) {
                        if (item == buffer) {
                            pids.push_back(pid);
                        }
                    } else {
                        if (fileName == Core::File::FileNameExtended(string(buffer))) {
                            pids.push_back(pid);
                        }
                    }
                }
            }

            (void)closedir(dp);
        }
    }

#endif

    // Get the Processes with this name.
    ProcessInfo::Iterator::Iterator(const string& name, const bool exact)
    {
#ifdef __WINDOWS__
        HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 processInfo;
        processInfo.dwSize = sizeof(PROCESSENTRY32);
        int index = 0;

        while (Process32Next(hSnapShot, &processInfo) != FALSE) {
            HANDLE Handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processInfo.th32ProcessID);

            if (Handle) {
                TCHAR Buffer[MAX_PATH];
                if (::GetModuleFileNameEx(Handle, 0, Buffer, MAX_PATH)) {
                    if (name == Buffer) {
                        // Add this entry to the list
                        _pids.push_back(static_cast<uint32_t>(processInfo.th32ProcessID));
                    }
                }
                CloseHandle(Handle);
            }
        }
#else
        FindPid(name, exact, _pids);
#endif
        Reset();
    }

    // Get the Children of the given PID.
    ProcessInfo::Iterator::Iterator(const uint32_t parentPID)
    {
#ifdef __WINDOWS__
        HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32 processInfo;
        processInfo.dwSize = sizeof(PROCESSENTRY32);
        int index = 0;

        while (Process32Next(hSnapShot, &processInfo) != FALSE) {
            if (static_cast<uint32_t>(processInfo.th32ParentProcessID) == parentPID) {
                // Add this entry to the list
                _pids.push_back(static_cast<uint32_t>(processInfo.th32ProcessID));
            }
        }
#else
        FindChildren(parentPID, _pids);
#endif

        Reset();
    }

    // Current Process Information
    ProcessInfo::ProcessInfo()
#ifdef __WINDOWS__
        : _pid(GetCurrentProcessId())
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#else
        : _pid(getpid())
#endif
    {
    }

    // Copy Info
    ProcessInfo::ProcessInfo(const ProcessInfo& copy)
        : _pid(copy._pid)
#ifdef __WINDOWS__
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#endif
    {
    }
    // Specifice Process Info
    ProcessInfo::ProcessInfo(const uint32_t id)
        : _pid(id)
#ifdef __WINDOWS__
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#endif
    {
    }

    ProcessInfo::~ProcessInfo()
    {
#ifdef __WINDOWS__
        if (_handle) {
            CloseHandle(_handle);
        }
#endif
    }

    ProcessInfo& ProcessInfo::operator=(const ProcessInfo& rhs)
    {
        _pid = rhs._pid;

#ifdef __WINDOWS__
        if (_handle) {
            CloseHandle(_handle);
        }
        _handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid);
#endif

        return (*this);
    }
    uint64_t ProcessInfo::Allocated() const
    {
        uint64_t result = 0;

#ifdef __WINDOWS__
        if (_handle) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(_handle, &pmc, sizeof(pmc))) {
                result = pmc.WorkingSetSize;
            }
        }
#else
        int fd;
        TCHAR buffer[128];
        int VmSize = 0;

        snprintf(buffer, sizeof(buffer), "/proc/%d/statm", _pid);
        if ((fd = open(buffer, O_RDONLY)) > 0) {
            if (read(fd, buffer, sizeof(buffer)) > 0) {
                sscanf(buffer, "%d", &VmSize);
                result = VmSize * PageSize;
            }
            close(fd);
        }
#endif

        return (result);
    }
    uint64_t ProcessInfo::Resident() const
    {
        uint64_t result = 0;

#ifdef __WINDOWS__
        if (_handle) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(_handle, &pmc, sizeof(pmc))) {
                result = pmc.QuotaPagedPoolUsage + pmc.QuotaNonPagedPoolUsage;
            }
        }
#else
        int fd;
        TCHAR buffer[128];
        int VmRSS = 0;

        snprintf(buffer, sizeof(buffer), "/proc/%d/statm", _pid);
        if ((fd = open(buffer, O_RDONLY)) > 0) {
            if (read(fd, buffer, sizeof(buffer)) > 0) {
                sscanf(buffer, "%*d %d", &VmRSS);
                result = VmRSS * PageSize;
            }
            close(fd);
        }
#endif

        return (result);
    }
    uint64_t ProcessInfo::Shared() const
    {
        uint64_t result = 0;

#ifdef __WINDOWS__
        if (_handle) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(_handle, &pmc, sizeof(pmc))) {
                result = pmc.QuotaPagedPoolUsage + pmc.QuotaNonPagedPoolUsage;
            }
        }
#else
        int fd;
        TCHAR buffer[128];
        int Share = 0;

        snprintf(buffer, sizeof(buffer), "/proc/%d/statm", _pid);
        if ((fd = open(buffer, O_RDONLY)) > 0) {
            if (read(fd, buffer, sizeof(buffer)) > 0) {
                sscanf(buffer, "%*d %*d %d", &Share);
                result = Share * PageSize;
            }
            close(fd);
        }
#endif

        return (result);
    }
    string ProcessInfo::Name() const
    {
#ifdef __WINDOWS__
        return (Core::File::FileName(ExecutableName(_handle)));
#else
        return (Core::File::FileName(ExecutableName(_pid)));
#endif
    }
    string ProcessInfo::Executable() const
    {
#ifdef __WINDOWS__
        return (ExecutableName(_handle));
#else
        return (ExecutableName(_pid));
#endif
    }
    uint32_t ProcessInfo::Group(const string& groupName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (groupName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WINDOWS__
            struct group* grp = getgrnam(groupName.c_str());
            if (grp != nullptr) {
                result = (::setpgid(_pid, grp->gr_gid) == 0 ? ERROR_NONE : ERROR_UNAVAILABLE);
            }
#endif
        }
        return (result);
    }
    string ProcessInfo::Group() const
    {
        string result;
#ifndef __WINDOWS__
        struct group* grp = getgrgid(::getpgid(_pid));
        if (grp != nullptr) {
            result = grp->gr_name;
        }
#endif
        return (result);
    }
    /* static */ uint32_t ProcessInfo::User(const string& userName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (userName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WINDOWS__
            struct passwd* pwd = getpwnam(userName.c_str());
            if (pwd != nullptr) {
                result = (::setuid(pwd->pw_uid) == 0 ? ERROR_NONE : ERROR_UNAVAILABLE);
            }
#endif
        }
        return (result);
    }
    /* static */ string ProcessInfo::User()
    {
        string result;
#ifndef __WINDOWS__
        struct passwd* pwd = getpwuid(::getuid());
        if (pwd != nullptr) {
            result = pwd->pw_name;
        }
#endif
        return (result);
    }
}
}

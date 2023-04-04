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

#include "ProcessInfo.h"
#include "FileSystem.h"
#include "SystemInfo.h"

#ifdef __WINDOWS__
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <sys/prctl.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <fstream>
#include <sstream>

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
                        if (buffer[size - 1] == '\n') {
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

    template <typename ACCEPTFUNCTION>
    static void FindChildren(std::list<uint32_t>& children, ACCEPTFUNCTION acceptfunction)
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
                    memset(buffer, 0, sizeof(buffer));
                    int fd;

                    snprintf(buffer, sizeof(buffer), "/proc/%d/stat", pid);
                    if ((fd = open(buffer, O_RDONLY)) > 0) {
                        if (read(fd, buffer, sizeof(buffer) - sizeof(buffer[0])) > 0) {
                            int ppid = 0;
                            sscanf(buffer, "%*d (%*[^)]) %*c %d", &ppid);

                            if (acceptfunction(ppid, pid) == true) {
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

    /**
     * Find PID of processes with a name specified by *item*.
     * If exact = true - search will match any process entry,
     * that starts with value of *item*. The search is case-sensitive
     * in both cases.
     */
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
                    TCHAR buffer[512];
                    ProcessName(pid, buffer, sizeof(buffer));

                    if (fullMatch == true) {
                        if (item == buffer) {
                            pids.push_back(pid);
                        }
                    } else {
                        auto entry = Core::File::FileNameExtended(string(buffer));
                        if (entry.rfind(item, 0) == 0) {
                            pids.push_back(pid);
                        }
                    }
                }
            }

            (void)closedir(dp);
        }
    }
#endif
    /*
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
*/

    // Get all processes
    ProcessInfo::Iterator::Iterator()
        : _pids()
        , _current()
        , _index(0)
    {
#ifndef __WINDOWS__
        FindChildren(_pids, [=](const uint32_t, const uint32_t) { return true; });
#endif
        Reset();
    }

    // Get the Child Processes with a name name from a Parent with a certain name
    ProcessInfo::Iterator::Iterator(const string& parentname, const string& childname, const bool removepath)
        : _pids()
        , _current()
        , _index(0)
    {
#ifndef __WINDOWS__
        FindChildren(_pids, [=](const process_t foundparentPID, const uint32_t childPID) {
            bool accept = false;
            char fullname[PATH_MAX];
            ProcessName(foundparentPID, fullname, sizeof(fullname));

            accept = (parentname == (removepath == true ? Core::File::FileNameExtended(fullname) : fullname));

            if (accept == true) {
                ProcessName(childPID, fullname, sizeof(fullname));
                accept = (childname == (removepath == true ? Core::File::FileNameExtended(fullname) : fullname));
            }
            return accept;
        });
#endif
        Reset();
    }

    // Get the Child Processes with a name name from a Parent pid
    ProcessInfo::Iterator::Iterator(const process_t parentPID, const string& childname, const bool removepath)
        : _pids()
        , _current()
        , _index(0)
    {
#ifndef __WINDOWS__
        FindChildren(_pids, [=](const process_t foundparentPID, const uint32_t childPID) {
            bool accept = false;

            if (parentPID == foundparentPID) {
                char fullname[PATH_MAX];
                ProcessName(childPID, fullname, sizeof(fullname));
                accept = (childname == (removepath == true ? Core::File::FileNameExtended(fullname) : fullname));
            }
            return accept;
        });
#endif
        Reset();
    }

    // Get the Children of the given PID.
    ProcessInfo::Iterator::Iterator(const process_t parentPID)
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
        FindChildren(_pids, [=](const uint32_t foundparentPID, const uint32_t) {
            return parentPID == foundparentPID;
        });
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
        , _memory(_pid)
    {
    }

    // Copy Info
    ProcessInfo::ProcessInfo(const ProcessInfo& copy)
        : _pid(copy._pid)
        , _memory(copy._memory)
#ifdef __WINDOWS__
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#endif
    {
    }
    // Specifice Process Info
    ProcessInfo::ProcessInfo(const process_t id)
        : _pid(id)
        , _memory(id)
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
        if (&rhs == this) {
            return *this;
        }

        _pid = rhs._pid;
        _memory = rhs._memory;

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
            ssize_t readAmount = 0;
            if ((readAmount = read(fd, buffer, sizeof(buffer))) > 0) {
                ssize_t nulIndex = std::min(readAmount, static_cast<ssize_t>(sizeof(buffer) - 1));
                buffer[nulIndex] = '\0';
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
            ssize_t readAmount = 0;
            if ((readAmount = read(fd, buffer, sizeof(buffer))) > 0) {
                ssize_t nulIndex = std::min(readAmount, static_cast<ssize_t>(sizeof(buffer) - 1));
                buffer[nulIndex] = '\0';
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
            ssize_t readAmount = 0;
            if ((readAmount = read(fd, buffer, sizeof(buffer))) > 0) {
                ssize_t nulIndex = std::min(readAmount, static_cast<ssize_t>(sizeof(buffer) - 1));
                buffer[nulIndex] = '\0';
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
    void ProcessInfo::Name(const string& name)
    {
#ifdef __WINDOWS__
        if (GetCurrentProcessId() == _pid) {
        }
#else
        if (static_cast<uint32_t>(::getpid()) == _pid) {
            prctl(PR_SET_NAME, name.c_str(), 0, 0, 0, 0);
        }
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

#ifndef __WINDOWS__
    std::list<string> ProcessInfo::CommandLine() const
    {
        char procPath[PATH_MAX];
        sprintf(procPath, "/proc/%u/cmdline", _pid);

        std::list<string> output;

        FILE* cmdFile = fopen(procPath, "rb");
        if (cmdFile == nullptr) {
            TRACE_L1("Failed to open /proc/.../cmdline for process %u", _pid);
        } else {
            uint32_t index = 0;
            const int bufferSize = 256;
            char buffer[bufferSize];
            while (true) {
                char c;
                size_t readChars = fread(&c, 1, 1, cmdFile);
                if (readChars == 0) {
                    break;
                }

                buffer[index] = c;
                if (c == '\0') {
                    output.emplace_back(buffer);
                    index = 0;
                    continue;
                }

                index++;
                if (index == (sizeof(buffer) - 1)) {
                    TRACE_L1("Command line argument is too big, will split in parts");
                    buffer[index] = '\0';
                    output.emplace_back(buffer);
                    index = 0;
                    continue;
                }
            }

            if (index != 0) {
                buffer[index] = '\0';
                output.emplace_back(buffer);
                index = 0;
            }

            fclose(cmdFile);
        }

        return output;
    }
#endif

    /* static */ void ProcessInfo::FindByName(const string& name, const bool exact, std::list<ProcessInfo>& processInfos)
    {
#ifndef __WINDOWS__
        std::list<uint32_t> pidList;
        FindPid(name, exact, pidList);

        processInfos.clear();
        for (const pid_t pid : pidList) {
            processInfos.emplace_back(pid);
        }

#endif // !__WINDOWS__
    }

    string ProcessCurrent::User() const
    {
        string userName;
#ifndef __WINDOWS__
        struct passwd* pwd = getpwuid(::getuid());
        if (pwd != nullptr) {
            userName = pwd->pw_name;
        }
#endif
        return (userName);
    }

    uint32_t ProcessCurrent::User(const string& userName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (userName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WINDOWS__
            struct passwd* pwd = getpwnam(userName.c_str());
            if (pwd != nullptr) {
                SupplementryGroups(userName);
                result = (::setuid(pwd->pw_uid) == 0 ? ERROR_NONE : ERROR_UNAVAILABLE);
            }
#endif
        }
        return (result);
    }

    void ProcessCurrent::SupplementryGroups(const string& userName)
    {
#ifndef __WINDOWS__
        struct passwd* pwd = getpwnam(userName.c_str());
        if (pwd != nullptr) {
            int numberOfGroups = 0;

            // Collect number of groups in which this user added
            getgrouplist(pwd->pw_name, pwd->pw_gid, nullptr, &numberOfGroups);
            gid_t groups[numberOfGroups];
            memset(groups, 0, sizeof(groups));

            // Collect actual groups details
            getgrouplist(pwd->pw_name, pwd->pw_gid, groups, &numberOfGroups);
            if (numberOfGroups > 0) {
                setgroups(numberOfGroups, groups);
            }
        }
#endif
    }

    string ProcessCurrent::Group() const
    {
        string groupName;
#ifndef __WINDOWS__
        struct group* grp = getgrgid(::getgid());
        if (grp != nullptr) {
            groupName = grp->gr_name;
        }
#endif
        return (groupName);
    }

    uint32_t ProcessCurrent::Group(const string& groupName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (groupName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WINDOWS__
            struct group* grp = getgrnam(groupName.c_str());
            if (grp != nullptr) {
                result = (::setgid(grp->gr_gid) == 0 ? ERROR_NONE : ERROR_UNAVAILABLE);
            }
#endif
        }
        return (result);
    }

    static void EnumerateChildProcesses(const ProcessInfo& processInfo, std::list<ProcessInfo>& pids)
    {
        pids.push_back(processInfo);

        Core::ProcessInfo::Iterator iterator(processInfo.Children());
        while (iterator.Next()) {
            EnumerateChildProcesses(iterator.Current(), pids);
        }
    }

    ProcessTree::ProcessTree(const ProcessInfo& processInfo)
    {
        EnumerateChildProcesses(processInfo, _processes);
    }

    bool ProcessTree::ContainsProcess(ThreadId pid) const
    {
PUSH_WARNING(DISABLE_WARNING_CONVERSION_TO_GREATERSIZE)
        auto comparator = [pid](const ProcessInfo& processInfo) { return ((ThreadId)(processInfo.Id()) == pid); };
POP_WARNING()
        std::list<ProcessInfo>::const_iterator i = std::find_if(_processes.cbegin(), _processes.cend(), comparator);
        return (i != _processes.cend());
    }

    void ProcessTree::GetProcessIds(std::list<ThreadId>& processIds) const
    {
        processIds.clear();

        for (const ProcessInfo& process : _processes) {
PUSH_WARNING(DISABLE_WARNING_CONVERSION_TO_GREATERSIZE)
            processIds.push_back((ThreadId)(process.Id()));
POP_WARNING()
        }
    }

    ThreadId ProcessTree::RootId() const
    {
PUSH_WARNING(DISABLE_WARNING_CONVERSION_TO_GREATERSIZE)
        return (ThreadId)(_processes.front().Id());
POP_WARNING()
    }

    ProcessInfo::Memory::Memory(const process_t pid)
        : _pid(pid)
        , _uss(0)
        , _pss(0)
        , _rss(0)
        , _vss(0)
        , _shared(0)
    {
    }

    ProcessInfo::Memory::Memory(const ProcessInfo::Memory& other)
        : _pid(other._pid)
        , _uss(0)
        , _pss(0)
        , _rss(0)
        , _vss(0)
        , _shared(0)
    {
    }

    ProcessInfo::Memory& ProcessInfo::Memory::operator=(const ProcessInfo::Memory& other)
    {
        if (&other == this) {
            return *this;
        }
        _pid = other._pid;
        _uss = 0;
        _pss = 0;
        _rss = 0;
        _shared = 0;
        return *this;
    }

    void ProcessInfo::Memory::MemoryStats()
    {
        _uss = 0;
        _pss = 0;
        _rss = 0;
        _vss = 0;
        _shared = 0;
        
        string path = "/proc/";
        path += std::to_string(_pid);
        path += "/smaps";
        
        std::ifstream smaps(path);
        if (!smaps.is_open()) {
            TRACE_L1(_T("Could not open /proc/%d/smaps. Memory monitoring of this process is unavailable!"), _pid);
        }

        std::string line;
        std::string key;
        uint64_t value;
        std::istringstream iss("");

        while (std::getline(smaps, line)) {

            iss.str(line);
            iss >> key;

            if (key == _T("Size:")) {
                iss >> value;
                _vss += value;
            } else if (key == _T("Rss:")) {
                iss >> value;
                _rss += value;
            } else if (key == _T("Pss:")) {
                iss >> value;
                _pss += value;
            } else if (key == _T("Private_Clean:")) {
                iss >> value;
                _uss += value;
            } else if (key == _T("Private_Dirty:")) {
                iss >> value;
                _uss += value;
            } else if (key == _T("Shared_Dirty:")) {
                iss >> value;
                _shared += value;
            } else if (key == _T("Shared_Clean:")) {
                iss >> value;
                _shared += value;
            }

            iss.clear();
        }
    }
}
}

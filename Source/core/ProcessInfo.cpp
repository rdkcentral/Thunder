#include "ProcessInfo.h"
#include "FileSystem.h"

#ifdef __WIN32__
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#ifdef  __APPLE__
#include <libproc.h>
#endif

namespace WPEFramework {
namespace Core {
#ifndef __WIN32__
    const uint32_t PageSize = getpagesize();
#endif

#ifdef __WIN32__
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
        proc_pidpath (pid, buffer, maxLength);
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
        snprintf(buffer, maxLength, "/proc/%d/exe", pid);
        int32_t length = readlink(buffer, buffer, maxLength - 1);

        if (length > 0) {
            buffer[length] = '\0';
        }
        else {
            int fd;

            snprintf(buffer, maxLength, "/proc/%d/status", pid);

            if ((fd = open(buffer, O_RDONLY)) > 0) {
                if (read(fd, buffer, (maxLength > 48 ? 48 : maxLength)) > 0) {
                    sscanf(buffer, "Name: %s", buffer);
                }
                else {
                    buffer[0] = '\0';
                }

                close(fd);
            }
            else {
                buffer[0] = '\0';
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
                    }
                    else {
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
#ifdef __WIN32__
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
#ifdef __WIN32__
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
#ifdef __WIN32__
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
#ifdef __WIN32__
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#endif
    {
    }
    // Specifice Process Info
    ProcessInfo::ProcessInfo(const uint32_t id)
        : _pid(id)
#ifdef __WIN32__
        , _handle(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, _pid))
#endif
    {
    }

    ProcessInfo::~ProcessInfo()
    {
#ifdef __WIN32__
        if (_handle) {
            CloseHandle(_handle);
        }
#endif
    }

    ProcessInfo& ProcessInfo::operator=(const ProcessInfo& rhs)
    {
        _pid = rhs._pid;

#ifdef __WIN32__
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

#ifdef __WIN32__
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

#ifdef __WIN32__
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

#ifdef __WIN32__
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
#ifdef __WIN32__
        return (Core::File::FileName(ExecutableName(_handle)));
#else
        return (Core::File::FileName(ExecutableName(_pid)));
#endif
    }
    string ProcessInfo::Executable() const
    {
#ifdef __WIN32__
        return (ExecutableName(_handle));
#else
        return (ExecutableName(_pid));
#endif
    }
}
}

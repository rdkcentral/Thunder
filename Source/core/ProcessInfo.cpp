#include "ProcessInfo.h"
#include "FileSystem.h"
#include "SystemInfo.h"

#ifdef __WIN32__
#include <psapi.h>
#include <tlhelp32.h>
#else
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <unistd.h>
#endif

#include <fstream>
#include <sstream>

#ifdef __APPLE__
#include <libproc.h>
#endif

namespace
{
    // Used to parse /proc/PID/maps
    struct MemRange
    {
        uintptr_t m_start;
        uintptr_t m_end;

        explicit MemRange(const string & mapsLine)
            : m_start(0)
            , m_end(0)
        {
            // TODO: sscanf seems to be perfect here
            size_t spaceIndex = mapsLine.find(' ');
            string rangeStr = mapsLine.substr(0, spaceIndex);
            size_t dashIndex = rangeStr.find('-');
            string startStr = rangeStr.substr(0, dashIndex);
            string endStr = rangeStr.substr(dashIndex + 1);

            std::istringstream issStart(startStr);
            issStart >> std::hex >> m_start;
            std::istringstream issEnd(endStr);
            issEnd >> std::hex >> m_end;
        }
    };
}

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
    template<typename ACCEPTFUNCTION>
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
                    int fd;

                    snprintf(buffer, sizeof(buffer), "/proc/%d/stat", pid);
                    if ((fd = open(buffer, O_RDONLY)) > 0) {
                        if (read(fd, buffer, sizeof(buffer)) > 0) {
                            int ppid = 0;
                            sscanf(buffer, "%*d (%*[^)]) %*c %d", &ppid);

                            if( acceptfunction(ppid, pid) == true ) {
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
/*
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
*/

    // Get all processes
    ProcessInfo::Iterator::Iterator()
    : _pids()
    , _current()
    , _index(0) {
#ifndef __WIN32__
        FindChildren(_pids, [=](const uint32_t foundparentPID, const uint32_t childPID) { return true; });
#endif
        Reset();
    }

    // Get the Child Processes with a name name from a Parent with a certain name
    ProcessInfo::Iterator::Iterator(const string& parentname, const string& childname, const bool removepath)
    : _pids()
    , _current()
    , _index(0) {
#ifndef __WIN32__
        FindChildren(_pids, [=](const uint32_t foundparentPID, const uint32_t childPID) {
            bool accept = false;
            char fullname[PATH_MAX];
            ProcessName(foundparentPID, fullname, sizeof(fullname));

            accept = ( parentname == ( removepath == true ? Core::File::FileNameExtended(fullname) : fullname ) );

            if ( accept == true ) {
                ProcessName(childPID, fullname, sizeof(fullname));
                accept = ( childname == ( removepath == true ? Core::File::FileNameExtended(fullname) : fullname ) );
            }
            return accept;
        });
#endif
        Reset();
    }

    // Get the Child Processes with a name name from a Parent pid
    ProcessInfo::Iterator::Iterator(const uint32_t parentPID, const string& childname, const bool removepath) 
    : _pids()
    , _current()
    , _index(0) {
#ifndef __WIN32__
        FindChildren(_pids, [=](const uint32_t foundparentPID, const uint32_t childPID) {
            bool accept = false;

            if ( parentPID == foundparentPID ) {
                char fullname[PATH_MAX];
                ProcessName(childPID, fullname, sizeof(fullname));
                accept = ( childname == ( removepath == true ? Core::File::FileNameExtended(fullname) : fullname ) );
            }
            return accept;
        });
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
        FindChildren(_pids, [=](const uint32_t foundparentPID, const uint32_t childPID) {
            return parentPID == foundparentPID;
        });
#endif

        Reset();
    }

    ProcessInfo::LibraryIterator::LibraryInfo::LibraryInfo(const string& firstline) 
    : _name()
    , _shortname() {
        size_t pos = firstline.rfind(' ');
        _name = firstline.substr( pos == string::npos ? 0 : ( pos+1 >= firstline.length() ? pos : pos+1 ) );
        pos = _name.rfind('/');
        _shortname = _name.substr( pos == string::npos ? 0 : ( pos+1 >= _name.length() ? pos : pos+1 ) );
    }

    bool ProcessInfo::LibraryIterator::LibraryInfo::ProcessLine(const string& firstline) {
        return std::isdigit(firstline[0]) == false; //skip first line of section as it starts with a number
    }

    ProcessInfo::LibraryIterator::LibraryIterator(const uint32_t processPID) 
        : _libraries()
        , _current()
        , _index(0) {
       std::string path("/proc/");
        path += std::to_string(processPID);
        path += "/smaps";
        std::ifstream file(path, std::ifstream::in);
        std::string line;
        while( std::getline(file, line).eof() == false && file.good() == true ) {
            if( _libraries.size() == 0 ) {
                _libraries.push_back(LibraryInfo(line));
            }
            else if( _libraries.back().ProcessLine(line) == false ) {
                _libraries.push_back(LibraryInfo(line));
            }
        }
        file.close();  
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

    std::list<string> ProcessInfo::CommandLine() const
    {
        char procPath[PATH_MAX];
        sprintf(procPath, "/proc/%u/cmdline", _pid);

        FILE * cmdFile = fopen(procPath, "rb");
        if (cmdFile == nullptr) {
            TRACE_L1("Failed to open /proc/.../cmdline for process %u", _pid);
            return std::list<string>();
        }

        uint32_t index = 0;
        std::list<string> output;
        char buffer[256];
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

        return output;
    }

    uint32_t ProcessInfo::Group(const string& groupName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (groupName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WIN32__
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
#ifndef __WIN32__
        struct group* grp = getgrgid(::getpgid(_pid));
        if (grp != nullptr) {
            result = grp->gr_name;
        }
#endif
        return (result);
    }

    // pagemap file is documented here:
    //   https://www.kernel.org/doc/Documentation/vm/pagemap.txt
    void ProcessInfo::MarkOccupiedPages(uint32_t bitSet[], const uint32_t size) const
    {
        uint32_t entryCount = size / sizeof(uint32_t);

        char mapsPath[PATH_MAX];
        sprintf(mapsPath, "/proc/%u/maps", _pid);
        std::ifstream is01(mapsPath);

        char pagemapPath[PATH_MAX];
        sprintf(pagemapPath, "/proc/%u/pagemap", _pid);

        FILE * pagemapFile = fopen(pagemapPath, "rb");
        while (!is01.eof()) {
            string readLine;
            getline(is01, readLine);
            if (readLine.empty())
                continue;

            MemRange range(readLine);
            uint32_t pageSize = Core::SystemInfo::Instance().GetPageSize();
            uint64_t pageCount = (range.m_end - range.m_start) / pageSize;
            uint64_t pageMapOffset = (range.m_start / pageSize) * sizeof(uint64_t);
            int fseekStatus = fseek(pagemapFile, pageMapOffset, SEEK_SET);
            if (fseekStatus != 0) {
                TRACE_L1("Failed to seek in %s", pagemapPath);
                continue;
            }

            for (uint64_t i = 0; i < pageCount; i++) {
                uint64_t pageData = 0;
                size_t readCount = fread(&pageData, sizeof(uint64_t), 1, pagemapFile);
                if (readCount != 1) {
                    TRACE_L1("Failed to read pageInfo from %s", pagemapPath);
                }

                // Skip pages that are swapped out.
                bool isSwapped = ((pageData >> 62) & 1) != 0;
                if (isSwapped) {
                    continue;
                }

                // Skip pages that aren't present.
                bool isPresent = ((pageData >> 63) & 1) != 0;
                if (!isPresent) {
                    continue;
                }

                // Skip pages mapped to files.
                bool isFilePage = ((pageData >> 61) & 1) != 0;
                if (isFilePage) {
                    continue;
                }

                // Lower 54 bits contain actual page frame number (PFN).
                uint64_t filter = (static_cast<uint64_t>(1) << 55) - 1;
                uint32_t pageFrameNumber = static_cast<uint32_t>(pageData & filter);

                uint32_t bufferIndex = pageFrameNumber / 32;
                if (bufferIndex > entryCount) {
                   TRACE_L1("Tried to mark page outside of buffer: %u", bufferIndex);
                   continue;
                }

                uint32_t bitIndex = pageFrameNumber % 32;

                bitSet[bufferIndex] |= static_cast<uint32_t>(1) << bitIndex;
            }
        }

        fclose(pagemapFile);
    }

    /* static */ uint32_t ProcessInfo::User(const string& userName)
    {
        uint32_t result = ERROR_BAD_REQUEST;
        if (userName.empty() == false) {
            result = ERROR_UNKNOWN_KEY;
#ifndef __WIN32__
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
#ifndef __WIN32__
        struct passwd* pwd = getpwuid(::getuid());
        if (pwd != nullptr) {
            result = pwd->pw_name;
        }
#endif
        return (result);
    }

    /* static */ void ProcessInfo::FindByName(const string& name, const bool exact, std::list<ProcessInfo>& processInfos)
    {
       std::list<uint32_t> pidList;
       FindPid(name, exact, pidList);

       processInfos.clear();
       for (const pid_t pid : pidList) {
          processInfos.emplace_back(pid);
       }
    }

    static void EnumerateChildProcesses(const Core::ProcessInfo& processInfo, std::list<uint32_t>& pids)
    {
        pids.push_back(processInfo.Id());

        Core::ProcessInfo::Iterator iterator(processInfo.Children());
        while (iterator.Next()) {
            EnumerateChildProcesses(iterator.Current(), pids);
        }
    }

    ProcessTree::ProcessTree(const uint32_t processId)
    {
        ProcessInfo processInfo(processId);
        EnumerateChildProcesses(processInfo, _pids);
    }

    void ProcessTree::MarkOccupiedPages(uint32_t bitSet[], const uint32_t size) const
    {
        for (uint32_t pid : _pids) {
            ProcessInfo processInfo(pid);
            processInfo.MarkOccupiedPages(bitSet, size);
        }
    }

    bool ProcessTree::ContainsProcess(uint32_t pid) const
    {
        std::list<uint32_t>::const_iterator i = std::find(_pids.cbegin(), _pids.cend(), pid);
        return (i != _pids.cend());
    }

    uint32_t ProcessTree::RootId() const
    {
       return _pids.front();
    }
}
}

#include "CGroupContainerInfo.h"


namespace WPEFramework {
namespace ProcessContainers {
    
    CGroupContainerInfo::MemoryInfo CGroupContainerInfo::Memory() const
    {
      MemoryInfo result {UINT64_MAX, UINT64_MAX, UINT64_MAX};

        // Load total allocated memory
        string _memoryInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.usage_in_bytes";      

        char buffer[2048];
        auto fd = open(_memoryInfoPath.c_str(), O_RDONLY);

        if (fd != 0) {
            size_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                result.allocated = std::stoll(buffer);
            }
            
            close(fd);
        } else {
            TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
        }

        // Load details about memory
        string memoryFullInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.stat";

        fd = open(memoryFullInfoPath.c_str(), O_RDONLY);
        if (fd != 0) {
            size_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                char* tmp;
                char* token = strtok_r(buffer, " \n", &tmp);

                while (token != nullptr) {
                    if (token == nullptr) 
                        break;

                    char* label = token;

                    token = strtok_r(NULL, " \n", &tmp);
                    if (token == nullptr) 
                        break;

                    uint64_t value = std::stoll(token);

                    if (strcmp(label, "rss") == 0) 
                        result.resident = value;
                    else if (strcmp(label, "mapped_file") == 0) 
                        result.shared = value;

                    token = strtok_r(NULL, " \n", &tmp);
                }
            }
            
            close(fd);
        } else {
            TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
        }

        return result;    
    }

    CGroupContainerInfo::CPUInfo CGroupContainerInfo::Cpu() const
    {
        CPUInfo output {UINT64_MAX, std::vector<uint64_t>()};

        // Load total cpu time
        string cpuUsagePath = "/sys/fs/cgroup/cpuacct/" + _name + "/cpuacct.usage";

        char buffer[2048];
        auto fd = open(cpuUsagePath.c_str(), O_RDONLY);

        if (fd != 0) {
            uint32_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                output.total = atoi((char*)buffer);
            }            

            close(fd);
        }

        // Load per-core cpu time
        string cpuPerCoreUsagePath = "/sys/fs/cgroup/cpuacct/" + _name + "/cpuacct.usage_percpu";
        fd = open(cpuPerCoreUsagePath.c_str(), O_RDONLY);

        if (fd != 0) {
            uint32_t bytesRead = read(fd, buffer, sizeof(buffer));

            if (bytesRead > 0) {
                char* tmp;
                char* token = strtok_r((char*)buffer, " \n", &tmp);

                while (token != nullptr) {
                    // Sometimes (but not always for some reason?) a nonprintable character is caught as a separate token.
                    if (isdigit(token[0])) {
                        output.cores.push_back(atoi(token));
                    }
                    token = strtok_r(NULL, " \n", &tmp);
                }
            }

            close(fd);
        }

        return output;
    }

} // ProcessContainers
} // WPEFramework
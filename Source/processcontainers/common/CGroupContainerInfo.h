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

#pragma once

#include "processcontainers/ProcessContainer.h"
#include "processcontainers/common/BaseRefCount.h"

namespace WPEFramework {
namespace ProcessContainers {

    struct CGroupMemoryInfo : public BaseRefCount<ProcessContainers::IMemoryInfo> {
        CGroupMemoryInfo()
            : _allocated(UINT64_MAX)
            , _resident(UINT64_MAX)
            , _shared(UINT64_MAX)
        {
        }

        uint64_t Allocated() const override
        {
            return _allocated;
        }

        uint64_t Resident() const override
        {
            return _resident;
        }

        uint64_t Shared() const override
        {
            return _shared;
        }

        void Allocated(uint64_t value)
        {
            _allocated = value;
        }

        void Resident(uint64_t value)
        {
            _resident = value;
        }

        void Shared(uint64_t value)
        {
            _shared = value;
        }

    private:
        uint64_t _allocated;
        uint64_t _resident;
        uint64_t _shared;
    };

    struct CGroupProcessorInfo : public BaseRefCount<ProcessContainers::IProcessorInfo> {
        CGroupProcessorInfo(std::vector<uint64_t>&& cores)
            : _coresUsage(std::move(cores))
        {
            _totalUsage = 0;
            for (auto& usage : _coresUsage) {
                _totalUsage += usage;
            }
        }

        uint64_t TotalUsage() const override
        {
            return _totalUsage;
        }

        uint64_t CoreUsage(uint32_t coreNum) const override
        {
            uint64_t usage;

            if (coreNum >= _coresUsage.size()) {
                usage = UINT64_MAX;
            } else {
                usage = _coresUsage[coreNum];
            }

            return usage;
        }

        uint16_t NumberOfCores() const override
        {
            return _coresUsage.size();
        }

    private:
        std::vector<uint64_t> _coresUsage;
        uint64_t _totalUsage;
    };

    // Helper Class to collect CGroup related metrics.
    class CGroupMetrics {
    public:
        CGroupMetrics(const string& name)
            : _name(name)
        {
        }

        IMemoryInfo* Memory() const
        {
            CGroupMemoryInfo* result = new CGroupMemoryInfo;

            // Load total allocated memory
            string _memoryInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.usage_in_bytes";

            char buffer[2048];
            auto fd = open(_memoryInfoPath.c_str(), O_RDONLY);

            if (fd >= 0) {
                size_t bytesRead = read(fd, buffer, sizeof(buffer));

                if (bytesRead > 0) {
                    result->Allocated(std::stoll(buffer));
                }

                close(fd);
            } else {
                TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
            }

            // Load details about memory
            string memoryFullInfoPath = "/sys/fs/cgroup/memory/" + _name + "/memory.stat";

            fd = open(memoryFullInfoPath.c_str(), O_RDONLY);
            if (fd >= 0) {
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
                            result->Resident(value);
                        else if (strcmp(label, "mapped_file") == 0)
                            result->Shared(value);

                        token = strtok_r(NULL, " \n", &tmp);
                    }
                }

                close(fd);
            } else {
                TRACE_L1("Cannot get memory information for container. Is device booted with memory cgroup enabled?");
            }

            return result;
        }

        IProcessorInfo* ProcessorInfo() const
        {
            std::vector<uint64_t> coresUsage;

            // Load per-core cpu time
            string cpuPerCoreUsagePath = "/sys/fs/cgroup/cpuacct/" + _name + "/cpuacct.usage_percpu";
            auto fd = open(cpuPerCoreUsagePath.c_str(), O_RDONLY);
            char buffer[2048];

            if (fd >= 0) {
                uint32_t bytesRead = read(fd, buffer, sizeof(buffer));

                // In about 30% of cases we got additional number information after new line
                for(uint32_t i = 0; buffer[i] != '\0'; i++) {
                    if (buffer[i] == '\n') {
                        buffer[i] = '\0';
                    }
                }

                if (bytesRead > 0) {
                    char* tmp;
                    char* token = strtok_r((char*)buffer, " ", &tmp);

                    while (token != nullptr) {
                        // Sometimes (but not always for some reason?) a nonprintable character is caught as a separate token.
                        if (isdigit(token[0])) {
                            coresUsage.push_back(atoi(token));
                        }
                        token = strtok_r(NULL, " ", &tmp);
                    }
                }

                close(fd);
            }

            return new CGroupProcessorInfo(std::move(coresUsage));
        }

    private:
        string _name;
    };

} // ProcessContainers
} // WPEFramework

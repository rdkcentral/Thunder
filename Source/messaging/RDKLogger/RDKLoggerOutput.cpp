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

#include "ExternalOutput.h"

#include "rdk_logger.h"

#include <string>

namespace {
    rdk_LogLevel Level(const ThunderExternalLogLevel level)
    {
        uint8_t value;

        switch (level) {
        case THUNDER_EXTERNAL_LOG_LEVEL_FATAL:
            value = 0;
            break;
        case THUNDER_EXTERNAL_LOG_LEVEL_ERROR:
            value = 1;
            break;
        case THUNDER_EXTERNAL_LOG_LEVEL_WARN:
            value = 2;
            break;
        case THUNDER_EXTERNAL_LOG_LEVEL_NOTICE:
            value = 3;
            break;
        case THUNDER_EXTERNAL_LOG_LEVEL_TRACE:
            value = 6;
            break;
        case THUNDER_EXTERNAL_LOG_LEVEL_INFO:
        default:
            value = 4;
            break;
        }

        return (static_cast<rdk_LogLevel>(value));
    }

    std::string Module(const char* module)
    {
        return (std::string("LOG.RDK.THUNDER.") + module);
    }
}

extern "C" {
    bool ThunderExternalOutput_Initialize(void)
    {
        return (RDKLOGGER_INIT() == RDK_SUCCESS);
    }

    bool ThunderExternalOutput_IsEnabled(const char* module, const ThunderExternalLogLevel level)
    {
        const std::string moduleName = Module(module);
        return (rdk_logger_is_logLevel_enabled(moduleName.c_str(), Level(level)) == TRUE);
    }

    void ThunderExternalOutput_Submit(const char* module, const ThunderExternalLogLevel level, const char* payload)
    {
        const std::string moduleName = Module(module);
        RDK_LOG(Level(level), moduleName.c_str(), "%s", payload);
    }

    void ThunderExternalOutput_Deinitialize(void)
    {
        rdk_logger_deinit();
    }
}
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
#include "../../core/Singleton.h"

#include "rdk_logger.h"

#include <cstring>
#include <map>
#include <string>

namespace {
    enum ThunderExternalLogLevel {
        THUNDER_EXTERNAL_LOG_LEVEL_FATAL,
        THUNDER_EXTERNAL_LOG_LEVEL_ERROR,
        THUNDER_EXTERNAL_LOG_LEVEL_WARN,
        THUNDER_EXTERNAL_LOG_LEVEL_NOTICE,
        THUNDER_EXTERNAL_LOG_LEVEL_INFO,
        THUNDER_EXTERNAL_LOG_LEVEL_TRACE
    };

    ThunderExternalLogLevel ThunderLevel(const ThunderMessageType type, const char* category)
    {
        ThunderExternalLogLevel level = THUNDER_EXTERNAL_LOG_LEVEL_INFO;

        switch (type) {
        case THUNDER_EXTERNAL_MESSAGE_ASSERT:
            level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL;
            break;
        case THUNDER_EXTERNAL_MESSAGE_REPORTING:
            level = THUNDER_EXTERNAL_LOG_LEVEL_WARN;
            break;
        case THUNDER_EXTERNAL_MESSAGE_OPERATIONAL_STREAM:
            level = THUNDER_EXTERNAL_LOG_LEVEL_TRACE;
            break;
        case THUNDER_EXTERNAL_MESSAGE_TELEMETRY:
            level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE;
            break;
        case THUNDER_EXTERNAL_MESSAGE_TRACING:
            if ((strcmp(category, "Fatal") == 0) || (strcmp(category, "Crash") == 0)) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL;
            } else if (strcmp(category, "Error") == 0) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_ERROR;
            } else if ((strcmp(category, "Warning") == 0) || (strcmp(category, "Warn") == 0)) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_WARN;
            } else if (strcmp(category, "Notice") == 0) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE;
            } else {
                level = THUNDER_EXTERNAL_LOG_LEVEL_TRACE;
            }
            break;
        case THUNDER_EXTERNAL_MESSAGE_LOGGING:
            if ((strcmp(category, "Fatal") == 0) || (strcmp(category, "Crash") == 0)) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL;
            } else if (strcmp(category, "Error") == 0) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_ERROR;
            } else if ((strcmp(category, "Warning") == 0) || (strcmp(category, "Warn") == 0)) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_WARN;
            } else if (strcmp(category, "Notice") == 0) {
                level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE;
            }
            break;
        default:
            break;
        }

        return (level);
    }

    rdk_LogLevel RDKLevel(const ThunderMessageType type, const char* category)
    {
        uint8_t value = 4;

        switch (ThunderLevel(type, category)) {
        case THUNDER_EXTERNAL_LOG_LEVEL_FATAL: value = 0; break;
        case THUNDER_EXTERNAL_LOG_LEVEL_ERROR: value = 1; break;
        case THUNDER_EXTERNAL_LOG_LEVEL_WARN: value = 2; break;
        case THUNDER_EXTERNAL_LOG_LEVEL_NOTICE: value = 3; break;
        case THUNDER_EXTERNAL_LOG_LEVEL_TRACE: value = 6; break;
        default: break;
        }

        return (static_cast<rdk_LogLevel>(value));
    }

    std::string Module(const char* module)
    {
        return (std::string("LOG.RDK.THUNDER.") + module);
    }

    struct BackendState {
        std::map<std::string, bool> controls;
    };

    std::map<std::string, bool>& Controls()
    {
        return (Thunder::Core::SingletonType<BackendState>::InstanceWithoutWarning().controls);
    }
}

extern "C" {
    bool ThunderExternalOutput_Initialize(void)
    {
        Controls().clear();
        return (RDKLOGGER_INIT() == RDK_SUCCESS);
    }

    bool ThunderExternalOutput_IsEnabled(const char* module, const ThunderMessageType type, const char* category)
    {
        const std::string moduleName = Module(module);
        const rdk_LogLevel level = RDKLevel(type, category);
        const std::string key = moduleName + '#' + std::to_string(static_cast<unsigned>(level));
        const auto entry = Controls().find(key);
        bool enabled = false;

        if (entry != Controls().end()) {
            enabled = entry->second;
        }
        else {
            enabled = (rdk_logger_is_logLevel_enabled(moduleName.c_str(), level) == TRUE);
            Controls().emplace(key, enabled);
        }

        return (enabled);
    }

    void ThunderExternalOutput_Submit(const char* module, const ThunderMessageType type, const char* category, const char* payload)
    {
        const std::string moduleName = Module(module);
        const ThunderExternalLogLevel level = ThunderLevel(type, category);

        if (((((type == THUNDER_EXTERNAL_MESSAGE_LOGGING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_INFO)) ||
              ((type == THUNDER_EXTERNAL_MESSAGE_TRACING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_TRACE))) &&
             (category[0] != '\0'))) {
            const std::string prefixedPayload = std::string(category) + ": " + payload;
            RDK_LOG(RDKLevel(type, category), moduleName.c_str(), "%s", prefixedPayload.c_str());
        }
        else {
            RDK_LOG(RDKLevel(type, category), moduleName.c_str(), "%s", payload);
        }
    }

    void ThunderExternalOutput_Deinitialize(void)
    {
        Controls().clear();
        rdk_logger_deinit();
    }
}
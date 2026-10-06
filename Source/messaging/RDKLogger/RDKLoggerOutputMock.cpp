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
#include "RDKLoggerOutputMock.h"

#include "Module.h"

namespace {
    ThunderExternalLogLevel Level(const ThunderMessageType type, const char* category)
    {
        ThunderExternalLogLevel level = THUNDER_EXTERNAL_LOG_LEVEL_INFO;

        switch (type) {
        case THUNDER_EXTERNAL_MESSAGE_ASSERT: level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL; break;
        case THUNDER_EXTERNAL_MESSAGE_REPORTING: level = THUNDER_EXTERNAL_LOG_LEVEL_WARN; break;
        case THUNDER_EXTERNAL_MESSAGE_OPERATIONAL_STREAM: level = THUNDER_EXTERNAL_LOG_LEVEL_TRACE; break;
        case THUNDER_EXTERNAL_MESSAGE_TELEMETRY: level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE; break;
        case THUNDER_EXTERNAL_MESSAGE_TRACING:
            if ((string(category) == "Fatal") || (string(category) == "Crash")) level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL;
            else if (string(category) == "Error") level = THUNDER_EXTERNAL_LOG_LEVEL_ERROR;
            else if ((string(category) == "Warning") || (string(category) == "Warn")) level = THUNDER_EXTERNAL_LOG_LEVEL_WARN;
            else if (string(category) == "Notice") level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE;
            else level = THUNDER_EXTERNAL_LOG_LEVEL_TRACE;
            break;
        case THUNDER_EXTERNAL_MESSAGE_LOGGING:
            if ((string(category) == "Fatal") || (string(category) == "Crash")) level = THUNDER_EXTERNAL_LOG_LEVEL_FATAL;
            else if (string(category) == "Error") level = THUNDER_EXTERNAL_LOG_LEVEL_ERROR;
            else if ((string(category) == "Warning") || (string(category) == "Warn")) level = THUNDER_EXTERNAL_LOG_LEVEL_WARN;
            else if (string(category) == "Notice") level = THUNDER_EXTERNAL_LOG_LEVEL_NOTICE;
            break;
        default: break;
        }

        return (level);
    }

    struct MockState {
        bool initializeSucceeds;
        bool enabled;
        uint32_t initializeCount;
        uint32_t deinitializeCount;
        uint32_t queryCount;
        uint32_t submitCount;
        string lastModule;
        ThunderExternalLogLevel lastLevel;
        string lastPayload;
        std::map<string, uint32_t> queryCounts;
        std::map<string, bool> enabledControls;
    };

    MockState& State()
    {
        static MockState state = { true, true, 0, 0, 0, 0, string(), THUNDER_EXTERNAL_LOG_LEVEL_INFO, string(), {}, {} };
        return (state);
    }
}

extern "C" {
    bool ThunderExternalOutput_Initialize(void)
    {
        State().initializeCount++;
        TRACE_L1("RDKLogger mock backend initialized");
        return (State().initializeSucceeds);
    }

    bool ThunderExternalOutput_IsEnabled(const char* module, const ThunderMessageType type, const char* category)
    {
        const ThunderExternalLogLevel level = Level(type, category);
        const string key = string(module) + '#' + Thunder::Core::NumberType<uint8_t>(static_cast<uint8_t>(level)).Text();
        const auto cached = State().enabledControls.find(key);
        bool enabled = State().enabled;

        if (cached != State().enabledControls.end()) {
            enabled = cached->second;
        }
        else {
            State().queryCount++;
            State().lastModule = module;
            State().lastLevel = level;
            State().queryCounts[string(module) + '#' + Thunder::Core::NumberType<uint8_t>(static_cast<uint8_t>(level)).Text()]++;
            TRACE_L1("RDKLogger mock enablement query: module=%s level=%u", module, static_cast<unsigned>(level));
            State().enabledControls.emplace(key, enabled);
        }

        return (enabled);
    }

    void ThunderExternalOutput_Submit(const char* module, const ThunderMessageType type, const char* category, const char* payload)
    {
        const ThunderExternalLogLevel level = Level(type, category);
        State().submitCount++;
        State().lastModule = module;
        State().lastLevel = level;
        State().lastPayload = payload;
        if (((type == THUNDER_EXTERNAL_MESSAGE_LOGGING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_INFO)) ||
            ((type == THUNDER_EXTERNAL_MESSAGE_TRACING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_TRACE) && (category[0] != '\0'))) {
            State().lastPayload = string(category) + ": " + payload;
        }
        TRACE_L1("RDKLogger mock message: module=%s level=%u payload=%s", module, static_cast<unsigned>(level), payload);
    }

    void ThunderExternalOutput_Deinitialize(void)
    {
        State().deinitializeCount++;
        TRACE_L1("RDKLogger mock backend deinitialized");
    }

    void ThunderExternalOutputMock_Reset(void)
    {
        State() = { true, true, 0, 0, 0, 0, string(), THUNDER_EXTERNAL_LOG_LEVEL_INFO, string(), {}, {} };
    }

    void ThunderExternalOutputMock_SetInitializeSucceeds(const bool succeeds)
    {
        State().initializeSucceeds = succeeds;
    }

    void ThunderExternalOutputMock_SetEnabled(const bool enabled)
    {
        State().enabled = enabled;
    }

    uint32_t ThunderExternalOutputMock_InitializeCount(void)
    {
        return (State().initializeCount);
    }

    uint32_t ThunderExternalOutputMock_DeinitializeCount(void)
    {
        return (State().deinitializeCount);
    }

    uint32_t ThunderExternalOutputMock_QueryCount(void)
    {
        return (State().queryCount);
    }

    uint32_t ThunderExternalOutputMock_QueryCountFor(const char* module, const ThunderExternalLogLevel level)
    {
        const string key = string(module) + '#' + Thunder::Core::NumberType<uint8_t>(static_cast<uint8_t>(level)).Text();
        const auto entry = State().queryCounts.find(key);
        return (entry == State().queryCounts.end() ? 0 : entry->second);
    }

    uint32_t ThunderExternalOutputMock_SubmitCount(void)
    {
        return (State().submitCount);
    }

    const char* ThunderExternalOutputMock_LastModule(void)
    {
        return (State().lastModule.c_str());
    }

    ThunderExternalLogLevel ThunderExternalOutputMock_LastLevel(void)
    {
        return (State().lastLevel);
    }

    const char* ThunderExternalOutputMock_LastPayload(void)
    {
        return (State().lastPayload.c_str());
    }
}

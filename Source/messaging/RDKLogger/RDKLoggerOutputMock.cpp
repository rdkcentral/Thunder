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
#include "../../core/Singleton.h"
#include "../../core/Sync.h"

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
        MockState()
            : lock()
            , initializeSucceeds(true)
            , enabled(true)
            , initializeCount(0)
            , deinitializeCount(0)
            , queryCount(0)
            , submitCount(0)
            , lastModule()
            , lastLevel(THUNDER_EXTERNAL_LOG_LEVEL_INFO)
            , lastPayload()
            , queryCounts()
            , enabledControls()
        {
        }

        void Reset()
        {
            initializeSucceeds = true;
            enabled = true;
            initializeCount = 0;
            deinitializeCount = 0;
            queryCount = 0;
            submitCount = 0;
            lastModule.clear();
            lastLevel = THUNDER_EXTERNAL_LOG_LEVEL_INFO;
            lastPayload.clear();
            queryCounts.clear();
            enabledControls.clear();
        }

        Thunder::Core::CriticalSection lock;
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
        return (Thunder::Core::SingletonType<MockState>::InstanceWithoutWarning());
    }
}

extern "C" {
    bool ThunderExternalOutput_Initialize(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.initializeCount++;
        const bool succeeds = state.initializeSucceeds;
        TRACE_L1("RDKLogger mock backend initialized");
        return (succeeds);
    }

    bool ThunderExternalOutput_IsEnabled(const char* module, const ThunderMessageType type, const char* category)
    {
        const ThunderExternalLogLevel level = Level(type, category);
        const string key = string(module) + '#' + Thunder::Core::NumberType<uint8_t>(static_cast<uint8_t>(level)).Text();
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        const auto cached = state.enabledControls.find(key);
        bool enabled = state.enabled;

        if (cached != state.enabledControls.end()) {
            enabled = cached->second;
        }
        else {
            state.queryCount++;
            state.lastModule = module;
            state.lastLevel = level;
            state.queryCounts[key]++;
            TRACE_L1("RDKLogger mock enablement query: module=%s level=%u", module, static_cast<unsigned>(level));
            state.enabledControls.emplace(key, enabled);
        }

        return (enabled);
    }

    void ThunderExternalOutput_Submit(const char* module, const ThunderMessageType type, const char* category, const char* payload)
    {
        const ThunderExternalLogLevel level = Level(type, category);
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.submitCount++;
        state.lastModule = module;
        state.lastLevel = level;
        state.lastPayload = payload;
        if (((type == THUNDER_EXTERNAL_MESSAGE_LOGGING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_INFO) && (category[0] != '\0')) ||
            ((type == THUNDER_EXTERNAL_MESSAGE_TRACING) && (level == THUNDER_EXTERNAL_LOG_LEVEL_TRACE) && (category[0] != '\0'))) {
            state.lastPayload = string(category) + ": " + payload;
        }
        TRACE_L1("RDKLogger mock message: module=%s level=%u payload=%s", module, static_cast<unsigned>(level), payload);
    }

    void ThunderExternalOutput_Deinitialize(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.deinitializeCount++;
        TRACE_L1("RDKLogger mock backend deinitialized");
    }

    void ThunderExternalOutputMock_Reset(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.Reset();
    }

    void ThunderExternalOutputMock_SetInitializeSucceeds(const bool succeeds)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.initializeSucceeds = succeeds;
    }

    void ThunderExternalOutputMock_SetEnabled(const bool enabled)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        state.enabled = enabled;
    }

    uint32_t ThunderExternalOutputMock_InitializeCount(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        return (state.initializeCount);
    }

    uint32_t ThunderExternalOutputMock_DeinitializeCount(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        return (state.deinitializeCount);
    }

    uint32_t ThunderExternalOutputMock_QueryCount(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        return (state.queryCount);
    }

    uint32_t ThunderExternalOutputMock_QueryCountFor(const char* module, const ThunderExternalLogLevel level)
    {
        const string key = string(module) + '#' + Thunder::Core::NumberType<uint8_t>(static_cast<uint8_t>(level)).Text();
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        const auto entry = state.queryCounts.find(key);
        return (entry == state.queryCounts.end() ? 0 : entry->second);
    }

    uint32_t ThunderExternalOutputMock_SubmitCount(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        return (state.submitCount);
    }

    const char* ThunderExternalOutputMock_LastModule(void)
    {
        static thread_local string lastModule;
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        lastModule = state.lastModule;
        return (lastModule.c_str());
    }

    ThunderExternalLogLevel ThunderExternalOutputMock_LastLevel(void)
    {
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        return (state.lastLevel);
    }

    const char* ThunderExternalOutputMock_LastPayload(void)
    {
        static thread_local string lastPayload;
        MockState& state = State();
        Thunder::Core::SafeSyncType<Thunder::Core::CriticalSection> lock(state.lock);
        lastPayload = state.lastPayload;
        return (lastPayload.c_str());
    }
}

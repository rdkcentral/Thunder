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
 
#ifndef __TRACE_INTERNAL_H
#define __TRACE_INTERNAL_H

#include "Module.h"
#include "Portability.h"

#ifndef __WINDOWS__
#include <syslog.h>
#endif

#ifdef __WINDOWS__
#define TRACE_PROCESS_ID ::GetCurrentProcessId()
#define ASSERT_LOGGER(message, ...) fprintf(stderr, message, ##__VA_ARGS__);
#else
#define TRACE_PROCESS_ID ::getpid()
#define ASSERT_LOGGER(message, ...) ::fprintf(stderr, message, ##__VA_ARGS__);
#endif

#ifndef _TRACE_LEVEL
#ifdef __DEBUG__
#define _TRACE_LEVEL 2
#else
#define _TRACE_LEVEL 0
#endif
#endif

#if _TRACE_LEVEL > 4
#define TRACE_L5(x, ...)                                                         \
    fprintf(stderr, "----- L5 [%d]: " #x "\n", TRACE_PROCESS_ID, ##__VA_ARGS__); \
    fflush(stderr);
#else
#define TRACE_L5(x, ...)
#endif

#if _TRACE_LEVEL > 3
#define TRACE_L4(x, ...)                                                         \
    fprintf(stderr, "----  L4 [%d]: " #x "\n", TRACE_PROCESS_ID, ##__VA_ARGS__); \
    fflush(stderr);
#else
#define TRACE_L4(x, ...)
#endif

#if _TRACE_LEVEL > 2
#define TRACE_L3(x, ...)                                                         \
    fprintf(stderr, "---   L3 [%d]: " #x "\n", TRACE_PROCESS_ID, ##__VA_ARGS__); \
    fflush(stderr);
#else
#define TRACE_L3(x, ...)
#endif

#if _TRACE_LEVEL > 1
#define TRACE_L2(x, ...)                                                         \
    fprintf(stderr, "--    L2 [%d]: " #x "\n", TRACE_PROCESS_ID, ##__VA_ARGS__); \
    fflush(stderr);
#else
#define TRACE_L2(x, ...)
#endif

#if _TRACE_LEVEL > 0
#define TRACE_L1(x, ...)                                                         \
    fprintf(stderr, "-     L1 [%d]: " #x "\n", TRACE_PROCESS_ID, ##__VA_ARGS__); \
    fflush(stderr);
#else
#define TRACE_L1(x, ...)
#endif

#ifdef ASSERT
#undef ASSERT
#endif
#ifdef VERIFY
#undef VERIFY
#endif

#ifdef __DEBUG__

#define ASSERT(x)                                                                                           \
    {                                                                                                       \
        if (!(x)) {                                                                                         \
            ASSERT_LOGGER("===== $$ [%d]: ASSERT [%s:%d] (" #x ")\n", TRACE_PROCESS_ID, __FILE__, __LINE__) \
            DumpCallStack();                                                                                \
            abort();                                                                                        \
        }                                                                                                   \
    }

#define ASSERT_VERBOSE(x, y, ...)                                                                                                           \
    {                                                                                                                                       \
        if (!(x)) {                                                                                                                         \
            ASSERT_LOGGER("===== $$ [%d]: ASSERT [%s:%d] (" #x ")\n         " #y "\n", TRACE_PROCESS_ID, __FILE__, __LINE__, ##__VA_ARGS__) \
            DumpCallStack();                                                                                                                \
            abort();                                                                                                                      \
        }                                                                                                                                   \
    }

#define VERIFY(x, y) assert(x == y)
#else
#define ASSERT(x)
#define VERIFY(x, y) x
#define ASSERT_VERBOSE(x, y, ...)
#endif

#define LOG(LEVEL, MESSAGE)                                                         \
    {                                                                               \
        uint32_t stamp = static_cast<uint32_t>(Core::Time::Now().Ticks());          \
                                                                                    \
        Core::ILogService::Instance()->Log(                                         \
            stamp, LEVEL, Core::ToString(typeid(*this).name()),                     \
            Core::LogMessage(Core::ToString(__FILE__).c_str(), __LINE__, MESSAGE)); \
    }

namespace WPEFramework {
namespace Core {
    class TextFragment;

    EXTERNAL TextFragment ClassNameOnly(const char className[]);
    EXTERNAL const char* FileNameOnly(const char fileName[]);
    EXTERNAL string LogMessage(const TCHAR filename[], const uint32_t LineNumber, const TCHAR* message);
}
} // namespace Core

#endif //__TRACE_INTERNAL_H

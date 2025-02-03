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
 
#ifndef __TRACE_INTERNAL_H
#define __TRACE_INTERNAL_H

#include "Module.h"
#include "Portability.h"

#ifdef __CORE_MESSAGING__
#include "AssertionControl.h"
#endif

#ifndef __WINDOWS__
#include <syslog.h>
#endif

namespace Thunder {
    namespace Core {
        template <typename T, size_t S>
        inline constexpr size_t FileNameOffset(const T(&str)[S], size_t i = S - 1)
        {
            return (str[i] == '/' || str[i] == '\\') ? i + 1 : (i > 0 ? FileNameOffset(str, i - 1) : 0);
        }
        template <typename T>
        inline constexpr size_t FileNameOffset(T(&str)[1] VARIABLE_IS_NOT_USED)
        {
            return 0;
        }
    }
}

#ifdef __WINDOWS__
#define TRACE_PROCESS_ID ::GetCurrentProcessId()
#define TRACE_THREAD_ID ::GetCurrentThreadId()
#else
#define TRACE_PROCESS_ID ::getpid()
#if __GLIBC__ == 2 && __GLIBC_MINOR__ < 30
#include <sys/syscall.h>
#define TRACE_THREAD_ID syscall(SYS_gettid)
#else
#if INTPTR_MAX == INT64_MAX
#define TRACE_THREAD_ID static_cast<uint64_t>(::gettid())
#else
#define TRACE_THREAD_ID static_cast<uint32_t>(::gettid())
#endif
#ifndef __APPLE__
#include <unistd.h>
#endif
#endif
#endif

#if defined(__GNUC__)
    #pragma GCC system_header
#elif defined(__clang__)
    #pragma clang system_header
#endif

#ifdef __WINDOWS__
#define TRACE_FORMATTING_IMPL(fmt, ...)                                                                                                     \
    do {                                                                                                                                    \
        ::fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" fmt "\033[0m\n", &__FILE__[Thunder::Core::FileNameOffset(__FILE__)], __LINE__, __FUNCTION__, TRACE_PROCESS_ID, TRACE_THREAD_ID, ##__VA_ARGS__);  \
        fflush(stderr);                                                                                                                 \
    } while (0)
#else
#if INTPTR_MAX == INT64_MAX
#include <inttypes.h>
#define TRACE_FORMATTING_IMPL(fmt, ...)                                                                                                     \
    do {                                                                                                                                    \
        ::fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%" PRId64 ">" fmt "\033[0m\n", &__FILE__[Thunder::Core::FileNameOffset(__FILE__)], __LINE__, __FUNCTION__, TRACE_PROCESS_ID, TRACE_THREAD_ID, ##__VA_ARGS__);  \
        fflush(stderr);                                                                                                                     \
    } while (0)
#else
#define TRACE_FORMATTING_IMPL(fmt, ...)                                                                                                     \
    do {                                                                                                                                    \
        ::fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" fmt "\033[0m\n", &__FILE__[Thunder::Core::FileNameOffset(__FILE__)], __LINE__, __FUNCTION__, TRACE_PROCESS_ID, TRACE_THREAD_ID, ##__VA_ARGS__);  \
        fflush(stderr);                                                                                                                     \
    } while (0)
#endif
#
#endif

#if defined(CORE_TRACE_NOT_ALLOWED) && !defined(__WINDOWS__) 
#define TRACE_FORMATTING(fmt, ...)                                                                            \
    _Pragma ("GCC warning \"Using 'TRACE_Lx' outside of Thunder Core is deprecated\"")                        \
    TRACE_FORMATTING_IMPL(fmt, ##__VA_ARGS__)
#else
#define TRACE_FORMATTING(fmt, ...)                                                                            \
    TRACE_FORMATTING_IMPL(fmt, ##__VA_ARGS__)
#endif

#ifdef __WINDOWS__
#define TRACE_PROCESS_ID ::GetCurrentProcessId()
#define ASSERT_LOGGER(message, ...) fprintf(stderr, message, ##__VA_ARGS__)
#else
#define TRACE_PROCESS_ID ::getpid()
#define ASSERT_LOGGER(message, ...) ::fprintf(stderr, message, ##__VA_ARGS__)
#endif

#ifndef _TRACE_LEVEL
#ifdef __DEBUG__
#define _TRACE_LEVEL 2
#else
#define _TRACE_LEVEL 0
#endif
#endif

#if _TRACE_LEVEL > 4
#define TRACE_L5(x, ...) TRACE_FORMATTING("<5>: " x, ##__VA_ARGS__)
#else
#define TRACE_L5(x, ...)
#endif

#if _TRACE_LEVEL > 3
#define TRACE_L4(x, ...) TRACE_FORMATTING("<4>: " x, ##__VA_ARGS__)
#else
#define TRACE_L4(x, ...)
#endif

#if _TRACE_LEVEL > 2
#define TRACE_L3(x, ...) TRACE_FORMATTING("<3>: " x, ##__VA_ARGS__)
#else
#define TRACE_L3(x, ...)
#endif

#if _TRACE_LEVEL > 1
#define TRACE_L2(x, ...) TRACE_FORMATTING("<2>: " x, ##__VA_ARGS__)
#else
#define TRACE_L2(x, ...)
#endif

#if _TRACE_LEVEL > 0
#define TRACE_L1(x, ...) TRACE_FORMATTING("<1>: " x, ##__VA_ARGS__)
#else
#define TRACE_L1(x, ...)
#endif

#ifdef ASSERT
#undef ASSERT
#endif
#ifdef VERIFY
#undef VERIFY
#endif

#if defined(__DEBUG__) || defined(_THUNDER_PRODUCTION)

    #ifdef __DEBUG__
        #define ASSERT_ABORT abort();
    #else
        #define ASSERT_ABORT
    #endif

    #define DIRECT_ASSERT                                                                                   \
        std::list<Thunder::Core::callstack_info> entries;                                                   \
        DumpCallStack(0, entries);                                                                          \
        for(const Thunder::Core::callstack_info& entry : entries) {                                         \
            fprintf(stderr, "[%s]:[%s]:[%d]\n", entry.module.c_str(), entry.function.c_str(), entry.line);  \
        }                                                                                                   \
        fflush(stderr);                                                                                     \
        ASSERT_ABORT

    #ifdef __CORE_MESSAGING__

        #ifdef __APPLE__
            #define PROGRAM_NAME getprogname()
        #elif defined(__WINDOWS__)
            #define PROGRAM_NAME Thunder::Core::GetProgramName()
        #else
            #define PROGRAM_NAME program_invocation_short_name
        #endif

        #define ASSERT_METADATA                                                                                            \
            Thunder::Core::Messaging::MessageInfo __messageInfo__(                                                         \
                Thunder::Assertion::BaseAssertType::Metadata());                                                           \
            std::list<Thunder::Core::callstack_info> __entries__;                                                          \
            DumpCallStack(0, __entries__);                                                                                 \
            std::string __callstack__;                                                                                     \
            for (const Thunder::Core::callstack_info& entry : __entries__) {                                               \
                __callstack__ += "[" + entry.module + "]:[" + entry.function + "]:[" + std::to_string(entry.line) + "]\n"; \
            }                                                                                                              \
            Thunder::Core::Messaging::IStore::Assert __assertMetadata__(                                                   \
                __messageInfo__,                                                                                           \
                TRACE_PROCESS_ID,                                                                                          \
                PROGRAM_NAME,                                                                                              \
                __FILE__,                                                                                                  \
                __LINE__,                                                                                                  \
                __callstack__);

        #define ASSERT_SENT                                                                                     \
            Thunder::Assertion::AssertionUnitProxy::Instance().AssertionEvent(                                  \
                __assertMetadata__,                                                                             \
                __message__);

        #define ASSERT(expr)                                                                                    \
            do {                                                                                                \
                if (!(expr)) {                                                                                  \
                    if (Thunder::Assertion::BaseAssertType::IsEnabled()) {                                      \
                        ASSERT_METADATA                                                                         \
                        Thunder::Core::Messaging::TextMessage __message__(#expr);                               \
                        ASSERT_SENT                                                                             \
                    }                                                                                           \
                    ASSERT_ABORT                                                                                \
                }                                                                                               \
            } while(0)

        #define ASSERT_VERBOSE(expr, format, ...)                                                               \
            do {                                                                                                \
                if (!(expr)) {                                                                                  \
                    if (Thunder::Assertion::BaseAssertType::IsEnabled()) {                                      \
                        ASSERT_METADATA                                                                         \
                        char __buffer__[256];                                                                   \
                        std::snprintf(__buffer__, sizeof(__buffer__), "%s: " #format, #expr, ##__VA_ARGS__);    \
                        Thunder::Core::Messaging::TextMessage __message__(__buffer__);                          \
                        ASSERT_SENT                                                                             \
                    }                                                                                           \
                    ASSERT_ABORT                                                                                \
                }                                                                                               \
            } while(0)

        #define INTERNAL_ASSERT(expr)                                                                                   \
            do {                                                                                                        \
                if (!(expr)) {                                                                                          \
                    ASSERT_LOGGER("===== $$ [%d]: ASSERT [%s:%d] (%s)\n", TRACE_PROCESS_ID, __FILE__, __LINE__, #expr); \
                    DIRECT_ASSERT                                                                                       \
                }                                                                                                       \
            } while(0)
    #else
        #define ASSERT(expr)                                                                                            \
            do {                                                                                                        \
                if (!(expr)) {                                                                                          \
                    ASSERT_LOGGER("===== $$ [%d]: ASSERT [%s:%d] (%s)\n", TRACE_PROCESS_ID, __FILE__, __LINE__, #expr); \
                    DIRECT_ASSERT                                                                                       \
                }                                                                                                       \
            } while(0)

        #define ASSERT_VERBOSE(expr, format, ...)                                                                                                            \
            do {                                                                                                                                             \
                if (!(expr)) {                                                                                                                               \
                    ASSERT_LOGGER("===== $$ [%d]: ASSERT [%s:%d] (%s)\n         " #format "\n", TRACE_PROCESS_ID, __FILE__, __LINE__, #expr, ##__VA_ARGS__); \
                    DIRECT_ASSERT                                                                                                                            \
                }                                                                                                                                            \
            } while(0)

        #define INTERNAL_ASSERT(expr) ASSERT(expr)
    #endif

    #define VERIFY(expr) ASSERT(expr)
#else
    #define ASSERT(x)

    #define ASSERT_VERBOSE(x, y, ...)

    #define INTERNAL_ASSERT(x)

    #define VERIFY(expr)                                                                                                   \
        do {                                                                                                               \
            if(!(expr)) {                                                                                                  \
                ASSERT_LOGGER("===== $$ [%d]: VERIFY FAILED [%s:%d] (%s)\n", TRACE_PROCESS_ID, __FILE__, __LINE__, #expr); \
        }                                                                                                                  \
        } while(0)
#endif

#define LOG(LEVEL, MESSAGE)                                                         \
    {                                                                               \
        uint32_t stamp = static_cast<uint32_t>(Core::Time::Now().Ticks());          \
                                                                                    \
        Core::ILogService::Instance()->Log(                                         \
            stamp, LEVEL, Core::ToString(typeid(*this).name()),                     \
            Core::LogMessage(Core::ToString(__FILE__).c_str(), __LINE__, MESSAGE)); \
    }

namespace Thunder {
namespace Core {
    EXTERNAL const char* FileNameOnly(const char fileName[]);
    EXTERNAL string LogMessage(const TCHAR filename[], const uint32_t LineNumber, const TCHAR* message);
#if defined(__CORE_MESSAGING__) && defined(__WINDOWS__)
    EXTERNAL const std::string& GetProgramName();
#endif
}
} // namespace Core

#endif //__TRACE_INTERNAL_H

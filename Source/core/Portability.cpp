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

#include <stdlib.h>

#include "Module.h"
#include "IPCConnector.h"
#include "Portability.h"
#include "Sync.h"
#include "SystemInfo.h"

#ifdef __LINUX__
#include <atomic>
#include <execinfo.h>
#include <signal.h>
#endif

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)


#ifdef __WINDOWS__

#include <ws2tcpip.h>

extern "C" {

    int inet_aton(const char* cp, struct in_addr* inp) {
#ifdef _UNICODE
        return (InetPtonW(AF_INET, cp, inp));
#else
        return (InetPton(AF_INET, cp, inp));
#endif
    }

    void usleep(CONST uint32_t usec)
    {
        HANDLE timer;
        LARGE_INTEGER ft;

        ft.QuadPart = 0 - (10 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

        timer = CreateWaitableTimer(NULL, TRUE, NULL);
        SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    }
}
#endif

#ifdef __APPLE__
extern "C" {
//@TODO @Pierre implement mremap and clock_gettime
void* mremap(void* old_address, size_t old_size, size_t new_size, int flags)
{
    return (nullptr);
}
};
int clock_gettime(int, struct timespec*)
{
    return 0;
}
#endif

#ifdef __LINUX__

static std::atomic<bool> g_lock(false);
static pthread_t g_targetThread;
static void** g_threadCallstackBuffer;
static int g_threadCallstackBufferSize;
static int g_threadCallstackBufferUsed;
static Core::Event g_callstackCompleted(true, true);

static void* GetPCFromUContext(void* secret)
{
    void* pnt = nullptr;

#if defined(__arm__)
    ucontext_t* ucp = reinterpret_cast<ucontext_t*>(secret);
    pnt = reinterpret_cast<void*>(ucp->uc_mcontext.arm_pc);
#elif defined(__APPLE__)
    ucontext_t* uc = (ucontext_t*)secret;
    pnt = reinterpret_cast<void*>(uc->uc_mcontext->__ss.__rip);
#elif defined(__x86_64__)
    ucontext_t* uc = (ucontext_t*)secret;
    pnt = (void*)uc->uc_mcontext.gregs[REG_RIP];
#elif defined(__i386__)
    ucontext_t* uc = (ucontext_t*)secret;
    pnt = (void*)uc->uc_mcontext.gregs[REG_EIP];
#elif defined(__mips__)
    ucontext_t* ucp = reinterpret_cast<ucontext_t*>(secret);
    pnt = reinterpret_cast<void*>(ucp->uc_mcontext.pc);
#else
#warning Failed to find right code to retrieve Program Counter.
#endif
    return pnt;
}

static void OverrideStackTopWithPC(void** stack, int stackSize, void* secret)
{
    // Move all stack entries one to the right, make sure not to write beyond buffer.
    uint32_t movedCount = std::min(stackSize, g_threadCallstackBufferSize - 1);
    memmove(stack + 1, stack, sizeof(void*) * movedCount);

    // Set rest to zeroes.
    memset(stack + 1 + movedCount, 0, sizeof(void*) * (g_threadCallstackBufferSize - movedCount - 1));

    // Assign PC to first entry.
    stack[0] = GetPCFromUContext(secret);
}

static void CallstackSignalHandler(int signr VARIABLE_IS_NOT_USED, siginfo_t* info VARIABLE_IS_NOT_USED, void* secret)
{
    if (pthread_self() == g_targetThread) {

        // Initialize buffer to zeroes.
        memset(g_threadCallstackBuffer, 0, (g_threadCallstackBufferSize * sizeof(void*)));

        g_threadCallstackBufferUsed = backtrace(g_threadCallstackBuffer, g_threadCallstackBufferSize);

        OverrideStackTopWithPC(g_threadCallstackBuffer, g_threadCallstackBufferSize, secret);

        g_callstackCompleted.SetEvent();
    }
}

uint32_t GetCallStack(const ThreadId threadId, void* addresses[], const uint32_t bufferSize)
{
#ifdef __LINUX__
    uint32_t result = 0;

    if ((threadId == 0) || (pthread_self() == threadId)) {
        result = backtrace(addresses, bufferSize);
    } else {
        while (std::atomic_exchange_explicit(&g_lock, true, std::memory_order_acquire))
            ; // spin until acquired

        struct sigaction original;
        struct sigaction callstack;
        sigfillset(&callstack.sa_mask);
        callstack.sa_flags = SA_SIGINFO;
        callstack.sa_sigaction = CallstackSignalHandler;
        sigaction(SA_SIGINFO, &callstack, &original);

        g_targetThread = (threadId == 0 ? pthread_self() : threadId);
        g_threadCallstackBuffer = addresses;
        g_threadCallstackBufferSize = bufferSize;
        g_threadCallstackBufferUsed = 0;
        g_callstackCompleted.ResetEvent();

        // call _callstack_signal_handler in target thread
        if (pthread_kill((pthread_t)threadId, SA_SIGINFO) == 0) {
            g_callstackCompleted.Lock(200); // This should definitely be possible in 200 ms :-)
        }

        // Resore the original signal handler
        sigaction(SA_SIGINFO, &original, nullptr);

        result = g_threadCallstackBufferUsed;

        std::atomic_store_explicit(&g_lock, false, std::memory_order_release);
    }

    return result;
#else
    DEBUG_VARIABLE(threadId);
    DEBUG_VARIABLE(addresses);
    DEBUG_VARIABLE(bufferSize);
    return 0;
#endif
}

#else

uint32_t GetCallStack(const ThreadId threadId, void* addresses[], const uint32_t bufferSize)
{
    __debugbreak();

    return (0);
}

#endif // __LINUX__

void* memrcpy(void* _Dst, const void* _Src, size_t _MaxCount)
{
    unsigned char* destination = static_cast<unsigned char*>(_Dst) + _MaxCount - 1;
    const unsigned char* source = static_cast<const unsigned char*>(_Src) + _MaxCount - 1;

    while (_MaxCount) {
        *destination-- = *source--;
        --_MaxCount;
    }

    return (destination);
}

extern "C" {

void DumpCallStack(const ThreadId threadId)
{
#ifdef __DEBUG__
#ifdef __LINUX__
    void* addresses[20];

    int addressCount = GetCallStack(threadId, addresses, (sizeof(addresses) / sizeof(addresses[0])));

    fprintf(stderr, "=== Stack traceback (most recent call first):\n");
    backtrace_symbols_fd(addresses, addressCount, fileno(stderr));
    fflush(stderr);
#else
    __debugbreak();
#endif
#endif
}
}

#ifdef __LINUX__

void SleepMs(unsigned int a_Time)
{
    struct timespec sleepTime;
    struct timespec waitedTime;

    sleepTime.tv_sec = (a_Time / 1000);
    sleepTime.tv_nsec = (a_Time - (sleepTime.tv_sec * 1000)) * 1000000;

    ::nanosleep(&sleepTime, &waitedTime);
}

#endif

#if !defined(__WINDOWS__) && !defined(__APPLE__)

uint64_t htonll(const uint64_t& value)
{
    // The answer is 42
    static const int num = 42;

    // Check the endianness
    if (*reinterpret_cast<const char*>(&num) == num) {
        const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
        const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    } else {
        return value;
    }
}

uint64_t ntohll(const uint64_t& value)
{
    // The answer is 42
    static const int num = 42;

    // Check the endianness
    if (*reinterpret_cast<const char*>(&num) == num) {
        const uint32_t high_part = ntohl(static_cast<uint32_t>(value >> 32));
        const uint32_t low_part = ntohl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    } else {
        return value;
    }
}
#endif

namespace WPEFramework {
namespace Core {

    /* virtual */ IIPC::~IIPC() {}
    /* virtual */ IIPCServer::~IIPCServer() {}
    /* virtual */ IPCChannel::~IPCChannel() {}
}
}

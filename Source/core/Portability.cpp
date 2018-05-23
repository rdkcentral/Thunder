#include <stdlib.h>

#include "Portability.h"
#include "SystemInfo.h"
#include "Sync.h"
#include "IPCConnector.h"

#ifdef __LINUX__
#include <signal.h>
#include <execinfo.h>

#define CALLSTACK_SIG SIGUSR2
#endif

using namespace WPEFramework;

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

#ifdef __APPLE__
extern "C" {
//@TODO @Pierre implement mremap and clock_gettime
void* mremap(void *old_address, size_t old_size, size_t new_size, int flags) {
    return (nullptr);
}

};
int clock_gettime(int, struct timespec*){
    return 0;
}
#endif

#ifdef __LINUX__
#if defined(__DEBUG__) || defined(CRITICAL_SECTION_LOCK_LOG)
static pthread_t g_TargetThread, g_CallingThread;
static void** g_ThreadCallstackBuffer;
static int g_ThreadCallstackBufferSize;
static int g_ThreadCallstackCount;
static Core::CriticalSection g_CallstackMutex;

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
    bool foundNull = false;

    int i;
    for (i = 0; i < stackSize; i++) {
        void* ptr = stack[i];

        if (ptr != nullptr && foundNull) {
            // Found first non-null entry.
            --i;
            break;
        }
        else if (ptr == nullptr) {
            foundNull = true;
        }
    }

    if (i == stackSize) {
        return;
    }

    stack[i] = GetPCFromUContext(secret);

    // Remove unneeded stack entries.
    memmove(stack, stack + i, sizeof(void*) * (stackSize - i));

    // Set rest to zeroes.
    memset(stack + stackSize - i, 0, sizeof(void*) * i);
}

static void CallstackSignalHandler(int signr VARIABLE_IS_NOT_USED, siginfo_t* info VARIABLE_IS_NOT_USED, void* secret)
{
    pthread_t myThread = pthread_self();
    if (myThread != g_TargetThread) {
        return;
    }

    // Initialize buffer to zeroes.
    memset(g_ThreadCallstackBuffer, 0, g_ThreadCallstackBufferSize);

    g_ThreadCallstackCount = backtrace(g_ThreadCallstackBuffer, g_ThreadCallstackBufferSize);

    OverrideStackTopWithPC(g_ThreadCallstackBuffer, g_ThreadCallstackBufferSize, secret);

    // continue calling thread
    pthread_kill((pthread_t)g_CallingThread, CALLSTACK_SIG);
}

static void SetupCallstackSignalHandler()
{
    struct sigaction sa;
    sigfillset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = CallstackSignalHandler;
    sigaction(CALLSTACK_SIG, &sa, nullptr);
}
#endif

int GetCallStack(void ** addresses, int bufferSize)
{
#if defined(__DEBUG__) || defined(CRITICAL_SECTION_LOCK_LOG)
    return backtrace(addresses, bufferSize);
#else
    DEBUG_VARIABLE(addresses);
    DEBUG_VARIABLE(bufferSize);
    return 0;
#endif
}

int GetCallStack(ThreadId threadId, void ** addresses, int bufferSize)
{
#if defined(__DEBUG__) || defined(CRITICAL_SECTION_LOCK_LOG)
    if (threadId == 0 || threadId == pthread_self()) {
        return GetCallStack(threadId, addresses, bufferSize);
    }

    g_CallstackMutex.Lock();
    g_CallingThread = pthread_self();
    g_TargetThread = threadId;
    g_ThreadCallstackBuffer = addresses;
    g_ThreadCallstackBufferSize = bufferSize;

    SetupCallstackSignalHandler();

    // call _callstack_signal_handler in target thread
    if (pthread_kill((pthread_t)threadId, CALLSTACK_SIG) != 0) {
        return 0;
    }

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, CALLSTACK_SIG);

    // wait for CALLSTACK_SIG on this thread
    sigsuspend(&mask);

    g_ThreadCallstackBuffer = nullptr;
    g_ThreadCallstackBufferSize = 0;
    int threadCallstackCount = g_ThreadCallstackCount;

    g_CallstackMutex.Unlock();

    return threadCallstackCount;
#else
    DEBUG_VARIABLE(threadId);
    DEBUG_VARIABLE(addresses);
    DEBUG_VARIABLE(bufferSize);
    return 0;
#endif
}

#else

int GetCallStack(void ** addresses, int bufferSize)
{
    __debugbreak();

	return (0);
}

int GetCallStack(ThreadId threadId, void ** addresses, int bufferSize)
{
    __debugbreak();

	return (0);
}

#endif // __LINUX__

void* memrcpy(void* _Dst, const void* _Src, size_t _MaxCount)
{
    unsigned char* destination = static_cast<unsigned char*>(_Dst) + _MaxCount;
    const unsigned char* source = static_cast<const unsigned char*>(_Src) + _MaxCount;

    while (_MaxCount) {
        *destination-- = *source--;
        --_MaxCount;
    }

    return (destination);
}

extern "C" {

void DumpCallStack()
{
#ifdef __DEBUG__
#ifdef __LINUX__
    void* addresses[20];

    int addressCount = GetCallStack(addresses, (sizeof(addresses) / sizeof(addresses[0])));

    backtrace_symbols_fd(addresses, addressCount, fileno(stderr));
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

#if !defined(__WIN32__) && !defined(__APPLE__)

uint64_t htonll(const uint64_t& value)
{
    // The answer is 42
    static const int num = 42;

    // Check the endianness
    if (*reinterpret_cast<const char*>(&num) == num) {
        const uint32_t high_part = htonl(static_cast<uint32_t>(value >> 32));
        const uint32_t low_part = htonl(static_cast<uint32_t>(value & 0xFFFFFFFFLL));

        return (static_cast<uint64_t>(low_part) << 32) | high_part;
    } 
    else {
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
    } 
    else {
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

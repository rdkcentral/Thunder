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

#include <sys/shm.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/futex.h>    // FUTEX_* constants
#include <sys/syscall.h>    // SYS_* constants
#include <poll.h>
#ifdef __cplusplus
}
#endif

#include <climits>
#include <limits>
#include <chrono>

#include "IPTestAdministrator.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE);

IPTestAdministrator::IPTestAdministrator(Callback parent, Callback child, const uint32_t initHandShakeValue, const uint32_t waitTime)
    : _sharedData{nullptr}
    , _pid{-1}
    , _waitTime(waitTime)
{
    ASSERT(waitTime > 0 && waitTime < ::Thunder::Core::infinite);

    _shm_id = shmget(IPC_PRIVATE, sizeof(struct SharedData) /* size */, IPC_CREAT | 0666 /* read and write for user, group and others */);

    ASSERT(_shm_id >= 0);

    _sharedData = reinterpret_cast<struct SharedData*>(shmat(_shm_id, nullptr, 0 /* attach for reading and writing */));

    ASSERT(_sharedData != nullptr);

    _sharedData->handshakeValue.exchange(initHandShakeValue);

    _pid = fork();

    ASSERT(_pid != -1);

    if (_pid == 0) {
        // Child process

        ASSERT(child !=  nullptr);
        child(*this);

        std::exit(0); // Avoid multiple / repeated test runs
    } else {
        ASSERT(parent !=  nullptr);
        parent(*this);
    }
}

IPTestAdministrator::~IPTestAdministrator()
{
    if (_pid > 0) {
        // Is the child still alive?

        int pid_fd = syscall(SYS_pidfd_open, _pid, 0);

        if (pid_fd != -1) {
            struct pollfd fds = { pid_fd, POLLIN, 0 };

            constexpr auto seconds2milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)).count();
            const int timeoutMs = (_waitTime >= (std::numeric_limits<int>::max() / seconds2milliseconds)) ? std::numeric_limits<int>::max() : static_cast<int>(_waitTime * seconds2milliseconds);

            switch (poll(&fds, sizeof(fds) / sizeof(struct pollfd), timeoutMs /* timeout in milliseconds */)) {
            case -1 : // error
                        switch(errno) {
                        case EFAULT :   // fds is not within address space
                                        do {} while(false);
                        case EINTR  :   // Signal occured before any requested event
                                        do {} while(false);
                        case EINVAL :   // Number of descriptors too large or invalid timeout value
                                        do {} while(false);
                        case ENOMEM :   // Unable to allocated supported memory
                                        do {} while(false);
                        default     :;
                        }
                        do {} while(false);
            case 0  :   // Timeout expired before events are available
                        {
                            // Potential leak of shared memory descriptor
                            VARIABLE_IS_NOT_USED int result = syscall(SYS_kill, _pid, SIGKILL);

                            ASSERT(result == 0);
                        }
                        do {} while(false);
            default :   // Number of events set because the descriptor is readable, eg, the process has terminated
                        ;
            }

            /* int */ close(pid_fd);
        }
    }

    if(shmdt(_sharedData) == -1) {
        switch(errno) {
        case EINVAL :   // The shared data is not the start address of the shared segment
                        do {} while(false);
        default     :   // Uninpsected or unknown error
                        ;
        }
    }

    if (shmctl(_shm_id, IPC_RMID, nullptr) == -1) {
        switch(errno) {
        case EPERM :    // User ID is not that of the creator, owner or insufficient privileges
                        do {} while(false);
        default     :   // Uninpsected or unknown error
                        ;
        }

    }
}

uint32_t IPTestAdministrator::Wait(uint32_t expectedHandshakeValue) const
{
    uint32_t result = ::Thunder::Core::ERROR_GENERAL;

    // Calculate absolute deadline using monotonic clock for robustness across spurious wakeups
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline) == -1) {
        // Clock read failed — emergency out
        return ::Thunder::Core::ERROR_GENERAL;
    }

    deadline.tv_sec += _waitTime;
    // nanoseconds are already 0 from clock_gettime initialization

    constexpr bool stop = { false };

    do {
        // Calculate remaining time for this futex call
        struct timespec now;
        if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
            // Clock read failed — emergency out
            return ::Thunder::Core::ERROR_GENERAL;
        }

        struct timespec remaining = deadline;
        remaining.tv_sec -= now.tv_sec;
        remaining.tv_nsec -= now.tv_nsec;

        // Handle underflow: if nanoseconds go negative, borrow from seconds
        if (remaining.tv_nsec < 0) {
            remaining.tv_sec--;
            remaining.tv_nsec += 1000000000L; // 1 second in nanoseconds
        }

        // Check if deadline has been reached
        if (remaining.tv_sec < 0) {
            result = ::Thunder::Core::ERROR_TIMEDOUT;
            break;
        }

        long futex_result = syscall(SYS_futex, reinterpret_cast<uint32_t*>(&(_sharedData->handshakeValue)), FUTEX_WAIT, expectedHandshakeValue, &remaining, nullptr, 0);

        switch(futex_result) {
        case 0 :    if (_sharedData->handshakeValue == expectedHandshakeValue) {
                        // True wake-up
                        result = ::Thunder::Core::ERROR_NONE;
                        break;
                    }

                    // Spurious wake-up — loop will recalculate remaining time and retry
                    continue;
        case -1 : //
                    switch(errno) {
                    case EAGAIN    :    // Value mismatch
                                        result = ::Thunder::Core::ERROR_INVALID_RANGE;
                                        break;
                    case ETIMEDOUT :    // Value has not changed within the specified timeout
                                        result = ::Thunder::Core::ERROR_TIMEDOUT;
                                        break;
                    default        :    // Uninspected conditions like EINTR and EINVAL
                                        ;
                    }
                    break;
        default :   // Serious error
                    ;
        }

        break;

    } while(!stop);

    return result;
}

uint32_t IPTestAdministrator::Signal(uint32_t expectedNextHandshakeValue, uint8_t maxRetries)
{
    constexpr std::chrono::seconds s {0};

    uint32_t result = ::Thunder::Core::ERROR_GENERAL;

    long futex_result = syscall(SYS_futex, &(_sharedData->handshakeValue), FUTEX_WAKE, INT_MAX /* number of waiters to wake-up */, nullptr, nullptr, 0);

    switch(futex_result) {
    case -1 :   // Error
                switch(errno) {
                case EINVAL :   // Inconsistent state
                                do {} while (false);
                default     :   // Uninspected // unknown conditions
                                ;
                }
                break;
    case 0  :   {
                    // No waiters
                    constexpr auto seconds2milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)).count();

                    if (maxRetries > 0) {
                        std::this_thread::yield(); // Hopefully the scheduler plays nice
                        std::this_thread::sleep_for(std::chrono::milliseconds(_waitTime * seconds2milliseconds / _waitTimeDivisor)); // Sleep a fraction of the maximum specified waiting time
                        result = Signal(expectedNextHandshakeValue, maxRetries - 1);
                    } else {
                        result = ::Thunder::Core::ERROR_BAD_REQUEST;
                    }
                }
                break;
    default :   result = ::Thunder::Core::ERROR_NONE;
                // Atomically replaces the current value by the expected value
                VARIABLE_IS_NOT_USED bool oldHandshakeValue = _sharedData->handshakeValue.exchange(expectedNextHandshakeValue);
    }

    return result;
}

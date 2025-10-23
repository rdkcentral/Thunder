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

#ifdef __APPLE__
#include <time.h>
#endif

#include <iostream>

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

#include "../IPTestAdministrator.h"

// The tests are 'equivalent' to the flow of ::Thunder::Core::CyclicBuffer without (AccessWrong)
// and with (AccessGood) the proposed path applied. They illustrate that pthread_cond_wait and
// pthread_cond_timedwait required that the mutex has to be owned, ie, pthread_mutex_lock has
// been executed succesfully.

namespace Thunder {
namespace Tests {
namespace Core {

    template <typename DERIVED>
    class SharedAccess {
    public :

        SharedAccess(const SharedAccess&) = delete;
        SharedAccess(SharedAccess&&) = delete;
        SharedAccess& operator=(const SharedAccess&) = delete;
        SharedAccess& operator=(SharedAccess&&) = delete;

        ~SharedAccess()
        {}

        uint32_t Lock(uint32_t waitTime = ::Thunder::Core::infinite)
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {
                result = static_cast<DERIVED*>(this)->LockImplementation(waitTime);
            }

            return result;
        }

        uint32_t Unlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_INVALID_PARAMETER };

            if (_initialized != false) {

                // Flow 'copied' at ::Thunder::Core::CyclicBuffer is:
                // 1. AdminLock()
                // 2. pthread_cond_signal
                // 3. AdminUnlock()

                if (   this->AdminLock() == ::Thunder::Core::ERROR_NONE
                    // Critical section
                    && (_errval = pthread_cond_signal(&_condition)) == 0
                    && this->AdminUnlock() == ::Thunder::Core::ERROR_NONE
                ) {
                    result = ::Thunder::Core::ERROR_NONE;
                } else {
                    result = ::Thunder::Core::ERROR_GENERAL;
                }

            }

            return result;
        }

        int Error() const
        {
            return _errval;
        }

    protected :

        SharedAccess()
        {}

        uint32_t AdminLock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            if ((errval = pthread_mutex_lock(&_mutex)) != 0) {
                _errval = errval;

                result = ::Thunder::Core::ERROR_GENERAL;
            }

            return result;
        }

        uint32_t AdminUnlock()
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            if ((errval = pthread_mutex_unlock(&_mutex)) != 0) {
                _errval = errval;

                result = ::Thunder::Core::ERROR_GENERAL;
            }

            return result;
        }

        uint32_t SignalLock(VARIABLE_IS_NOT_USED const uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_NONE };

            int errval{ 0 };

            if (waitTime != ::Thunder::Core::infinite) {

                // Flow 'copied' at ::Thunder::Core::CyclicBuffer is:
                // 1. clock_gettime
                // 2. adjust maximum time to wait
                // 3. pthread_cond_timedwait

                struct timespec structTime{ 0, 0 };

                // In the original ::Thunder::Core::CyclicBuffer the return value is ignored

                if (clock_gettime(CLOCK_MONOTONIC, &structTime) != 0) {
                    _errval = errno;

                    result = ::Thunder::Core::ERROR_GENERAL;
                }

                // Some arbitrary moment in the future
                structTime.tv_sec += 10;

                if (    result == ::Thunder::Core::ERROR_NONE
                    && (errval = pthread_cond_timedwait(&_condition, &_mutex, &structTime)) != 0
                ) {
                    // In the original ::Thunder::Core::CyclicBuffer the actual time spent is calculated

                    result = ::Thunder::Core::ERROR_TIMEDOUT;

                    // Do not consider a timeout
                    if (errval != ETIMEDOUT) {
                        _errval = errval;

                        result = ::Thunder::Core::ERROR_GENERAL;
                    }
                }

            } else {

                if ((errval = pthread_cond_wait(&_condition, &_mutex)) != 0) {
                    _errval = errval;

                    result = ::Thunder::Core::ERROR_GENERAL;
                }

            }

            return result;
        }

        pthread_cond_t _condition{};
        pthread_mutex_t _mutex{};

    private :

        bool Initialize()
        {
          std::cout << __PRETTY_FUNCTION__ << std::endl;
            pthread_condattr_t condition_attr;
            pthread_mutexattr_t mutex_attr;

            bool result =    pthread_condattr_init(&condition_attr) == 0
                          && pthread_condattr_setclock(&condition_attr, CLOCK_MONOTONIC) == 0
                          && pthread_cond_init(&_condition, &condition_attr) == 0
                          && pthread_mutexattr_init(&mutex_attr) == 0
                          && pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK) == 0
                          && pthread_mutex_init(&_mutex, &mutex_attr) == 0
                          ;

            return result;
        }

        const bool _initialized{ Initialize() };
        int _errval{ 0 };
    };

    class SharedAccessWrong : public SharedAccess<SharedAccessWrong> {
    public :
        SharedAccessWrong(const SharedAccessWrong&) = delete;
        SharedAccessWrong(SharedAccessWrong&&) = delete;
        SharedAccessWrong& operator=(const SharedAccessWrong&) = delete;
        SharedAccessWrong& operator=(SharedAccessWrong&&) = delete;

        SharedAccessWrong() = default;
        ~SharedAccessWrong() = default;

        uint32_t LockImplementation(VARIABLE_IS_NOT_USED uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_TIMEDOUT };

            // In the original ::Thunder::Core::CyclicBuffer the flow is:
            // 1. AdminLock
            // 2. Do while
            // 3. AdminUnlock

            result = AdminLock();

            do {

                // Force the second branch for illustrative purpose

                if (false) {

                } else if (true) {

                    // In the original :: Thunder::Core::CyclicBuffer the flow is:
                    // 1. AdminUnlock
                    // 2. SignalLock
                    // 3. AdminLock

                    if (   result == ::Thunder::Core::ERROR_NONE
                        && AdminUnlock() == ::Thunder::Core::ERROR_NONE
                        && ((result = SignalLock(waitTime)) == ::Thunder::Core::ERROR_NONE || result == ::Thunder::Core::ERROR_TIMEDOUT)
                        && AdminLock() == ::Thunder::Core::ERROR_NONE
                    ) {
                        result = ::Thunder::Core::ERROR_NONE;
                    }

                    // Critical section
                }

                // No need to keep running for illustrative purpose

            } while (false);

            if (   (result == ::Thunder::Core::ERROR_NONE)
                || (result == ::Thunder::Core::ERROR_TIMEDOUT)
            ) {
                result = AdminUnlock();
            }

            return result;
        }

    private :
    };

    class SharedAccessGood : public SharedAccess<SharedAccessGood> {
    public :
        SharedAccessGood(const SharedAccessGood&) = delete;
        SharedAccessGood(SharedAccessGood&&) = delete;
        SharedAccessGood& operator=(const SharedAccessGood&) = delete;
        SharedAccessGood& operator=(SharedAccessGood&&) = delete;

        SharedAccessGood() = default;
        ~SharedAccessGood() = default;

        uint32_t LockImplementation(VARIABLE_IS_NOT_USED uint32_t waitTime)
        {
            uint32_t result{ ::Thunder::Core::ERROR_TIMEDOUT };

            // In the original ::Thunder::Core::CyclicBuffer the flow is:
            // 1. AdminLock
            // 2. Do while
            // 3. AdminUnlock

            result = AdminLock();

            do {

                // Force the second branch for illustrative purpose

                if (false) {

                } else if (true) {

                    // In the original :: Thunder::Core::CyclicBuffer the flow is:
                    // 1. AdminUnlock
                    // 2. SignalLock
                    // 3. AdminLock

                    // The proposed flow is:
                    // 1. SignalLock
                    // 2. AdminUnlock
                    // 3. AdminLock

                    if (   result == ::Thunder::Core::ERROR_NONE
                        && ((result = SignalLock(waitTime)) == ::Thunder::Core::ERROR_NONE || result == ::Thunder::Core::ERROR_TIMEDOUT)
                        // Critical section
                        && AdminUnlock() == ::Thunder::Core::ERROR_NONE
                        && AdminLock() == ::Thunder::Core::ERROR_NONE
                    ) {
                        result = ::Thunder::Core::ERROR_NONE;
                    }

                }

                // No need to keep running for illustrative purpose

            } while (false);

            if (   (result == ::Thunder::Core::ERROR_NONE)
                || (result == ::Thunder::Core::ERROR_TIMEDOUT)
            ) {
                result = AdminUnlock();
            }

            return result;
        }

    private :

    };


    // Illustrate the result without the proposed patch applied
    // As a result, this test fails.
    TEST(METROL_1188, AccessWrong)
    {
        constexpr uint32_t maxWaitTime{ 1000 };

        SharedAccessWrong access;

        // This should time out as no other thread / process will be able to call Unlock
        // However, th flow of execution is erroneous hence a ::Thunder::Core::ERROR_GENERAL
        // is returned
        EXPECT_EQ(access.Lock(maxWaitTime), ::Thunder::Core::ERROR_NONE);

        // A value 1 (EPERM == the current thread does not own the mutex) is returned.
        EXPECT_EQ(access.Error(), 0);

        EXPECT_EQ(access.Unlock(), ::Thunder::Core::ERROR_NONE);

        ::Thunder::Core::Singleton::Dispose();
    }

    // Illustrate the result with the proposed patch applied
    // As a result, this test succeeds.
    TEST(METROL_1188, AccessGood)
    {
        constexpr uint32_t maxWaitTime{ 1000 };

        SharedAccessGood access;

        // This will time out as no other thread / process will be able to call Unlock
        // However, the values return is still ::Thunder::Core::ERROR_NONE, by design
        EXPECT_EQ(access.Lock(maxWaitTime), ::Thunder::Core::ERROR_NONE);

        EXPECT_EQ(access.Error(), 0);

        EXPECT_EQ(access.Unlock(), ::Thunder::Core::ERROR_NONE);

        ::Thunder::Core::Singleton::Dispose();
    }


} // Core
} // Tests
} // Thunder

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
 
#include "SharedBuffer.h"

// TODO: remove if no longer needed for simple tracing.
#include <iostream>

namespace Thunder {

namespace Core {

#ifdef __WINDOWS__
    SharedBuffer::Semaphore::Semaphore(const TCHAR sourceName[])
        : _semaphore(::CreateSemaphore(nullptr, 1, 1, sourceName))
    {
    }
#else
    SharedBuffer::Semaphore::Semaphore(sem_t* storage)
        : _semaphore(storage)
    {
        ASSERT(storage != nullptr);
    }
#endif
    SharedBuffer::Semaphore::~Semaphore()
    {
#ifdef __WINDOWS__
        if (_semaphore != nullptr) {
            ::CloseHandle(_semaphore);
        }
#else
        sem_destroy(_semaphore);
#endif
    }

    uint32_t SharedBuffer::Semaphore::Unlock()
    {
#ifdef __WINDOWS__
        if (_semaphore != nullptr) {
            BOOL result = ::ReleaseSemaphore(_semaphore, 1, nullptr);

            ASSERT(result != FALSE);
        }
#else
        VARIABLE_IS_NOT_USED int result = sem_post(_semaphore);

        ASSERT((result == 0) || (errno == EOVERFLOW));
#endif
        return ERROR_NONE;
    }

    bool SharedBuffer::Semaphore::IsLocked()
    {
#ifdef __WINDOWS__
        bool locked = (::WaitForSingleObjectEx(_semaphore, 0, FALSE) != WAIT_OBJECT_0);

        if (locked == false) {
            ::ReleaseSemaphore(_semaphore, 1, nullptr);
        }

        return (locked);
#else
        int semValue = 0;
        sem_getvalue(_semaphore, &semValue);
        return (semValue == 0);
#endif
    }

    uint32_t SharedBuffer::Semaphore::Lock(const uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_GENERAL;
#ifdef __WINDOWS__
        if (_semaphore != nullptr) {
            return (::WaitForSingleObjectEx(_semaphore, waitTime, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
        }
#elif defined(__APPLE__)

        uint32_t timeLeft = waitTime;
        int semResult;
        while (((semResult = sem_trywait(_semaphore)) != 0) && timeLeft > 0) {
            ::SleepMs(100);
            if (timeLeft != Core::infinite) {
                timeLeft -= (timeLeft > 100 ? 100 : timeLeft);
            }
        }
        result = semResult == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT;
#else

        struct timespec structTime = {0,0};

        clock_gettime(CLOCK_MONOTONIC, &structTime);
        structTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
        structTime.tv_sec += (waitTime / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
        structTime.tv_nsec = structTime.tv_nsec % 1000000000;

        do {
            if (sem_clockwait(_semaphore, CLOCK_MONOTONIC, &structTime) == 0) {
                result = Core::ERROR_NONE;
            }
            else if ( errno == EINTR ) {
                continue;
            }
            else if ( errno == ETIMEDOUT ) {
                result = Core::ERROR_TIMEDOUT;
            }
            else {
                ASSERT(false);
            }
            break;
        } while (true);
#endif
        return (result);
    }

    SharedBuffer::SharedBuffer(const TCHAR name[])
        : DataElementFile(name, File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administrationBuffer((string(name) + ".admin"), File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
#ifdef __WINDOWS__
        , _producer((string(name) + ".producer").c_str())
        , _consumer((string(name) + ".consumer").c_str())
#else
        , _producer(&(_administration->_producer))
        , _consumer(&(_administration->_consumer))
#endif
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)])))
    {
        Align<uint64_t>();
    }
    SharedBuffer::SharedBuffer(const TCHAR name[], const uint32_t mode, const uint32_t bufferSize, const uint16_t administratorSize)
        : DataElementFile(name, mode | File::SHAREABLE | File::CREATE, bufferSize)
        , _administrationBuffer((string(name) + ".admin"), mode | File::SHAREABLE | File::CREATE, administratorSize + sizeof(Administration) + (2 * sizeof(void*)) + 8 /* Align buffer on 64 bits boundary */)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
#ifdef __WINDOWS__
        , _producer((string(name) + ".producer").c_str())
        , _consumer((string(name) + ".consumer").c_str())
#else
        , _producer(&(_administration->_producer))
        , _consumer(&(_administration->_consumer))
#endif
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)])))
    {

#ifndef __WINDOWS__
        memset(_administration, 0, sizeof(Administration));

        sem_init(&(_administration->_producer), 1, 1); /* Initial value is 1. */
        sem_init(&(_administration->_consumer), 1, 0); /* Initial value is 0. */
#endif
        Align<uint64_t>();
    }

    SharedBuffer::~SharedBuffer()
    {
    }
}
}

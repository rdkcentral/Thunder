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

#include <CyclicBuffer.h>
#include <iostream>
#include <Thread.h>

namespace WPEFramework {
namespace Tests {

constexpr char fileName[] = "/tmp/SharedCyclicBuffer";
constexpr uint32_t requiredSize = 10; 
constexpr uint8_t threadWorkerInterval = 10; // milliseconds
constexpr uint8_t lockTimeout = 100; // milliseconds
constexpr uint16_t totalRuntime = 100000; // milliseconds

// A deliberate sync, no errors occur in full sync
pthread_mutex_t mutex;

class Reader : public Core::Thread {
public :

    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader()
        : Core::Thread{}
        , _buffer{
              fileName
            ,   Core::File::USER_READ  // not relevant for readers
              | Core::File::USER_WRITE // open the existing file with write permission
                                       // readers normally require readonly but the cyclic buffer administration is updated after read
//              | Core::File::CREATE     // readers access exisitng underlying memory mapped files
              | Core::File::SHAREABLE  // updates are visible to other processes, but a reader should not update any content except when read data is (automatically) removed
            , 0 // readers do not specify sizes
            , false // overwrite unread data
          }
    {}

    ~Reader() {
//        ASSERT(_buffer.LockPid() != gettid()); // thunder schedules Worker on a different Thread

        Stop();

        // no way to recover if the lock is taken indefinitly, eg, Core::infinite
        do {
        } while (!Wait(Thread::STOPPED, threadWorkerInterval));

        _buffer.Close();
    }

    bool Enable() {
        return    _buffer.IsValid()
               && _buffer.Open() // it already should
               ;
    }

    uint32_t Worker() override {
//        ASSERT(pthread_mutex_lock(&mutex) == 0);

        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /*lockTimeout*/ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            TRACE_L1(_T("Reading data"));

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: reader unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun = threadWorkerInterval; // Some random value, but currently fixed
            }
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: reader lock timed out"));
            } else {
                TRACE_L1(_T("Error: reader lock failed"));
                ASSERT(false);
            }
        }

//        ASSERT(pthread_mutex_unlock(&mutex) == 0);

        return waitTimeForNextRun; // schedule ms from now to be called (again) eg interval time;
    }

private :

    Core::CyclicBuffer _buffer;
};

class Writer : public Core::Thread {
public :

    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    Writer() 
        : Core::Thread{}
        , _buffer{ 
              fileName
            ,   Core::File::USER_READ  // enable read permissions on the underlying file for other users
              | Core::File::USER_WRITE // enable write permission on the underlying file
              | Core::File::CREATE     // create a new underlying memory mapped file
              | Core::File::SHAREABLE  // allow other processes to access the content of the file
            , requiredSize // requested size 
            , false // overwrite unread data
          }
    {}

    ~Writer() {
//        ASSERT(_buffer.LockPid() != gettid()); // thunder schedules Worker on a different Thread

        Stop();

        // no way to recover if the lock is taken indefinitly, eg, Core::infinite
        do {
        } while (!Wait(Thread::STOPPED, threadWorkerInterval));

        _buffer.Close();
    }

    bool Enable() {
        return    _buffer.IsValid()
               && _buffer.Open() // it already should
               ;
    }

    uint32_t Worker() override {
//        ASSERT(pthread_mutex_lock(&mutex) == 0);

        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /*lockTimeout*/ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            TRACE_L1(_T("Writing data"));

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: writer unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun =threadWorkerInterval; // Some random value, but currently fixed
            } 
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: writer lock timed out"));
            } else {
                TRACE_L1(_T("Error: writer lock failed"));
                ASSERT(false);
            }
        }

//        ASSERT(pthread_mutex_unlock(&mutex) == 0);

        return waitTimeForNextRun; // schedule ms from now to be called (again) eg interval time;
    }

private :

    std::atomic<uint8_t> _locking;

    Core::CyclicBuffer _buffer;
};

} // Tests
} // WPEFramework


int main(int argc, char* argv[])
{
    pthread_mutexattr_t mutex_attr;

    int ret = pthread_mutexattr_init(&mutex_attr);
    ASSERT(ret == 0); DEBUG_VARIABLE(ret);

    ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    ASSERT(ret == 0); DEBUG_VARIABLE(ret);

    using namespace WPEFramework::Core;
    using namespace WPEFramework::Tests;

    // The order is important
    Writer writer;
    Reader reader;

    // The underlying memory mapped file is created and opened via DataElementFile construction
    if (   File(fileName).Exists()
        && writer.Enable()
        && reader.Enable()
    ) {
        std::cout << "Shared cyclic buffer created and ready" << std::endl;

        writer.Run();
        reader.Run();

        SleepMs(totalRuntime);

        // The destructors may 'win' the race to the end
        writer.Stop();
        reader.Stop();
    } else {
        std::cout << "Error: Unable to create shared cyclic buffer" << std::endl;
    }

    return 0;
}

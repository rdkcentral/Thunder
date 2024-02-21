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
#include <Thread.h>
#include <cstdlib>

namespace WPEFramework {
namespace Tests {

constexpr uint8_t threadWorkerInterval = 10; // milliseconds
constexpr uint8_t lockTimeout = 100; // milliseconds
constexpr uint32_t totalRuntime = Core::infinite; // milliseconds
constexpr uint8_t sampleSizeInterval = 5;

template <size_t N>
class Reader : public Core::Thread {
public :

    Reader() = delete;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(const std::string& fileName)
        : Core::Thread{}
        , _index{0}
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
    {
        _output.fill('\0');
    }

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
        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /*lockTimeout*/ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            const uint32_t count = std::rand() % sampleSizeInterval;

            if (   Read(count) == count
                && count
               ) {
                TRACE_L1(_T("Data read"));
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: reader unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun = threadWorkerInterval; // some random value, but currently fixed
            }
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: reader lock timed out"));
            } else {
                TRACE_L1(_T("Error: reader lock failed"));
                ASSERT(false);
            }
        }

        return waitTimeForNextRun; // schedule ms from now to be called (again) eg interval time;
    }

private :

    uint32_t Read(uint32_t count) {
        uint32_t read = 0;

        if (   (_index + count) > N
            // two passes
            && _buffer.Read(&_output[_index], N - _index, false) != 0
            && _buffer.Read(&_output[0], _index + count - N, false) != 0
        ) {
            read = count;
            _index = _index + count - N;
        } else {
            // one pass
            if (_buffer.Read(&_output[_index], count, false) != 0) {
                if ((_index + count) == N) {
                    _index = 0;
                } else {
                    ASSERT(_index + count < N);
                    _index += count;
                }
                read = count;
            } else {
                // Unable to read data, possibly no data available
            }
        }

        return read;
    }

    uint32_t _index;

    std::array<uint8_t, N> _output;

    Core::CyclicBuffer _buffer;
};


template <size_t N>
class Writer : public Core::Thread {
public :

    Writer() = delete;
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    Writer(const std::string& fileName, uint32_t requiredSharedBufferSize)
        : Core::Thread{}
        , _index{0}
        , _buffer{
              fileName
            ,   Core::File::USER_READ  // enable read permissions on the underlying file for other users
              | Core::File::USER_WRITE // enable write permission on the underlying file
              | Core::File::CREATE     // create a new underlying memory mapped file
              | Core::File::SHAREABLE  // allow other processes to access the content of the file
            , requiredSharedBufferSize // requested size
            , false // overwrite unread data
          }
    {
        static_assert(N > 0, "Specify a data set with at least one character (N > 0).");

        // https://en.wikipedia.org/wiki/Lorem_ipsum
        memcpy(_input.data(), "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.", N);
    }

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
        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /*lockTimeout*/ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            const uint32_t count = std::rand() % sampleSizeInterval;

            if (   Write(count) == count
                && count
               ) {
                TRACE_L1(_T("Data written"));
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: writer unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun =threadWorkerInterval; // some random value, but currently fixed
            } 
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: writer lock timed out"));
            } else {
                TRACE_L1(_T("Error: writer lock failed"));
                ASSERT(false);
            }
        }

        return waitTimeForNextRun; // schedule ms from now to be called (again) eg interval time;
    }

private :

    uint32_t Write(uint32_t count) {
        uint32_t written = 0;

        if (   (_index + count) > N
            // two passes
            && _buffer.Write(&_input[_index], N - _index) != 0
            && _buffer.Write(&_input[0], _index + count - N) != 0
        ) {
            written = count;
            _index = _index + count - N;
        } else {
            // one pass
            if (_buffer.Write(&_input[_index], count) != 0) {
                if ((_index + count) == N) {
                    _index = 0;
                } else {
                    ASSERT(_index + count < N);
                    _index += count;
                }
                written = count;
            } else {
                // Unable to write data, possibly buffer full
            }
        }

        return written;
    }

    uint32_t _index;

    std::array<uint8_t, N> _input;

    Core::CyclicBuffer _buffer;
};

} // Tests
} // WPEFramework


int main(int argc, char* argv[])
{
    using namespace WPEFramework::Core;
    using namespace WPEFramework::Tests;

    constexpr char fileName[] = "/tmp/SharedCyclicBuffer";

    // add some randomness
    std::srand(std::time(nullptr));

    // The order is important
    Writer<446> writer(fileName, 446);
    Reader<446> reader(fileName);

    // The underlying memory mapped file is created and opened via DataElementFile construction
    if (   File(fileName).Exists()
        && writer.Enable()
        && reader.Enable()
    ) {
        std::cout << "Shared cyclic buffer created and ready" << std::endl;

        writer.Run();
        reader.Run();

        SleepMs(totalRuntime);

        // the destructors may 'win' the race to the end
        writer.Stop();
        reader.Stop();
    } else {
        std::cout << "Error: Unable to create shared cyclic buffer" << std::endl;
    }

    return 0;
}

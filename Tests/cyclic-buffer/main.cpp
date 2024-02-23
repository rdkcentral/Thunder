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
#include <array>
#include <memory>
#include <algorithm>

namespace WPEFramework {
namespace Tests {

constexpr uint8_t threadWorkerInterval = 10; // Milliseconds
constexpr uint8_t lockTimeout = 100; // Milliseconds
constexpr uint32_t totalRuntime = Core::infinite; // Milliseconds
constexpr uint8_t sampleSizeInterval = 5;

template <size_t N>
class Reader : public Core::Thread {
public :

    Reader() = delete;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(const std::string& fileName)
        : Core::Thread{}
        , _index{ 0 }
        , _buffer{
              fileName
            ,   Core::File::USER_READ  // Not relevant for readers
              | Core::File::USER_WRITE // Open the existing file with write permission
                                       // Readers normally require readonly but the cyclic buffer administration is updated after read
//              | Core::File::CREATE     // Readers access exisitng underlying memory mapped files
              | Core::File::SHAREABLE  // Updates are visible to other processes, but a reader should not update any content except when read data is (automatically) removed
            , 0 // Readers do not specify sizes
            , false // Overwrite unread data
          }
    {
        _output.fill('\0');
    }

    ~Reader()
    {
//        ASSERT(_buffer.LockPid() != gettid()); // Thunder schedules Worker on a different Thread

        Stop();

        // No way to recover if the lock is taken indefinitly, eg, Core::infinite
        do {
        } while (!Wait(Thread::STOPPED, threadWorkerInterval));constexpr size_t internalBufferSize = 446;

        _buffer.Close();
    }

    bool Enable()
    {
        return    _buffer.IsValid()
               && _buffer.Open() // It already should
               ;
    }

    uint32_t Worker() override
    {
        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /* lockTimeout */ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            const uint32_t count = std::rand() % sampleSizeInterval;

            if (   count
                && Read(count) == count
               ) {
                TRACE_L1(_T("Data read"));
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: reader unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun = std::rand() % threadWorkerInterval;
            }
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: reader lock timed out"));
            } else {
                TRACE_L1(_T("Error: reader lock failed"));
                ASSERT(false);
            }
        }

        return waitTimeForNextRun; // Schedule milliseconds from now to be called (again) eg interval time
    }

private :

    uint32_t Read(uint32_t count)
    {
        uint32_t read = 0;

        if ((_index + count) > N) {
            // Two passes when passing the boundary
            read = _buffer.Read(&_output[_index], N - _index, false);
            read += _buffer.Read(&_output[0], count - read, false);
        } else {
            // One pass
            read = _buffer.Read(&_output[_index], count, false);
        }

        _index = (_index + read) % N;

        return read;
    }

    uint64_t _index;

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
        , _index{ 0 }
        , _buffer{
              fileName
            ,   Core::File::USER_READ  // Enable read permissions on the underlying file for other users
              | Core::File::USER_WRITE // Enable write permission on the underlying file
              | (requiredSharedBufferSize ? Core::File::CREATE : 0) // Create a new underlying memory mapped file
              | Core::File::SHAREABLE  // Allow other processes to access the content of the file
            , requiredSharedBufferSize // Requested size
            , false // Overwrite unread data
          }
    {
        static_assert(N > 0, "Specify a data set with at least one character (N > 0).");

        // https://en.wikipedia.org/wiki/Lorem_ipsum
        memcpy(_input.data(), "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.", N);
    }

    ~Writer()
    {
//        ASSERT(_buffer.LockPid() != gettid()); // Thunder schedules Worker on a different Thread

        Stop();

        // No way to recover if the lock is taken indefinitly, eg, Core::infinite
        do {
        } while (!Wait(Thread::STOPPED, threadWorkerInterval));

        _buffer.Close();
    }

    bool Enable() {
        return    _buffer.IsValid()
               && _buffer.Open() // It already should
               ;
    }

    uint32_t Worker() override
    {
        ASSERT(lockTimeout < Core::infinite);

        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, /* Core::infinite */ Core::infinite /* lockTimeout */ /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            const uint32_t count = std::rand() % sampleSizeInterval;

            if (   count
                && Write(count) == count
               ) {
                TRACE_L1(_T("Data written"));
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: writer unlock failed"));
//                Stop();
                ASSERT(false);
            } else {
                waitTimeForNextRun = std::rand() % threadWorkerInterval;
            } 
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: writer lock timed out"));
            } else {
                TRACE_L1(_T("Error: writer lock failed"));
                ASSERT(false);
            }
        }

        return waitTimeForNextRun; // Schedule milliseconds from now to be called (again) eg interval time
    }

private :

    uint32_t Write(uint32_t count)
    {
        uint32_t written = 0;

        if ((_index + count) > N) {
            // Two passes when passing the boundary
            written = _buffer.Write(&_input[_index], N - _index);
            written += _buffer.Write(&_input[0], count - written);
        } else {
            // One pass
            written = _buffer.Write(&_input[_index], count);
        }

        _index = (_index + written) % N;

        return written;
    }

    uint64_t _index;

    std::array<uint8_t, N> _input;

    Core::CyclicBuffer _buffer;
};

template<size_t mmemoryMappedFileRequestedSize, size_t internalBufferSize>
class BufferCreator
{
public :

    BufferCreator() = delete;
    BufferCreator(const BufferCreator&) = delete;
    BufferCreator& operator=(const BufferCreator&) = delete;

    BufferCreator(const std::string& fileName)
        : _writer(fileName, mmemoryMappedFileRequestedSize)
    {
        static_assert(mmemoryMappedFileRequestedSize, "Specify mmemoryMappedFileRequestedSize > 0");
    }

    bool Enable()
    {
        return _writer.Enable();
    }

    bool Start() const
    {
        constexpr bool result = true;

        _writer.Run();

        return result;
    }

    bool Stop() const
    {
        constexpr bool result = true;

        _writer.Stop();

        return result;
    }

private :

    Writer<internalBufferSize> _writer;
};

template<size_t internalBufferSize>
class BufferUsers
{
public :
    BufferUsers() = delete;
    BufferUsers(const BufferUsers&) = delete;
    BufferUsers& operator=(const BufferUsers&) = delete;

    BufferUsers(const std::string& fileName)
        : _writers{ std::unique_ptr<Writer<internalBufferSize>>(new Writer<internalBufferSize>(fileName, 0))}
        , _readers{ std::unique_ptr<Reader<internalBufferSize>>(new Reader<internalBufferSize>(fileName)) }
    {}

    ~BufferUsers()
    {
        /* bool */  Stop();
    }

    bool Enable()
    {
        return    std::all_of(_writers.begin(), _writers.end(), [] (std::unique_ptr<Writer<internalBufferSize>>& writer){ return writer->Enable(); })
               && std::all_of(_readers.begin(), _readers.end(), [] (std::unique_ptr<Reader<internalBufferSize>>& reader){ return reader->Enable(); })
        ;
    }

    bool Start() const
    {
        constexpr bool result = true;

        for_each(_writers.begin(), _writers.end(), [] (const std::unique_ptr<Writer<446>>& writer){ writer->Run(); });
        for_each(_readers.begin(), _readers.end(), [] (const std::unique_ptr<Reader<446>>& reader){ reader->Run(); });

        return result;
    }

    bool Stop() const
    {
        constexpr bool result = true;

        for_each(_writers.begin(), _writers.end(), [] (const std::unique_ptr<Writer<446>>& writer){ writer->Stop(); });
        for_each(_readers.begin(), _readers.end(), [] (const std::unique_ptr<Reader<446>>& reader){ reader->Stop(); });

        return result;
    }

private :

    std::array<std::unique_ptr<Writer<internalBufferSize>>, 1> _writers;
    std::array<std::unique_ptr<Reader<internalBufferSize>>, 1> _readers;
};


} // Tests
} // WPEFramework



int main(int argc, char* argv[])
{
    using namespace WPEFramework::Core;
    using namespace WPEFramework::Tests;

    constexpr char fileName[] = "/tmp/SharedCyclicBuffer";

    // Add some randomness
    std::srand(std::time(nullptr));

    constexpr uint32_t mmemoryMappedFileRequestedSize = 446;
    constexpr uint32_t internalBufferSize = 446;

    // The order is important
    BufferCreator<mmemoryMappedFileRequestedSize, internalBufferSize> creator(fileName);
    BufferUsers<internalBufferSize> users(fileName);

    // The underlying memory mapped file is created and opened via DataElementFile construction
    if (   creator.Enable()
        && File(fileName).Exists()
        && users.Enable()
    ) {
        std::cout << "Shared cyclic buffer created and ready" << std::endl;

//        creator.Run(); // We can but we do not like writing
        /* bool */ users.Start();

        SleepMs(totalRuntime);

        // The destructors may 'win' the race to the end
//        creator.Stop();
        /* bool */ users.Stop();
    } else {
        std::cout << "Error: Unable to create shared cyclic buffer" << std::endl;
    }

    return 0;
}

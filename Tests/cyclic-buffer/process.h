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
#include <sys/mman.h>
#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>
#include <sys/wait.h>

// https://en.wikipedia.org/wiki/Lorem_ipsum
#define LOREM_IPSUM_TEXT "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."

// Define in unit that includes this file
//#define CREATOR_WRITE
//#define CREATOR_EXTRA_USERS

namespace WPEFramework {
namespace Tests {

class Thread : public Core::Thread
{
private :

    static constexpr uint32_t n = sizeof(LOREM_IPSUM_TEXT);

public :

    Thread()
        : _status{ true }
    {
        memcpy(_data.data(), LOREM_IPSUM_TEXT, n);
    }

    virtual ~Thread() = default;

protected :

    bool Validate(const uint8_t data[], uint32_t count)
    {
        _status = false;

        std::queue<uint8_t> reference;

        std::for_each(_data.begin(), _data.end(), [&reference] (uint8_t value){reference.push(value);} );

        // Just a little further than the buffer size since the matching pattern might be shifted
        size_t stop = 2 * reference.size();

        uint32_t offset = 0;

        while (   stop > 0
               && offset < count
              ) {
            auto element = reference.front();

            if (element == data[offset]) {
                ++offset;
            } else {
                offset = 0;
            }
                // Shift by one and match pattern again

                reference.push(element);
                reference.pop();

            --stop;
        }

        _status = offset == count;

        return _status;
    }

    bool Status() const
    {
        return _status;
    }

    static constexpr uint8_t threadWorkerInterval = 10; // Milliseconds
    static constexpr uint32_t lockTimeout =  Core::infinite; // Milliseconds
    static constexpr uint8_t sampleSizeInterval = 5; // Number of uint8_t elements
    static constexpr uint8_t forceUnlockRetryCount = 5; // Attemps until Alert() at destruction

    static constexpr uint32_t N = n;

    mutable std::array<uint8_t, N> _data;

    std::atomic<bool> _status;
};

template <size_t N>
class Reader : public Thread
{
public :

    Reader() = delete;
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    Reader(const std::string& fileName)
        : Reader(fileName, 0)
    {}

    Reader(const std::string& fileName, size_t numReservedBlocks)
        : Thread{}
        , _enabled{ false }
        , _fileName{ fileName }
        , _index{ 0 }
        , _numReservedBlocks{ numReservedBlocks }
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
        uint8_t count { forceUnlockRetryCount };
        do {
            if (count == 0) {
                _buffer.Alert();
            } else {
                --count;
            }
        } while (!Wait(Thread::STOPPED | Thread::BLOCKED, threadWorkerInterval));

        _buffer.Close();
    }

    bool Enable()
    {
        _enabled =     _enabled
                    || (   _buffer.IsValid()
                        && _buffer.Open() // It already should
                        && _buffer.Storage().Name() == _fileName
                        && Core::File(_fileName).Exists()
                        && _buffer.Storage().Exists()
                       )
                    ;

        ASSERT(_enabled);

        return _enabled;
    }

    bool IsEnabled() const
    {
        return _enabled;
    }

    bool HasError() const
    {
        return !Status();
    }

    uint32_t Worker() override
    {
        uint32_t waitTimeForNextRun = Core::infinite;

        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, lockTimeout /* waiting time to give up lock */);

        if (status == Core::ERROR_NONE) {
            const uint32_t count = std::rand() % sampleSizeInterval;

            uint32_t read = Read(count);
            if (read == count) {
//                TRACE_L1(_T("Data read"));
            } else if (count > 0 ){
//                TRACE_L1(_T("Less data read than requested")); // Possibly too few writes
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: reader unlock failed"));
                Stop();
            } else {
                waitTimeForNextRun = std::rand() % threadWorkerInterval;
            }
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: reader lock timed out"));
            } else {
                TRACE_L1(_T("Error: reader lock failed"));
                Stop();
            }
        }

        return waitTimeForNextRun; // Schedule milliseconds from now to be called (again) eg interval time
    }

private :

    uint32_t Read(uint32_t count)
    {
        uint32_t read = 0;

        if (count > 0) {
            std::vector<uint8_t> data(count, '\0');

            if ((_index + count) > N) {
                // Two passes when passing the boundary
                read = _buffer.Read(&(data.data()[0]), N - _index, false);
                memcpy(&_output[_index], &(data.data()[0]), read);

                if (   read == (N - _index)
                    && count > read
                   ) {
                    const uint32_t position = read;

                    read = _buffer.Read(&(data.data()[position]), count - read, false);
                    memcpy(&_output[0], &(data.data()[position]), read);

                    read = read + position;
                }
            } else {
                read = _buffer.Read(&(data.data()[0]), count, false);
                memcpy(&_output[_index], &(data.data()[0]), read);
            }

            if (   read > 0
                && _buffer.IsOverwrite() // The data may have been overwritten
                && !Validate(data.data(), read)
               ) {
                TRACE_L1("Error: detected read data corruption.");
            }

            _index = (_index + read) % N;
        }

        return read;
    }

    // Only relevant with reserve being actively used, and, in single writer context
    bool Validate(const uint8_t data[], uint32_t count)
    {
        bool result = true;

        if (   _numReservedBlocks > 0
            && count > 0
           ) {
               result = Thread::Validate(data, count);
        }

        return result;
    }

    bool _enabled;

    const std::string _fileName;

    uint64_t _index;

    const size_t _numReservedBlocks;

    std::array<uint8_t, N> _output;

    Core::CyclicBuffer _buffer;
};


template <size_t N>
class Writer : public Thread
{
public :

    Writer() = delete;
    Writer(const Writer&) = delete;
    Writer& operator=(const Writer&) = delete;

    Writer(const std::string& fileName, uint32_t requiredSharedBufferSize)
        : Writer(fileName, requiredSharedBufferSize, 0)
    {}

    Writer(const std::string& fileName, uint32_t requiredSharedBufferSize, size_t numReservedBlocks)
        : Thread{}
        , _enabled{ false }
        , _fileName { fileName }
        , _index{ 0 }
        , _numReservedBlocks{ numReservedBlocks }
        , _reserved{ 0 }
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
        static_assert(N > 0 || Thread::N > N, "Specify a data set with at least one character (N > 0) with N <= Thread::N.");

        memcpy(_input.data(), Thread::_data.data(), N);
    }

    ~Writer()
    {
//        ASSERT(_buffer.LockPid() != gettid()); // Thunder schedules Worker on a different Thread

        Stop();

        // No way to recover if the lock is taken indefinitly, eg, Core::infinite
        uint8_t count { forceUnlockRetryCount };
        do {
            if (count == 0) {
                _buffer.Alert();
            } else {
                --count;
            }
        } while (!Wait(Thread::STOPPED | Thread::BLOCKED, threadWorkerInterval));

        _buffer.Close();
    }

    bool Enable()
    {
        _enabled =     _enabled
                    || (   _buffer.IsValid()
                        && _buffer.Open() // It already should
                        && _buffer.Storage().Name() == _fileName
                        && Core::File(_fileName).Exists()
                        && _buffer.Storage().Exists()
                       )
                    ;

        ASSERT(_enabled);

        return _enabled;
    }

    bool IsEnabled() const
    {
        return _enabled;
    }

    bool HasError() const
    {
        return !Status();
    }

    uint32_t Worker() override
    {
        uint32_t waitTimeForNextRun = Core::infinite;

        // Write(), Free() and Reserve() may all experience race conditions due to thread scheduling
        uint32_t status = _buffer.Lock(false /* data present, false == signalling path, true == PID path */, lockTimeout /* waiting time to give up lock */);
        if (status == Core::ERROR_NONE) {
            uint32_t count = std::rand() % sampleSizeInterval;

            if (   _numReservedBlocks > 0
                && _reserved == 0
               ) {
                _reserved = _buffer.Size() / _numReservedBlocks;

//                TRACE_L1(_T("reserved : %ld"), reserved);
                ASSERT(_buffer.Size() % _numReservedBlocks == 0);

                // Both Free() and Reserve() may experience a race condition with other writers
                if (   _reserved > _buffer.Free()
                    || _buffer.Reserve(_reserved) != Core::ERROR_NONE) {
                    // Another has already made a reservation or the block is unavailable
                    _reserved = 0;
                }
            }

            if (   _numReservedBlocks > 0
                && _reserved <= count
               ) {
                count = _reserved;
            }

            uint32_t written = 0;

            if (   _numReservedBlocks > 0
                && _reserved == 0
               ) {
                // Reservation mode but failed to 'allocate' a block
            } else {
                written = Write(count);
                _reserved -= (_numReservedBlocks > 0 ? written : 0);
            }

            if (   written > 0
                && written == count
               ) {
//                TRACE_L1(_T("Data written"));
            } else if (count > 0) {
//                TRACE_L1(_T("Less data written than requested")); // Possibly too few reads
            }

            status = _buffer.Unlock();

            if (status != Core::ERROR_NONE) {
                TRACE_L1(_T("Error: writer unlock failed"));
                Block();
            } else {
                waitTimeForNextRun = std::rand() % threadWorkerInterval;
            }
        } else {
            if (status == Core::ERROR_TIMEDOUT) {
                TRACE_L1(_T("Warning: writer lock timed out"));
            } else {
                TRACE_L1(_T("Error: writer lock failed"));
                Block();
            }
        }

        return waitTimeForNextRun; // Schedule milliseconds from now to be called (again) eg interval time
    }

private :

    uint32_t Write(uint32_t count)
    {
        uint32_t written = 0;

        if (count > 0) {
            std::vector<uint8_t> data(count, '\0');

            // _index 0..N-1 for N
            if ((_index + count) > N) {
                // Two passes when passing the boundary
                // Note: full data writes with insufficient (total free) space typically are refused but here might be bypassed

                data.assign(&_input[_index], &_input[_index + N -_index]);
                written = _buffer.Write(&(data.data()[0]), N - _index);

                if (   written == N - _index
                    && count > written
                   ) {
                    data.insert(data.begin() + written, &_input[0], &_input[count - written]);
                    written += _buffer.Write(&(data.data()[written]), count - written);
                }
            } else {
                // One pass
                data.assign(&_input[_index], &_input[_index + count]);
                written = _buffer.Write(&(data.data()[0]), count);
            }

            if (   written > 0
                && !Validate(data.data(), written)
               ) {
                TRACE_L1("Error: detected written data corruption.");
            }

            _index = (_index + written) % N;
        }

        return written;
    }

    // Only 'relevant' with reserve being actively used
    bool Validate(const uint8_t data[], uint32_t count)
    {
        bool result = true;

        if (   _numReservedBlocks > 0
            && count > 0
           ) {
               result = Thread::Validate(data, count);
        }

        return result;
    }

    bool _enabled;

    const std::string _fileName;

    uint64_t _index;

    const size_t _numReservedBlocks;

    uint32_t _reserved;

    mutable std::array<uint8_t, N> _input;

    Core::CyclicBuffer _buffer;
};

template<size_t memoryMappedFileRequestedSize, size_t internalBufferSize>
class BufferCreator
{
public :

    BufferCreator() = delete;
    BufferCreator(const BufferCreator&) = delete;
    BufferCreator& operator=(const BufferCreator&) = delete;

    BufferCreator(const std::string& fileName)
        : BufferCreator(fileName, 0)
    {}

    BufferCreator(const std::string& fileName, size_t numReservedBlocks)
        : _writer(fileName, memoryMappedFileRequestedSize, numReservedBlocks)
    {
        static_assert(memoryMappedFileRequestedSize, "Specify memoryMappedFileRequestedSize > 0");
    }

    bool Enable()
    {
        return _writer.Enable();
    }

    bool Start()
    {
        constexpr bool result = true;

        _writer.Run();

        return result;
    }

    bool Stop()
    {
        _writer.Stop();

        return !_writer.HasError();
    }

    bool IsEnabled() const
    {
        return _writer.IsEnabled();
    }

private :

    Writer<internalBufferSize> _writer;
};

template<size_t internalBufferSize, size_t W, size_t R>
class BufferUsers
{
public :

    BufferUsers() = delete;
    BufferUsers(const BufferUsers&) = delete;
    BufferUsers& operator=(const BufferUsers&) = delete;

    BufferUsers(const std::string& fileName)
    : BufferUsers(fileName, W, R, 0)
    {
    }

    BufferUsers(const std::string& fileName, size_t maxWriters, size_t maxReaders)
    : BufferUsers(fileName, maxWriters, maxReaders, 0)
    {}

    BufferUsers(const std::string& fileName, size_t maxWriters, size_t maxReaders, size_t numReservedBlocks)
    : _writers(maxWriters > W ? W : maxWriters)
    , _readers(maxReaders > R ? R : maxReaders)
    {
        for_each(_writers.begin(), _writers.end(), [&fileName, &numReservedBlocks] (std::unique_ptr<Writer<internalBufferSize>>& writer){ writer = std::move(std::unique_ptr<Writer<internalBufferSize>>(new Writer<internalBufferSize>(fileName, 0, numReservedBlocks))); });
        for_each(_readers.begin(), _readers.end(), [&fileName, &numReservedBlocks] (std::unique_ptr<Reader<internalBufferSize>>& reader){ reader = std::move(std::unique_ptr<Reader<internalBufferSize>>(new Reader<internalBufferSize>(fileName, numReservedBlocks))); });
    }

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

        for_each(_writers.begin(), _writers.end(), [] (const std::unique_ptr<Writer<internalBufferSize>>& writer){ writer->Run(); });
        for_each(_readers.begin(), _readers.end(), [] (const std::unique_ptr<Reader<internalBufferSize>>& reader){ reader->Run(); });

        return result;
    }

    bool Stop() const
    {
        bool result = true;

        for_each(_writers.begin(), _writers.end(), [&result] (const std::unique_ptr<Writer<internalBufferSize>>& writer){ writer->Stop(); result = result && !writer->HasError(); });
        for_each(_readers.begin(), _readers.end(), [&result] (const std::unique_ptr<Reader<internalBufferSize>>& reader){ reader->Stop(); result = result && !reader->HasError(); });


        return result;
    }

private :

    std::vector<std::unique_ptr<Writer<internalBufferSize>>> _writers;
    std::vector<std::unique_ptr<Reader<internalBufferSize>>> _readers;
};

template<uint32_t memoryMappedFileRequestedSize, uint32_t internalBufferSize, uint8_t maxChildren>
class Process
{
public :

    Process() = delete;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    Process(const std::string& fileName)
    : _fileName{ fileName }
    , _sync{ nullptr }
    , _childUsersSet{ maxReadersPerProcess, maxReadersPerProcess }
    , _parentUsersSet{ maxWritersPerProcess, maxReadersPerProcess }
    , _setupTime{ Core::infinite }
    , _runTime{ Core::infinite }
    , _numReservedBlocks{ 0 }
    {
        std::for_each(_children.begin(), _children.end(), [] (pid_t& child){ child = 0; } );

        // Add some randomness
        std::srand(std::time(nullptr));

        bool result = PrepareIPSync();

        ASSERT(result); DEBUG_VARIABLE(result);
    }

    ~Process()
    {
        bool result = CleanupIPSync();

        ASSERT(result); DEBUG_VARIABLE(result);
    }

    bool Execute()
    {
        return ForkAndExecute();
    }

    bool SetChildUsers(uint8_t numWriters, uint8_t numReaders)
    {
        if (numWriters <= maxWritersPerProcess) {
            _childUsersSet[0] = numWriters;
        }

        if (numReaders <= maxReadersPerProcess) {
            _childUsersSet[1] = numReaders;
        }

        return    _childUsersSet[0] == numWriters
               && _childUsersSet[1] == numReaders
               ;
    }

    bool SetParentUsers(uint8_t numWriters, uint8_t numReaders)
    {
        if (numWriters <= maxWritersPerProcess) {
            _parentUsersSet[0] = numWriters;
        }

        if (numReaders <= maxReadersPerProcess) {
            _parentUsersSet[1] = numReaders;
        }

        return    _parentUsersSet[0] == numWriters
               && _parentUsersSet[1] == numReaders
               ;
    }

    bool SetTotalRuntime(uint32_t runTime /* milliseconds */)
    {
        if (runTime <= maxTotalRuntime) {
            _runTime = runTime;
        }

        return _runTime == runTime;
    }

    bool SetAllowedSetupTime(uint32_t setupTime /* seconds */)
    {
        if (setupTime <= maxSetupTime) {
            _setupTime = setupTime;
        }

        return _setupTime == setupTime;
    }

    bool SetNumReservedBlocks(uint8_t numReservedBlocks)
    {
        if (numReservedBlocks <= maxNumReservedBlocks) {
            _numReservedBlocks = numReservedBlocks;
        }

        return _numReservedBlocks == numReservedBlocks;
    }

private :

    // Hard limits

    static constexpr uint32_t maxSetupTime = 10; // Seconds
    static constexpr uint32_t maxTotalRuntime = Core::infinite; // Milliseconds

    static constexpr uint8_t maxWritersPerProcess = 2;
    static constexpr uint8_t maxReadersPerProcess = 2;

    static constexpr uint8_t maxNumReservedBlocks = 2;

    // The order is important, sync variable
    enum status : uint8_t {
          uninitialized = 1
        , initialized = 2
        , ready = 4
    };

    bool PrepareIPSync()
    {
        if (_sync == nullptr) {
            _sync = reinterpret_cast<struct sync_wrapper*>(mmap(nullptr, sizeof(struct sync_wrapper), PROT_READ | PROT_WRITE, MAP_SHARED/*_VALIDATE*/ | MAP_ANONYMOUS, -1, 0));
        }

        bool result = _sync != nullptr;

        if (_sync != nullptr) {
            _sync->level = status::uninitialized;
        }

        return result;
    }

    bool CleanupIPSync()
    {
        bool result =    _sync != nullptr
                      && munmap(_sync, sizeof(struct sync_wrapper)) == 0
                      ;

        return result;
    }

    bool LockIPSync(const std::array<status, 2>& flags, const struct timespec& timeout) const
    {
        long retval = 0;
        int state = 0;

        do {
            if (_sync->level == flags[0]) {
                retval = syscall(SYS_futex, reinterpret_cast<uint32_t*>(_sync), FUTEX_WAIT, static_cast<uint32_t>(flags[0]), &timeout, nullptr, 0);
                state = errno;
            }
        } while ((    retval == -1
                   && (   (state & ETIMEDOUT) ==  ETIMEDOUT
                         || (state & EAGAIN)  ==  EAGAIN
                      )
                 )
                 && _sync->level != flags[1]
                );

        // ETIMEDOUT : The parent / child is not yet ready
        // EAGAIN    : The handshake between parent and (another) child has been completed as the value has already been altered

        return retval >= 0 || _sync->level != flags[0]; // 0 == woken up or parent / child already ready
    }

    bool UnlockIPSync(const std::array<status,2>& flags, long& retval)
    {
       if (   _sync->level == flags[0]
            && _sync->level != flags[1]
           ) {
            _sync->level = flags[1];
            /*long*/ retval = syscall(SYS_futex, reinterpret_cast<uint32_t*>(_sync), FUTEX_WAKE, static_cast<uint32_t>(std::numeric_limits<int>::max()) /* INT_MAX,  number of waiters to wake up */, nullptr, 0, 0);
        }

        return retval >= 0; // The parent / child might already be ready
    }

    bool ForkAndExecute()
    {
        bool result = true;

        for (auto it = _children.begin(), end = _children.end(); it != end; it++) {
            pid_t pid = fork();

            // Format specifier %ld, pid_t, gettid() and getpid()
            static_assert(sizeof(pid_t) <= sizeof(long), "Specify a more suitable formmat specifier for pid_t.");

            switch (pid) {
            case -1 :   // Error
                        {
                            result = pid < 0;

                            TRACE_L1(_T("Error: failed to create the remote process."));
                            break;
                        }
            case 0  :   // Child
                        {
                            const struct timespec timeout = {.tv_sec = _setupTime, .tv_nsec = 0};

                            std::array<status, 2> flags = {status::uninitialized, status::initialized};

                            result = LockIPSync(flags, timeout);

                            ASSERT(result);

                            // ETIMEDOUT : The parent is not yet ready
                            // EAGAIN    : The handshake between parent and (another) child has been completed as the value has already been altered

                            TRACE_L1(_T("Child %ld knows its parent is ready [true/false]: %s."), getpid(), _sync->level != status::uninitialized ? _T("true") : _T("false"));

                            //BufferUsers<internalBufferSize, maxWritersPerProcess, maxReadersPerProcess> users(_fileName, _childUsersSet[0], _childUsersSet[1]); // # writer(s), # reader(s)
                            BufferUsers<internalBufferSize, maxWritersPerProcess, maxReadersPerProcess> users(_fileName, _childUsersSet[0], _childUsersSet[1], _numReservedBlocks); // # writer(s), # reader(s)

                            flags = {status::initialized, status::ready};

                            long retval = 0;

                            result = UnlockIPSync(flags, retval);

                            ASSERT(result);

                            TRACE_L1(_T("Child %ld has woken up %ld parent(s)."), getpid(), retval);

                            result =    CleanupIPSync()
                                     && Core::File(_fileName).Exists()
                                     && users.Enable()
                                     && users.Start()
                                     ;

                            if (result) {
                                SleepMs(_runTime);

                                result = users.Stop();
                            } else {
                                TRACE_L1(_T("Error: Unable to access the CyclicBuffer."));
                            }

                            return result;
                        }
            default :   // Parent
                        {
                            // Keep track of conceived children
                            *it = pid;

                            result =    pid > 0
                                     && result
                                     ;
                        }
            }
        }

        return    result
               && ExecuteParent()
               ;
    }

    bool ExecuteParent()
    {
        // The underlying memory mapped file will be created and opened via DataElementFile construction
        std::unique_ptr<BufferCreator<memoryMappedFileRequestedSize, internalBufferSize>> creator { std::move(std::unique_ptr<BufferCreator<memoryMappedFileRequestedSize, internalBufferSize>>(new BufferCreator<memoryMappedFileRequestedSize, internalBufferSize>(_fileName, _numReservedBlocks))) };

        bool result =    creator.get() != nullptr
                         // Immunize, signal to use for unresponsive child processes, default action is terminate
                      && signal(SIGUSR1, SIG_IGN) != SIG_ERR
                      && creator->Enable()
                      && creator->IsEnabled()
#ifdef CREATOR_WRITE
                      && creator->Start()
#endif
                      ;

        if (result) {
            TRACE_L1(_T("'creator' and shared cyclic buffer ready."));

#ifdef CREATOR_EXTRA_USERS
            BufferUsers<internalBufferSize, maxWritersPerProcess, maxReadersPerProcess> users(_fileName, _parentUsersSet[0], _parentUsersSet[1]); // # writer(s), # reader(s)

            result =    users.Enable()
                     && users.Start()
                     ;

            if (result) {
#endif
                _sync->level = status::uninitialized;

                std::array<status, 2> flags = {status::uninitialized, status::initialized};

                long retval = 0;

                result = UnlockIPSync(flags, retval);

                ASSERT(result);

                TRACE_L1(_T("Parent %ld has woken up %ld child(ren)."), gettid(), retval);

                const struct timespec timeout = {.tv_sec = _setupTime, .tv_nsec = 0};

                flags = {status::initialized, status::ready};

                bool result = LockIPSync(flags, timeout);

                ASSERT(result);

                TRACE_L1(_T("Parent %ld has been woken up by child [true/false]: %s."), gettid(), retval == 0 ? _T("true") : _T("false"));

                result =    CleanupIPSync()
                         && WaitForCompletion(/*timeout for waitpid*/);

#ifdef CREATOR_EXTRA_USERS
            result =    users.Stop()
                     && result
                     ;
            } else {
                TRACE_L1(_T("Error: Unable to start 'extra users'."));
            }
#endif

#ifdef CREATOR_WRITE
            result =    creator->Stop()
                     && result
                     ;
#endif

        } else {
            TRACE_L1(_T("Error: 'creator' and shared cyclic buffer unavailable."));
        }

        return result;
    }

    bool WaitForCompletion(/*timeout*/)
    {
        bool result = true;

        // Reap
        for (auto it = _children.begin(), end = _children.end(); it != end; it++) {
            int status = 0;

//            pid_t pid = waitpid(-1 /* wait for child */, &status, 0); // Wait out of order
            pid_t pid = waitpid(*it /* wait for child */, &status, 0); // Wait in order

            switch (pid) {
            case -1 :   // No more children / child has died
                        if (errno == ECHILD) {
                            TRACE_L1(_T("Child %ld is not our offspring."), pid);
                            result = false;
                        }
                        break;
            case 0  :   // Child has not changed state, see WNOHANG
                        result = false;
                        break;
            default :   // Child's pid
                        if (WIFEXITED(status)) {
                            TRACE_L1(_T("Child %ld died NORMALLY with status %ld."), pid, WEXITSTATUS(status));
                            result =    result
                                     && true;
                        } else {
                            result = false;

                            if (WIFSIGNALED(status)) {
                                TRACE_L1(_T("Child %ld died ABNORMALLY due to signal %ld."), pid, WTERMSIG(status));
                            } else if (WIFSTOPPED(status)) {
                                TRACE_L1(_T("Child %ld died ABNORMALLY due to stop signal %ld."), pid, WSTOPSIG(status));
                            } else if (errno != EAGAIN) {// pid non-blocking
                                TRACE_L1(_T("Child %ld died ABNORMALLY."), pid);
                            } else {
                                TRACE_L1(_T("Error: unprocessed child %ld status."), pid);
                            }
                        }
            }
        }

        return result;
    }

    const std::string _fileName;

    struct sync_wrapper {
        status level;
    }* _sync;

    std::array<pid_t, maxChildren> _children;

    std::array<uint8_t, 2> _childUsersSet;
    std::array<uint8_t, 2> _parentUsersSet;

    uint32_t _runTime; // Milliseconds
    uint32_t _setupTime; // Seconds

    uint8_t _numReservedBlocks;
};

} // Tests
} // WPEFramework


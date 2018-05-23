#include "CyclicBuffer.h"
#include "ProcessInfo.h"

namespace WPEFramework {
namespace Core {

    CyclicBuffer::CyclicBuffer(const string& fileName, const uint32_t bufferSize, const bool overwrite)
        : _buffer(
              fileName,
              static_cast<DataElementFile::FileState>(DataElementFile::WRITABLE | DataElementFile::READABLE | DataElementFile::SHAREABLE | (bufferSize > 0 ? DataElementFile::CREATE : 0)),
              (bufferSize == 0 ? 0 : (bufferSize + sizeof(const control))))
        , _realBuffer(&(_buffer.Buffer()[sizeof(struct control)]))
        , _alert(false)
        , _administration(_buffer.IsValid() ? reinterpret_cast<struct control*>(_buffer.Buffer()) : nullptr)
    {


    	 if (_buffer.IsValid() == true) {
#ifdef __WIN32__
            string strippedName(Core::File::PathName(fileName) + Core::File::FileName(fileName));
            _mutex = CreateSemaphore(nullptr, 1, 1, (strippedName + ".mutex").c_str());
            _signal = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, (strippedName + ".signal").c_str());
            _event = CreateEvent(nullptr, FALSE, FALSE, (strippedName + ".event").c_str());
#else

#endif
            if (bufferSize != 0) {

#ifndef __WIN32__
                (_administration)->_signal = PTHREAD_COND_INITIALIZER;
                (_administration)->_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

                _administration->_head.store(0);
                _administration->_tail.store(0);
                _administration->_agents.store(0);
                _administration->_state.store(state::UNLOCKED /* state::EMPTY */ | (overwrite ? state::OVERWRITE : 0));
                _administration->_lockPID = 0;
                _administration->_size = bufferSize;

                _administration->_reserved = 0;
                _administration->_reservedWritten = 0;
                _administration->_reservedPID = 0;

            	_administration->_tailIndexMask = 1;
            	_administration->_roundCountModulo = 1L << 31;
            	while (_administration->_tailIndexMask < bufferSize) {
                    _administration->_tailIndexMask = (_administration->_tailIndexMask << 1) + 1;
            		_administration->_roundCountModulo = _administration->_roundCountModulo >> 1;
            	}
            }

            _maxSize = _administration->_size;
        }
    }

    CyclicBuffer::~CyclicBuffer()
    {
    }

    void CyclicBuffer::AdminLock()
    {
#ifdef __POSIX__
        pthread_mutex_lock(&(_administration->_mutex));
#else
#ifdef __DEBUG__
        if (::WaitForSingleObjectEx(_mutex, 2000, FALSE) != WAIT_OBJECT_0) {
            // Seems we did not get the lock within 2S :-(
            ASSERT(false);
        }
#else
        ::WaitForSingleObjectEx(_mutex, INFINITE, FALSE);
#endif
#endif
    }

    // This is in MS...
    uint32_t CyclicBuffer::SignalLock(const uint32_t waitTime)
    {

        uint32_t result = waitTime;

        if (waitTime != Core::infinite) {
#ifdef __POSIX__
            struct timespec structTime;

            clock_gettime(CLOCK_REALTIME, &structTime);

            structTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
            structTime.tv_sec += (waitTime / 1000); // + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
            structTime.tv_nsec = structTime.tv_nsec % 1000000000;

            if (pthread_cond_timedwait(&(_administration->_signal), &(_administration->_mutex), &structTime) != 0) {
                struct timespec nowTime;

                clock_gettime(CLOCK_REALTIME, &nowTime);
                if (nowTime.tv_nsec > structTime.tv_nsec) {

                    result = (nowTime.tv_sec - structTime.tv_sec) * 1000 + ((nowTime.tv_nsec - structTime.tv_nsec) / 1000000);
                }
                else {

                    result = (nowTime.tv_sec - structTime.tv_sec - 1) * 1000 + ((1000000000 - (structTime.tv_nsec - nowTime.tv_nsec)) / 1000000);
                }
                printf("End wait. %d\n", result);
            }
#else
            if (::WaitForSingleObjectEx(_signal, waitTime, FALSE) == WAIT_OBJECT_0) {

                // Calculate the time we used, and subtract it from the waitTime.
                result = 100;
            }
#endif

            // We can not wait longer than the set time.
            ASSERT(result <= waitTime);
        }
        else {
#ifdef __POSIX__
            pthread_cond_wait(&(_administration->_signal), &(_administration->_mutex));
#else
            ::WaitForSingleObjectEx(_signal, INFINITE, FALSE);
#endif
        }
        return (result);
    }

    void CyclicBuffer::AdminUnlock()
    {
#ifdef __POSIX__
        pthread_mutex_unlock(&(_administration->_mutex));
#else
        ReleaseSemaphore(_mutex, 1, nullptr);
#endif
    }
	/* virtual */ void CyclicBuffer::DataAvailable()
	{
	}

    void CyclicBuffer::Reevaluate()
    {

        // See if we need to have some interested actor reevaluate its state..
        if (_administration->_agents.load() > 0) {

#ifdef __POSIX__
            for (int index = _administration->_agents.load(); index != 0; index--) {
                pthread_cond_signal(&(_administration->_signal));
            }
#else
            ReleaseSemaphore(_signal, _administration->_agents.load(), nullptr);
#endif

            // Wait till all waiters have seen the trigger..
            while (_administration->_agents.load() > 0) {
                SleepMs(0);
            }
        }
    }

    inline void CyclicBuffer::Alert()
    {

        // Lock the administrator..
        AdminLock();

        _alert = true;

        Reevaluate();

        AdminUnlock();
    }

    uint32_t CyclicBuffer::Read(uint8_t buffer[], const uint32_t length)
    {
        ASSERT(length <= _maxSize);

        bool foundData = false;

        uint32_t result;
        while (!foundData) {
            uint32_t oldTail = _administration->_tail;
            uint32_t head = _administration->_head;
            uint32_t offset = oldTail & _administration->_tailIndexMask;

            result = Used(head, offset);
            if (result == 0) {
            	// No data, just return 0.
            	return 0;
            }

            Cursor cursor(*this, oldTail, length);
            result = GetReadSize(cursor);

            if ((result == 0) || (result > length)) {
            	// No data, or too much, return 0.
            	return 0;
            }

            foundData = true;

            uint32_t roundCount = oldTail / (1 + _administration->_tailIndexMask);
            if ((offset + result) < _maxSize) {
                memcpy(buffer, _realBuffer + offset, result);

                uint32_t newTail = offset + result + roundCount * (1 + _administration->_tailIndexMask);
                if(!std::atomic_compare_exchange_weak(&(_administration->_tail), &oldTail, newTail)) {
                	foundData = false;
                }
            } else {
                uint32_t part1(_maxSize - offset);
                uint32_t part2(result - part1);

                memcpy(buffer, _realBuffer + offset, part1);
                memcpy(buffer + part1, _realBuffer, part2);

                // Add one round, but prevent overflow.
                roundCount = (roundCount + 1) % _administration->_roundCountModulo;
                uint32_t newTail = part2 + roundCount * (1 + _administration->_tailIndexMask);
                if (!std::atomic_compare_exchange_weak(&(_administration->_tail), &oldTail, newTail)) {
                	foundData = false;
                }
            }
        }

        return (result);
    }

    uint32_t CyclicBuffer::Write(const uint8_t buffer[], const uint32_t length)
    {
    	ASSERT(length < _maxSize);

    	uint32_t head = _administration->_head;
    	bool startingEmpty = (Used() == 0);
    	uint32_t writeStart = head;
    	bool shouldMoveHead = true;

    	if (_administration->_reservedPID != 0) {
#ifdef __WIN32__
    		// We are writing because of reservation.
    		ASSERT(_administration->_reservedPID == ::GetCurrentProcessId());
#else
			// We are writing because of reservation.
			ASSERT(_administration->_reservedPID == ::getpid());
#endif

    		// Check if we are not writing more than reserved.
    		uint32_t newReservedWritten = _administration->_reservedWritten + length;
    		ASSERT(newReservedWritten <= _administration->_reserved);

    		// Set up everything for actual write operation.
    		writeStart = (head + _administration->_reservedWritten) % _maxSize;

    		// Update amount written.
    		_administration->_reservedWritten = newReservedWritten;

    		if (newReservedWritten == _administration->_reserved) {
    			// We have all the data that was reserved for, clear reserving PID.
    			_administration->_reservedPID = 0;
    		} else {
    			// Not yet all data, hold off with moving head.
    			shouldMoveHead = false;
    		}
    	} else {
    		// A write without reservation, make sure we have the space.
    		AssureFreeSpace(length);

    		// Just start writing after head.
    		writeStart = head;
    	}

    	// Perform actual copy.
    	uint32_t writeEnd = (writeStart + length) % _maxSize;
    	if (writeEnd > writeStart) {
    		// Easy case: one pass.
            memcpy(_realBuffer + writeStart, buffer, length);
    	} else {
    		// Copying beyond end of buffer, do in two passes.
			uint32_t firstLength = _maxSize - writeStart;
			uint32_t secondLength = length - firstLength;

			memcpy(_realBuffer + writeStart, buffer, firstLength);
			memcpy(_realBuffer, buffer + firstLength, secondLength);
    	}

    	if (shouldMoveHead) {
    		_administration->_head = writeEnd;

    		if (startingEmpty) {
    			// Was empty before, tell observers about new data.
                AdminLock();

                Reevaluate();
                DataAvailable();

                AdminUnlock();
            }
    	}

    	return length;
    }

    void CyclicBuffer::AssureFreeSpace(uint32_t required)
    {
    	uint32_t oldTail = _administration->_tail;
    	uint32_t head = _administration->_head;

    	uint32_t tail = oldTail & _administration->_tailIndexMask;
    	uint32_t free = Free(head, tail);

    	while (free < required) {
    	    uint32_t remaining = required - free;
    		Cursor cursor(*this, oldTail, remaining);
        	uint32_t offset = GetOverwriteSize(cursor);
        	ASSERT((offset + free) >= required);

    		uint32_t newTail = cursor.GetCompleteTail(offset);

    		if(!std::atomic_compare_exchange_weak(&(_administration->_tail), &oldTail, newTail)) {
    			tail = oldTail & _administration->_tailIndexMask;
    			free = Free(head, tail);
    		} else {
    			ASSERT(Free(head, newTail & _administration->_tailIndexMask) >= required);
    		}
    	}

    	ASSERT(Free() >= required);
    }

    uint32_t CyclicBuffer::Reserve(const uint32_t length)
    {
#ifdef __WIN32__
    	DWORD processId = GetCurrentProcessId();
    	DWORD expectedProcessId = static_cast<DWORD>(0);
#else
		pid_t processId = ::getpid();
		pid_t expectedProcessId = static_cast<pid_t>(0);
#endif

    	bool noOtherReservation = atomic_compare_exchange_strong(&(_administration->_reservedPID), &expectedProcessId, processId);
    	ASSERT(noOtherReservation);

    	if (!noOtherReservation)
    		return Core::ERROR_ILLEGAL_STATE;

        uint32_t actualLength = length;
        if (length >= _maxSize) {
            // Maximum write size is _maxSize-1, to differentiate from empty situation.
            actualLength = _maxSize - 1;
        }

        AssureFreeSpace(actualLength);
        ASSERT(actualLength <= Free());

        _administration->_reserved = actualLength;
        _administration->_reservedWritten = 0;

        return actualLength;
    }

    uint32_t CyclicBuffer::Lock(const bool dataPresent, const uint32_t waitTime)
    {
        uint32_t result = Core::ERROR_TIMEDOUT;
        uint32_t timeLeft = waitTime;

        // Lock can not be called recursive, unlock if you would like to lock it..
        ASSERT(_administration->_lockPID == 0);

        // Lock the administrator..
        AdminLock();

        do {

            if ((((_administration->_state.load()) & state::LOCKED) != state::LOCKED) && ((dataPresent == false) || (Used() > 0))) {
                std::atomic_fetch_or(&(_administration->_state), static_cast<uint16_t>(state::LOCKED));

                // Remember that we, as a process, took the lock
                _administration->_lockPID = Core::ProcessInfo().Id();
                result = Core::ERROR_NONE;
            }
            else if (timeLeft > 0) {

                _administration->_agents++;

                AdminUnlock();

                timeLeft = SignalLock(timeLeft);

                _administration->_agents--;

                AdminLock();

                if (_alert == true) {
                    _alert = false;
                    result = Core::ERROR_ASYNC_ABORTED;
                }
            }

        } while ((timeLeft > 0) && (result == Core::ERROR_TIMEDOUT));

        AdminUnlock();

        return (result);
    }

    uint32_t CyclicBuffer::Unlock()
    {

        uint32_t result(Core::ERROR_ILLEGAL_STATE);

        // Lock the administrator..
        AdminLock();

        // Lock can not be called recursive, unlock if you would like to lock it..
        ASSERT(_administration->_lockPID == Core::ProcessInfo().Id());
        ASSERT((_administration->_state.load() & state::LOCKED) == state::LOCKED);

        // Only unlock if it is "our" lock.
        if (_administration->_lockPID == Core::ProcessInfo().Id()) {

            _administration->_lockPID = 0;
            std::atomic_fetch_and(&(_administration->_state), static_cast<uint16_t>(~state::LOCKED));

            Reevaluate();

            result = Core::ERROR_NONE;
        }

        AdminUnlock();

        return (result);
    }

    uint32_t CyclicBuffer::Peek(uint8_t buffer[], const uint32_t length) const
    {
        ASSERT(length <= _maxSize);

        bool foundData = false;

        uint32_t result = 0;
        while (!foundData) {
            uint32_t oldTail = _administration->_tail;
            uint32_t tail = oldTail & _administration->_tailIndexMask;
            result = Used(_administration->_head, oldTail);

            if (result == 0) {
            	// No data.
            	return 0;
            }

            // Clip to the requested length to read if required, and not more.
            if (result > length) {
                result = length;
            }

            uint32_t readEnd = tail + result;

            if (readEnd < _maxSize) {
            	// Can be done in one pass.
            	memcpy(buffer, _realBuffer + tail, result);
            } else {
            	// Needs to be done in two passes.
    			uint32_t firstLength = _maxSize - tail;
    			uint32_t secondLength = length - firstLength;

    			memcpy(buffer, _realBuffer + tail, firstLength);
    			memcpy(buffer + firstLength, _realBuffer, secondLength);
            }

            // We found valid data if the tail is still where it was when we started.
            foundData = (_administration->_tail == oldTail);
        }

        return (result);
    }

	/* virtual */ uint32_t CyclicBuffer::GetOverwriteSize(Cursor& cursor)
	{
		// Easy case: just return requested bytes.
		return cursor.Size();
	}

	/* virtual */ uint32_t CyclicBuffer::GetReadSize(Cursor& cursor)
	{
		// Easy case: just return requested bytes.
		return cursor.Size();
	}
}
} // WPEFramework::Core

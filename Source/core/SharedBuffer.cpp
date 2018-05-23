#include "SharedBuffer.h"

// TODO: remove if no longer needed for simple tracing.
#include <iostream>

namespace WPEFramework {

namespace Core {

#ifdef __WIN32__
		SharedBuffer::Semaphore::Semaphore(const TCHAR sourceName[]) {
			_semaphore = ::CreateSemaphore(nullptr, 1, 1, sourceName);
                }
#else
		SharedBuffer::Semaphore::Semaphore(sem_t* storage)
		    : _semaphore(storage)
		{
		}
#endif
		SharedBuffer::Semaphore::~Semaphore() {
#ifdef __WIN32__
			if (_semaphore != nullptr)
			{
				::CloseHandle(_semaphore);
			}
#else
			sem_destroy(_semaphore);
#endif
		}

		uint32_t SharedBuffer::Semaphore::Unlock() {
#ifdef __WIN32__
			if (_semaphore != nullptr)
			{
				BOOL result = ::ReleaseSemaphore(_semphore, 1, nullptr);

				ASSERT(result != FALSE);
			}
#else
			VARIABLE_IS_NOT_USED int result = sem_post(_semaphore);

			ASSERT((result == 0) || (errno == EOVERFLOW));
			
			return ERROR_NONE;
#endif
		}

		bool SharedBuffer::Semaphore::IsLocked() {
		    int semValue = 0;
		    sem_getvalue(_semaphore, &semValue);
		    return (semValue == 0);
		}

		uint32_t SharedBuffer::Semaphore::Lock(const uint32_t waitTime) {
			uint32_t result = Core::ERROR_GENERAL;
#ifdef __WIN32__
            		if (_sempahore != nullptr) {
            		    return (::WaitForSingleObjectEx(_semaphore, waitTime, FALSE) == WAIT_OBJECT_0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT);
			}
#elif defined(__APPLE__)

                	uint32_t timeLeft = waitTime;
                	int semResult;
                	while(((semResult = sem_trywait(_semaphore)) != 0) && timeLeft > 0) {
                	    ::SleepMs(100);
                	    if (timeLeft != Core::infinite) {
                	        timeLeft -= (timeLeft > 100 ? 100 : timeLeft);
                	    }
                	}
                	result = semResult == 0 ? Core::ERROR_NONE : Core::ERROR_TIMEDOUT;
#else

               		 struct timespec structTime;

                	clock_gettime(CLOCK_REALTIME, &structTime);
                	structTime.tv_nsec += ((waitTime % 1000) * 1000 * 1000); /* remainder, milliseconds to nanoseconds */
                	structTime.tv_sec += (waitTime / 1000) + (structTime.tv_nsec / 1000000000); /* milliseconds to seconds */
                	structTime.tv_nsec = structTime.tv_nsec % 1000000000;

                	if (sem_timedwait(_semaphore, &structTime) == 0) {
                	    result = Core::ERROR_NONE;
                	}
                	else if ( (errno == EINTR) || (errno == ETIMEDOUT) ) {
                	    result = Core::ERROR_TIMEDOUT;
                	}
                	else {
               		    ASSERT(false);
                }
#endif
                return (result);
            }

    SharedBuffer::SharedBuffer(const TCHAR name[]) 
        : DataElementFile(name, READABLE|WRITABLE|SHAREABLE )
        , _administrationBuffer((string(name) + ".admin"), READABLE|WRITABLE|SHAREABLE )
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
#ifdef __WIN32__
        , _producer((string(name) + ".producer")
        , _consumer((string(name) + ".consumer")
#else
        , _producer(&(_administration->_producer))
        , _consumer(&(_administration->_consumer))
#endif
	, _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)]))) {
    }	
    SharedBuffer::SharedBuffer(const TCHAR name[], const uint32_t bufferSize, const uint16_t administratorSize) 
        : DataElementFile(name, READABLE|WRITABLE|SHAREABLE|CREATE, bufferSize)
        , _administrationBuffer((string(name) + ".admin"), READABLE|WRITABLE|SHAREABLE|CREATE, administratorSize + sizeof(Administration) + (2 * sizeof(void*)))
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
#ifdef __WIN32__
        , _producer((string(name) + ".producer")
        , _consumer((string(name) + ".consumer")
#else
        , _producer(&(_administration->_producer))
        , _consumer(&(_administration->_consumer))
#endif
	, _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)]))) {

#ifndef __WIN32__
        memset(_administration, 0, sizeof(Administration));

	sem_init (&(_administration->_producer), 1, 1); /* Initial value is 1. */
	sem_init (&(_administration->_consumer), 1, 0); /* Initial value is 0. */
#endif
    }

    SharedBuffer::~SharedBuffer() {
    }
    
}
}

    	

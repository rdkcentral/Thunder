#ifndef __SHARED_BUFFER_H
#define __SHARED_BUFFER_H

#include <memory> // align

// ---- Include local include files ----
#include "Module.h"
#include "DataElementFile.h"

#ifndef __WIN32__
#include <semaphore.h>
#endif

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

namespace WPEFramework {

namespace Core {
    // Rationale:
    // This class allows to share data over process boundaries. This is a simple version of the
    // CyclicBuffer. 
    // The rationale behind this buffer is to share buffer space (SharedMemory file) between two 
    // porocesses. One process produces data, the othere process consumes it. The signalling
    // between the two processes is based on a semaphore (binairy semaphore) The Producer creates
    // the SharedBuffer object, indicting it has the Producer role. It will automatically own 
    // the producer lock. If the producer has placed the data in the buffer and would like the 
    // consumer to handle it, it signals this by releasing/unlocking the semaphore.
    // The producer has to call the constructor with the administrationSize parameter, and specify an
    // administration area.
    // The consumer, who signals it role during construction, can, if the system is operational,
    // Wait for the Producer lock. Once it holds the producer lock, it can modify/use the SharedBuffer.
    // If the Consumer is finished with the shared buffer, it should signal this by releasing the
    // shared lock again.
    // The consumer construct should be done with
    // TODO use resize from base class
    class EXTERNAL SharedBuffer : public DataElementFile {
    private:
        SharedBuffer() = delete;
        SharedBuffer(const SharedBuffer&) = delete;
        SharedBuffer& operator=(const SharedBuffer&) = delete;

    private:
        class Semaphore {
        private:
            Semaphore() = delete;
            Semaphore(const Semaphore&) = delete;
            Semaphore& operator= (const Semaphore&) = delete;

        public:
#ifdef __WIN32__
            Semaphore(const TCHAR name[]) {
            }
#else
            Semaphore(sem_t* storage);
            //Semaphore(sem_t* storage, bool initialize) {
            //}
#endif
            ~Semaphore();
            //~Semaphore() {
            //}

        public:
            uint32_t Lock(const uint32_t waitTime);
            uint32_t Unlock();
            bool IsLocked();

        private:
#ifdef __WIN32__
            HANDLE _semaphore;
#else
            sem_t* _semaphore;
#endif
        };

        struct Administration {

            uint32_t _bytesWritten;

#ifndef __WIN32__
            sem_t _producer;
            sem_t _consumer;
#endif
        };


    public:
	virtual ~SharedBuffer();

        // This is the consumer constructor. It should always take place, after, the producer
        // construct. The producer will create the Administration area, and the shared buffer,
        // default size.
        SharedBuffer (const TCHAR name[]);

        // This is the Producer constructor. It sets up all the information and locks the 
        // Semaphore by default.
        SharedBuffer (const TCHAR name[], const uint32_t bufferSize, const uint16_t administrationSize);

        inline uint32_t RequestProduce(const uint32_t waitTime) {
            // fprintf(stderr, "Entering RequestProduce: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            uint32_t output = _producer.Lock(waitTime);

            // fprintf(stderr, "Leaving RequestProduce: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            return output;
        }
        inline uint32_t Produced() {
            // fprintf(stderr, "Entering Produced: %d %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked(), static_cast<const uint32_t>(Size()));

            _administration->_bytesWritten = static_cast<const uint32_t>(Size()); 

            uint32_t output = _consumer.Unlock();

            // fprintf(stderr, "Leaving Produced: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            return output;
        }

        uint32_t RequestConsume(const uint32_t waitTime) {
            // fprintf(stderr, "Entering RequestConsumer: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            uint32_t result = _consumer.Lock(waitTime);

            // fprintf(stderr, "Leaving RequestConsumer: %d %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked(), (int)_administration->_bytesWritten);

            // Realign the buffer with what has been written
            Size(_administration->_bytesWritten);

            return result;
        }

        uint32_t Consumed() {
            // fprintf(stderr, "Entering Consumed: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            _administration->_bytesWritten = static_cast<const uint32_t>(Size()); 

            // _administration->_bytesWritten = Size(); 
            uint32_t output = _producer.Unlock();

            // fprintf(stderr, "Leaving Consumed: %d %d\n", (int)_producer.IsLocked(), (int)_consumer.IsLocked());

            return output;
        }

        uint8_t* AdministrationBuffer() {
            return _customerAdministration;
        }

        const uint8_t* AdministrationBuffer() const {
            return _customerAdministration;
        }

        uint32_t BytesWritten() const {
            return(_administration->_bytesWritten);
        }

    private:
        DataElementFile  _administrationBuffer;
        Administration* _administration;
        Semaphore _producer;
        Semaphore _consumer;
        uint8_t* _customerAdministration;
    };
}
} // namespace WPEFramework::Core

#endif // __SHARED_BUFFER_H


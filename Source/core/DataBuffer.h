#ifndef __DATABUFFER_H
#define __DATABUFFER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"
#include "Proxy.h"

namespace WPEFramework {
namespace Core {
    // ---- Referenced classes and types ----

    // ---- Helper types and constants ----

    // ---- Helper functions ----
    template <typename BUFFER>
    class CyclicDataBuffer : public BUFFER {
    private:
        // ----------------------------------------------------------------
        // Never, ever allow cyclicDataBuffer objects to be copied/assigned
        // ----------------------------------------------------------------
        CyclicDataBuffer(const CyclicDataBuffer<BUFFER>& a_Copy);
        CyclicDataBuffer<BUFFER>& operator=(const CyclicDataBuffer<BUFFER>& a_RHS);

    public:
        CyclicDataBuffer()
            : BUFFER()
            , _head(0)
            , _tail(0)
            , _size(static_cast<uint32_t>(BUFFER::Size()))
            , _buffer(BUFFER::Buffer())
        {
        }
        virtual ~CyclicDataBuffer()
        {
        }

    public:
        inline uint32_t Filled() const
        {
            return ((_head >= _tail) ? (_head - _tail) : _size - (_tail - _head));
        }
        inline uint32_t Free() const
        {
            return ((_head >= _tail) ? (_size - (_head - _tail)) : (_tail - _head));
        }
        inline bool IsEmpty() const
        {
            return (_head == _tail);
        }
        uint16_t Read(uint8_t* dataFrame, const uint16_t maxSendSize) const
        {
            uint16_t result = 0;

            if (_head != _tail) {
                if (_tail > _head) {
                    result = ((_size - _tail) < maxSendSize ? (_size - _tail) : maxSendSize);
                    ::memcpy(dataFrame, &(_buffer[_tail]), result);
                    _tail = (_size == (_tail + result) ? 0 : (_tail + result));
                }

                if ((_tail < _head) && (result < maxSendSize)) {
                    uint16_t copySize = (static_cast<uint16_t>(_head - _tail) < static_cast<uint16_t>(maxSendSize - result) ? (_head - _tail) : (maxSendSize - result));
                    ::memcpy(&dataFrame[result], &(_buffer[_tail]), copySize);
                    _tail += copySize;
                    result += copySize;
                }
            }

            return (result);
        }
        uint16_t Write(const uint8_t* dataFrame, const uint16_t receivedSize)
        {
            uint32_t freeBuffer = Free();
            uint32_t result = ((receivedSize + _head) > _size ? (_size - _head) : receivedSize);

            ::memcpy(&(_buffer[_head]), dataFrame, result);
            _head = ((_head + result) < _size ? (_head + result) : 0);

            while (result < receivedSize) {
                // we continue at the beginning.
                uint32_t copySize = ((receivedSize - result) > static_cast<uint16_t>(_size) ? _size : (receivedSize - result));

                ::memcpy(_buffer, &(dataFrame[result]), copySize);
                _head = ((_head + copySize) < _size ? (_head + copySize) : 0);
                result += copySize;
            }

            if (freeBuffer < receivedSize) {
                // We have an override, adapt the tail.
                _tail = ((_head + 1) < static_cast<uint16_t>(_size) ? (_head + 1) : 0);
            }

            return (result);
        }

    private:
        uint32_t _head;
        mutable uint32_t _tail;
        const uint32_t _size;
        uint8_t* _buffer;
    };
}

} // namespace Core

#endif // __DATABUFFER_H

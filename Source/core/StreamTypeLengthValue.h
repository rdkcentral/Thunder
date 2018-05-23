#ifndef __STREAMTYPELENGTHVALUES_H
#define __STREAMTYPELENGTHVALUES_H

#include "Portability.h"
#include "Module.h"
#include "Number.h"

namespace WPEFramework {
namespace Core {
    template <typename TYPE, typename LENGTH = TYPE>
    class TypeLengthValueType {
    private:
        TypeLengthValueType();
        TypeLengthValueType(const TypeLengthValueType<TYPE, LENGTH>& copy);
        TypeLengthValueType<TYPE, LENGTH>& operator=(const TypeLengthValueType<TYPE, LENGTH>& copy);

    public:
        inline TypeLengthValueType(const TYPE& type, const LENGTH& length, const uint8_t* value)
            : _type(type)
            , _length(length)
            , _value(value)
        {
            ASSERT((value != nullptr) || (length == 0));
        }
        inline ~TypeLengthValueType()
        {
        }

    public:
        inline const TYPE& Type() const
        {
            return (_type);
        }
        inline const LENGTH& Length() const
        {
            return (_length);
        }
        inline const uint8_t* Value() const
        {
            return (_value);
        }

    private:
        TYPE _type;
        LENGTH _length;
        const uint8_t* _value;
    };

    template <typename SOURCE, typename TYPE, typename LENGTH, const uint32_t DEFAULTVALUESIZE>
    class StreamTypeLengthValueType {
    private:
        class Handler : public SOURCE {
        private:
            Handler();
            Handler(const Handler&);
            Handler& operator=(const Handler&);

        public:
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent)
                : SOURCE()
                , _parent(parent)
            {
            }
            template <typename Arg1>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1)
                : SOURCE(arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1, Arg2 arg2)
                : SOURCE(arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : SOURCE(arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : SOURCE(arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : SOURCE(arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            Handler(StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : SOURCE(arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
            {
            }
            ~Handler()
            {
            }

        protected:
            virtual void StateChange()
            {
                _parent.StateChange();
            }
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& _parent;
        };

        StreamTypeLengthValueType(const StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>&);
        StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>& operator=(const StreamTypeLengthValueType<SOURCE, TYPE, LENGTH, DEFAULTVALUESIZE>&);

    public:
		#ifdef __WIN32__ 
		#pragma warning( disable : 4355 )
		#endif
        StreamTypeLengthValueType()
            : _channel(*this)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1>
        StreamTypeLengthValueType(Arg1 arg1)
            : _channel(*this, arg1)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2>
        StreamTypeLengthValueType(Arg1 arg1, Arg2 arg2)
            : _channel(*this, arg1, arg2)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        StreamTypeLengthValueType(Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, arg1, arg2, arg3)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        StreamTypeLengthValueType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, arg1, arg2, arg3, arg4)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        StreamTypeLengthValueType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        StreamTypeLengthValueType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
            , _sendMaxSize(DEFAULTVALUESIZE)
            , _sendBuffer(new uint8_t[_sendMaxSize])
            , _sendSize(0)
            , _receiveMaxSize(DEFAULTVALUESIZE)
            , _receiveBuffer(new uint8_t[_receiveMaxSize])
            , _receiveSize(0)
            , _adminLock()
        {
        }
		#ifdef __WIN32__ 
		#pragma warning( default : 4355 )
		#endif
        virtual ~StreamTypeLengthValueType()
        {
            delete[] _sendBuffer;
            delete[] _receiveBuffer;
        }

    public:
        inline SOURCE& Link()
        {
            return (_channel);
        }
        inline string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline uint32_t Open(uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline uint32_t Flush()
        {
            _adminLock.Lock();

            _channel.Flush();
            _sendSize = 0;
            _receiveSize = 0;

            _adminLock.Unlock();

            return (OK);
        }

        // Send a TLV, make it blocking and wait maximumly till the given time until it should be send.
        void Submit(const TypeLengthValueType<TYPE, LENGTH>& element)
        {
            _adminLock.Lock();

            // Prepare the buffer to hold the TLV to send..
            if ((_sendSize + element.Length() + sizeof(TYPE) + sizeof(LENGTH)) > _sendMaxSize) {
                _sendMaxSize = ((_sendSize + element.Length() + sizeof(TYPE) + sizeof(LENGTH) + DEFAULTVALUESIZE) / DEFAULTVALUESIZE) * DEFAULTVALUESIZE;
                uint8_t* newBuffer = new uint8_t[_sendMaxSize];
                ::memcpy(newBuffer, _sendBuffer, _sendSize);
                delete[] _sendBuffer;
                _sendBuffer = newBuffer;
            }

            TYPE orderedType = NumberType<TYPE>::ToNetwork(element.Type());
            LENGTH orderedLength = NumberType<LENGTH>::ToNetwork(element.Length());

            // Copy the data into the send buffer
            ::memcpy(&(_sendBuffer[_sendSize]), &orderedType, sizeof(TYPE));
            ::memcpy(&(_sendBuffer[_sendSize + sizeof(TYPE)]), &orderedLength, sizeof(LENGTH));

            if (element.Length() > 0) {
                ::memcpy(&(_sendBuffer[_sendSize + sizeof(TYPE) + sizeof(LENGTH)]), element.Value(), element.Length());
            }

            _sendSize += sizeof(TYPE) + sizeof(LENGTH) + element.Length();

            if (_sendSize == (sizeof(TYPE) + sizeof(LENGTH) + element.Length())) {
                _sendType = element.Type();
                _sendLength = element.Length();
                _sendOffset = 0;

                _adminLock.Unlock();

                // There is no data, so we will trigger a new transfer.
                _channel.Trigger();
            }
            else {
                // do not forget to unlock.
                _adminLock.Unlock();
            }
        }

        virtual void StateChange() = 0;
        virtual void Send(const TypeLengthValueType<TYPE, LENGTH>& element) = 0;
        virtual void Received(const TypeLengthValueType<TYPE, LENGTH>& element) = 0;

    private:
        // Methods to extract and insert data into the socket buffers
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            _adminLock.Lock();

            uint16_t result = 0;

            while ((result < maxSendSize) && (_sendSize > 0)) {
                uint32_t chunkLeft = ((_sendLength + sizeof(TYPE) + sizeof(LENGTH)) - _sendOffset);
                uint16_t chunkSize = (maxSendSize > chunkLeft ? chunkLeft : maxSendSize);

                // copy our buffer in, as far as we can..
                ::memcpy(dataFrame, &(_sendBuffer[_sendOffset]), chunkSize);
                result += chunkSize;

                if (chunkSize != chunkLeft) {
                    _sendOffset += chunkSize;
                }
                else {
                    uint32_t totalLength = _sendLength + sizeof(TYPE) + sizeof(LENGTH);

                    // We send it all. Report and move on..
                    Send(TypeLengthValueType<TYPE, LENGTH>(_sendType, _sendLength, (_sendLength > 0 ? _sendBuffer : nullptr)));

                    // Remove the current TLV..
                    ::memcpy(&(_sendBuffer[0]), &(_sendBuffer[totalLength]), _sendSize - totalLength);
                    _sendSize -= totalLength;
                    _sendOffset = 0;

                    // Select a new TLV to send
                    if (_sendSize > 0) {
                        _sendType = NumberType<TYPE>::FromNetwork(*static_cast<const TYPE*>(&(_sendBuffer[0])));
                        _sendLength = NumberType<LENGTH>::FromNetwork(*static_cast<const LENGTH*>(&(_sendBuffer[sizeof(TYPE)])));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            uint16_t result = 0;

            _adminLock.Lock();

            while (result != availableData) {
                if (_receiveSize < sizeof(TYPE)) {
                    // First fill the first areas of data
                    uint8_t* buffer = static_cast<uint8_t*>(&_receiveType);

                    while ((_receiveSize < sizeof(TYPE)) && (result < availableData)) {
                        buffer[_receiveSize++] = dataFrame[result++];
                    }

                    if (_receiveSize == sizeof(TYPE)) {
                        _receiveType = NumberType<TYPE>::FromNetwork(_receiveType);
                    }
                }

                if ((_receiveSize >= sizeof(TYPE)) && (_receiveSize < (sizeof(TYPE) + sizeof(LENGTH)))) {
                    uint8_t* buffer = static_cast<uint8_t*>(&_receiveLength);

                    while ((_receiveSize < (sizeof(TYPE) + sizeof(LENGTH))) && (result < availableData)) {
                        buffer[(_receiveSize - sizeof(TYPE))] = dataFrame[result++];
                        _receiveSize++;
                    }

                    // Check if the buffer fits the content, if not, extend!!
                    if (_receiveSize == (sizeof(TYPE) + sizeof(LENGTH))) {
                        _receiveLength = NumberType<TYPE>::FromNetwork(_receiveLength);

                        if (_receiveLength > _receiveMaxSize) {
                            delete[] _receiveBuffer;
                            _receiveMaxSize = ((_receiveLength + DEFAULTVALUESIZE) / DEFAULTVALUESIZE) * DEFAULTVALUESIZE;
                            _receiveBuffer = new uint8_t[_receiveMaxSize];
                        }
                    }
                }

                if (_receiveSize >= (sizeof(TYPE) + sizeof(LENGTH))) {
                    // We have got a type and length, now we need the actual data, and process it if it is complete
                    if ((_receiveSize - sizeof(TYPE) - sizeof(LENGTH)) < _receiveLength) {
                        // Add the data we are missing.
                        while (((_receiveSize - sizeof(TYPE) - sizeof(LENGTH)) < _receiveLength) && (result < availableData)) {
                            _receiveBuffer[(_receiveSize - sizeof(TYPE) - sizeof(LENGTH))] = dataFrame[result++];
                            _receiveSize++;
                        }
                    }

                    // See if we can dispatch it
                    if ((_receiveSize - sizeof(TYPE) - sizeof(LENGTH)) == _receiveLength) {
                        Received(TypeLengthValueType<TYPE, LENGTH>(_receiveType, _receiveLength, (_receiveLength > 0 ? _receiveBuffer : nullptr)));

                        // Dispatch and reset
                        _receiveSize = 0;
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        Handler _channel;

        uint32_t _sendMaxSize;
        uint8_t* _sendBuffer;
        uint32_t _sendSize;
        TYPE _sendType;
        LENGTH _sendLength;
        uint32_t _sendOffset;

        uint32_t _receiveMaxSize;
        uint8_t* _receiveBuffer;
        uint32_t _receiveSize;
        TYPE _receiveType;
        LENGTH _receiveLength;

        Core::CriticalSection _adminLock;
    };
}
} // namespace Core

#endif // __STREAMTYPELENGTHVALUES_H

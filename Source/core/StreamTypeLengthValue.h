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
 
#pragma once

#include "Module.h"
#include "Synchronize.h"

#include <atomic>

namespace Thunder {

namespace Exchange {

    template <typename TYPE, typename LENGTH, const uint16_t BUFFERSIZE>
    struct TypeLengthValueType {
        class Request {
        private:
            Request() = delete;
            Request(const Request& copy) = delete;
            Request& operator=(const Request& copy) = delete;

        protected:
            inline Request(const uint8_t* value)
                : _length(0)
                , _offset(0)
                , _value(value)
            {
                ASSERT(value != nullptr);
            }
            inline Request(const TYPE& type, const uint8_t* value)
                : _type(type)
                , _length(0)
                , _offset(0)
                , _value(value)
            {
                ASSERT(value != nullptr);
            }

        public:
            inline Request(const TYPE& type, const LENGTH& length, const uint8_t* value)
                : _type(type)
                , _length(length)
                , _offset(0)
                , _value(value)
            {
                ASSERT((value != nullptr) || (length == 0));
            }
            inline ~Request()
            {
            }

        public:
            inline void Reset()
            {
                Offset(0);
            }
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
            inline uint16_t Serialize(uint8_t stream[], const uint16_t length) const
            {
                uint16_t result = 0;

                if (_offset < sizeof(TYPE)) {
                    result = Store(_type, stream, length, _offset);
                    _offset += result;
                }
                if ((_offset < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
                    uint16_t loaded = Store(_length, &(stream[result]), length - result, _offset - sizeof(TYPE));
                    result += loaded;
                    _offset += loaded;
                }
                if ((length - result) > 0) {
                    uint16_t copyLength = std::min(static_cast<uint16_t>(_length - (_offset - (sizeof(TYPE) + sizeof(LENGTH)))), static_cast<uint16_t>(length - result));

                    if (copyLength > 0) {
                        ::memcpy(&(stream[result]), &(_value[_offset - (sizeof(TYPE) + sizeof(LENGTH))]), copyLength);
                        _offset += copyLength;
                        result += copyLength;
                    }
                }
                return (result);
            }

        protected:
            inline void Type(const TYPE type)
            {
                _type = type;
            }
            inline void Length(const LENGTH length)
            {
                _length = length;
            }
            inline LENGTH Offset() const
            {
                return (_offset);
            }
            inline void Offset(const LENGTH offset)
            {
                _offset = offset;
            }

        private:
            template <typename FIELD>
            uint8_t Store(const FIELD number, uint8_t stream[], const uint16_t length, const uint8_t offset) const
            {
                uint8_t stored = 0;
                uint8_t index = offset;
                ASSERT(offset < sizeof(FIELD));
                while ((index < sizeof(FIELD)) && (stored < length)) {
                    stream[index] = static_cast<uint8_t>(number >> (8 * (sizeof(TYPE) - 1 - index)));
                    index++;
                    stored++;
                }
                return (stored);
            }

        private:
            TYPE _type;
            LENGTH _length;
            mutable LENGTH _offset;
            const uint8_t* _value;
        };
        class Response : public Request {
        private:
            Response() = delete;
            Response(const Response& copy) = delete;
            Response& operator=(const Response& copy) = delete;

        public:
            inline Response(const TYPE type)
                : Request(type, _value)
            {
            }
            inline ~Response()
            {
            }

        public:
            bool Copy(const Request& buffer)
            {
                bool result = false;
                if (buffer.Type() == Request::Type()) {
                    LENGTH copyLength = std::min(buffer.Length(), static_cast<LENGTH>(BUFFERSIZE));
                    ::memcpy(_value, buffer.Value(), copyLength);
                    Request::Length(copyLength);
                    result = true;
                }

                return (result);
            }

        private:
            uint8_t _value[BUFFERSIZE];
        };
        class Buffer : public Request {
        private:
            Buffer(const Buffer& copy) = delete;
            Buffer& operator=(const Buffer& copy) = delete;

        public:
            inline Buffer()
                : Request(_value)
                , _used(0)
            {
            }
            inline ~Buffer()
            {
            }

        public:
            inline bool Next()
            {
                bool result = false;

                // Clear the current selected block, move on to the nex block, return true, if
                // we have a next block..
                if ((Request::Offset() > (sizeof(TYPE) + sizeof(LENGTH))) && ((Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH))) == Request::Length())) {
                    Request::Offset(0);
                    if (_used > 0) {
                        uint16_t length = _used;
                        _used = 0;
                        ::memcpy(&(_value[0]), &(_value[Request::Length()]), length);
                        result = Deserialize(_value, length);
                    }
                }
                return (result);
            }
            inline bool Deserialize(const uint8_t stream[], const uint16_t length)
            {
                uint16_t result = 0;
                if (Request::Offset() < sizeof(TYPE)) {
                    LENGTH offset = Request::Offset();
                    TYPE current = (offset > 0 ? Request::Type() : 0);
                    uint8_t loaded = Load(current, &(stream[result]), length - result, offset);
                    result += loaded;
                    Request::Offset(offset + loaded);
                    Request::Type(current);
                }
                if ((Request::Offset() < (sizeof(TYPE) + sizeof(LENGTH))) && ((length - result) > 0)) {
                    LENGTH offset = Request::Offset();
                    LENGTH current = (offset > sizeof(TYPE) ? Request::Length() : 0);
                    uint8_t loaded = Load(current, &(stream[result]), length - result, offset - sizeof(TYPE));
                    result += loaded;
                    Request::Offset(offset + loaded);
                    Request::Length(current);
                }
                if ((length - result) > 0) {
                    uint16_t copyLength = std::min(static_cast<uint16_t>(Request::Length() - (Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH)))), static_cast<uint16_t>(length - result));
                    if (copyLength > 0) {
                        ::memcpy(&(_value[Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH))]), &(stream[result]), copyLength);
                        Request::Offset(Request::Offset() + copyLength);
                        result += copyLength;
                    }
                }

                if (result < length) {
                    uint16_t copyLength = std::min(static_cast<uint16_t>((2 * BUFFERSIZE) - Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH)) - _used), static_cast<uint16_t>(length - result));
                    ::memcpy(&(_value[Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH)) + _used]), &(stream[result]), copyLength);
                    _used += copyLength;
                }

                return ((Request::Offset() >= (sizeof(TYPE) + sizeof(LENGTH))) && ((Request::Offset() - (sizeof(TYPE) + sizeof(LENGTH))) == Request::Length()));
            }

        private:
            template <typename FIELD>
            uint8_t Load(FIELD& number, const uint8_t stream[], const uint16_t length, const uint8_t offset) const
            {
                uint8_t loaded = 0;
                uint8_t index = offset;
                ASSERT(offset < sizeof(FIELD));
                while ((index < sizeof(FIELD)) && (loaded < length)) {
                    number |= (stream[loaded++] >> (8 * (sizeof(FIELD) - 1 - index)));
                    index++;
                }
                return (loaded);
            }

        private:
            uint16_t _used;
            uint8_t _value[2 * BUFFERSIZE];
        };
    };

} // namespace Exchange

namespace Core {
    template <typename SOURCE, typename DATAEXCHANGE>
    class MessageExchangeType {
    private:
        MessageExchangeType(const MessageExchangeType<SOURCE, DATAEXCHANGE>&) = delete;
        MessageExchangeType<SOURCE, DATAEXCHANGE>& operator=(const MessageExchangeType<SOURCE, DATAEXCHANGE>&) = delete;

        class Handler : public SOURCE {
        private:
            Handler() = delete;
            Handler(const Handler&) = delete;
            Handler& operator=(const Handler&) = delete;

        public:
            template <typename... Args>
            Handler(MessageExchangeType<SOURCE, DATAEXCHANGE>& parent, Args... args)
                : SOURCE(args...)
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
            MessageExchangeType<SOURCE, DATAEXCHANGE>& _parent;
        };

    public:
        using Request = typename DATAEXCHANGE::Request;
        using Response = typename DATAEXCHANGE::Response;

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        MessageExchangeType(Args... args)
            : _channel(*this, args...)
            , _current(nullptr)
            , _reevaluate(false, true)
            , _waitCount(0)
            , _responses()

        {
        }
POP_WARNING()
        virtual ~MessageExchangeType() = default;

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
            _channel.Flush();

            _responses.Lock();

            _responses.Flush();
            _buffer.Flush();

            _current = nullptr;
            Reevaluate();

            _responses.Unlock();

            return (Core::ERROR_NONE);
        }
        uint32_t Submit(const typename DATAEXCHANGE::Request& request, const uint32_t allowedTime = Core::infinite)
        {
            _responses.Lock();

            uint32_t result = ClaimSlot(request, allowedTime);

            if (_current == &request) {

                _responses.Unlock();

                _channel.Trigger();

                _responses.Lock();

                result = CompletedSlot(request, allowedTime);

                _current = nullptr;
                Reevaluate();
            }

            _responses.Unlock();

            return (result);
        }
        uint32_t Exchange(const typename DATAEXCHANGE::Request& request, typename DATAEXCHANGE::Response& response, const uint32_t allowedTime)
        {

            _responses.Lock();

            uint32_t result = ClaimSlot(request, allowedTime);

            if (_current == &request) {

                _responses.Load(response);
                _responses.Unlock();

                _channel.Trigger();

                result = _responses.Acquire(allowedTime);

                _responses.Lock();

                _current = nullptr;
                Reevaluate();
            }

            _responses.Unlock();

            return (result);
        }

        virtual void StateChange()
        {
        }
        virtual void Send(const typename DATAEXCHANGE::Request& element)
        {
        }
        virtual void Received(const typename DATAEXCHANGE::Request& element)
        {
        }

    private:
        void Reevaluate()
        {
            _reevaluate.SetEvent();

            while (_waitCount != 0) {
                std::this_thread::yield();
            }

            _reevaluate.ResetEvent();
        }
        uint32_t ClaimSlot(const typename DATAEXCHANGE::Request& request, const uint32_t allowedTime)
        {
            uint32_t result = Core::ERROR_NONE;

            while ((_current != nullptr) && (result == Core::ERROR_NONE)) {

                _waitCount++;

                _responses.Unlock();

                result = _reevaluate.Lock(allowedTime);

                _waitCount--;

                _responses.Lock();
            }

            if (result == Core::ERROR_NONE) {
                _current = &request;
            }

            return (result);
        }
        uint32_t CompletedSlot(const typename DATAEXCHANGE::Request& request, const uint32_t allowedTime)
        {
            uint32_t result = Core::ERROR_NONE;

            if (_current == &request) {

                _waitCount++;

                _responses.Unlock();

                result = _reevaluate.Lock(allowedTime);

                _waitCount--;

                _responses.Lock();
            }

            ASSERT(_current != &request);

            return (result);
        }

        // Methods to extract and insert data into the socket buffers
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _responses.Lock();

            if ((_current != nullptr) && (_current != reinterpret_cast<typename DATAEXCHANGE::Request*>(~0))) {
                result = _current->Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    Send(*_current);
                    _current = reinterpret_cast<typename DATAEXCHANGE::Request*>(~0);
                    Reevaluate();
                }
            }

            _responses.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            _responses.Lock();

            if (_buffer.Deserialize(dataFrame, availableData) == true) {
                do {
                    if (_responses.Evaluate(_buffer) == false) {
                        Received(_buffer);
                    }
                } while (_buffer.Next() == true);
            }

            _responses.Unlock();

            return (availableData);
        }

    private:
        Handler _channel;
        const typename DATAEXCHANGE::Request* _current;
        Core::Event _reevaluate;
        std::atomic<uint32_t> _waitCount;
        typename DATAEXCHANGE::Buffer _buffer;
        typename Core::SynchronizeType<typename DATAEXCHANGE::Response> _responses;
    };

    template <typename SOURCE, typename TYPE, typename LENGTH, const uint32_t BUFFER_SIZE>
    using StreamTypeLengthValueType = MessageExchangeType<SOURCE, Exchange::TypeLengthValueType<TYPE, LENGTH, BUFFER_SIZE>>;
}
}

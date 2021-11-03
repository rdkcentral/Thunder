/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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
#include "UUID.h"
#include "DataRecord.h"


namespace WPEFramework {

namespace Bluetooth {

namespace SDP {

    struct EXTERNAL use_descriptor_t { explicit use_descriptor_t() = default; };
    constexpr use_descriptor_t use_descriptor = use_descriptor_t{}; // tag for selecting a proper overload

    struct EXTERNAL use_length_t { explicit use_length_t() = default; };
    constexpr use_length_t use_length = use_length_t{};

    class EXTERNAL Payload : public DataRecordBE {
    public:
        enum elementtype : uint8_t {
            NIL = 0x00,
            UINT = 0x08,
            INT = 0x10,
            UUID = 0x18,
            TEXT = 0x20,
            BOOL = 0x28,
            SEQ = 0x30,
            ALT = 0x38,
            URL = 0x40,
        };

        enum sizetype {
            SIZE_8 = 0,
            SIZE_16 = 1,
            SIZE_32 = 2,
            SIZE_64 = 3,
            SIZE_128 = 4,
            SIZE_U8_FOLLOWS = 5,
            SIZE_U16_FOLLOWS = 6,
            SIZE_U32_FOLLOWS = 7
        };

    public:
        using Builder = std::function<void(Payload&)>;
        using Inspector = std::function<void(const Payload&)>;

        enum class Continuation : uint8_t {
            ABSENT = 0,
            FOLLOWS
        };

    public:
        using DataRecordBE::DataRecordBE;
        using DataRecordBE::Pop;
        using DataRecordBE::Push;
        using DataRecordBE::Peek;

        ~Payload() = default;

    public:
        void Push(const Continuation cont, const Buffer& data = Buffer())
        {
            if (cont == Continuation::ABSENT) {
                Push(static_cast<uint8_t>(0));
            } else {
                ASSERT((data.length() > 0) && (data.length() <= 16));
                Push(static_cast<uint8_t>(data.length()));
                Push(data);
            }
        }
        void Pop(Continuation& cont, Buffer& data) const
        {
            uint8_t size = 0;
            Pop(size);
            cont = (size == 0? Continuation::ABSENT : Continuation::FOLLOWS);
            if (size != 0) {
                Pop(data, size);
            } else {
                data.clear();
            }
        }

    public:
        void Push(use_descriptor_t)
        {
            PushDescriptor(NIL);
        }
        void Push(const Bluetooth::UUID& value)
        {
            ASSERT(Free() >= value.Length());
            uint8_t size = value.Length();
            while (size-- > 0) {
                _buffer[_writerOffset++] = value.Data()[size]; // reverse!
            }
        }
        void Push(use_descriptor_t, const Bluetooth::UUID& value)
        {
            PushDescriptor(UUID, value.Length());
            Push(value);
        }
        void Push(use_descriptor_t, const string& value, bool url = false)
        {
            PushDescriptor((url? URL : TEXT), value.length());
            Push(value);
        }
        void Push(use_descriptor_t, const bool value)
        {
            PushDescriptor(BOOL, 1);
            Push(value);
        }
        template<typename TYPE, /* if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
        void Push(use_descriptor_t, const TYPE value)
        {
            PushDescriptor((std::numeric_limits<TYPE>::is_signed? INT : UINT), sizeof(TYPE));
            Push(value);
        }
        template<typename TYPE, /* if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        void Push(use_descriptor_t, const TYPE value)
        {
            PushDescriptor(UINT, sizeof(TYPE));
            Push(value);
        }
        void Push(use_descriptor_t, const Payload& sequence, const bool alternative = false)
        {
            if (sequence.Length() != 0) {
                PushDescriptor((alternative? ALT : SEQ), sequence.Length());
                Push(sequence);
            }
        }
        void Push(use_length_t, const Payload& sequence, const bool = false)
        {
            Push(sequence.Length()); // opposite to use_descriptor, here do store the zero length
            if (sequence.Length() != 0) {
                Push(sequence);
            }
        }
        void Push(use_descriptor_t, const Buffer& sequence, const bool alternative = false)
        {
            if (sequence.size() != 0) {
                PushDescriptor((alternative? ALT : SEQ), sequence.size());
                Push(sequence);
            }
        }
        void Push(use_length_t, const Buffer& sequence, const bool = false)
        {
            ASSERT(sequence.size() < 0x10000);
            Push(static_cast<uint16_t>(sequence.size()));
            if (sequence.size() != 0) {
                Push(sequence);
            }
        }
        void Push(use_descriptor_t, const uint8_t sequence[], const uint16_t length, const bool alternative = false)
        {
            if (length != 0) {
                PushDescriptor((alternative? ALT : SEQ), length);
                Push(sequence, length);
            }
        }
        void Push(use_length_t, const uint8_t sequence[], const uint16_t length)
        {
            Push(length);
            if (length != 0) {
                Push(sequence, length);
            }
        }
        void Push(const Builder& builder, const uint16_t scratchPadSize = 2048)
        {
            uint8_t scratchPad[scratchPadSize];
            Payload sequence(scratchPad, sizeof(scratchPad));
            builder(sequence);
            Push(sequence);
        }
        template<typename TAG>
        void Push(TAG tag, const Builder& builder, const bool alternative = false, const uint16_t scratchPadSize = 2048)
        {
            uint8_t scratchPad[scratchPadSize];
            Payload sequence(scratchPad, sizeof(scratchPad));
            builder(sequence);
            Push(tag, sequence, alternative);
        }
        template<typename TYPE>
        void Push(const std::list<TYPE>& list, const bool alternative = false, const uint16_t scratchPadSize = 2048)
        {
            if (list.size() != 0) {
                ASSERT(Free() >= (list.size() * sizeof(TYPE)));
                Push([&](Payload& sequence){
                    for (const auto& item : list) {
                        sequence.Push(item);
                    }
                }, alternative, scratchPadSize);
            }
        }
        template<typename TYPE>
        void Push(use_descriptor_t, const std::list<TYPE>& list, const bool alternative = false, const uint16_t scratchPadSize = 2048)
        {
            if (list.size() != 0) {
                Push(use_descriptor, [&](Payload& sequence){
                    for (const auto& item : list) {
                        sequence.Push(use_descriptor, item);
                    }
                }, alternative, scratchPadSize);
            }
        }

    public:
        void Pop(use_descriptor_t, string& value) const
        {
            elementtype type;
            uint32_t size = 0;
            _readerOffset += ReadDescriptor(type, size);
            if ((type == TEXT) || (type == URL)) {
                Pop(value, size);
            } else {
                TRACE_L1("SDP: Unexpected descriptor in payload [0x%02x], expected TEXT or URL", type);
                _readerOffset += size;
            }
        }
        template<typename TYPE, /* if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        void Pop(use_descriptor_t, TYPE& value)
        {
            elementtype type;
            uint32_t size = 0;
            _readerOffset += ReadDescriptor(type, size);
            if (type == UINT) {
                typename std::underlying_type<TYPE>::type temp;
                if (size != sizeof(temp)) {
                    TRACE_L1("SDP: Warning: enum underlying type size does not match!");
                }
                Pop(temp);
                value = static_cast<TYPE>(temp);
            } else {
                TRACE_L1("SDP: Unexpected descriptor in payload [0x%02x], expected a UINT for enum", type);
                _readerOffset += size;
            }
        }
        template<typename TYPE, /* if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
        void Pop(use_descriptor_t, TYPE& value, uint32_t* outSize = nullptr) const
        {
            elementtype type;
            uint32_t size = 0;
            _readerOffset += ReadDescriptor(type, size);
            if (type == (std::numeric_limits<TYPE>::is_signed? INT : UINT)) {
                if (size > sizeof(TYPE)) {
                    TRACE_L1("SDP: Warning: integer value possibly truncated!");
                }
                if (size == 1) {
                    uint8_t val{};
                    DataRecord::PopIntegerValue(val);
                    value = val;
                } else if (size == 2) {
                    uint16_t val{};
                    PopIntegerValue(val);
                    value = val;
                } else if (size == 4) {
                    uint32_t val{};
                    PopIntegerValue(val);
                    value = val;
                } else {
                    TRACE_L1("SDP: Unexpected integer size");
                    _readerOffset += size;
                }
                if (outSize != nullptr) {
                    (*outSize) = size;
                }
            } else {
                TRACE_L1("SDP: Unexpected descriptor in payload [0x%02x], expected %s", type, (std::numeric_limits<TYPE>::is_signed? "an INT" : "a UINT"));
                _readerOffset += size;
            }
        }
        template<typename TYPE>
        void Pop(std::list<TYPE>& list, const uint16_t count) const
        {
            if (Available() >= (count * sizeof(TYPE))) {
                uint16_t i = count;
                while (i-- > 0) {
                    TYPE item;
                    Pop(item);
                    list.push_back(item);
                }
            } else {
                TRACE_L1("SDP: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }
        template<typename TYPE>
        void Pop(use_descriptor_t, std::list<TYPE>& list, const uint16_t count) const
        {
            uint16_t i = count;
            while (i-- > 0) {
                TYPE item;
                Pop(use_descriptor, item);
                list.push_back(item);
            }
        }
        void Pop(use_descriptor_t, Bluetooth::UUID& uuid) const
        {
            uint32_t size = 0;
            elementtype type;
            _readerOffset += ReadDescriptor(type, size);
            if (type == UUID) {
                if (Available() >= size) {
                    if (size == 2) {
                        uuid = Bluetooth::UUID((_buffer[_readerOffset] << 8) | _buffer[_readerOffset + 1]);
                    } else if (size == 4) {
                        ASSERT(false && "SDP: 32-bit UUID not supported\n");
                    } else {
                        uint8_t buffer[size];
                        uint8_t i = size;
                        while (i-- > 0) {
                            buffer[i] = _buffer[_readerOffset++];
                        }
                        uuid = Bluetooth::UUID(buffer);
                    }
                    _readerOffset += size;
                } else {
                    TRACE_L1("SDP: Truncated payload");
                    _readerOffset = _writerOffset;
                }
            } else {
                TRACE_L1("SDP: Unexpected descriptor in payload [0x%02x], expected a UUID", type);
                _readerOffset += size;
            }
        }
        void Pop(use_descriptor_t, const Inspector& inspector) const
        {
            uint32_t size = 0;
            elementtype type;
            _readerOffset += ReadDescriptor(type, size);
            if (type == SEQ) {
                if (Available() >= size) {
                    Payload sequence(&_buffer[_readerOffset], size, size);
                    inspector(sequence);
                    _readerOffset += size;
                } else {
                    TRACE_L1("SDP: Truncated payload");
                    _readerOffset = _writerOffset;
                }
            } else {
               TRACE_L1("SDP: Unexpected descriptor in payload [0x%02x], expected a SEQ", type);
               _readerOffset += size;
            }
        }
        void Pop(use_descriptor_t, Buffer& element) const
        {
            elementtype elemType;
            uint32_t elemSize;
            uint8_t descriptorSize = ReadDescriptor(elemType, elemSize);
            // Don't assume any type of data here.
            if (Available() >= (descriptorSize + elemSize)) {
                element.assign(&_buffer[_readerOffset], (descriptorSize + elemSize));
                _readerOffset += (descriptorSize + elemSize);
            } else {
                TRACE_L1("SDP: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }
        void Pop(use_descriptor_t, Buffer& element, const uint16_t size) const
        {
            // Don't assume any type of data here.
            if (Available() >= size) {
                element.assign(&_buffer[_readerOffset], size);
                _readerOffset += size;
            } else {
                TRACE_L1("SDP: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }

    public:
        void Peek(use_descriptor_t, Payload& element) const
        {
            elementtype elemType;
            uint32_t elemSize;
            uint8_t descriptorSize = ReadDescriptor(elemType, elemSize);
            if (Available() >= (descriptorSize + elemSize)) {
                element.Assign(&_buffer[_readerOffset], (descriptorSize + elemSize));
                _readerOffset += (descriptorSize + elemSize);
            } else {
                TRACE_L1("SDP: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }

    private:
        void PushDescriptor(const elementtype type, const uint32_t size = 0);
        uint8_t ReadDescriptor(elementtype& type, uint32_t& size) const;
    }; // class Payload

    class EXTERNAL ClientSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint8_t SDP_PSM = 1;

    private:
        class Callback : public Core::IOutbound::ICallback
        {
        public:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator= (const Callback&) = delete;

            Callback(ClientSocket& parent)
                : _parent(parent)
            {
            }
            ~Callback() = default;

        public:
            void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                _parent.CommandCompleted(data, error_code);
            }

        private:
            ClientSocket& _parent;
        }; // class Callback

    public:
        static constexpr uint32_t CommunicationTimeout = 2000; /* 2 seconds. */

        class EXTERNAL PDU {
        public:
            static constexpr uint8_t HEADER_SIZE = 5;
            static constexpr uint8_t MAX_CONTINUATION_INFO_SIZE = 16;
            static constexpr uint16_t DEFAULT_SCRATCHPAD_SIZE = 4096;

        public:
            enum pduid : uint8_t {
                Invalid = 0,
                ErrorResponse = 1,
                ServiceSearchRequest = 2,
                ServiceSearchResponse = 3,
                ServiceAttributeRequest = 4,
                ServiceAttributeResponse = 5,
                ServiceSearchAttributeRequest = 6,
                ServiceSearchAttributeResponse = 7,
            };

            enum errorid : uint16_t {
                Success = 0,
                UnsupportedSdpVersion = 1,
                InvalidServiceRecordHandle = 2,
                InvalidRequestSyntax = 3,
                InvalidPduSize = 4,
                InvalidContinuationState = 5,
                InsufficientResources = 6,
                Reserved = 255,
                DeserializationFailed,
                PacketContinuation
            };

        public:
            static constexpr uint16_t MIN_BUFFER_SIZE = (HEADER_SIZE + 1 + MAX_CONTINUATION_INFO_SIZE);
            static constexpr uint16_t DEFAULT_BUFFER_SIZE = (HEADER_SIZE + DEFAULT_SCRATCHPAD_SIZE + MAX_CONTINUATION_INFO_SIZE);

        public:
            explicit PDU(const uint16_t bufferSize)
                : _bufferSize(bufferSize)
                , _buffer(static_cast<uint8_t*>(::malloc(_bufferSize)))
                , _size(0)
                , _transactionId(-1)
                , _continuationOffset(0)
                , _payloadSize(0)
            {
                ASSERT(_buffer != nullptr);
                ASSERT(_bufferSize > MIN_BUFFER_SIZE);
            }
            ~PDU()
            {
                ::free(_buffer);
            }

        public:
            void Clear()
            {
                ::memset(_buffer, 0, HEADER_SIZE);
                _size = 0;
                _continuationOffset = 0;
                _payloadSize = 0;
            }
            bool IsValid() const
            {
                return ((_buffer != nullptr) && (_bufferSize > MIN_BUFFER_SIZE) && (Type() != Invalid));
            }
            uint32_t Length() const
            {
                return (_size);
            }
            uint16_t Capacity() const
            {
                return (_bufferSize - MIN_BUFFER_SIZE);
            }
            const uint8_t* Data() const
            {
                return (_buffer);
            }
            pduid Type() const
            {
                return (static_cast<pduid>(_buffer[0]));
            }
            uint16_t TransactionId() const
            {
                return ((_buffer[1] << 8) | _buffer[2]);
            }
            void NextTransaction(const uint16_t id = ~0)
            {
                if (id != static_cast<uint16_t>(~0)) {
                    _transactionId = id;
                } else {
                    _transactionId++;
                }
            }
            void Finalize(const Buffer& continuation = Buffer())
            {
                // Called during command construction or by retrigger due to continuation
                ASSERT(_size >= HEADER_SIZE);
                ASSERT(_continuationOffset >= HEADER_SIZE);

                // Increment transaction ID
                _buffer[1] = (_transactionId >> 8);
                _buffer[2] = _transactionId;

                // Update size
                const uint16_t payloadSize = (_payloadSize + 1 + continuation.size());
                _buffer[3] = (payloadSize >> 8);
                _buffer[4] = payloadSize;

                if (Type() != PDU::ErrorResponse) {
                    // Attach continuation information
                    _buffer[_continuationOffset] = continuation.size();
                    ::memcpy(_buffer + _continuationOffset + 1, continuation.data(), continuation.size());
                    _size = (HEADER_SIZE + payloadSize);
                }
            }
            void Construct(const pduid type, const SDP::Payload& parameters)
            {
                ASSERT(Capacity() >= (parameters.Length() + MIN_BUFFER_SIZE));

                Clear();

                if (Capacity() >= (parameters.Length() + MIN_BUFFER_SIZE)) {
                    ::memset(_buffer, 0, HEADER_SIZE);
                    ::memcpy(_buffer + HEADER_SIZE, parameters.Data(), parameters.Length());
                    _buffer[0] = type;
                    _payloadSize = parameters.Length();
                    _size = (HEADER_SIZE + _payloadSize);
                    _continuationOffset = _size;

                    Finalize();
                } else {
                    TRACE(Trace::Error, (_T("Parameters to large to fit in PDU [%d]"), parameters.Length()));
                }
            }
            void Construct(const pduid type, const SDP::Payload::Builder& builder, const uint32_t scratchPadSize = DEFAULT_SCRATCHPAD_SIZE)
            {
                uint8_t scratchPad[scratchPadSize];
                SDP::Payload parameters(scratchPad, sizeof(scratchPad));
                builder(parameters);
                Construct(type, parameters);
            }

        private:
            uint16_t _bufferSize;
            uint8_t* _buffer;
            uint16_t _size;
            uint16_t _transactionId;
            uint16_t _continuationOffset;
            uint16_t _payloadSize;
        }; // class PDU

        class EXTERNAL Outward {
        public:
            Outward() = delete;
            Outward(const Outward&) = delete;
            Outward& operator=(const Outward&) = delete;

            explicit Outward(uint16_t pduBufferSize)
                : _pdu(pduBufferSize)
                , _offset(0)
            {
                ASSERT(pduBufferSize >= PDU::MIN_BUFFER_SIZE);
            }
            ~Outward() = default;

        public:
            void Reload() const
            {
                _offset = 0;
            }
            bool IsValid() const
            {
                return (_pdu.IsValid());
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const
            {
                ASSERT(stream != nullptr);

                uint16_t result = std::min(static_cast<uint16_t>(_pdu.Length() - _offset), length);
                if (result > 0) {
                    ::memcpy(stream, (_pdu.Data() + _offset), result);
                    _offset += result;
                }
                return (result);
            }
            void Finalize(const Buffer& cont)
            {
                _pdu.NextTransaction();
                _pdu.Finalize(cont);
            }
            uint16_t TransactionId() const
            {
                return (_pdu.TransactionId());
            }
            void NextTransaction(const uint16_t id)
            {
                _pdu.NextTransaction(id);
            }

        protected:
            PDU _pdu;
            mutable uint16_t _offset;
        }; // class Outward

        class EXTERNAL Inward {
        private:
            Inward() = delete;
            Inward(const Inward&) = delete;
            Inward& operator=(const Inward&) = delete;

        public:
            explicit Inward(uint32_t payloadSize)
                : _type(PDU::Invalid)
                , _status(PDU::Reserved)
                , _transactionId(-1)
                , _scratchPad(static_cast<uint8_t*>(::malloc(payloadSize)))
                , _payload(_scratchPad, payloadSize)
            {
                ASSERT(payloadSize >= PDU::MIN_BUFFER_SIZE);
                ASSERT(_scratchPad != nullptr);
            }
            ~Inward()
            {
                ::free(_scratchPad);
            }

        public:
            void Clear()
            {
                _status = PDU::Reserved;
                _type = PDU::Invalid;
                _continuationData.clear();
                _payload.Clear();
            }
            PDU::pduid Type() const
            {
                return (_type);
            }
            PDU::errorid Status() const
            {
                return (_status);
            }
            const Buffer& Continuation() const
            {
                return (_continuationData);
            }
            uint16_t TransactionId() const
            {
                return (_transactionId);
            }

        protected:
            PDU::pduid _type;
            PDU::errorid _status;
            uint16_t _transactionId;
            uint8_t* _scratchPad;
            SDP::Payload _payload;
            Buffer _continuationData;
        }; // class Inward

        class EXTERNAL Command : public Core::IOutbound, public Core::IInbound {
        public:
            class EXTERNAL Request : public Outward {
            public:
                Request() = delete;
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;

                explicit Request(const uint16_t pduBufferSize)
                    : Outward(pduBufferSize)
                {
                }
                ~Request() = default;

            public:
                void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults)
                {
                    ASSERT((services.size() > 0) && (services.size() <= 12)); // As per spec
                    ASSERT(maxResults > 0);

                    _pdu.NextTransaction();
                    _pdu.Construct(PDU::ServiceSearchRequest, [&](SDP::Payload& parameters) {
                        parameters.Push(SDP::use_descriptor, services);
                        parameters.Push(maxResults);
                    });
                }
                void ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT(serviceHandle != 0);
                    ASSERT((attributeIdRanges.size() > 0) && (attributeIdRanges.size() <= 256));

                    _pdu.NextTransaction();
                    _pdu.Construct(PDU::ServiceAttributeRequest, [&](SDP::Payload& parameters) {
                        parameters.Push(serviceHandle);
                        parameters.Push(_pdu.Capacity());
                        parameters.Push(SDP::use_descriptor, attributeIdRanges);
                    });
                }
                void ServiceSearchAttribute(const std::list<UUID>& services, const std::list<uint32_t>& attributeIdRanges)
                {
                    ASSERT((services.size() > 0) && (services.size() <= 12));
                    ASSERT((attributeIdRanges.size() > 0) && (attributeIdRanges.size() <= 256));

                    _pdu.NextTransaction();
                    _pdu.Construct(PDU::ServiceSearchAttributeRequest, [&](SDP::Payload& parameters) {
                        parameters.Push(SDP::use_descriptor, services);
                        parameters.Push(_pdu.Capacity());
                        parameters.Push(SDP::use_descriptor, attributeIdRanges);
                    });
                }
            }; // class Request

        public:
            class EXTERNAL Response : public Inward {
            public:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;

                explicit Response(uint32_t payloadSize)
                    : Inward(payloadSize)
                {
                }

                ~Response() = default;

            public:
                void Clear()
                {
                    Inward::Clear();
                    _handles.clear();
                    _attributes.clear();
                }
                const std::list<uint32_t>& Handles() const
                {
                    return (_handles);
                }
                const std::map<uint16_t, Buffer>& Attributes() const
                {
                    return (_attributes);
                }

                uint16_t Deserialize(const uint16_t reqTransactionId, const uint8_t stream[], const uint16_t length);

            private:
                PDU::errorid DeserializeServiceSearchResponse(const SDP::Payload& params);
                PDU::errorid DeserializeServiceAttributeResponse(const SDP::Payload& params);

            private:
                std::list<uint32_t> _handles;
                std::map<uint16_t, Buffer> _attributes;
            }; // class Response

        public:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;

            Command()
                : _status(~0)
                , _request(PDU::DEFAULT_BUFFER_SIZE)
                , _response(PDU::DEFAULT_BUFFER_SIZE)
            {
            }
            ~Command() = default;

        public:
            void ServiceSearch(const UUID& serviceId, const uint16_t maxResults = 256) // single
            {
                ServiceSearch(std::list<UUID>{serviceId}, maxResults);
            }
            void ServiceSearch(const std::list<UUID>& services, const uint16_t maxResults = 256) // list
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceSearch(services, maxResults);
            }
            void ServiceAttribute(const uint32_t serviceHandle) // all
            {
                ServiceAttribute(serviceHandle, std::list<uint32_t>{0x0000FFFF});
            }
            void ServiceAttribute(const uint32_t serviceHandle, const uint16_t attributeId) // single
            {
                ServiceAttribute(serviceHandle, std::list<uint32_t>{(static_cast<uint32_t>(attributeId) << 16) | attributeId});
            }
            void ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges) // ranges
            {
                _response.Clear();
                _status = ~0;
                _request.ServiceAttribute(serviceHandle, attributeIdRanges);
            }

        public:
            Response& Result() {
                return (_response);
            }
            const Response& Result() const
            {
                return (_response);
            }
            uint32_t Status() const
            {
                return(_status);
            }
            bool IsValid() const
            {
                return (_request.IsValid());
            }
            void Status(const uint32_t code)
            {
                _status = code;
            }

        private:
            void Reload() const override
            {
                _request.Reload();
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                ASSERT(stream != nullptr);

                const uint16_t result = _request.Serialize(stream, length);

                if (result != 0) {
                    CMD_DUMP("SDP client send", stream, result);
                }

                return (result);
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
            {
                ASSERT(stream != nullptr);

                CMD_DUMP("SDP client received", stream, length);

                uint16_t result = _response.Deserialize(_request.TransactionId(), stream, length);
                if ((_response.Continuation().empty() == false)) {
                    // Data transmission is not complete yet, so update transaction ID and continuation information
                    _request.Finalize(_response.Continuation());
                }
                return (result);
            }
            Core::IInbound::state IsCompleted() const override
            {
                if (_response.Continuation().empty() == false) {
                    return (Core::IInbound::RESEND);
                } else {
                    return (_response.Type() != PDU::Invalid? Core::IInbound::COMPLETED : Core::IInbound::INPROGRESS);
                }
            }

        private:
            uint32_t _status;
            Request _request;
            Response _response;
        }; // class Command

    private:
        typedef std::function<void(const Command&)> Handler;

        class Entry {
        public:
            Entry() = delete;
            Entry(const Entry&) = delete;
            Entry& operator= (const Entry&) = delete;
            Entry(const uint32_t waitTime, Command& cmd, const Handler& handler)
                : _waitTime(waitTime)
                , _cmd(cmd)
                , _handler(handler)
            {
            }
            ~Entry() = default;

        public:
            Command& Cmd()
            {
                return (_cmd);
            }
            uint32_t WaitTime() const
            {
                return (_waitTime);
            }
            bool operator==(const Core::IOutbound* rhs) const
            {
                return (rhs == &_cmd);
            }
            bool operator!=(const Core::IOutbound* rhs) const
            {
                return (!operator==(rhs));
            }
            void Completed(const uint32_t error_code)
            {
                _cmd.Status(error_code);
                _handler(_cmd);
            }

        private:
            uint32_t _waitTime;
            Command& _cmd;
            Handler _handler;
        }; // class Entry

    public:
        ClientSocket(const ClientSocket&) = delete;
        ClientSocket& operator=(const ClientSocket&) = delete;

        ClientSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, 1024, 2048)
            , _adminLock()
            , _callback(*this)
            , _queue()
        {
        }
        ClientSocket(const SOCKET& connector, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, connector, remoteNode, 1024, 2048)
            , _adminLock()
            , _callback(*this)
            , _queue()
        {
        }
        ~ClientSocket() = default;

    public:
        void Execute(const uint32_t waitTime, Command& cmd, const Handler& handler)
        {
            _adminLock.Lock();

            if (cmd.IsValid() == true) {
                _queue.emplace_back(waitTime, cmd, handler);
                if (_queue.size() == 1) {
                    Send(waitTime, cmd, &_callback, &cmd);
                }
            } else {
                cmd.Status(Core::ERROR_BAD_REQUEST);
                handler(cmd);
            }

            _adminLock.Unlock();
        }
        void Revoke(const Command& cmd)
        {
            Revoke(cmd);
        }

    private:
        virtual void Operational(const bool upAndRunning) = 0;

        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);

                Operational(true);
            } else {
                Operational(false);
            }
        }

        uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
        {
            TRACE_L1("Not expecting SDP requests on this socket!");
            return (0);
        }

        void CommandCompleted(const Core::IOutbound& data, const uint32_t error_code)
        {
            _adminLock.Lock();

            if ((_queue.size() == 0) || (*(_queue.begin()) != &data)) {
                ASSERT(false && "Always the first one should be the one to be handled!!");
            } else {
                _queue.begin()->Completed(error_code);
                _queue.erase(_queue.begin());

                if (_queue.size() > 0) {
                    Entry& entry(*(_queue.begin()));
                    Command& cmd (entry.Cmd());

                    Send(entry.WaitTime(), cmd, &_callback, &cmd);
                }
            }

            _adminLock.Unlock();
        }

    private:
        Core::CriticalSection _adminLock;
        Callback _callback;
        std::list<Entry> _queue;
        struct l2cap_conninfo _connectionInfo;
    }; // class ClientSocket

    class EXTERNAL ServerSocket : public ClientSocket {
    private:
        class EXTERNAL Request : public Inward {
        public:
            Request(const Request&) = delete;
            Request& operator=(const Request&) = delete;

        public:
            Request(ServerSocket& server, const uint16_t payloadSize)
                : Inward(payloadSize)
                , _server(server)
            {
            }
            ~Request() = default;

        public:
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length);

        private:
            PDU::errorid DeserializeServiceSearchRequest(const SDP::Payload& params);
            PDU::errorid DeserializeServiceAttributeRequest(const SDP::Payload& params);
            PDU::errorid DeserializeServiceSearchAttributeRequest(const SDP::Payload& params);

        private:
            ServerSocket& _server;
        }; // class Request

        class EXTERNAL Response : public Outward, public Core::IOutbound  {
        public:
            Response(const Request&) = delete;
            Response& operator=(const Response&) = delete;

            Response(ServerSocket& server, const uint16_t pduBufferSize)
                : Outward(pduBufferSize)
                , _server(server)
            {
            }
            ~Response() = default;

        public:
            void Reload() const override
            {
                Outward::Reload();
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                ASSERT(stream != nullptr);

                const uint16_t result = Outward::Serialize(stream, length);

                if (result != 0) {
                    CMD_DUMP("SDP server send", stream, result);
                }

                return (result);
            }

        public:
            void SerializeErrorResponse(PDU::errorid error);
            void SerializeServiceSearchResponse(const std::list<UUID>& serviceUuids, const uint16_t maxResults);
            void SerializeServiceAttributeResponse(const uint32_t serviceHandle, const uint16_t maxBytes, const std::list<uint32_t>& ranges);
            void SerializeServiceSearchAttributeResponse(const std::list<UUID>& serviceUuids, uint16_t maxBytes, const std::list<uint32_t>& ranges);

        private:
            ServerSocket& _server;
        }; // class Response

    public:
        ServerSocket(const ServerSocket&) = delete;
        ServerSocket& operator=(const ServerSocket&) = delete;

        ServerSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : ClientSocket(localNode, remoteNode)
            , _request(*this, PDU::DEFAULT_BUFFER_SIZE)
            , _response(*this, PDU::DEFAULT_BUFFER_SIZE)
        {
        }
        ServerSocket(const SOCKET& connector, const Core::NodeId& remoteNode)
            : ClientSocket(connector, remoteNode)
            , _request(*this, PDU::DEFAULT_BUFFER_SIZE)
            , _response(*this, PDU::DEFAULT_BUFFER_SIZE)
        {
        }
        ~ServerSocket() = default;

    public:
        uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
        {
            // This is a SDP request from a client.
            CMD_DUMP("SDP server received", stream, length);
            return (_request.Deserialize(stream, length));
        }

    protected:
        virtual void Services(const UUID& uuid, std::list<uint32_t>& serviceHandles) = 0;
        virtual void Serialize(const uint32_t serviceHandle, const std::pair<uint16_t, uint16_t>& range,
                                std::function<void(const uint16_t id, const Buffer& buffer)> store) const = 0;

    private:
        void OnError(const uint16_t transactionId, const PDU::errorid error)
        {
            _response.NextTransaction(transactionId);
            _response.SerializeErrorResponse(error);
            Reply(_response);
        }
        void OnServiceSearch(const uint16_t transactionId, const std::list<UUID>& serviceUuids, const uint16_t maxResults)
        {
            _response.NextTransaction(transactionId);
            _response.SerializeServiceSearchResponse(serviceUuids, maxResults);
            Reply(_response);
        }
        void OnServiceAttribute(const uint16_t transactionId, const uint32_t serviceHandle, uint16_t maxBytes, const std::list<uint32_t>& attributeRanges)
        {
            _response.NextTransaction(transactionId);
            _response.SerializeServiceAttributeResponse(serviceHandle, maxBytes, attributeRanges);
            Reply(_response);
        }
        void OnServiceSearchAttribute(const uint16_t transactionId, const std::list<UUID>& serviceUuids, uint16_t maxBytes, const std::list<uint32_t>& attributeRanges)
        {
            _response.NextTransaction(transactionId);
            _response.SerializeServiceSearchAttributeResponse(serviceUuids, maxBytes, attributeRanges);
            Reply(_response);
        }
        void Reply(Response& response)
        {
            Send(CommunicationTimeout, response, nullptr, nullptr);
        }

    private:
        Request _request;
        Response _response;
    };

} // namespace SDP

} // namespace Bluetooth

} // namespace WPEFramework

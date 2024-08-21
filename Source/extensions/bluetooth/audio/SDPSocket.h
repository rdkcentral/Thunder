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
#include "DataRecord.h"

namespace Thunder {

namespace Bluetooth {

namespace SDP {

    static constexpr uint8_t PSM = 1;
    static constexpr uint16_t SocketBufferSize = 2048;

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
        using DataRecordBE::PopAssign;

        ~Payload() = default;

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
            PushDescriptor((alternative? ALT : SEQ), sequence.Length());
            if (sequence.Length() != 0) {
                Push(sequence);
            }
        }
        void Push(use_length_t, const Payload& sequence, const bool = false)
        {
            Push(sequence.Length());
            if (sequence.Length() != 0) {
                Push(sequence);
            }
        }
        void Push(use_descriptor_t, const Buffer& sequence, const bool alternative = false)
        {
            PushDescriptor((alternative? ALT : SEQ), sequence.size());
            if (sequence.size() != 0) {
                Push(sequence);
            }
        }
        void Push(use_length_t, const Buffer& sequence, const bool = false)
        {
            ASSERT(sequence.size() < 0x10000);
            Push<uint16_t>(sequence.size());
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
        void Push(const Builder& Build, const uint16_t scratchPadSize = 2048)
        {
            uint8_t* scratchPad = static_cast<uint8_t*>(ALLOCA(scratchPadSize));
            Payload sequence(scratchPad, scratchPadSize, 0);
            Build(sequence);
            Push(sequence);
        }
        template<typename TAG>
        void Push(TAG tag, const Builder& Build, const bool alternative = false, const uint16_t scratchPadSize = 2048)
        {
            uint8_t* scratchPad = static_cast<uint8_t*>(ALLOCA(scratchPadSize));
            Payload sequence(scratchPad, scratchPadSize, 0);
            Build(sequence);
            Push(tag, sequence, alternative);
        }
        template<typename TYPE>
        void Push(const std::list<TYPE>& list, const uint16_t scratchPadSize = 2048)
        {
            if (list.size() != 0) {
                ASSERT(Free() >= (list.size() * sizeof(TYPE)));
                Push([&](Payload& sequence){
                    for (const auto& item : list) {
                        sequence.Push(item);
                    }
                }, scratchPadSize);
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
        void Pop(std::vector<TYPE>& vect, const uint16_t count) const
        {
            if (Available() >= (count * sizeof(TYPE))) {
                vect.reserve(count);

                uint16_t i = count;
                while (i-- > 0) {
                    TYPE item;
                    Pop(item);
                    vect.push_back(item);
                }
            } else {
                TRACE_L1("SDP: Truncated payload while reading a vector");
                _readerOffset = _writerOffset;
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
                TRACE_L1("SDP: Truncated payload while reading a list");
                _readerOffset = _writerOffset;
            }
        }
        template<typename TYPE>
        void Pop(use_descriptor_t, std::list<TYPE>& list, const uint16_t count) const
        {
            if (Available() >= (count * sizeof(TYPE))) {
                uint16_t i = count;
                while (i-- > 0) {
                    TYPE item;
                    Pop(use_descriptor, item);
                    list.push_back(item);
                }
}            else {
                TRACE_L1("SDP: Truncated payload while reading a list");
                _readerOffset = _writerOffset;
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
                        uuid = Bluetooth::UUID((_buffer[_readerOffset] << 24) | (_buffer[_readerOffset + 1] << 16)
                                                | (_buffer[_readerOffset + 2] << 8) | _buffer[_readerOffset + 3]);
                    } else {
                        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(size));
                        uint8_t i = size;
                        while (i-- > 0) {
                            buffer[i] = _buffer[_readerOffset++];
                        }
                        uuid = Bluetooth::UUID(buffer);
                    }
                    _readerOffset += size;
                } else {
                    TRACE_L1("SDP: Truncated payload while reading UUID");
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
                    TRACE_L1("SDP: Truncated payload while reading a sequence");
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
                TRACE_L1("SDP: Truncated payload while reading a sequence");
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
                TRACE_L1("SDP: Truncated payload while reading a buffer");
                _readerOffset = _writerOffset;
            }
        }
        void Pop(use_length_t, const Inspector& inspector) const
        {
            uint16_t size{};
            Pop(size);
            if (Available() >= size) {
                Payload sequence(&_buffer[_readerOffset], size);
                inspector(sequence);
                _readerOffset += size;
            } else {
                TRACE_L1("SDP: Truncated payload while reading a buffer");
                _readerOffset = _writerOffset;
            }
        }
        void Pop(use_length_t, Buffer& element) const
        {
            uint16_t size{};
            Pop(size);
            if (Available() >= size) {
                element.assign(&_buffer[_readerOffset], size);
                _readerOffset += size;
            } else {
                TRACE_L1("SDP: Truncated payload while reading a buffer");
                _readerOffset = _writerOffset;
            }
        }

    public:
        void PopAssign(use_descriptor_t, Payload& element) const
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

    class EXTERNAL PDU {
    public:
        static constexpr uint8_t HeaderSize = 5;
        static constexpr uint8_t MaxContinuationSize = 16;

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
            InProgress,
            SerializationFailed,
            DeserializationFailed,
        };

    public:
        PDU(const PDU&) = delete;
        PDU& operator=(const PDU&) = delete;
        ~PDU() = default;

        explicit PDU(const uint16_t bufferSize = SocketBufferSize)
            : _buffer(new uint8_t[bufferSize])
            , _payload(_buffer.get(), bufferSize, 0)
            , _type(Invalid)
            , _transactionId(~0)
            , _errorCode(Reserved)
        {
            ASSERT(bufferSize > (HeaderSize + (1 /* continuation length */) + MaxContinuationSize));
            ASSERT(_buffer.get() != nullptr);
        }

    public:
        string AsString() const
        {
#ifdef __DEBUG__
            const char* labels[] = { "Invalid", "ErrorResponse", "ServiceSearchRequest", "ServiceSearchResponse", "ServiceAttributeRequest",
                                   "ServiceAttributeResponse", "ServiceSearchAttributeRequest", "ServiceSearchAttributeResponse" };

            ASSERT(Type() <= ServiceSearchAttributeResponse);
            return (Core::Format("PDU #%d '%s'", _transactionId, labels[_type]));
#else
            return (Core::Format("PDU #%d type %d", _transactionId, _type));
#endif
        }

    public:
        bool IsValid() const {
            return ((Type() != Invalid) && (TransactionId() != static_cast<uint16_t>(~0)));
        }
        pduid Type() const {
            return (_type);
        }
        uint16_t TransactionId() const {
            return (_transactionId);
        }
        errorid Error() const {
            return (_errorCode);
        }
        uint16_t Capacity() const {
            return (_payload.Capacity());
        }

    public:
        void InspectPayload(const Payload::Inspector& inspectorCb) const
        {
            ASSERT(inspectorCb != nullptr);

            _payload.Rewind();
            inspectorCb(_payload);
        }

    public:
        void Reload() const
        {
            _payload.Rewind();
        }
        void Clear()
        {
            _payload.Clear();
            _type = Invalid;
            _transactionId = -1;
            _errorCode = Reserved;
        }
        void Set(const uint16_t transactionId, const pduid type, const Payload::Builder& buildCb)
        {
            _errorCode = SerializationFailed;
            _transactionId = transactionId;
            _type = type;
            _payload.Clear();

            if (buildCb != nullptr) {
                buildCb(_payload);
            }
        }

    public:
        uint16_t Serialize(uint8_t stream[], const uint16_t length) const;
        uint16_t Deserialize(const uint8_t stream[], const uint16_t length);

    private:
        std::unique_ptr<uint8_t[]> _buffer;
        Payload _payload;
        pduid _type;
        uint16_t _transactionId;
        mutable errorid _errorCode;
    }; // class PDU

    class EXTERNAL ClientSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint32_t CommunicationTimeout = 2000; /* 2 seconds. */

        class EXTERNAL Command : public Core::IOutbound, public Core::IInbound {
        public:
            class EXTERNAL Request : public PDU {
            public:
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;
                ~Request() = default;

                Request()
                    : PDU()
                    , _counter(~0)
                {
                }

            public:
                using PDU::Set;

                void Set(const PDU::pduid type, const Payload::Builder& buildCb = nullptr)
                {
                    Set(Counter(), type, buildCb);
                }

            private:
                uint16_t Counter() const {
                    return (++_counter);
                }

            private:
                mutable uint16_t _counter;
            }; // class Request

        public:
            class EXTERNAL Response : public PDU {
            public:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;
                ~Response() = default;

                Response()
                    : PDU()
                {
                }
            }; // class Response

        public:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;

            Command(ClientSocket& socket)
                : _request()
                , _response()
                , _socket(socket)
            {
            }
            ~Command() = default;

        public:
            template<typename... Args>
            void Set(Args&&... args)
            {
                _response.Clear();
                _request.Set(std::forward<Args>(args)...);
            }

        public:
            Response& Result() {
                return (_response);
            }
            const Response& Result() const
            {
                return (_response);
            }
            const Request& Call() const
            {
                return (_request);
            }
            bool IsValid() const
            {
                return (_request.IsValid());
            }

        private:
            void Reload() const override
            {
                _request.Reload();
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                ASSERT(stream != nullptr);

                const uint16_t result = _request.Serialize(stream, std::min(_socket.OutputMTU(), length));

                if (result != 0) {
                    CMD_DUMP("SDP client send", stream, result);
                }

                return (result);
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
            {
                ASSERT(stream != nullptr);

                CMD_DUMP("SDP client received", stream, length);

                return (_response.Deserialize(stream, length));
            }
            Core::IInbound::state IsCompleted() const override
            {
                return (Core::IInbound::COMPLETED);
            }

        private:
            Request _request;
            Response _response;
            ClientSocket& _socket;
        }; // class Command

    public:
        ClientSocket(const ClientSocket&) = delete;
        ClientSocket& operator=(const ClientSocket&) = delete;
        ~ClientSocket() = default;

        ClientSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED,
                        localNode, remoteNode, SocketBufferSize, SocketBufferSize)
            , _adminLock()
            , _omtu(0)
        {
        }
        ClientSocket(const SOCKET& connector, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED,
                        connector, remoteNode, SocketBufferSize, SocketBufferSize)
            , _adminLock()
            , _imtu(0)
            , _omtu(0)
        {
        }

    public:
        uint16_t InputMTU() const {
            return (_omtu);
        }
        uint16_t OutputMTU() const {
            return (_omtu);
        }

    private:
        virtual void Operational(const bool upAndRunning) = 0;

        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                struct l2cap_options options{};
                socklen_t len = sizeof(options);

                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_OPTIONS, &options, &len);

                ASSERT(options.omtu <= SendBufferSize());
                ASSERT(options.imtu <= ReceiveBufferSize());

                TRACE(Trace::Information, (_T("SDP channel input MTU: %d, output MTU: %d"), options.imtu, options.omtu));

                _omtu = options.omtu;
                _imtu = options.imtu;

                Operational(true);

            } else {
                Operational(false);

                _omtu = 0;
                _imtu = 0;
            }
        }

        uint16_t Deserialize(const uint8_t stream[] VARIABLE_IS_NOT_USED, const uint16_t length) override
        {
            if (length != 0) {
                TRACE_L1("SDP: Unexpected data for deserialization [%d bytes]", length);
                CMD_DUMP("SDP client received unexpected", stream, length);
            }

            return (length);
        }

     private:
        Core::CriticalSection _adminLock;
        uint16_t _imtu;
        uint16_t _omtu;
    }; // class ClientSocket

    class EXTERNAL ServerSocket : public ClientSocket {
    public:
        class EXTERNAL ResponseHandler {
        public:
            ResponseHandler(const ResponseHandler&) = default;
            ResponseHandler& operator=(const ResponseHandler&) = default;
            ~ResponseHandler() = default;

            ResponseHandler(const std::function<void(const PDU::pduid newId, const Payload::Builder&)>& acceptor,
                            const std::function<void(const PDU::errorid)>& rejector)
                : _acceptor(acceptor)
                , _rejector(rejector)
            {
            }

        public:
            void operator ()(const PDU::pduid newId, const Payload::Builder& buildCb = nullptr) const
            {
                _acceptor(newId, buildCb);
            }
            void operator ()(const PDU::errorid result) const
            {
                _rejector(result);
            }

        private:
            std::function<void(const PDU::pduid newId, const Payload::Builder& buildCb)> _acceptor;
            std::function<void(const PDU::errorid)> _rejector;
        }; // class ResponseHandler

    private:
        class EXTERNAL Request : public PDU {
        public:
            Request(const Request&) = delete;
            Request& operator=(const Request&) = delete;
            ~Request() = default;

            Request()
                : PDU()
            {
            }
        }; // class Request

        class EXTERNAL Response : public PDU, public Core::IOutbound  {
        public:
            Response(const Response&) = delete;
            Response& operator=(const Response&) = delete;
            Response(ServerSocket& socket)
                : PDU()
                , _socket(socket)
            {
            }
            ~Response() = default;

        public:
            using PDU::Set;

            void Set(const uint16_t transactionId, PDU::errorid code)
            {
                Set(transactionId, PDU::ErrorResponse, [code](Payload& payload) {
                    payload.Push(code);
                });
            }

        public:
            const ServerSocket& Socket() const {
                return (_socket);
            }

        public:
            void Reload() const override
            {
                PDU::Reload();
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                const uint16_t result = PDU::Serialize(stream, std::min(_socket.OutputMTU(), length));

                if (result != 0) {
                    CMD_DUMP("SDP server sent", stream, result);
                }

                return (result);
            }

        private:
            ServerSocket& _socket;
        }; // class Response

    public:
        ServerSocket(const ServerSocket&) = delete;
        ServerSocket& operator=(const ServerSocket&) = delete;

        ServerSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : ClientSocket(localNode, remoteNode)
            , _request()
            , _response(*this)
        {
        }
        ServerSocket(const SOCKET& connector, const Core::NodeId& remoteNode)
            : ClientSocket(connector, remoteNode)
            , _request()
            , _response(*this)
        {
        }
        ~ServerSocket() = default;

    public:
        uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
        {
            uint16_t result = 0;

            CMD_DUMP("SDP server received", stream, length);

            result = _request.Deserialize(stream, length);

            if (result != 0) {
                Received(_request, _response);
                _request.Clear();
            }

            return (result);
        }

    private:
        void Received(const Request& request, Response& response)
        {
            if (request.IsValid() == true) {
                OnPDU(*this, request, ResponseHandler(
                    [&](const PDU::pduid newId, const Payload::Builder& buildCb) {
                        TRACE_L1("SDP server: accepting %s", request.AsString().c_str());
                        response.Set(request.TransactionId(), newId, buildCb);
                    },
                    [&](const PDU::errorid result) {
                        TRACE_L1("SDP server: rejecting %s, reason: %d", request.AsString().c_str(), result);
                        response.Set(request.TransactionId(), result);
                    }));
            } else {
                TRACE_L1("SDP server: Invalid request!");
                response.Set(request.TransactionId(), PDU::InvalidRequestSyntax);
            }

            Send(CommunicationTimeout, response, nullptr, nullptr);
        }

    protected:
        virtual void OnPDU(const ServerSocket& socket, const PDU& request, const ResponseHandler& handler) = 0;

    private:
        Request _request;
        Response _response;
    };

} // namespace SDP

} // namespace Bluetooth

} // namespace Thunder

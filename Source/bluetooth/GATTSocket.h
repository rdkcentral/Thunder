/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

namespace WPEFramework {

namespace Bluetooth {

    class Attribute {
    public:
        enum type {
            NIL = 0x00,
            INTEGER_UNSIGNED = 0x08,
            INTEGER_SIGNED = 0x10,
            UUID = 0x18,
            TEXT = 0x20,
            BOOLEAN = 0x28,
            SEQUENCE = 0x30,
            ALTERNATIVE = 0x38,
            URL = 0x40
        };

        Attribute()
            : _type(~0)
            , _offset(0)
            , _length(~0)
            , _bufferSize(64)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_bufferSize)))
        {
        }
        Attribute(const uint16_t size, const uint8_t stream[])
            : _type(~0)
            , _offset(0)
            , _length(~0)
            , _bufferSize(size)
            , _buffer(reinterpret_cast<uint8_t*>(::malloc(_bufferSize))) {
            Deserialize(size, stream);
        }
        ~Attribute()
        {
            if (_buffer != nullptr) {
                ::free(_buffer);
            }
        }

    public:
        void Clear()
        {
            _type = ~0;
            _offset = 0;
            _length = ~0;
        }
        bool IsLoaded() const
        {
            return (_offset == static_cast<uint32_t>(~0));
        }
        type Type() const
        {
            return (static_cast<type>(_type & 0xF8));
        }
        uint32_t Length() const
        {
            return ((_type & 0x3) <= 4 ? (_type == 0 ? 0 : 1 << (_type & 0x03)) : _length);
        }
        template <typename TYPE>
        void Load(TYPE& value) const
        {
            value = 0;
            for (uint8_t index = 0; index < sizeof(TYPE); index++) {
                value = (value << 8) | _buffer[index];
            }
        }
        void Load(bool& value) const
        {
            value = (_buffer[0] != 0);
        }
        void Load(string& value) const
        {
            value = string(reinterpret_cast<const char*>(_buffer), _length);
        }

        uint16_t Deserialize(const uint16_t size, const uint8_t stream[]);

    private:
        uint8_t _type;
        uint32_t _offset;
        uint32_t _length;
        uint32_t _bufferSize;
        uint8_t* _buffer;
    };

    class GATTSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    public:
        static constexpr uint8_t LE_ATT_CID = 4;

    private:
        GATTSocket(const GATTSocket&) = delete;
        GATTSocket& operator=(const GATTSocket&) = delete;

        static constexpr uint8_t ATT_OP_ERROR = 0x01;
        static constexpr uint8_t ATT_OP_MTU_REQ = 0x02;
        static constexpr uint8_t ATT_OP_MTU_RESP = 0x03;
        static constexpr uint8_t ATT_OP_FIND_INFO_REQ = 0x04;
        static constexpr uint8_t ATT_OP_FIND_INFO_RESP = 0x05;
        static constexpr uint8_t ATT_OP_FIND_BY_TYPE_REQ = 0x06;
        static constexpr uint8_t ATT_OP_FIND_BY_TYPE_RESP = 0x07;
        static constexpr uint8_t ATT_OP_READ_BY_TYPE_REQ = 0x08;
        static constexpr uint8_t ATT_OP_READ_BY_TYPE_RESP = 0x09;
        static constexpr uint8_t ATT_OP_READ_REQ = 0x0A;
        static constexpr uint8_t ATT_OP_READ_RESP = 0x0B;
        static constexpr uint8_t ATT_OP_READ_BLOB_REQ = 0x0C;
        static constexpr uint8_t ATT_OP_READ_BLOB_RESP = 0x0D;
        static constexpr uint8_t ATT_OP_READ_MULTI_REQ = 0x0E;
        static constexpr uint8_t ATT_OP_READ_MULTI_RESP = 0x0F;
        static constexpr uint8_t ATT_OP_READ_BY_GROUP_REQ = 0x10;
        static constexpr uint8_t ATT_OP_READ_BY_GROUP_RESP = 0x11;
        static constexpr uint8_t ATT_OP_WRITE_REQ = 0x12;
        static constexpr uint8_t ATT_OP_WRITE_RESP = 0x13;
        static constexpr uint8_t ATT_OP_HANDLE_NOTIFY = 0x1B;


        static constexpr uint8_t ATT_ECODE_INVALID_HANDLE = 0x01;
        static constexpr uint8_t ATT_ECODE_READ_NOT_PERM = 0x02;
        static constexpr uint8_t ATT_ECODE_WRITE_NOT_PERM = 0x03;
        static constexpr uint8_t ATT_ECODE_INVALID_PDU = 0x04;
        static constexpr uint8_t ATT_ECODE_AUTHENTICATION = 0x05;
        static constexpr uint8_t ATT_ECODE_REQ_NOT_SUPP = 0x06;
        static constexpr uint8_t ATT_ECODE_INVALID_OFFSET = 0x07;
        static constexpr uint8_t ATT_ECODE_AUTHORIZATION = 0x08;
        static constexpr uint8_t ATT_ECODE_PREP_QUEUE_FULL = 0x09;
        static constexpr uint8_t ATT_ECODE_ATTR_NOT_FOUND = 0x0A;
        static constexpr uint8_t ATT_ECODE_ATTR_NOT_LONG = 0x0B;
        static constexpr uint8_t ATT_ECODE_INSUFF_ENCR_KEY_SIZE = 0x0C;
        static constexpr uint8_t ATT_ECODE_INVAL_ATTR_VALUE_LEN = 0x0D;
        static constexpr uint8_t ATT_ECODE_UNLIKELY = 0x0E;
        static constexpr uint8_t ATT_ECODE_INSUFF_ENC = 0x0F;
        static constexpr uint8_t ATT_ECODE_UNSUPP_GRP_TYPE = 0x10;
        static constexpr uint8_t ATT_ECODE_INSUFF_RESOURCES = 0x11;

        /* Application error */
        static constexpr uint8_t ATT_ECODE_IO = 0x80;
        static constexpr uint8_t ATT_ECODE_TIMEOUT = 0x81;
        static constexpr uint8_t ATT_ECODE_ABORTED = 0x82;

        class CommandSink : public Core::IOutbound::ICallback, public Core::IOutbound, public Core::IInbound
        {
        public:
            CommandSink() = delete;
            CommandSink(const CommandSink&) = delete;
            CommandSink& operator= (const CommandSink&) = delete;

            CommandSink(GATTSocket& parent, const uint16_t preferredMTU) : _parent(parent), _mtu(preferredMTU) {
                Reload();
            }
            virtual ~CommandSink() {
            }

        public:
            inline uint16_t MTU () const {
                return (_mtu & 0xFFFF);
            }
            inline bool HasMTU() const {
                return (_mtu <= 0xFFFF);
            }
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) override
            {
                _parent.Completed(data, error_code);
            }
                
            virtual void Reload() const override
            {
               _mtu = ((_mtu & 0xFFFF) | 0xFF000000);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                uint16_t result = 0;
                if ((_mtu >> 24) == 0xFF) {
                    ASSERT(length >= 3);
                    stream[0] = ATT_OP_MTU_REQ;
                    stream[1] = (_mtu & 0xFF);
                    stream[2] = ((_mtu >> 8) & 0xFF);
                    _mtu = ((_mtu & 0xFFFF) | 0xF0000000);
                    result = 3;
                }
                return (result);
            }
            virtual Core::IInbound::state IsCompleted() const override
            {
                return ((_mtu >> 24) == 0x00 ? Core::IInbound::COMPLETED : Core::IInbound::INPROGRESS);
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override {
                uint16_t result = 0;

                // See if we need to retrigger..
                if (stream[0] == ATT_OP_MTU_RESP) {
                    _mtu = ((stream[2] << 8) | stream[1]);
                    result = length;
                } else if ((stream[0] == ATT_OP_ERROR) && (stream[1] == ATT_OP_MTU_RESP)) {
                    TRACE_L1("Error on receiving MTU: [%d]", stream[2]);
                    result = length;
                } else {
                    TRACE_L1("Unexpected L2CapSocket message. Expected: %d, got %d [%d]", ATT_OP_MTU_RESP, stream[0], stream[1]);
                    result = 0;
                } 
                return (result);
            }
 
        private:
            GATTSocket& _parent;
            mutable uint32_t _mtu;
        };

    public:
        static constexpr uint32_t CommunicationTimeOut = 2000; /* 2 seconds. */

        class Command : public Core::IOutbound, public Core::IInbound {
        private:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;

            static constexpr uint16_t BLOCKSIZE = 64;

            class Exchange {
            private:
                Exchange(const Exchange&) = delete;
                Exchange& operator=(const Exchange&) = delete;

            public:
                Exchange()
                    : _offset(~0)
                    , _size(0)
                {
                }
                ~Exchange()
                {
                }

            public:
                bool IsSend() const
                {
                    return (_offset >= _size);
                }
                void Reload() const
                {
                    _offset = 0;
                }
                void ReadByGroupType(const uint16_t start)
                {
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    Reload();
                }
                uint8_t ReadByGroupType(const uint16_t start, const uint16_t end, const UUID& id)
                {
                    _buffer[0] = ATT_OP_READ_BY_GROUP_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), id.Data(), id.Length());
                    _size = id.Length() + 5;
                    _end = end;
                    return (ATT_OP_READ_BY_GROUP_RESP);
                }
                void FindByType(const uint16_t start)
                {
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    Reload();
                }
                uint8_t FindByType(const uint16_t start, const uint16_t end, const UUID& id, const uint8_t length, const uint8_t data[])
                {
                    _buffer[0] = ATT_OP_FIND_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _buffer[5] = (id.Short() & 0xFF);
                    _buffer[6] = (id.Short() >> 8) & 0xFF;
                    ::memcpy(&(_buffer[7]), data, length);
                    _size = 7 + length;
                    _end = end;
                    return (ATT_OP_FIND_BY_TYPE_RESP);
                }
                uint8_t FindByType(const uint16_t start, const uint16_t end, const UUID& id, const uint16_t handle)
                {
                    _buffer[0] = ATT_OP_FIND_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _buffer[5] = (id.Short() & 0xFF);
                    _buffer[6] = (id.Short() >> 8) & 0xFF;
                    _buffer[7] = (handle & 0xFF);
                    _buffer[8] = (handle >> 8) & 0xFF;
                    _size = 9;
                    _end = end;
                    return (ATT_OP_FIND_BY_TYPE_RESP);
                }
                void ReadByType(const uint16_t start)
                {
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    Reload();
                }
                uint8_t ReadByType(const uint16_t start, const uint16_t end, const UUID& id)
                {
                    _buffer[0] = ATT_OP_READ_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), id.Data(), id.Length());
                    _size = id.Length() + 5;
                    _end = end;
                    return (ATT_OP_READ_BY_TYPE_RESP);
                }
                void FindInformation(const uint16_t start)
                {
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    Reload();
                }
                uint8_t FindInformation(const uint16_t start, const uint16_t end)
                {
                    _buffer[0] = ATT_OP_FIND_INFO_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _size = 5;
                    _end = end;
                    return (ATT_OP_FIND_INFO_RESP);
                }
                uint8_t Read(const uint16_t handle)
                {
                    _buffer[0] = ATT_OP_READ_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    _size = 3;
                    _end = 0;
                    return (ATT_OP_READ_RESP);
                }
                uint8_t ReadBlob(const uint16_t handle, const uint16_t offset)
                {
                    _buffer[0] = ATT_OP_READ_BLOB_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    _buffer[3] = (offset & 0xFF);
                    _buffer[4] = (offset >> 8) & 0xFF;
                    _size = 5;
                    _end = 0;
                    return (ATT_OP_READ_BLOB_RESP);
                }
                uint8_t Write(const uint16_t handle, const uint8_t length, const uint8_t data[])
                {
                    _buffer[0] = ATT_OP_WRITE_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    ::memcpy(&(_buffer[3]), data, length);
                    _size = 3 + length;
                    _end = 0;
                    return (ATT_OP_WRITE_RESP);
                }
                uint16_t Serialize(uint8_t stream[], const uint16_t length) const
                {
                    uint16_t result = std::min(static_cast<uint16_t>(_size - _offset), length);
                    if (result > 0) {
                        ::memcpy(stream, &(_buffer[_offset]), result);
                        _offset += result;

                        // printf("L2CAP send [%d]: ", result);
                        // for (uint8_t index = 0; index < (result - 1); index++) { printf("%02X:", stream[index]); } printf("%02X\n", stream[result - 1]);
                    }

                    return (result);
                }
                uint16_t Handle() const
                {
                    return (((_buffer[0] == ATT_OP_READ_BLOB_REQ) || (_buffer[0] == ATT_OP_READ_REQ)) ? ((_buffer[2] << 8) | _buffer[1]) : 0);
                }
                uint16_t Offset() const
                {
                    return ((_buffer[0] == ATT_OP_READ_BLOB_REQ) ? ((_buffer[4] << 8) | _buffer[3]) : 0);
                }
                uint16_t End() const {
                    return (_end);
                }

            private:
                mutable uint16_t _offset;
                uint16_t _end;
                uint8_t _size;
                uint8_t _buffer[BLOCKSIZE];
            };

        public:
            class Response {
            private:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;

                typedef std::pair<uint16_t, std::pair<uint16_t, uint16_t> > Entry;

            public:
                Response()
                    : _maxSize(BLOCKSIZE)
                    , _loaded(0)
                    , _result()
                    , _iterator()
                    , _storage(reinterpret_cast<uint8_t*>(::malloc(_maxSize)))
                    , _preHead(true)
                    , _min(0x0001)
                    , _max(0xFFFF)
                    , _type(ATT_OP_ERROR)
                {
                }
                ~Response()
                {
                    if (_storage != nullptr) {
                        ::free(_storage);
                    }
                }

            public:
                void Type(const uint8_t response) {
                    _type = response;
                }
                void Clear()
                {
                    Reset();
                    _result.clear();
                    _loaded = 0;
                    _min = 0xFFFF;
                    _max = 0x0001;
                    _type = ATT_OP_ERROR;
                }
                uint8_t Error() const {
                    return (_type == ATT_OP_ERROR ? static_cast<uint8_t>(_min) : 0);
                }
                void Reset()
                {
                    _preHead = true;
                }
                bool IsValid() const
                {
                    return ((_preHead == false) && (_iterator != _result.end()));
                }
                bool Next()
                {
                    if (_preHead == true) {
                        _preHead = false;
                        _iterator = _result.begin();
                    } else {
                        _iterator++;
                    }
                    return (_iterator != _result.end());
                }
                uint16_t Handle() const
                {
                    return (_iterator->first);
                }
                uint16_t MTU() const
                {
                    return ((_storage[0] << 8) | _storage[1]);
                }
                uint16_t Group() const
                {
                    return (_type == ATT_OP_READ_BY_TYPE_RESP ? ((_storage[_iterator->second.second + 2] << 8) | (_storage[_iterator->second.second + 1])) : _iterator->second.first);
                }
                UUID Attribute() const {
                    uint8_t        offset = (_type == ATT_OP_READ_BY_TYPE_RESP ? 3 : 0);
                    uint16_t       length = Delta() - offset;
                    const uint8_t* data   = &(_storage[_iterator->second.second + offset]);
 
                    if ((length != 2) && (length != 16)) {
                        TRACE_L1("**** Unexpected Attribute length [%d] !!!!", length);
                    }

                    return (length == 2  ? UUID(static_cast<uint16_t>((data[1] << 8) | (*data))) :
                            length == 16 ? UUID(data) :
                                           UUID());
                }
                uint8_t Rights() const {
                    return (_storage[_iterator->second.second]);
                }
                uint16_t Count() const
                {
                    return (_result.size());
                }
                bool Empty() const
                {
                    return (_result.empty());
                }
                uint16_t Length() const
                {
                    uint16_t result = _loaded;
                    if (IsValid() == true) {
                        result = Delta();
                    }
                    return (result);
                }
                const uint8_t* Data() const
                {
                    return (IsValid() == true ? &(_storage[_iterator->second.second]) : (((_result.size() <= 1) && (_loaded > 0)) ? _storage : nullptr));
                }
                uint16_t Min() const {
                    return(_min);
                }
                uint16_t Max() const {
                    return(_max);
                }

            private:
                friend class Command;
                uint16_t Delta() const {
                    std::list<Entry>::iterator next(_iterator);
                    return (++next == _result.end() ? (_loaded - _iterator->second.second) : (next->second.second - _iterator->second.second));
                }
                void SetMTU(const uint16_t MTU)
                {
                    _storage[0] = (MTU >> 8) & 0xFF;
                    _storage[1] = MTU & 0xFF;
                }
                void Add(const uint16_t handle, const uint16_t group)
                {
                    if (_min > handle)
                        _min = handle;
                    if (_max < group)
                        _max = group;
                    _result.emplace_back(Entry(handle, std::pair<uint16_t,uint16_t>(group,_loaded)));
                }
                void Add(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
                {
                    if (_min > handle)
                        _min = handle;
                    if (_max < handle)
                        _max = handle;

                    _result.emplace_back(Entry(handle, std::pair<uint16_t,uint16_t>(0,_loaded)));
                    Extend(length, buffer);
                }
                void Add(const uint16_t handle, const uint16_t group, const uint8_t length, const uint8_t buffer[])
                {
                    if (_min > handle)
                        _min = handle;
                    if (_max < group)
                        _max = group;
                    _result.emplace_back(Entry(handle, std::pair<uint16_t,uint16_t>(group,_loaded)));
                    Extend(length, buffer);
                }
                void Extend(const uint8_t length, const uint8_t buffer[])
                {
                    if (length > 0) {
                        if ((_loaded + length) > _maxSize) {
                            _maxSize = ((((_loaded + length) / BLOCKSIZE) + 1) * BLOCKSIZE);
                            _storage = reinterpret_cast<uint8_t*>(::realloc(_storage, _maxSize));
                        }

                        ::memcpy(&(_storage[_loaded]), buffer, length);
                        _loaded += length;
                    }
                }
                uint16_t Offset() const
                {
                    return (_result.size() != 0 ? _loaded : _loaded - _result.back().second.second);
                }

            private:
                uint16_t _maxSize;
                uint16_t _loaded;
                std::list<Entry> _result;
                std::list<Entry>::iterator _iterator;
                uint8_t* _storage;
                bool _preHead;
                uint16_t _min;
                uint16_t _max;
                uint8_t _type;
            };

        public:
            Command()
                : _error(~0)
                , _mtu(0)
                , _id(0)
                , _frame()
                , _response()
            {
            }
            virtual ~Command()
            {
            }

        public:
            Response& Result() {
                return (_response);
            }
            const Response& Result() const {
                return (_response);
            }
            uint8_t Id() const {
                return(_id);
            }
            uint16_t Error() const {
                return(_error);
            }
            void FindInformation(const uint16_t min, const uint16_t max)
            {
                _response.Clear();
                _error = ~0;
                _id = _frame.FindInformation(min, max);
            }
            void ReadByGroupType(const uint16_t min, const uint16_t max, const UUID& uuid)
            {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadByGroupType(min, max, uuid);
            }
            void ReadByType(const uint16_t min, const uint16_t max, const UUID& uuid)
            {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadByType(min, max, uuid);
            }
            void ReadBlob(const uint16_t handle)
            {
                _response.Clear();
                _error = ~0;
                _id = _frame.ReadBlob(handle, 0);
            }
            void Read(const uint16_t handle)
            {
                _response.Clear();
                _error = ~0;
                _id = _frame.Read(handle);
            }
            void Write(const uint16_t handle, const uint8_t length, const uint8_t data[])
            {
                _response.Clear();
                _response.Extend(length, data);
                _error = ~0;
                _id = _frame.Write(handle, length, data);
            }
            void FindByType(const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[])
            {
                ASSERT(uuid.HasShort() == true);
                _response.Clear();
                _error = ~0;
                _id = _frame.FindByType(min, max, uuid, length, data);
            }
            void FindByType(const uint16_t min, const uint16_t max, const UUID& uuid, const uint16_t handle)
            {
                ASSERT(uuid.HasShort() == true);
                _response.Clear();
                _error = ~0;
                _id = _frame.FindByType(min, max, uuid, handle);
            }
 
            void Error(const uint32_t error_code) {
                _error = error_code;
            }
            void MTU(const uint16_t mtu) {
                _mtu = mtu;
            }
 
        private:
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override;
            virtual void Reload() const override
            {
                _frame.Reload();
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const override
            {
                return (_frame.Serialize(stream, length));
            }
            virtual Core::IInbound::state IsCompleted() const override
            {
                return (_error != static_cast<uint16_t>(~0) ? Core::IInbound::COMPLETED : (_frame.IsSend() ? Core::IInbound::INPROGRESS : Core::IInbound::RESEND));
            }

        private:
            uint16_t _error;
            uint16_t _mtu;
            uint8_t  _id;
            Exchange _frame;
            Response _response;
        };

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
                , _handler(handler) {
            }
            ~Entry() {
            }

        public:
            Command& Cmd() {
                return(_cmd);
            }
            uint32_t WaitTime() const {
                return(_waitTime);
            }
            bool operator== (const Core::IOutbound* rhs) const {
                return (rhs == &_cmd);
            }
            bool operator!= (const Core::IOutbound* rhs) const {
                return(!operator==(rhs));
            }
            void Completed(const uint32_t error_code) {
                _cmd.Error(error_code);
                _handler(_cmd);
            }

        private:
            uint32_t _waitTime;
            Command& _cmd;
            Handler _handler;
        };

    public:
        GATTSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t maxMTU)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, maxMTU, maxMTU)
            , _adminLock()
            , _sink(*this, maxMTU)
            , _queue()
        {
        }
        virtual ~GATTSocket()
        {
        }

    public:
        bool Security(const uint8_t level);

        inline uint16_t MTU() const {
            return (_sink.MTU());
        }
        void Execute(const uint32_t waitTime, Command& cmd, const Handler& handler)
        {
            cmd.MTU(_sink.MTU());
            _adminLock.Lock();
            _queue.emplace_back(waitTime, cmd, handler);
            if (_queue.size() ==  1) {
                Send(waitTime, cmd, &_sink, &cmd);
            }
            _adminLock.Unlock();
        }
        void Revoke(const Command& cmd)
        {
            Revoke(cmd);
        }

    private:
        virtual void Notification(const uint16_t handle, const uint8_t[], const uint16_t) = 0;
        virtual void Operational() = 0;

        void StateChange() override;

        uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t availableData) override {
            uint32_t result = 0;

            if (availableData >= 1) {
                const uint8_t& opcode = dataFrame[0];

                if ((opcode == ATT_OP_HANDLE_NOTIFY) && (availableData >= 3)) {
                    uint16_t handle = ((dataFrame[2] << 8) | dataFrame[1]);
                    Notification(handle, &dataFrame[3], (availableData - 3));
                    result = availableData;
                }
                else {
                    printf ("**** Unexpected data, TYPE [%02X] !!!!\n", dataFrame[0]);
                }
            }
            else {
                TRACE_L1("**** Unexpected data for deserialization [%d] !!!!", availableData);
            }

            return (result);
        }
        void Completed(const Core::IOutbound& data, const uint32_t error_code) 
        {
            if ( (&data == &_sink) && (_sink.HasMTU() == true) ) {
                Operational();
            }
            else {
                _adminLock.Lock();

                if ((_queue.size() == 0) || (*(_queue.begin()) != &data)) {
                    ASSERT (false && _T("Always the first one should be the one to be handled!!"));
                }
                else {
                    // Command completion...
                    _queue.begin()->Completed(error_code);
                    _queue.erase(_queue.begin());

                    if (_queue.size() > 0) {
                        Entry& entry(*(_queue.begin()));
                        Command& cmd (entry.Cmd());

                        Send(entry.WaitTime(), cmd, &_sink, &cmd);
                    }
                }

                _adminLock.Unlock();
            }
        }

    private:
        Core::CriticalSection _adminLock;
        CommandSink _sink;
        std::list<Entry> _queue;
        uint32_t _mtuSize;
        struct l2cap_conninfo _connectionInfo;
    };

} // namespace Bluetooth

} // namespace WPEFramework

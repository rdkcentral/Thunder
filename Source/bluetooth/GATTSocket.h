#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

    class UUID {
    private:
        static const uint8_t BASE[];

    public:
        UUID(const uint16_t uuid)
        {
            _uuid[0] = 2;
            ::memcpy(&(_uuid[1]), BASE, sizeof(_uuid) - 3);
            _uuid[15] = (uuid & 0xFF);
            _uuid[16] = (uuid >> 8) & 0xFF;
        }
        UUID(const uint8_t uuid[16])
        {
            ::memcpy(&(_uuid[1]), uuid, 16);

            // See if this contains the Base, cause than it can be a short...
            if (::memcmp(BASE, uuid, 14) == 0) {
                _uuid[0] = 2;
            }
            else {
                _uuid[0] = 16;
            }
        }
        UUID(const UUID& copy)
        {
            ::memcpy(_uuid, copy._uuid, copy._uuid[0] + 1);
        }
        ~UUID()
        {
        }

        UUID& operator=(const UUID& rhs)
        {
            ::memcpy(_uuid, rhs._uuid, rhs._uuid[0] + 1);
            return (*this);
        }

    public:
        uint16_t Short() const
        {
            ASSERT(_uuid[0] == 2);
            return ((_uuid[16] << 8) | _uuid[15]);
        }
        bool operator==(const UUID& rhs) const
        {
            return ((rhs._uuid[0] == _uuid[0]) && 
                    ((_uuid[0] == 2) ? ((rhs._uuid[15] == _uuid[15]) && (rhs._uuid[16] == _uuid[16])) : 
                                       (::memcmp(_uuid, rhs._uuid, _uuid[0] + 1) == 0)));
        }
        bool operator!=(const UUID& rhs) const
        {
            return !(operator==(rhs));
        }
        bool HasShort() const
        {
            return (_uuid[0] == 2);
        }
        uint8_t Length() const
        {
            return (_uuid[0]);
        }
        const uint8_t* Data() const
        {
             return (_uuid[0] == 2 ? &(_uuid[15]) :  &(_uuid[1]));
        }
        string ToString(const bool full = false) const {

            // 00002a23-0000-1000-8000-00805f9b34fb
            static const TCHAR hexArray[] = "0123456789abcdef";

            uint8_t index = 0;
            string result;

            if ((HasShort() == false) || (full == true)) {
                result.resize(36);
                for (uint8_t byte = 12 + 4; byte > 12; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 10 + 2; byte > 10; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 8 + 2; byte > 8; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 6 + 2; byte > 6; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 0 + 6; byte > 0; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
            }
            else {
                result.resize(4);

                for (uint8_t byte = 14 + 2; byte > 14; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
            }
            return (result);
        }

    private:
        uint8_t _uuid[17];
    };

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
                    printf(_T("Error on receiving MTU: [%d]\n"), stream[2]);
                    result = length;
                } else {
                    printf(_T("Unexpected L2CapSocket message. Expected: %d, got %d [%d]\n"), ATT_OP_MTU_RESP, stream[0], stream[1]);
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
                uint8_t ReadByGroupType(const uint16_t start, const uint16_t end, const UUID& id)
                {
                    _buffer[0] = ATT_OP_READ_BY_GROUP_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), id.Data(), id.Length());
                    _size = id.Length() + 5;
                    return (ATT_OP_READ_BY_GROUP_RESP);
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
                    return (ATT_OP_FIND_BY_TYPE_RESP);
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
                    return (ATT_OP_READ_BY_TYPE_RESP);
                }
                uint8_t FindInformation(const uint16_t start, const uint16_t end)
                {
                    _buffer[0] = ATT_OP_FIND_INFO_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    _size = 5;
                    return (ATT_OP_FIND_INFO_RESP);
                }
                uint8_t Read(const uint16_t handle)
                {
                    _buffer[0] = ATT_OP_READ_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    _size = 3;
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
                    return (ATT_OP_READ_BLOB_RESP);
                }
                uint8_t Write(const uint16_t handle, const uint8_t length, const uint8_t data[])
                {
                    _buffer[0] = ATT_OP_WRITE_REQ;
                    _buffer[1] = (handle & 0xFF);
                    _buffer[2] = (handle >> 8) & 0xFF;
                    ::memcpy(&(_buffer[3]), data, length);
                    _size = 3 + length;
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

            private:
                mutable uint16_t _offset;
                uint8_t _size;
                uint8_t _buffer[64];
            };

        public:
            class Response {
            private:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;

                static constexpr uint16_t BLOCKSIZE = 64;
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
                {
                }
                ~Response()
                {
                    if (_storage != nullptr) {
                        ::free(_storage);
                    }
                }

            public:
                void Clear()
                {
                    Reset();
                    _result.clear();
                    _loaded = 0;
                    _min = 0xFFFF;
                    _max = 0x0001;
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
                    return (_iterator->second.first);
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
                        std::list<Entry>::iterator next(_iterator);
                        result = (++next == _result.end() ? (_loaded - _iterator->second.second) : (next->second.second - _iterator->second.second));
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
        inline uint16_t MTU() const {
            return (_sink.MTU());
        }
        bool Security(const uint8_t level, const uint8_t keySize);

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
        virtual void Operational() = 0;
        virtual void StateChange() override;
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

    class Profile {
    private:
        static constexpr uint16_t PRIMARY_SERVICE_UUID = 0x2800;
        static constexpr uint16_t CHARACTERISTICS_UUID = 0x2803;

    public:
        class Service {
        public:
            enum type : uint16_t {
                GenericAccess               = 0x1800,
                AlertNotificationService    = 0x1811,
                AutomationIO                = 0x1815,
                BatteryService              = 0x180F,
                BinarySensor                = 0x183B,
                BloodPressure               = 0x1810,
                BodyComposition             = 0x181B,
                BondManagementService       = 0x181E,
                ContinuousGlucoseMonitoring = 0x181F,
                CurrentTimeService          = 0x1805,
                CyclingPower                = 0x1818,
                CyclingSpeedAndCadence      = 0x1816,
                DeviceInformation           = 0x180A,
                EmergencyConfiguration      = 0x183C,
                EnvironmentalSensing        = 0x181A,
                FitnessMachine              = 0x1826,
                GenericAttribute            = 0x1801,
                Glucose                     = 0x1808,
                HealthThermometer           = 0x1809,
                HeartRate                   = 0x180D,
                HTTPProxy                   = 0x1823,
                HumanInterfaceDevice        = 0x1812,
                ImmediateAlert              = 0x1802,
                IndoorPositioning           = 0x1821,
                InsulinDelivery             = 0x183A,
                InternetProtocolSupport     = 0x1820,
                LinkLoss                    = 0x1803,
                LocationAndNavigation       = 0x1819,
                MeshProvisioningService     = 0x1827,
                MeshProxyService            = 0x1828,
                NextDSTChangeService        = 0x1807,
                ObjectTransferService       = 0x1825,
                PhoneAlertStatusService     = 0x180E,
                PulseOximeterService        = 0x1822,
                ReconnectionConfiguration   = 0x1829,
                ReferenceTimeUpdateService  = 0x1806,
                RunningSpeedAndCadence      = 0x1814,
                ScanParameters              = 0x1813,
                TransportDiscovery          = 0x1824,
                TxPower                     = 0x1804,
                UserData                    = 0x181C,
                WeightScale                 = 0x181D,
            };

        public:
            typedef Core::IteratorType< const std::list<Attribute>, const Attribute&, std::list<Attribute>::const_iterator> Iterator;

            Service(const Service&) = delete;
            Service& operator= (const Service&) = delete;

            Service(uint16_t serviceId, const uint16_t handle, const uint16_t group)
                : _index(handle + 1)
                , _group(group)
                , _serviceId(serviceId) {
            }
            Service(const uint8_t serviceId[16], const uint16_t handle, const uint16_t group)
                : _index(handle + 1)
                , _group(group)
                , _serviceId(serviceId) {
            }
            ~Service() {
            }

        public:
            const UUID& Type() const {
                return (_serviceId);
            }
            string Name() const {
                string result;
                if (_serviceId.HasShort() == false) {
                    result = _serviceId.ToString();
                }
                else {
                    Core::EnumerateType<type> value(static_cast<type>(_serviceId.Short()));
                    result = (value.IsSet() == true ? string(value.Data()) : _serviceId.ToString(false));
                }
                return (result);
            }
            Iterator Characteristics() const {
                return (Iterator(_characteristics));
            }

        private:
            friend class Profile;
            uint16_t Handle () const {
                return (_index <= _group ? _index : 0);
            }
            void Characteristic (const uint16_t length, const uint8_t buffer[]) {
                _characteristics.emplace_back(length, buffer);
                _index++;
            }

        private:
            uint16_t _index;
            uint16_t _group;
            UUID _serviceId;
            std::list<Attribute> _characteristics;
        };

    public:
        typedef std::function<void(const uint32_t)> Handler;
        typedef Core::IteratorType< const std::list<Service>, const Service&, std::list<Service>::const_iterator> Iterator;

        Profile (const Profile&) = delete;
        Profile& operator= (const Profile&) = delete;

        Profile(const bool includeVendorCharacteristics)
            : _adminLock()
            , _services()
            , _index()
            , _includeVendorCharacteristics(includeVendorCharacteristics)
            , _socket(nullptr)
            , _command()
            , _handler()
            , _expired(0) {
        }
        ~Profile() {
        }

    public:
        uint32_t Discover(const uint32_t waitTime, GATTSocket& socket, const Handler& handler) {
            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();
            if (_socket == nullptr) {
                result = Core::ERROR_NONE;
                _socket = &socket;
                _expired = Core::Time::Now().Add(waitTime).Ticks();
                _handler = handler;
                _services.clear();
                _command.ReadByGroupType(0x0001, 0xFFFF, UUID(PRIMARY_SERVICE_UUID));
                _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnServices(cmd); });
            }
            _adminLock.Unlock();

            return(result);
        }
        void Abort () {
            Report(Core::ERROR_ASYNC_ABORTED);
        }
        bool IsValid() const {
            return ((_services.size() > 0) && (_expired == Core::ERROR_NONE));
        }
        Iterator Services() const {
            return (Iterator(_services));
        }

    private:
        std::list<Service>::iterator ValidService(const std::list<Service>::iterator& input) {
            if (_includeVendorCharacteristics == false) {
                std::list<Service>::iterator index(input);
                while ( (index != _services.end()) && (index->Type().HasShort() == false) ) {
                    index++;
                }
                return (index);
            }
            return (input);
        }
        void OnServices(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if (cmd.Error() != Core::ERROR_NONE) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {
                    GATTSocket::Command::Response& response(_command.Result());

                    while (response.Next() == true) {
                        const uint8_t* service = response.Data();
                        if (response.Length() == 2) {
                            _services.emplace_back( (service[0] | (service[1] << 8)), response.Handle(), response.Group() );
                        }
                        else if (response.Length() == 16) {
                            _services.emplace_back( service, response.Handle(), response.Group() );
                        }
                    }

                    if (_services.size() == 0) {
                        Report (Core::ERROR_UNAVAILABLE);
                    }
                    else if ((response.Count() > 0) && (response.Max() < 0xFFFF)) {
                        _command.ReadByGroupType(response.Max() + 1, 0xFFFF, UUID(PRIMARY_SERVICE_UUID));
                        _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnServices(cmd); });
                    }
                    else {
                        _index = ValidService(_services.begin());

                        if (_index == _services.end()) {
                            Report (Core::ERROR_NONE);
                        }
                        else {
                            _adminLock.Lock();
                            if (_socket != nullptr) {
                                _command.Read(_index->Handle());
                                _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnCharacteristics(cmd); });
                            }
                            _adminLock.Unlock();
                        }
                    }
                }
            }
        }
        void OnCharacteristics(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if (cmd.Error() != Core::ERROR_NONE) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {
                    GATTSocket::Command::Response& response(_command.Result());
                    _index->Characteristic(response.Length(), response.Data());
                    uint16_t next = _index->Handle();
                    if (next == 0) {
                        _index = ValidService(++_index);
                        if (_index != _services.end()) {
                            next = _index->Handle();
                        }
                    }
                    if (next == 0) {
                        Report(Core::ERROR_NONE);
                    }
                    else {
                        _adminLock.Lock();
                        if (_socket != nullptr) {
                            _command.Read(_index->Handle());
                            _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnCharacteristics(cmd); });
                        }
                        _adminLock.Unlock();
                    }
                }
            }
        }
        void Report(const uint32_t result) {
            _adminLock.Lock();
            if (_socket != nullptr) {
                Handler caller = _handler;
                _socket = nullptr;
                _handler = nullptr;
                _expired = result;

                caller(result);
            }
            _adminLock.Unlock();
        }
        uint32_t AvailableTime () {
            uint64_t now = Core::Time::Now().Ticks();
            uint32_t result = (now >= _expired ? 0 : static_cast<uint32_t>((_expired - now) / Core::Time::TicksPerMillisecond));

            if (result == 0) {
                Report(Core::ERROR_TIMEDOUT);
            }
            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        std::list<Service> _services;
        std::list<Service>::iterator _index;
        bool _includeVendorCharacteristics;
        GATTSocket* _socket;
        GATTSocket::Command _command;
        Handler _handler;
        uint64_t _expired;
    };

} // namespace Bluetooth

} // namespace WPEFramework

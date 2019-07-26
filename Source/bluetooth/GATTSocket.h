#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

    class GATTSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        GATTSocket(const GATTSocket&) = delete;
        GATTSocket& operator=(const GATTSocket&) = delete;

    public:
        class Attribute {
        public:
            class UUID {
            public:
                // const uint8_t BASE[] = { 00000000-0000-1000-8000-00805F9B34FB };

            public:
                UUID(const uint16_t uuid)
                {
                    _uuid[0] = 2;
                    _uuid[1] = (uuid & 0xFF);
                    _uuid[2] = (uuid >> 8) & 0xFF;
                }
                UUID(const uint8_t uuid[16])
                {
                    _uuid[0] = 16;
                    ::memcpy(&(_uuid[1]), uuid, 16);
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
                    return ((_uuid[2] << 8) | _uuid[1]);
                }
                bool operator==(const UUID& rhs) const
                {
                    return (::memcmp(_uuid, rhs._uuid, _uuid[0] + 1) == 0);
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
                const uint8_t& Data() const
                {
                    return (_uuid[1]);
                }

            private:
                uint8_t _uuid[17];
            };

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

            uint16_t Deserialize(const uint8_t stream[], const uint16_t size);

        private:
            uint8_t _type;
            uint32_t _offset;
            uint32_t _length;
            uint32_t _bufferSize;
            uint8_t* _buffer;
        };

        class Command : public Core::IOutbound::ICallback, public Core::IOutbound, public Core::IInbound {
        private:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;

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
                    return (_offset != 0);
                }
                void Reload() const
                {
                    _offset = 0;
                }
                uint8_t GetMTU(const uint16_t mySize)
                {
                    _buffer[0] = ATT_OP_MTU_REQ;
                    _buffer[1] = (mySize & 0xFF);
                    _buffer[2] = (mySize >> 8) & 0xFF;
                    _size = 3;
                    return (ATT_OP_MTU_RESP);
                }
                uint8_t ReadByGroupType(const uint16_t start, const uint16_t end, const UUID& id)
                {
                    _buffer[0] = ATT_OP_READ_BY_GROUP_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), &id.Data(), id.Length());
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
                uint8_t Write(const UUID& id, const uint8_t length, const uint8_t data[])
                {
                    _buffer[0] = ATT_OP_WRITE_REQ;
                    _buffer[1] = (id.Short() & 0xFF);
                    _buffer[2] = (id.Short() >> 8) & 0xFF;
                    ::memcpy(&(_buffer[3]), data, length);
                    _size = 3 + length;
                    return (ATT_OP_WRITE_RESP);
                }
                uint8_t ReadByType(const uint16_t start, const uint16_t end, const UUID& id)
                {
                    _buffer[0] = ATT_OP_READ_BY_TYPE_REQ;
                    _buffer[1] = (start & 0xFF);
                    _buffer[2] = (start >> 8) & 0xFF;
                    _buffer[3] = (end & 0xFF);
                    _buffer[4] = (end >> 8) & 0xFF;
                    ::memcpy(&(_buffer[5]), &id.Data(), id.Length());
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
                uint16_t Serialize(uint8_t stream[], const uint16_t length) const
                {
                    uint16_t result = std::min(static_cast<uint16_t>(_size - _offset), length);
                    if (result > 0) {
                        ::memcpy(stream, &(_buffer[_offset]), result);
                        _offset += result;

                        printf("L2CAP send [%d]: ", result);
                        for (uint8_t index = 0; index < (result - 1); index++) {
                            printf("%02X:", stream[index]);
                        }
                        printf("%02X\n", stream[result - 1]);
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
            struct ICallback {
                virtual ~ICallback() {}

                virtual void Completed(const uint32_t result) = 0;
            };

            class Response {
            private:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;

                static constexpr uint16_t BLOCKSIZE = 255;
                typedef std::pair<uint16_t, uint16_t> Entry;

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
                    return (_iterator->second);
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
                        result = (++next == _result.end() ? (_loaded - _iterator->second) : (next->second - _iterator->second));
                    }
                    return (result);
                }
                const uint8_t* Data() const
                {
                    return (IsValid() == true ? &(_storage[_iterator->second]) : (((_result.size() <= 1) && (_loaded > 0)) ? _storage : nullptr));
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
                    if (_max < handle)
                        _max = handle;
                    _result.emplace_back(Entry(handle, group));
                }
                void Add(const uint16_t handle, const uint8_t length, const uint8_t buffer[])
                {
                    if (_min > handle)
                        _min = handle;
                    if (_max < handle)
                        _max = handle;

                    _result.emplace_back(Entry(handle, _loaded));
                    Extend(length, buffer);
                }
                void Extend(const uint8_t length, const uint8_t buffer[])
                {
                    if ((_loaded + length) > _maxSize) {
                        _maxSize = ((((_loaded + length) / BLOCKSIZE) + 1) * BLOCKSIZE);
                        _storage = reinterpret_cast<uint8_t*>(::realloc(_storage, _maxSize));
                    }

                    ::memcpy(&(_storage[_loaded]), buffer, length);
                    _loaded += length;
                }
                uint16_t Offset() const
                {
                    return (_result.size() != 0 ? _loaded : _loaded - _result.back().second);
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
            Command(ICallback* completed, const uint16_t bufferSize)
                : _frame()
                , _callback(completed)
                , _id(~0)
                , _error(~0)
                , _bufferSize(bufferSize)
                , _response()
            {
            }
            virtual ~Command()
            {
            }

        public:
            void SetMTU(const uint16_t bufferSize)
            {
                _bufferSize = bufferSize;
            }
            void GetMTU()
            {
                _error = ~0;
                _id = _frame.GetMTU(_bufferSize);
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
            void WriteByType(const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[])
            {
                _response.Clear();
                _response.Extend(length, data);
                _error = ~0;
                _id = _frame.ReadByType(min, max, uuid);
            }
            void FindByType(const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[])
            {
                ASSERT(uuid.HasShort() == true);
                _response.Clear();
                _error = ~0;
                _id = _frame.FindByType(min, max, uuid, length, data);
            }
            Response& Result()
            {
                return (_response);
            }

        private:
            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code)
            {
                ASSERT(&data == this);

                _callback->Completed(error_code);
            }
            virtual uint16_t Id() const override
            {
                return (_id);
            }
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
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override;

        private:
            Exchange _frame;
            ICallback* _callback;
            uint16_t _id;
            uint16_t _error;
            uint16_t _bufferSize;
            Response _response;
        };

        static constexpr uint8_t LE_ATT_CID = 4;
        static constexpr uint8_t ATT_OP_HANDLE_NOTIFY = 0x1B;

    private:
        class LocalCommand : public Command, public Command::ICallback {
        private:
            LocalCommand() = delete;
            LocalCommand(const LocalCommand&) = delete;
            LocalCommand& operator=(const LocalCommand&) = delete;

        public:
            LocalCommand(GATTSocket& parent, const uint16_t bufferSize)
                : Command(this, bufferSize)
                , _parent(parent)
                , _callback(reinterpret_cast<Command::ICallback*>(~0))
            {
            }
            virtual ~LocalCommand()
            {
            }

            void SetCallback(Command::ICallback* callback)
            {
                ASSERT((callback == nullptr) ^ (_callback == nullptr));
                _callback = callback;
            }

        private:
            virtual void Completed(const uint32_t result) override
            {
                if (_callback == reinterpret_cast<Command::ICallback*>(~0)) {
                    if (result != Core::ERROR_NONE) {
                        TRACE_L1("Could not receive the MTU size correctly. Can not reach Operationl.");
                    } else {
                        _callback = nullptr;
                        _parent.Operational();
                    }
                } else if (_callback != nullptr) {
                    Command::ICallback* callback = _callback;
                    _callback = nullptr;
                    callback->Completed(result);
                }
            }

        private:
            GATTSocket& _parent;
            Command::ICallback* _callback;
        };

    public:
        GATTSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode, const uint16_t bufferSize)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, bufferSize, bufferSize)
            , _command(*this, bufferSize)
        {
        }
        virtual ~GATTSocket()
        {
        }

    public:
        bool Security(const uint8_t level, const uint8_t keySize);

        void ReadByGroupType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, Command::ICallback* completed)
        {
            _command.ReadByGroupType(min, max, uuid);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void ReadByType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, Command::ICallback* completed)
        {
            _command.ReadByType(min, max, uuid);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void ReadBlob(const uint32_t waitTime, const uint16_t& handle, Command::ICallback* completed)
        {
            _command.ReadBlob(handle);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void FindByType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const UUID& type, Command::ICallback* completed)
        {
            FindByType(waitTime, min, max, uuid, type.Length(), &(type.Data()), completed);
        }
        void FindByType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[], Command::ICallback* completed)
        {
            _command.FindByType(min, max, uuid, length, data);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void WriteByType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const UUID& type, Command::ICallback* completed)
        {
            WriteByType(waitTime, min, max, uuid, type.Length(), &(type.Data()), completed);
        }
        void WriteByType(const uint32_t waitTime, const uint16_t min, const uint16_t max, const UUID& uuid, const uint8_t length, const uint8_t data[], Command::ICallback* completed)
        {
            _command.WriteByType(min, max, uuid, length, data);
            _command.SetCallback(completed);
            Send(waitTime, _command, &_command, &_command);
        }
        void Abort()
        {
            Revoke(_command);
        }
        Command::Response& Result()
        {
            return (_command.Result());
        }

        virtual void Operational() = 0;
        virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) = 0;

    private:
        virtual void StateChange() override;

    private:
        struct l2cap_conninfo _connectionInfo;
        LocalCommand _command;
    };

} // namespace Bluetooth

} // namespace WPEFramework

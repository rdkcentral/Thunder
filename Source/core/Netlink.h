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

#ifndef NETLINK_MESSAGE_H_
#define NETLINK_MESSAGE_H_

#ifndef __WINDOWS__

#include "Module.h"
#include "NodeId.h"
#include "Portability.h"
#include "SocketPort.h"
#include "Sync.h"

#include <linux/connector.h>
#include <linux/rtnetlink.h>

namespace Thunder {

namespace Core {

    class EXTERNAL Netlink {
    public:
        template <typename HEADER>
        class Parameters {
        public:
            Parameters() = delete;
            Parameters(const Parameters&) = delete;
            Parameters& operator=(const Parameters&) = delete;

            Parameters(HEADER& header, uint8_t buffer[], const uint16_t bufferSize)
                : _buffer(&(buffer[NLMSG_ALIGN(sizeof(HEADER))]))
                , _size(bufferSize >= NLMSG_ALIGN(sizeof(HEADER)) ? bufferSize - NLMSG_ALIGN(sizeof(HEADER)) : 0)
                , _offset(0)
            {

                ASSERT(NLMSG_ALIGN(sizeof(HEADER)) <= bufferSize);

                ::memcpy(buffer, &header, (sizeof(HEADER) < bufferSize ? sizeof(HEADER) : bufferSize));
            }
            ~Parameters() = default;

        public:
            uint16_t Size() const
            {
                return (NLMSG_ALIGN(sizeof(HEADER)) + _offset);
            }
            template <typename OBJECT>
            void Add(const uint16_t type, const OBJECT& value)
            {
                struct rtattr* data = reinterpret_cast<struct rtattr*>(&(_buffer[NLMSG_ALIGN(_offset)]));

                data->rta_type = type;
                data->rta_len = RTA_LENGTH(sizeof(OBJECT));
                ::memcpy(RTA_DATA(data), &value, sizeof(OBJECT));

                _offset += data->rta_len;
            }

        private:
            uint8_t* _buffer;
            uint16_t _size;
            uint16_t _offset;
        };

    public:
        class EXTERNAL Frames {
        public:
            Frames() = delete;
            Frames(const Frames&) = delete;
            Frames& operator=(const Frames&) = delete;

            Frames(const uint8_t dataFrame[], const uint16_t receivedSize)
                : _data(dataFrame)
                , _size(receivedSize)
                , _offset(~0)
            {
            }
            ~Frames() = default;

        public:
            inline bool IsValid() const
            {
                return (_offset < _size);
            }
            inline bool Next()
            {

                if (_offset == static_cast<uint16_t>(~0)) {
                    _offset = 0;
                } else if (_offset < _size) {
                    _offset += NLMSG_ALIGN(Message()->nlmsg_len);
                }

                if ((_offset < _size) && (NLMSG_OK(Message(), _size - _offset) == false)) {
                    _offset = _size;
                }

                return (IsValid());
            }
            inline uint32_t Type() const
            {
                return (Message()->nlmsg_type);
            }
            inline uint32_t Sequence() const
            {
                return (Message()->nlmsg_seq);
            }
            inline uint32_t Flags() const
            {
                return (Message()->nlmsg_flags);
            }
            inline const uint8_t* Data() const
            {
                return (reinterpret_cast<const uint8_t*>(NLMSG_DATA(Message())));
            }
            inline uint16_t Size() const
            {
                return (Message()->nlmsg_len - NLMSG_HDRLEN);
            }
            inline const uint8_t* RawData() const
            {
                return (&(_data[_offset]));
            }
            inline uint16_t RawSize() const
            {
                return (Message()->nlmsg_len);
            }

        private:
            inline const struct nlmsghdr* Message() const
            {
                return (reinterpret_cast<const struct nlmsghdr*>(&(_data[_offset])));
            }

        private:
            const uint8_t* _data;
            uint16_t _size;
            uint16_t _offset;
        };

    public:
        Netlink& operator=(const Netlink&) = delete;

        Netlink()
            : _type(NLMSG_DONE)
            , _flags(0)
            , _mySequence(~0)
        {
        }
        Netlink(const Netlink& copy)
            : _type(copy._type)
            , _flags(copy._flags)
            , _mySequence(copy._mySequence)
        {
        }
        Netlink(Netlink&& move)
            : _type(move._type)
            , _flags(move._flags)
            , _mySequence(move._mySequence)
        {
            _type = NLMSG_DONE;
            _flags = 0;
            _mySequence = ~0;    
        }
        virtual ~Netlink() = default;

    public:
        uint32_t Sequence() const
        {
            return (_mySequence);
        }
        uint32_t Type() const
        {
            return (_type);
        }
        uint32_t Flags() const
        {
            return (_flags);
        }
        uint16_t Serialize(uint8_t stream[], const uint16_t length) const;
        uint16_t Deserialize(const uint8_t stream[], const uint16_t streamLength);

    protected:
        inline void Type(const uint32_t value) const
        {
            _type = value;
        }
        inline void Flags(const uint32_t value) const
        {
            _flags = value;
        }
        virtual uint16_t Write(uint8_t stream[], const uint16_t length) const = 0;
        virtual uint16_t Read(const uint8_t stream[], const uint16_t length) = 0;

    private:
        mutable uint32_t _type;
        mutable uint32_t _flags;
        mutable uint32_t _mySequence;
        static uint32_t _sequenceId;
    };

    // https://www.kernel.org/doc/Documentation/connector/connector.txt
    template <const uint32_t IDX, const uint32_t VAL>
    class ConnectorType : public Netlink {
    public:
        ConnectorType<IDX, VAL>& operator=(const ConnectorType<IDX, VAL>&) = delete;

        ConnectorType()
            : Netlink()
            , _ack(0)
        {
        }
        ConnectorType(const ConnectorType<IDX, VAL>& copy)
            : Netlink(copy)
            , _ack(copy._ack)
        {
        }
        ConnectorType(ConnectorType<IDX, VAL>&& move)
            : Netlink(move)
        {
        }
        ~ConnectorType() = default;

    public:
        inline uint32_t Acknowledge() const
        {
            return (_ack);
        }
        inline bool Ingest(const uint8_t stream[], const uint16_t length)
        {
            return (Netlink::Deserialize(stream, length) == length);
        }

    private:
        virtual uint16_t Message(uint8_t stream[], const uint16_t length) const = 0;
        virtual uint16_t Message(const uint8_t stream[], const uint16_t length) = 0;

        uint16_t Write(uint8_t stream[], const uint16_t length) const override
        {

            static_assert(NLMSG_ALIGNTO == 4, "Assuming the message element are 32 bits aligned!!");

            uint32_t value = IDX;
            memcpy(&(stream[0]), &value, 4);
            value = VAL;
            memcpy(&(stream[4]), &value, 4);
            value = Sequence();
            memcpy(&(stream[8]), &value, 4);
            value = 0x00000000;
            memcpy(&(stream[12]), &value, 4);
            uint16_t result = 0; //Flags

            memcpy(&(stream[18]), &result, 2);
            result = Message(&(stream[20]), (length - 20));

            memcpy(&(stream[16]), &result, 2);

            return (result + 20);
        }
        uint16_t Read(const uint8_t stream[], const uint16_t length) override
        {
            /* parse & filter out connector msgs chain */
            const struct cn_msg* internal(reinterpret_cast<const struct cn_msg*>(stream));
            uint16_t size = 0;
            bool completed = false;

            while ((completed == false) && ((size + (sizeof(struct cn_msg) + (internal->len))) <= length)) {

                if ((internal->len > 0) && (internal->id.idx == IDX) && (internal->id.val == VAL)) {

                    ::memcpy(&_ack, &(internal->ack), 4);

                    completed = (Message(internal->data, internal->len) != 0);
                }

                if (completed == false) {
                    /* move to the next cn_msg */
                    size += sizeof(struct cn_msg) + internal->len;
                    internal = reinterpret_cast<const struct cn_msg*>(reinterpret_cast<const uint8_t*>(stream + size));
                }
            }
            return (completed == false ? 0 : length);
        }

    private:
        uint32_t _ack;
    };

    class EXTERNAL SocketNetlink : public SocketDatagram {
    private:
        SocketNetlink(const SocketNetlink&) = delete;
        SocketNetlink& operator=(const SocketNetlink&) = delete;

        class Message {
        private:
            Message() = delete;
            Message(const Message&) = delete;
            Message& operator=(const Message&) = delete;

            enum state {
                LOADED,
                SEND,
                FAILURE
            };

        public:
            Message(const Netlink& outbound)
                : _outbound(outbound)
                , _inbound(nullptr)
                , _signaled(false, true)
                , _state(LOADED)
            {
            }
            Message(const Netlink& outbound, Netlink& inbound)
                : _outbound(outbound)
                , _inbound(&inbound)
                , _signaled(false, true)
                , _state(LOADED)
            {
            }
            ~Message()
            {
            }

        public:
            inline bool IsSend() const
            {
                return (_state != LOADED);
            }
            inline uint32_t Sequence() const
            {
                return (_outbound.Sequence());
            }
            inline void Deserialize(const Netlink::Frames& frame)
            {

                ASSERT(frame.IsValid() == true);

                if (frame.Type() == NLMSG_ERROR) {
                    // It means we received an error.
                    _state = FAILURE;
                    _signaled.SetEvent();
                } else if ((_inbound != nullptr) && (_inbound->Deserialize(frame.RawData(), frame.RawSize()) != 0)) {
                    _signaled.SetEvent();
                }
            }
            inline uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
            {
                _state = SEND;
                uint16_t handled = _outbound.Serialize(buffer, length);
                if (_inbound == nullptr) {
                    _signaled.SetEvent();
                }
                return (handled);
            }
            inline bool operator==(const Netlink& rhs) const
            {
                return (&rhs == &_outbound);
            }
            inline bool operator!=(const Netlink& rhs) const
            {
                return (!operator==(rhs));
            }
            inline bool Wait(const uint32_t waitTime)
            {
                bool result = (_signaled.Lock(waitTime) == ERROR_NONE ? true : false);
                return (result);
            }

        private:
            const Netlink& _outbound;
            Netlink* _inbound;
            mutable Event _signaled;
            mutable state _state;
        };
        typedef std::list<Message> PendingList;

    public:
        SocketNetlink(const NodeId& source)
            : SocketDatagram(true, source, NodeId(), 512, 8192, ~0, ~0)
            , _adminLock()
        {
        }
        ~SocketNetlink()
        {
            SocketDatagram::Close(infinite);
        }

    public:
        uint32_t Send(const Netlink& outbound, const uint32_t waitTime);
        uint32_t Exchange(const Netlink& outbound, Netlink& inbound, const uint32_t waitTime);

        virtual uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t receivedSize) = 0;

    private:
        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;
        virtual void StateChange() override;

    private:
        CriticalSection _adminLock;
        PendingList _pending;
    };
} // namespace Core
} // namespace Thunder

#endif // __WINDOWS__

#endif // NETLINK_MESSAGE_H_

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

namespace WPEFramework {

namespace Core {

    class EXTERNAL Netlink {
    private:
        Netlink& operator=(const Netlink&) = delete;

        template <typename HEADER>
        class Parameters {
        private:
            Parameters() = delete;
            Parameters(const Parameters&) = delete;
            Parameters& operator=(const Parameters&) = delete;

        public:
            Parameters(HEADER& header, uint8_t buffer[], const uint16_t bufferSize)
                : _buffer(&(buffer[NLMSG_ALIGN(sizeof(HEADER))]))
                , _size(bufferSize >= NLMSG_ALIGN(sizeof(HEADER)) ? bufferSize - NLMSG_ALIGN(sizeof(HEADER)) : 0)
                , _offset(0)
            {

                ASSERT(NLMSG_ALIGN(sizeof(HEADER)) <= bufferSize);

                ::memcpy(buffer, &header, (sizeof(HEADER) < bufferSize ? sizeof(HEADER) : bufferSize));
            }
            ~Parameters()
            {
            }

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
        private:
            Frames() = delete;
            Frames(const Frames&) = delete;
            Frames& operator=(const Frames&) = delete;

        public:
            Frames(const uint8_t dataFrame[], const uint16_t receivedSize)
                : _data(dataFrame)
                , _size(receivedSize)
                , _header(nullptr)
                , _dataLeft(~0)
            {
            }
            ~Frames()
            {
            }

        public:
            inline bool IsValid() const
            {
                return (_header != nullptr) && (NLMSG_OK(_header, _dataLeft));
            }
            inline bool Next()
            {
                if (_header == nullptr) {
                    _header = reinterpret_cast<nlmsghdr*>(const_cast<uint8_t*>(_data));
                    _dataLeft = _size;
                } else {
                    _header = NLMSG_NEXT(_header, _dataLeft);
                }

                return (IsValid());
            }
            inline uint32_t Type() const
            {
                ASSERT(IsValid());
                return (_header->nlmsg_type);
            }
            inline uint32_t Sequence() const
            {
                ASSERT(IsValid());
                return (_header->nlmsg_seq);
            }
            inline uint32_t Flags() const
            {
                ASSERT(IsValid());
                return (_header->nlmsg_flags);
            }
            template <typename T>
            inline const T* Payload() const
            {
                ASSERT(IsValid());
                return (reinterpret_cast<const T*>(NLMSG_DATA(_header)));
            }
            inline uint16_t PayloadSize() const
            {
                ASSERT(IsValid());
                return NLMSG_PAYLOAD(_header, 0);
            }
            inline const uint8_t* RawData() const
            {
                ASSERT(IsValid());
                return reinterpret_cast<const uint8_t*>(_header);
            }
            inline uint16_t RawSize() const
            {
                ASSERT(IsValid());
                return _header->nlmsg_len;
            }

        private:
            inline const struct nlmsghdr* Header() const
            {
                ASSERT(IsValid());
                return _header;
            }

        private:
            const uint8_t* _data;
            uint16_t _size;
            nlmsghdr* _header;
            uint16_t _dataLeft;
        };

    public:
        Netlink()
            : _type(0)
            , _flags(0)
            , _mySequence(~0)
            , _isMultimessage(false)
        {
        }
        Netlink(const Netlink& copy)
            : _type(copy._type)
            , _flags(copy._flags)
            , _mySequence(copy._mySequence)
            , _isMultimessage(false)
        {
        }
        virtual ~Netlink()
        {
        }

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
        mutable bool _isMultimessage;
        static uint32_t _sequenceId;
    };

    // https://www.kernel.org/doc/Documentation/connector/connector.txt

    template <const uint32_t IDX, const uint32_t VAL>
    class ConnectorType : public Core::Netlink {
    private:
        ConnectorType<IDX, VAL>& operator=(const ConnectorType<IDX, VAL>&) = delete;

    public:
        ConnectorType()
            : Core::Netlink()
        {
            Type(NLMSG_DONE);
            Flags(0);
        }
        ConnectorType(const ConnectorType<IDX, VAL>& copy)
            : Core::Netlink(copy)
        {
        }
        ~ConnectorType()
        {
        }

        inline uint32_t Acknowledge() const
        {
            return (_ack);
        }
        inline bool Ingest(const uint8_t stream[], const uint16_t length)
        {
            return (Core::Netlink::Deserialize(stream, length) == length);
        }

    private:
        virtual uint16_t Message(uint8_t stream[], const uint16_t length) const = 0;
        virtual uint16_t Message(const uint8_t stream[], const uint16_t length) = 0;

        virtual uint16_t Write(uint8_t stream[], const uint16_t length) const
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
        virtual uint16_t Read(const uint8_t stream[], const uint16_t length) override
        {

            /* parse & filter out connector msgs chain */
            const struct cn_msg* internal(reinterpret_cast<const struct cn_msg*>(stream));
            uint16_t size = 0;
            bool completed = false;

            while ((completed == false) && ((size + (sizeof(struct cn_msg) + (internal->len))) <= length)) {

                if ((internal->len > 0) && (internal->id.idx == IDX) && (internal->id.val == VAL)) {

                    ::memcpy(&_ack, &(internal->ack), 4);

                    completed = (Message(internal->data, internal->len) < internal->len);
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

    class SocketNetlink : public Core::SocketDatagram {
    private:
        SocketNetlink(const SocketNetlink&) = delete;
        SocketNetlink& operator=(const SocketNetlink&) = delete;
    protected:
        class Message {
        private:
            Message() = delete;
            Message(const Message&) = delete;
            Message& operator=(const Message&) = delete;

            enum state {
                LOADED,
                SEND,
                FAILURE,
                PROCESSED
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
            inline bool IsProcessed() const
            {
                return (_state == PROCESSED || _state == FAILURE);
            }
            inline bool NeedResponse() const 
            {
                return _inbound != nullptr;
            }
            inline uint32_t Sequence() const
            {
                return (_outbound.Sequence());
            }
            inline uint16_t Deserialize(const uint8_t buffer[], const uint16_t length)
            {
                uint16_t result;

                Netlink::Frames frame(buffer, length);
                if (frame.Next()) {

                    bool isMultimessage = false;

                    if (frame.Type() != NLMSG_NOOP) {
                        if (frame.Type() == NLMSG_DONE) {
                            isMultimessage = false;
                        } else {                    
                            _inbound->Deserialize(
                                frame.RawData(),
                                frame.RawSize()
                            );

                            isMultimessage = frame.Flags() & NLM_F_MULTI;
                        }
                    }

                    // We are done only if all response messages arrived.
                    // If message is still in multimessage state, we should
                    // wait for more data to signal a success
                    if ((isMultimessage == false) || (frame.Type() == NLMSG_ERROR)) {
                        
                        _state = (frame.Type() == NLMSG_ERROR) ? FAILURE : PROCESSED;
                        _signaled.SetEvent();
                    }

                    result = frame.RawSize();
                }

                return result;
            }
            inline uint16_t Serialize(uint8_t buffer[], const uint16_t length) const
            {
                _state = SEND;
                uint16_t handled = _outbound.Serialize(buffer, length);

                if (NeedResponse() == false) {
                    _signaled.SetEvent();
                }
                return (handled);
            }
            inline bool operator==(const Core::Netlink& rhs) const
            {
                return (&rhs == &_outbound);
            }
            inline bool operator!=(const Core::Netlink& rhs) const
            {
                return (!operator==(rhs));
            }
            inline bool Wait(const uint32_t waitTime)
            {
                bool result = (_signaled.Lock(waitTime) == Core::ERROR_NONE ? true : false);

                return (result);
            }

        private:
            const Netlink& _outbound;
            Netlink* _inbound;
            mutable Core::Event _signaled;
            mutable state _state;
        };
        typedef std::list<Message> PendingList;

    public:
        SocketNetlink(const Core::NodeId& destination)
            : SocketDatagram(false, destination, Core::NodeId(), 4096, 8192)
            , _adminLock()
        {
        }
        ~SocketNetlink()
        {
            Core::SocketDatagram::Close(Core::infinite);
        }

    public:
        uint32_t Send(const Core::Netlink& outbound, const uint32_t waitTime);
        uint32_t Exchange(const Core::Netlink& outbound, Core::Netlink& inbound, const uint32_t waitTime);

        virtual uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t receivedSize) = 0;

    private:
        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override;
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize) override;
        virtual void StateChange() override;

    private:
        uint32_t _bufferBefore;
        Core::CriticalSection _adminLock;
        uint32_t _bufferAfter;
        PendingList _pending;
    };
} // namespace Core
} // namespace WPEFramework

#endif // __WINDOWS__

#endif // NETLINK_MESSAGE_H_

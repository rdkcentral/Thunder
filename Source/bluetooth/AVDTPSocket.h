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

namespace WPEFramework {

namespace Bluetooth {

    class EXTERNAL AVDTPSocket : public Core::SynchronousChannelType<Core::SocketPort> {
    private:
        AVDTPSocket(const AVDTPSocket&) = delete;
        AVDTPSocket& operator=(const AVDTPSocket&) = delete;

        class Callback : public Core::IOutbound::ICallback
        {
        public:
            Callback() = delete;
            Callback(const Callback&) = delete;
            Callback& operator= (const Callback&) = delete;

            Callback(AVDTPSocket& parent)
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
            AVDTPSocket& _parent;
        }; // class Callback

    public:
        static constexpr uint32_t CommunicationTimeout = 2000; /* ms */

    public:
        using SEPConfiguration = std::map<uint8_t, Buffer>;

    public:
        class EXTERNAL Command : public Core::IOutbound, public Core::IInbound {
        public:
            class Signal : public DataRecord  {
            public:
                enum signalidentifier : uint8_t {
                    INVALID                     = 0x00,
                    AVDTP_DISCOVER              = 0x01,
                    AVDTP_GET_CAPABILITIES      = 0x02,
                    AVDTP_SET_CONFIGURATION     = 0x03,
                    AVDTP_GET_CONFIGURATION     = 0x04,
                    AVDTP_RECONFIGURE           = 0x05,
                    AVDTP_OPEN                  = 0x06,
                    AVDTP_START                 = 0x07,
                    AVDTP_CLOSE                 = 0x08,
                    AVDTP_SUSPEND               = 0x09,
                    AVDTP_ABORT                 = 0x0A,
                    AVDTP_SECURITY_CONTROL      = 0x0B,
                    AVDTP_GET_ALL_CAPABILITIES  = 0x0C,
                    AVDTP_DELAYREPORT           = 0x0D
                };

                enum packettype : uint8_t {
                    SINGLE      = 0x00,
                    START       = 0x01,
                    CONTINUE    = 0x02,
                    END         = 0x03
                };

                enum messagetype : uint8_t {
                    COMMAND             = 0x00,
                    GENERAL_REJECT      = 0x01,
                    RESPONSE_ACCEPT     = 0x02,
                    RESPONSE_REJECT     = 0x03
                };

                enum errorcode : uint8_t {
                    SUCCESS                         = 0x00,

                    // Header errors
                    BAD_HEADER_FORMAT               = 0x01,

                    // Payload format errors
                    BAD_LENGTH                      = 0x11,
                    BAD_ACP_SEID                    = 0x12,
                    SEP_IN_USE                      = 0x13,
                    SEP_NOT_IN_USE                  = 0x14,
                    BAD_SERV_CATEGORY               = 0x17,
                    BAD_PAYLOAD_FORMAT              = 0x18,
                    NOT_SUPPORTED_COMMAND           = 0x19,
                    INVALID_CAPABILITIES            = 0x1A,

                    // Transport service errors
                    BAD_RECOVERY_TYPE               = 0x22,
                    BAD_MEDIA_TRANSPORT_FORMAT      = 0x23,
                    BAD_RECOVERY_FORMAT             = 0x25,
                    BAD_ROHC_FORMAT                 = 0x26,
                    BAD_CP_FORMAT                   = 0x27,
                    BAD_MULTIPLEXING_FORMAT         = 0x28,
                    UNSUPPORTED_CONFIGURATION       = 0x29,

                    // Procedure errors
                    BAD_STATE                       = 0x31,

                    GENERAL_ERROR                   = 0xFF
                };

            public:
                using DataRecord::DataRecord;
                using DataRecord::Push;
                using DataRecord::Pop;
                ~Signal() = default;

            public:
                void Push(const uint8_t label, const signalidentifier signalId, const messagetype msgType = COMMAND,
                          const packettype pktType = SINGLE, const uint8_t packets = 0)
                {
                    ASSERT(Length() == 0); // this is expected to be at the beginning of the signal
                    ASSERT(Free() >= 3);

                    Push(static_cast<uint8_t>((label << 4) | (static_cast<uint8_t>(pktType) << 2) | static_cast<uint8_t>(msgType)));
                    if (pktType == START) {
                        Push(packets);
                    }

                    if ((pktType == START) || (pktType == SINGLE)) {
                        Push(static_cast<uint8_t>(signalId & 0x3F));
                    }
                }

                void Pop(uint8_t& label, signalidentifier& signalId, messagetype& msgType, packettype& pktType, uint8_t& packets)
                {
                    bool truncated = false;

                    if (Available() >= 1) {
                        uint8_t octet;

                        Pop(octet);
                        label = (octet >> 4);
                        msgType = static_cast<messagetype>(octet & 0x3);
                        pktType = static_cast<packettype>((octet >> 2) & 0x3);

                        if (pktType == START) {
                            if (Available() >= 1) {
                                Pop(packets);
                            } else {
                                truncated = true;
                            }
                        }

                        if ((pktType == START) || (pktType == SINGLE)) {
                            if (Available() >= 1) {
                                Pop(octet);
                                signalId = static_cast<signalidentifier>(octet & 0x3F);
                            } else {
                                truncated = true;
                            }
                        }
                    } else {
                        truncated = true;
                    }

                    if (truncated == true) {
                        TRACE_L1("AVDTP: Truncated signal header!");
                    }
                }
            }; // class Signal

            class EXTERNAL Request {
            public:
                Request(const Request&) = delete;
                Request& operator=(const Request&) = delete;
                Request(const uint16_t bufferSize = 256)
                    : _offset(0)
                    , _signalScratchPad(static_cast<uint8_t*>(::malloc(bufferSize)))
                    , _signal(_signalScratchPad, bufferSize, 0)
                    , _label(0xF)
                {
                    ASSERT(bufferSize != 0);
                    ASSERT(_signalScratchPad != nullptr);
                }
                ~Request()
                {
                    ::free(_signalScratchPad);
                }

            public:
                void Reload() const
                {
                    _offset = 0;
                }
                bool IsValid() const
                {
                    return (_signal.Length() >= 1);
                }
                uint16_t Serialize(uint8_t stream[], const uint16_t length) const
                {
                    uint16_t result = std::min(static_cast<uint16_t>(_signal.Length() - _offset), length);
                    if (result > 0) {
                        ::memcpy(stream, (_signal.Data() + _offset), result);
                        _offset += result;

                        CMD_DUMP("AVDTP send", stream, result);
                    }
                    return (result);
                }

            public:
                void Discover()
                {
                    _signal.Clear();
                    _signal.Push(NewLabel(), Signal::AVDTP_DISCOVER);
                }
                void GetCapabilities(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_GET_CAPABILITIES, acpSeid);
                }
                void GetAllCapabilities(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_GET_ALL_CAPABILITIES, acpSeid);
                }
                void GetConfiguration(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_GET_CONFIGURATION, acpSeid);
                }
                void SetConfiguration(const uint8_t acpSeid, const uint8_t intSeid, const SEPConfiguration& caps)
                {
                    ConfigurationSignal(Signal::AVDTP_SET_CONFIGURATION, acpSeid, intSeid, caps);
                }
                void Reconfigure(const uint8_t acpSeid, const uint8_t intSeid, const SEPConfiguration& caps)
                {
                    ConfigurationSignal(Signal::AVDTP_RECONFIGURE, acpSeid, intSeid, caps);
                }
                void Open(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_OPEN, acpSeid);
                }
                void Start(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_START, acpSeid);
                }
                void Suspend(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_SUSPEND, acpSeid);
                }
                void Close(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_CLOSE, acpSeid);
                }
                void Abort(const uint8_t acpSeid)
                {
                    GenericSignal(Signal::AVDTP_ABORT, acpSeid);
                }
                void SecurityControl(const uint8_t acpSeid, const Buffer& data)
                {
                    GenericSignal(Signal::AVDTP_SECURITY_CONTROL, acpSeid);
                    _signal.Push(data);
                }

            public:
                uint8_t Label() const
                {
                    return (_label);
                }

            private:
                void GenericSignal(const Signal::signalidentifier signal, const uint8_t acpSeid)
                {
                    _signal.Clear();
                    _signal.Push(NewLabel(), signal);
                    _signal.Push(static_cast<uint8_t>(acpSeid << 2));
                }
                void ConfigurationSignal(const Signal::signalidentifier signal,
                                         const uint8_t acpSeid, const uint8_t intSeid, const SEPConfiguration& caps)
                {
                    GenericSignal(signal, acpSeid);
                    _signal.Push(static_cast<uint8_t>(intSeid << 2));

                    for (auto const& c : caps) {
                        ASSERT(_signal.Free() >= 3);
                        _signal.Push(c.first);
                        _signal.Push(static_cast<uint8_t>(c.second.size()));
                        _signal.Push(c.second);
                    }
                }

            private:
                uint8_t NewLabel() const
                {
                    _label = ((_label + 1) & 0xF);
                    return (_label);
                }

            private:
                mutable uint16_t _offset;
                uint8_t* _signalScratchPad;
                Signal _signal;
                mutable uint8_t _label;
            }; // class Request

        public:
            class EXTERNAL Response {
            private:
                typedef std::pair<uint16_t, std::pair<uint16_t, uint16_t>> Entry;

            public:
                Response(const Response&) = delete;
                Response& operator=(const Response&) = delete;
                Response(const uint16_t bufferSize = 256)
                    : _payloadScratchPad(static_cast<uint8_t*>(::malloc(bufferSize)))
                    , _payload(_payloadScratchPad, bufferSize)
                {
                    ASSERT(_payloadScratchPad != nullptr);
                    ASSERT(bufferSize != 0);
                }
                ~Response()
                {
                    ::free(_payloadScratchPad);
                }

            public:
                void Clear()
                {
                    _type = Signal::INVALID;
                    _payload.Clear();
                }
                Signal::signalidentifier Type() const
                {
                    return (_type);
                }
                Signal::errorcode Status() const
                {
                    return (_status);
                }

            public:
                uint16_t Deserialize(const uint8_t expectedLabel, const uint8_t stream[], const uint16_t length);

            public:
                void ReadDiscovery(const std::function<void(const Buffer&)>& handler) const;
                void ReadConfiguration(const std::function<void(const uint8_t /* category */, const Buffer&)>& handler) const;

            private:
                uint8_t* _payloadScratchPad;
                DataRecord _payload;
                Signal::signalidentifier _type;
                Signal::errorcode _status;
            }; // class Response

        public:
            Command(const Command&) = delete;
            Command& operator=(const Command&) = delete;
            Command()
                : _status(~0)
                , _request()
                , _response()
            {
            }
            ~Command() = default;

        public:
            void Discover()
            {
                _response.Clear();
                _status = ~0;
                _request.Discover();
            }
            void GetCapabilities(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.GetCapabilities(acpSeid);
            }
            void GetAllCapabilities(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.GetAllCapabilities(acpSeid);
            }
            void GetConfiguration(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.GetConfiguration(acpSeid);
            }
            void SetConfiguration(const uint8_t acpSeid, const uint8_t intSeid, const SEPConfiguration& config)
            {
                _response.Clear();
                _status = ~0;
                _request.SetConfiguration(acpSeid, intSeid, config);
            }
            void Reconfigure(const uint8_t acpSeid, const uint8_t intSeid, const SEPConfiguration& config)
            {
                _response.Clear();
                _status = ~0;
                _request.Reconfigure(acpSeid, intSeid, config);
            }
            void Open(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.Open(acpSeid);
            }
            void Start(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.Start(acpSeid);
            }
            void Suspend(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.Suspend(acpSeid);
            }
            void Close(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.Close(acpSeid);
            }
            void Abort(const uint8_t acpSeid)
            {
                _response.Clear();
                _status = ~0;
                _request.Abort(acpSeid);
            }
            void SecurityControl(const uint8_t acpSeid, const Buffer& data)
            {
                _response.Clear();
                _status = ~0;
                _request.SecurityControl(acpSeid, data);
            }

        public:
            Response& Result()
            {
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
                return (_request.Serialize(stream, length));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length) override
            {
                uint16_t result = _response.Deserialize(_request.Label(), stream, length);
                return (result);
            }
            Core::IInbound::state IsCompleted() const override
            {
                return (Core::IInbound::COMPLETED);
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
            Entry& operator=(const Entry&) = delete;
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
            const Command& Cmd() const
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
        AVDTPSocket(const Core::NodeId& localNode, const Core::NodeId& remoteNode)
            : Core::SynchronousChannelType<Core::SocketPort>(SocketPort::SEQUENCED, localNode, remoteNode, 1024, 2048)
            , _adminLock()
            , _callback(*this)
            , _queue()
        {
        }
        ~AVDTPSocket() = default;

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
        virtual void Operational() = 0;

        void StateChange() override
        {
            Core::SynchronousChannelType<Core::SocketPort>::StateChange();

            if (IsOpen() == true) {
                socklen_t len = sizeof(_connectionInfo);
                ::getsockopt(Handle(), SOL_L2CAP, L2CAP_CONNINFO, &_connectionInfo, &len);

                Operational();
            }
        }

        uint16_t Deserialize(const uint8_t dataFrame[], const uint16_t availableData) override
        {
            uint32_t result = 0;

            if (availableData != 0) {
                TRACE_L1("Unexpected data for deserialization [%d bytes]", availableData);
                CMD_DUMP("AVTDP received unexpected", dataFrame, availableData);
            }

            return (result);
        }
        void CommandCompleted(const Core::IOutbound& data, const uint32_t error_code)
        {
            _adminLock.Lock();

            if ((_queue.size() == 0) || (*(_queue.begin()) != &data)) {
                ASSERT(false && "Always the first one should be the one to be handled!");
            }
            else {
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
    }; // class AVDTPSocket

} // namespace Bluetooth

}

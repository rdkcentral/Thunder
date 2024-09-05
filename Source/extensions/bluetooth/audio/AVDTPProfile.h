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
#include "AVDTPSocket.h"

namespace Thunder {

namespace Bluetooth {

namespace AVDTP {

    class Server;
    class Client;

    class EXTERNAL StreamEndPointData {
    public:
        class EXTERNAL Service {
        public:
            enum categorytype : uint8_t {
                INVALID             = 0,

                MEDIA_TRANSPORT     = 0x01,
                REPORTING           = 0x02,
                RECOVERY            = 0x03,
                CONTENT_PROTECTION  = 0x04,
                HEADER_COMPRESSION  = 0x05,
                MULTIPLEXING        = 0x06,
                MEDIA_CODEC         = 0x07,
                DELAY_REPORTING     = 0x08,
            };

        public:
            template<typename T>
            Service(const categorytype category, T&& params)
                : _category(category)
                , _params(std::forward<T>(params))
            {
                if ((category > DELAY_REPORTING) || (category == INVALID)) {
                    TRACE_L1("Invalid category %d", category);
                }
            }
            ~Service() = default;

        public:
            categorytype Category() const {
                return (_category);
            }
            const Buffer& Params() const {
                return (_params);
            }

        public:
            static constexpr bool IsValidCategory(const categorytype category) {
                return ((category != INVALID) && (category <= DELAY_REPORTING));
            }
            static constexpr bool IsBasicCategory(const categorytype category) {
                // which are basic capabilities?
                return ((category != INVALID) && (category < DELAY_REPORTING));
            }
            static constexpr bool IsApplicationCategory(const categorytype category) {
                // which are Application Service capabilities?
                return ((category == MEDIA_CODEC) || (category == CONTENT_PROTECTION) || (category == DELAY_REPORTING));
            }
            static constexpr bool IsTransportCategory(const categorytype category) {
                // which are Transport Service capabilities?
                return ((category == MEDIA_TRANSPORT) || (category == REPORTING) || (category == RECOVERY)
                            || (category == HEADER_COMPRESSION) || (category == MULTIPLEXING));
            }

        public:
#ifdef __DEBUG__
            string AsString() const
            {
                static const char *categoryLabels[] = {
                    "INVALID", "MEDIA_TRANSPORT", "REPORTINGS", "RECOVERY",
                    "CONTENT_PROTECTION", "HEADER_COMPRESSION", "MULTIPLEXING", "MEDIA_CODEC",
                    "DELAY_REPORTING"
                };

                return (Core::Format(_T("%02x '%s' with %d bytes <%s>)"),
                    _category, categoryLabels[static_cast<uint8_t>(_category)], Params().size(), Params().ToString().c_str()));
            }
#endif // __DEBUG__

        private:
            const categorytype _category;
            const Buffer _params;
        }; // class Service

    public:
        enum septype : uint8_t {
            SOURCE  = 0x00,
            SINK    = 0x01,
        };

        enum mediatype : uint8_t {
            AUDIO       = 0x00,
            VIDEO       = 0x01,
            MULTIMEDIA  = 0x02
        };

        enum statetype : uint8_t {
            IDLE,
            CONFIGURED,
            OPENED,
            STARTED,
            CLOSING,
            ABORTING
        };

    public:
        StreamEndPointData() = delete;
        StreamEndPointData(const StreamEndPointData&) = delete;
        StreamEndPointData& operator=(const StreamEndPointData&) = delete;
        virtual ~StreamEndPointData() = default;

        StreamEndPointData(const uint8_t id, const septype service, const mediatype media,
                       const std::function<void(StreamEndPointData&)>& fillerCb = nullptr)
            : _id(id)
            , _remoteId(0)
            , _state(IDLE)
            , _type(service)
            , _mediaType(media)
            , _capabilities()
            , _configuration()
        {
            ASSERT(id != 0);

            if (fillerCb != nullptr) {
                fillerCb(*this);
            }
        }
        StreamEndPointData(StreamEndPointData&& other)
            : _id(other._id)
            , _remoteId(other._remoteId)
            , _state(other._state)
            , _type(other._type)
            , _mediaType(other._mediaType)
            , _capabilities(std::move(other._capabilities))
            , _configuration(std::move(other._configuration))
        {
        }
        StreamEndPointData(const uint8_t data[2])
            : _id(0)
            , _remoteId(0)
            , _state(IDLE)
            , _type()
            , _mediaType()
            , _capabilities()
            , _configuration()
        {
            Deserialize(data);
        }

    public:
        using ServiceMap = std::map<Service::categorytype, Service>;

    public:
        uint32_t Id() const {
            return (_id);
        }
        uint32_t RemoteId() const {
            return (_remoteId);
        }
        septype Type() const {
            return (_type);
        }
        mediatype MediaType() const {
            return (_mediaType);
        }
        bool IsFree() const {
            return (_state == IDLE);
        }
        statetype State() const {
            return (_state);
        }
        const ServiceMap& Capabilities() const {
            return (_capabilities);
        }
        ServiceMap& Capabilities() {
            return (_capabilities);
        }
        const ServiceMap& Configuration() const {
            return (_configuration);
        }
        ServiceMap& Configuration() {
            return (_configuration);
        }

    public:
        struct capability_t { explicit capability_t() = default; };
        static constexpr capability_t capability = capability_t{}; // tag for selecting a proper overload

        void Add(capability_t, const Service::categorytype category)
        {
            _capabilities.emplace(std::piecewise_construct,
                            std::forward_as_tuple(category),
                            std::forward_as_tuple(category, Buffer()));
        }
        void Add(const Service::categorytype category)
        {
            _configuration.emplace(std::piecewise_construct,
                            std::forward_as_tuple(category),
                            std::forward_as_tuple(category, Buffer()));
        }

        template<typename T>
        void Add(capability_t, const Service::categorytype category, T&& buffer)
        {
            _capabilities.emplace(std::piecewise_construct,
                            std::forward_as_tuple(category),
                            std::forward_as_tuple(category, std::forward<T>(buffer)));
        }
        template<typename T>
        void Add(const Service::categorytype category, T&& buffer)
        {
            _configuration.emplace(std::piecewise_construct,
                            std::forward_as_tuple(category),
                            std::forward_as_tuple(category, std::forward<T>(buffer)));
        }
        void RemoteId(const uint8_t id)
        {
            _remoteId = id;
        }

    public:
        void Serialize(Payload& payload) const
        {
            uint8_t octet;
            octet = ((Id() << 2) | ((!IsFree()) << 1));
            payload.Push(octet);

            octet = ((_mediaType << 4) | (_type << 3));
            payload.Push(octet);
        }

#ifdef __DEBUG__
        string AsString() const
        {
            static const char *sepTypeLabel[] = {
                "Source", "Sink"
            };

            static const char *mediaTypeLabel[] = {
                "Audio", "Video", "Multimedia"
            };

            ASSERT(_type <= 1);
            ASSERT(_mediaType <= 2);

            return (Core::Format(_T("Stream Endpoint SEID %02x; '%s %s' (%s)"),
                Id(), mediaTypeLabel[MediaType()], sepTypeLabel[Type()], (IsFree()? "free" : "in-use")));
        }
#endif // __DEBUG__

    protected:
        statetype State(const statetype newState)
        {
            const statetype old = _state;

            _state = newState;

            return (old);
        }

    private:
        void Deserialize(const uint8_t data[2])
        {
            _id = (data[0] >> 2);
            _mediaType = static_cast<mediatype>(data[1] >> 4);
            _type = static_cast<septype>(!!(data[1] & 0x08));
            _state = ((data[0] & 0x02) != 0? OPENED : IDLE);
            _capabilities.clear();
            _configuration.clear();
        }

    private:
        uint8_t _id;
        uint8_t _remoteId;
        statetype _state;
        septype _type;
        mediatype _mediaType;
        ServiceMap _capabilities;
        ServiceMap _configuration;
    }; // class StreamEndPointData

    struct EXTERNAL IStreamEndPointControl {
        virtual ~IStreamEndPointControl() { }

        virtual uint32_t OnSetConfiguration(uint8_t& outFailedCategory, Socket* channel) = 0;
        virtual uint32_t OnReconfigure(uint8_t& outFailedCategory) = 0;
        virtual uint32_t OnStart() = 0;
        virtual uint32_t OnSuspend() = 0;
        virtual uint32_t OnOpen() = 0;
        virtual uint32_t OnClose() = 0;
        virtual uint32_t OnAbort() = 0;
        virtual uint32_t OnSecurityControl() = 0;
    };

    class EXTERNAL StreamEndPoint : public StreamEndPointData,
                           public IStreamEndPointControl {
    public:
        StreamEndPoint() = delete;
        StreamEndPoint(const StreamEndPoint&) = delete;
        StreamEndPoint& operator=(const StreamEndPoint&) = delete;
        ~StreamEndPoint() override = default;

        StreamEndPoint(const uint8_t id, const septype service, const mediatype media,
                       const std::function<void(StreamEndPointData&)>& fillerCb = nullptr)
            : StreamEndPointData(id, service, media, fillerCb)
        {
        }
    };

    class EXTERNAL Client {
    public:
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        ~Client() = default;

        explicit Client(Socket* socket)
            : _socket(socket)
            , _command(*this)
        {
        }

    public:
        Socket* Channel() {
            return (_socket);
        }
        const Socket* Channel() const {
            return (_socket);
        }

    public:
        void Channel(Socket* socket)
        {
            _socket = socket;
        }

    public:
        uint32_t Discover(const std::function<void(StreamEndPointData&&)>& reportCb) const;

        uint32_t SetConfiguration(StreamEndPoint& ep, const uint8_t remoteId);

        uint32_t GetConfiguration(const uint8_t remoteId, const std::function<void(const uint8_t, Buffer&&)>& reportCb) const;

        uint32_t Start(StreamEndPoint& ep)
        {
            ASSERT(ep.RemoteId() != 0);

            _command.Set(Signal::AVDTP_START, ep.RemoteId());

            return (Execute(_command));
        }
        uint32_t Suspend(StreamEndPoint& ep)
        {
            ASSERT(ep.RemoteId() != 0);

            _command.Set(Signal::AVDTP_SUSPEND, ep.RemoteId());

            return (Execute(_command));
        }
        uint32_t Open(StreamEndPoint& ep)
        {
            ASSERT(ep.RemoteId() != 0);

            _command.Set(Signal::AVDTP_OPEN, ep.RemoteId());

            return (Execute(_command));
        }
        uint32_t Close(StreamEndPoint& ep)
        {
            ASSERT(ep.RemoteId() != 0);

            _command.Set(Signal::AVDTP_CLOSE, ep.RemoteId());

            const uint32_t result = Execute(_command);

            return (result);
        }
        uint32_t Abort(StreamEndPoint& ep)
        {
            ASSERT(ep.RemoteId() != 0);

            _command.Set(Signal::AVDTP_ABORT, ep.RemoteId());

            const uint32_t result = Execute(_command);

            return (result);
        }

    private:
        uint32_t Execute(Socket::CommandType<Client>& cmd, const Payload::Inspector& inspectorCb = nullptr) const;

    private:
        Socket* _socket;
        mutable AVDTP::Socket::CommandType<Client> _command;
    }; // class Client

    class EXTERNAL Server {
        using Handler = Socket::ResponseHandler;

    public:
        Server() = default;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        virtual ~Server() = default;

    public:
        virtual bool Visit(const uint8_t id, const std::function<void(StreamEndPoint&)>& inspectCb) = 0;

    public:
        void OnSignal(const Signal& signal, const Handler& handler)
        {
            switch (signal.Id()) {
            case Signal::AVDTP_DISCOVER:
                OnDiscover(handler);
                break;
            case Signal::AVDTP_GET_CAPABILITIES:
                OnGetCapabilities(SEID(signal), handler);
                break;
                break;
            case Signal::AVDTP_GET_ALL_CAPABILITIES:
                OnGetAllCapabilities(SEID(signal), handler);
                break;
            case Signal::AVDTP_SET_CONFIGURATION:
                OnSetConfiguration(signal, handler);
                break;
            case Signal::AVDTP_OPEN:
                OnOpen(SEID(signal), handler);
                break;
            case Signal::AVDTP_CLOSE:
                OnClose(SEID(signal), handler);
                break;
            case Signal::AVDTP_START:
                OnStart(SEID(signal), handler);
                break;
            case Signal::AVDTP_SUSPEND:
                OnSuspend(SEID(signal), handler);
                break;
            case Signal::AVDTP_ABORT:
                OnAbort(SEID(signal), handler);
                break;
            default:
                TRACE_L1("Usupported signal %d", signal.Id());
                handler(Signal::errorcode::NOT_SUPPORTED_COMMAND);
                break;
            }
        }

    private:
        void OnDiscover(const Handler&);
        void OnGetCapabilities(const uint8_t seid, const Handler& handler);
        void OnGetAllCapabilities(const uint8_t seid, const Handler& handler);
        void OnSetConfiguration(const Signal& signal, const Handler& handler);
        void OnReconfigure(const Signal& signal, const Handler& handler);
        void OnOpen(const uint8_t seid, const Handler& handler);
        void OnClose(const uint8_t seid, const Handler& handler);
        void OnStart(const uint8_t seid, const Handler& handler);
        void OnSuspend(const uint8_t seid, const Handler& handler);
        void OnAbort(const uint8_t seid, const Handler& handler);

    private:
        uint8_t SEID(const Signal& signal) const
        {
            uint8_t seid = 0;

            signal.InspectPayload([&seid](const Payload& payload) {
                if (payload.Available() >= 1) {
                    payload.Pop(seid);
                    seid >>= 2;
                }
            });

            return (seid);
        }
        Signal::errorcode ToSignalCode(const uint32_t result)
        {
            switch (result)
            {
            case Core::ERROR_NONE:
                return (Signal::errorcode::SUCCESS);
            case Core::ERROR_UNAVAILABLE:
                return (Signal::errorcode::NOT_SUPPORTED_COMMAND);
            case Core::ERROR_ALREADY_CONNECTED:
                return (Signal::errorcode::SEP_IN_USE);
            case Core::ERROR_ALREADY_RELEASED:
                return (Signal::errorcode::SEP_NOT_IN_USE);
            case Core::ERROR_BAD_REQUEST:
                return (Signal::errorcode::UNSUPPORTED_CONFIGURATION);
            case Core::ERROR_ILLEGAL_STATE:
                return (Signal::errorcode::BAD_STATE);
            default:
                ASSERT(!"Undefined error");
                return (Signal::errorcode::BAD_STATE);
            }
        }

    private:
        Signal::errorcode DeserializeConfig(const Payload& config, StreamEndPoint& ep, uint8_t& invalidCategory,
                    const std::function<Signal::errorcode(const StreamEndPoint::Service::categorytype)>& verifyFn);

    }; // class Server

} // namespace AVDTP

} // namespace Bluetooth

}
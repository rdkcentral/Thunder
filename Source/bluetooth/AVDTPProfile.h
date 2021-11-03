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

namespace WPEFramework {

namespace Bluetooth {

    class EXTERNAL AVDTPProfile {
    public:
        typedef std::function<void(const uint32_t)> Handler;

        class EXTERNAL StreamEndPoint {
            friend class AVDTPProfile;

        public:
            class EXTERNAL ServiceCapabilities {
            public:
                enum category : uint8_t {
                    MEDIA_TRANSPORT     = 0x01,
                    REPORTING           = 0x02,
                    RECOVERY            = 0x03,
                    CONTENT_PROTECTION  = 0x04,
                    HEADER_COMPRESSION  = 0x05,
                    MULTIPLEXING        = 0x06,
                    MEDIA_CODEC         = 0x07,
                    DELAY_REPORTING     = 0x08
                };

            public:
                ServiceCapabilities(const uint8_t serviceCategory, const Buffer& data)
                    : _category(static_cast<category>(serviceCategory))
                    , _data(data)
                {
                    // capabilities data is beyond AVDTP, let the client deserialize
                }
                ~ServiceCapabilities() = default;

            public:
                category Category() const
                {
                    return (_category);
                }
                string Name() const
                {
                    Core::EnumerateType<category> value(_category);
                    string name = (value.IsSet() == true? string(value.Data()) : Core::ToString(_category));
                    return (name);
                }
                const Buffer& Data() const
                {
                    return (_data);
                }

            private:
                const category _category;
                const Buffer _data;
            }; // class ServiceCapabilities

        public:
            enum servicetype : uint8_t {
                SOURCE  = 0x00,
                SINK    = 0x01,
            };

            enum mediatype : uint8_t {
                AUDIO       = 0x00,
                VIDEO       = 0x01,
                MULTIMEDIA  = 0x02
            };

        public:
            StreamEndPoint() = delete;
            StreamEndPoint(const StreamEndPoint&) = delete;
            StreamEndPoint& operator=(const StreamEndPoint&) = delete;

            StreamEndPoint(const Buffer& endpointData)
                : _id(0)
                , _inUse(false)
                , _serviceType(SOURCE)
                , _mediaType(AUDIO)
                , _capabilities()
            {
                // deserialize the endpoint data
                const uint8_t* data = reinterpret_cast<const uint8_t*>(endpointData.data());
                _id = (data[0] >> 2);
                _inUse = (data[0] & 0x02);
                _mediaType = static_cast<mediatype>(data[1] >> 4);
                _serviceType = static_cast<servicetype>(!!(data[1] & 0x08));
            }
            ~StreamEndPoint() = default;

        public:
            uint32_t SEID() const
            {
                return (_id);
            }
            servicetype ServiceType() const
            {
                return (_serviceType);
            }
            mediatype MediaType() const
            {
                return (_mediaType);
            }
            bool IsFree() const
            {
                return (!_inUse);
            }
            const std::map<uint8_t, ServiceCapabilities>& Capabilities() const
            {
                return (_capabilities);
            }

        private:
            void CollectCapability(const uint8_t category, const Buffer& value)
            {
                _capabilities.emplace(std::piecewise_construct,
                                      std::forward_as_tuple(category),
                                      std::forward_as_tuple(category, value));
            }

        private:
            uint8_t _id;
            bool _inUse;
            servicetype _serviceType;
            mediatype _mediaType;
            std::map<uint8_t, ServiceCapabilities> _capabilities;
        }; // class StreamEndPoint

    public:
        AVDTPProfile()
            : _socket(nullptr)
            , _command()
            , _handler(nullptr)
            , _seps()
            , _sepsIterator(_seps.end())
            , _expired(0)
        {
        }
        AVDTPProfile(const AVDTPProfile&) = delete;
        AVDTPProfile& operator=(const AVDTPProfile&) = delete;
        ~AVDTPProfile() = default;

    public:
        const std::list<StreamEndPoint>& StreamEndPoints() const
        {
            return (_seps);
        }
        uint32_t Discover(const uint32_t waitTime, AVDTPSocket& socket, const Handler& handler)
        {
            uint32_t result = Core::ERROR_INPROGRESS;

            _handler = handler;
            _socket = &socket;
            _expired = Core::Time::Now().Add(waitTime).Ticks();

            // Firstly, pick up available SEPs
            _command.Discover();

            _socket->Execute(waitTime, _command, [&](const AVDTPSocket::Command& cmd) {
                if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {

                    _seps.clear();

                    cmd.Result().ReadDiscovery([this](const Buffer& sep) {
                        _seps.emplace_back(sep);
                    });

                    if (_seps.empty() == false) {
                        _sepsIterator = _seps.begin();
                        RetrieveBasicCapabilities();
                    } else {
                        // No SEPs...?
                        Report(Core::ERROR_NONE);
                    }
                } else {
                    Report(Core::ERROR_GENERAL);
                }
            });

            return (result);
        }

    private:
        void RetrieveBasicCapabilities()
        {
            // Secondly, for each endpoint pick up capabilities
            if (_sepsIterator != _seps.end()) {
                const uint32_t waitTime = AvailableTime();
                if (waitTime > 0) {
                    _command.GetCapabilities((*_sepsIterator).SEID());

                    _socket->Execute(waitTime, _command, [&](const AVDTPSocket::Command& cmd) {
                        if ((cmd.Status() == Core::ERROR_NONE) && (cmd.Result().Status() == AVDTPSocket::Command::Signal::SUCCESS)) {

                            cmd.Result().ReadConfiguration([this](const uint8_t category, const Buffer& data) {
                                (*_sepsIterator).CollectCapability(category, data);
                            });

                            _sepsIterator++;
                            RetrieveBasicCapabilities();
                        } else {
                            Report(Core::ERROR_GENERAL);
                        }
                    });
                }
            } else {
                // Out of endpoints to query
                Report(Core::ERROR_NONE);
            }
        }
        void Report(const uint32_t result)
        {
            if (_socket != nullptr) {
                Handler caller = _handler;
                _socket = nullptr;
                _handler = nullptr;
                _expired = 0;

                caller(result);
            }
        }
        uint32_t AvailableTime()
        {
            uint64_t now = Core::Time::Now().Ticks();
            uint32_t result = (now >= _expired ? 0 : static_cast<uint32_t>((_expired - now) / Core::Time::TicksPerMillisecond));
            if (result == 0) {
                Report(Core::ERROR_TIMEDOUT);
            }
            return (result);
        }

    public:
        AVDTPSocket* _socket;
        AVDTPSocket::Command _command;
        Handler _handler;
        std::list<StreamEndPoint> _seps;
        std::list<StreamEndPoint>::iterator _sepsIterator;
        uint64_t _expired;
    }; // class AVDTPProfile

} // namespace Bluetooth

}
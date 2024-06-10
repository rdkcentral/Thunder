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

#ifndef __WEBBRIDGEPROBE_H
#define __WEBBRIDGEPROBE_H

#include "Module.h"
#if defined(SECURITY_LIBRARY_INCLUDED)
#include <provision/Signature.h>
#endif

namespace Thunder {
namespace Plugin {

    class Probe {
    private:
        static const string SearchTarget;

        class WebTransform {
        private:
            WebTransform(const WebTransform&) = delete;
            WebTransform& operator=(const WebTransform&) = delete;

        public:
            inline WebTransform()
                : _keywordLength(static_cast<uint8_t>(_tcslen(Web::Request::MSEARCH)))
            {
            }
            inline ~WebTransform()
            {
            }

            // Methods to extract and insert data into the socket buffers
            uint16_t Transform(Web::Request::Deserializer& deserializer, uint8_t* dataFrame, const uint16_t maxSendSize);

        private:
            uint8_t _keywordLength;
        };

        class Listener : public Web::WebLinkType<Core::SocketDatagram, Web::Request, Web::Response, Core::ProxyPoolType<Web::Request>, WebTransform> {
        private:
            typedef Web::WebLinkType<Core::SocketDatagram, Web::Request, Web::Response, Core::ProxyPoolType<Web::Request>, WebTransform> BaseClass;

            Listener() = delete;
            Listener(const Listener&) = delete;
            Listener& operator=(const Listener&) = delete;

            struct Destination {
                Core::NodeId _destination;
                uint64_t _incomingTime;
                uint64_t _requestTime;
            };

        public:
            Listener(const Core::NodeId& address, const PluginHost::IShell* service, const string& model);
            virtual ~Listener();

        public:
            // Notification of a Partial Request received, time to attach a body..
            virtual void LinkBody(Core::ProxyType<Web::Request>& element);

            // Notification of a Request received.
            virtual void Received(Core::ProxyType<Web::Request>& text);

            // Notification of a Response send.
            virtual void Send(const Core::ProxyType<Web::Response>& text);

            // Notification of a channel state change..
            virtual void StateChange();

        private:
            bool ValidRequest(const Web::Request& request) const;
            Core::ProxyType<Web::Response> CreateResponse(const uint64_t incomingTime, const uint64_t requestTime);

        private:
            // This should be the "Response" as depicted by the parent/DIALserver.
            const PluginHost::IShell* _service;
            const string _model;
            std::list<Destination> _destinations;
        };

        class Broadcaster : public Core::SocketDatagram {
        private:
            Broadcaster() = delete;
            Broadcaster(const Broadcaster&) = delete;
            Broadcaster& operator=(const Broadcaster&) = delete;

            struct Destination {
                Core::NodeId _destination;
                uint8_t _timeToLive;
            };

        public:
            Broadcaster(Probe& parent, const Core::NodeId address,  const uint8_t timeToLive)
                : Core::SocketDatagram(false, address.AnyInterface(), address, 1024, 1024)
                , _parent(parent)
                , _adminLock()
                , _request()
                , _body(Core::ProxyType<Web::TextBody>::Create())
                , _answer(Core::ProxyType<Web::TextBody>::Create())
                , _destinations()
            {
                string text;
                _request.Verb = Web::Request::HTTP_MSEARCH;
                _request.ST = SearchTarget;

                if (Open(1000) == Core::ERROR_NONE) {
                    Destination newEntry;
                    newEntry._destination = address;
                    newEntry._timeToLive = timeToLive;
                    _destinations.push_back(newEntry);
                    Core::SocketDatagram::Trigger();
                } else {
                    ASSERT(false);
                }
            }
            virtual ~Broadcaster()
            {
                Close(Core::infinite);
            }

        public:
            void Ping(const uint8_t timeToLive)
            {
                if (IsOpen()) {
                    Destination newEntry;
                    newEntry._destination = RemoteNode();
                    newEntry._timeToLive = timeToLive;

                    _adminLock.Lock();
                    _destinations.push_back(newEntry);
                    _adminLock.Unlock();

                    Core::SocketDatagram::Trigger();
                }
            }
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {

                // We assume that this all fit a datagram. The datagram will *NOT* cross the datagram boundries.
                // Hence why this method will only be called once !!
                uint16_t size(0);

                _adminLock.Lock();

                if (_destinations.size() == 0) {
                    _adminLock.Unlock();
                } else {
                    Destination info(_destinations.front());
                    _destinations.pop_front();
                    _adminLock.Unlock();

                    std::string data;

                    // Set the Time-To-Live, to only the requested value. This means that the package can *NOT* cross any
                    // router in the network. We are looking for PluginHosts in *THIS* network.
                    // Not allowed to cross a VPN !!!!!
                    if ((TTL(info._timeToLive) == Core::ERROR_NONE) && ((data = CreateRequest()).empty() == false)) {

                        Core::SocketDatagram::RemoteNode(info._destination);

                        size = (maxSendSize > data.length() ? static_cast<uint8_t>(data.length()) : maxSendSize);
                        ::memcpy(dataFrame, data.c_str(), size);

                        _parent.Issued();
                    }
                }
                return (size);
            }

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
            }

            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize);

        private:
            std::string CreateRequest()
            {
                string text;
                string sendTimerInUS(Core::NumberType<uint64_t>(Core::Time::Now().Ticks()).Text());
                *_body = sendTimerInUS;

#if defined(SECURITY_LIBRARY_INCLUDED)
                uint8_t base64Buffer[Crypto::HASH_SHA256];

// The signblob returns a HMAC calculated over the data in Base64 format.
#ifdef __DEBUG__
                int length =
#endif
                    signBlob(
                        sendTimerInUS.length() * sizeof(TCHAR),
                        reinterpret_cast<const unsigned char*>(sendTimerInUS.c_str()),
                        sizeof(base64Buffer),
                        base64Buffer);

                ASSERT(length == Crypto::HASH_SHA256);

                _request.ContentSignature = Web::Signature(Crypto::HASH_SHA256, base64Buffer);
#endif

                _request.Body(_body);
                _request.ToString(text);
                return (Core::ToString(text));
            }

            bool ValidResponse(const Web::Response& response) const;

        private:
            Probe& _parent;
            Core::CriticalSection _adminLock;
            Web::Request _request;
            Core::ProxyType<Web::TextBody> _body;
            Core::ProxyType<Web::TextBody> _answer;
            std::list<Destination> _destinations;
        };

        Probe() = delete;
        Probe(const Probe&) = delete;
        Probe& operator=(const Probe&) = delete;

    public:
        class Instance {
        public:
            Instance()
                : _URL()
                , _latency(0)
                , _model()
                , _secure(false)
            {
            }
            Instance(const Core::URL& url, const uint32_t latency, const string& model, const bool secure)
                : _URL(url)
                , _latency(latency)
                , _model(model)
                , _secure(secure)
            {
            }
            Instance(const Instance& copy)
                : _URL(copy._URL)
                , _latency(copy._latency)
                , _model(copy._model)
                , _secure(copy._secure)
            {
            }
            Instance(Instance&& move)
                : _URL(std::move(move._URL))
                , _latency(move._latency)
                , _model(std::move(move._model))
                , _secure(move._secure)
            {
                move._latency = 0;
                move._secure = false;
            }
            ~Instance()
            {
            }

            Instance& operator=(const Instance& rhs)
            {
                _URL = rhs._URL;
                _latency = rhs._latency;
                _model = rhs._model;
                _secure = rhs._secure;

                return (*this);
            }

            Instance& operator=(Instance&& move)
            {
                if (this != &move) {
                    _URL = std::move(move._URL);
                    _latency = std::move(move._latency);
                    _model = std::move(move._model);
                   _secure = std::move(move._secure);
                }
                return (*this);
            }

        public:
            inline const Core::URL& URL() const
            {
                return (_URL);
            }
            inline uint32_t Latency() const
            {
                return (_latency);
            }
            inline const string& Model() const
            {
                return (_model);
            }
            inline bool IsSecure() const
            {
                return (_secure);
            }

        private:
            Core::URL _URL;
            uint32_t _latency;
            string _model;
            bool _secure;
        };
        typedef Core::IteratorType<const std::list<Instance>, const Instance&, std::list<Instance>::const_iterator> Iterator;

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        Probe(const Core::NodeId& source, const PluginHost::IShell* service, const uint8_t timeToLive, const string& model)
            : _adminLock()
            , _partners()
            , _probe(source, service, model)
            , _broadcast(*this, source, timeToLive)
            , _timeToLive(timeToLive)
        {

            ASSERT(timeToLive != 0);
        }
POP_WARNING()
        ~Probe()
        {
        }

    public:
        inline void Ping(const uint8_t timeToLive = 0)
        {
            _broadcast.Ping(timeToLive == 0 ? _timeToLive : timeToLive);
        }
        inline Iterator Instances() const
        {
            return (Iterator(_partners));
        }

    private:
        void Issued()
        {
            _adminLock.Lock();
            _partners.clear();
            _adminLock.Unlock();
        }

        void Report(const Core::URL& remote, const uint32_t latencyinUs, const string& model, const bool secure);

    private:
        Core::CriticalSection _adminLock;
        std::list<Instance> _partners;
        Listener _probe;
        Broadcaster _broadcast;
        uint8_t _timeToLive;
    };
}
}

#endif // __WEBBRIDGEPROBE_H

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

#include "Probe.h"

namespace Thunder {
namespace Plugin {

    /* static */ const string Probe::SearchTarget(_T("urn:metrological-com:service:wpe:1"));
    /* static  const Core::NodeId Probe::Interface(_T("239.255.255.250"), 1900, Core::NodeId::TYPE_IPV4); */

    static Core::ProxyPoolType<Web::TextBody> _textResponseFactory(2);

    class Discovery {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Discovery(const Discovery& a_Copy) = delete;
        Discovery& operator=(const Discovery& a_RHS) = delete;

    public:
        Discovery(const string& message)
        {
            _text = Core::ToString(message);
        }
        Discovery(const string& message, const Core::NodeId& nodeId)
        {
            _text = Core::ToString(message + ": [" + nodeId.HostAddress() + "]");
        }
        Discovery(const Core::ProxyType<Web::Request>& request, const Core::NodeId& nodeId)
        {
            if (request.IsValid() == true) {
                string text;
                request->ToString(text);
                _text = Core::ToString(string("\n[" + nodeId.HostAddress() + ']' + text + '\n'));
            }
        }
        Discovery(const Core::URL& remote, const uint32_t latencyinUs)
        {
            _text = "Discovered: [" + remote.Text() + "] with latency: " + Core::NumberType<uint32_t>(latencyinUs).Text() + " uS";
        }
        Discovery(const uint32_t latencyinUs)
        {
            _text = "Internal adjustment latency: [" + Core::NumberType<uint32_t>(latencyinUs).Text() + "] uS";
        }
        Discovery(const Core::ProxyType<Web::Response>& response, const Core::NodeId& nodeId)
        {
            if (response.IsValid() == true) {
                string text;
                response->ToString(text);
                _text = Core::ToString(string("\n[" + nodeId.HostAddress() + ']' + text + '\n'));
            }
        }
        ~Discovery()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    uint16_t Probe::WebTransform::Transform(Web::Request::Deserializer& deserializer, uint8_t* dataFrame, const uint16_t maxSendSize)
    {
        uint16_t index = 0;
        const uint8_t* checker = dataFrame;

        // This is a UDP service, so a message should be complete. If the first keyword is not a keyword we
        // expect, ignore the full message, it is not a m-search package, it does not require any further
        // processing.
        // First skip the white sace, if applicable...
        while (isspace(*checker)) {
            checker++;
        }

        // Now see if the first keyword is "M-SEARCH"
        while ((index < _keywordLength) && (toupper(*checker) == Web::Request::MSEARCH[index])) {
            checker++;
            index++;
        }

        // Now we need to find the keyword "M-SEARCH" see if we have it..
        if (index == _keywordLength) {
            deserializer.Flush();
            index = deserializer.Deserialize(dataFrame, maxSendSize);
        } else {
            index = maxSendSize;
        }

        return (index);
    }

    Probe::Listener::Listener(const Core::NodeId& node, const PluginHost::IShell* service, const string& model)
        : BaseClass(5, false, node, node.AnyInterface(), 1024, 1024)
        , _service(service)
        , _model(model)
        , _destinations()
    {

        if (Link().Open(1000) != Core::ERROR_NONE) {
            ASSERT(false && "Seems we can not open the discovery port");
        }

        Link().Join(node);
    }

    /* virtual */ Probe::Listener::~Listener()
    {
        Link().Leave(Link().LocalNode());
        Link().Close(Core::infinite);
    }

    // Notification of a Partial Request received, time to attach a body..
    /* virtual */ void Probe::Listener::LinkBody(Core::ProxyType<Web::Request>& element)
    {
        // We expect some text data.
        element->Body(_textResponseFactory.Element());
    }
    // Notification of a Partial Request received, time to attach a body..
    // upnp requests are empty so no body needed...
    /* virtual */ void Probe::Listener::Received(Core::ProxyType<Web::Request>& request)
    {
        uint64_t incomingTime = Core::Time::Now().Ticks();
        Core::NodeId sourceNode(Link().ReceivedNode());

        if ((request->Verb == Web::Request::HTTP_MSEARCH) && (request->ST.IsSet() == true)) {
            if ((request->ST.Value() == Probe::SearchTarget) && (request->HasBody() == true) && (ValidRequest(*request) == true)) {
                string body(*(request->Body<Web::TextBody>()));
                Destination newEntry;
                newEntry._destination = sourceNode;
                newEntry._requestTime = Core::NumberType<uint64_t>(body.c_str(), static_cast<uint32_t>(body.length())).Value();
                newEntry._incomingTime = incomingTime;
                _destinations.push_back(newEntry);
                TRACE(Discovery, (_T("Valid discovery request. Responding to"), sourceNode));
                // remember the NodeId where this comes from.
                if (_destinations.size() == 1) {

                    Link().RemoteNode(_destinations.front()._destination);

                    Submit(CreateResponse(_destinations.front()._incomingTime, _destinations.front()._requestTime));
                }
            }
        }
    }

    // Notification of a Response send.
    /* virtual */ void Probe::Listener::Send(const Core::ProxyType<Web::Response>& response)
    {
        TRACE(Discovery, (response, _destinations.front()._destination));

        // Drop the current destination.
        _destinations.pop_front();

        if (_destinations.size() > 0) {

            // Move on to notifying the next one.
            Link().RemoteNode(_destinations.front()._destination);

            Submit(CreateResponse(_destinations.front()._incomingTime, _destinations.front()._requestTime));
        }
    }

    // Notification of a channel state change..
    /* virtual */ void Probe::Listener::StateChange()
    {
    }

    bool Probe::Listener::ValidRequest(const Web::Request& request) const
    {

#if defined(SECURITY_LIBRARY_INCLUDED)
        if ((request.ContentSignature.IsSet() == true) && (request.ContentSignature.Value().Type() == Crypto::HASH_SHA256)) {
            const string value(*(request.Body<Web::TextBody>()));
            bool valid = (validateBlob(
                              value.length(),
                              reinterpret_cast<const unsigned char*>(value.c_str()),
                              Crypto::HASH_SHA256,
                              request.ContentSignature.Value().Data())
                == 0);
            if (valid == false) {
                TRACE(Discovery, (string(_T("Received an invalid Discovery request."))));
            }
            return (valid);
        }
        TRACE(Discovery, (string(_T("Received a Discovery request without signature."))));
#else
        DEBUG_VARIABLE(request);
#endif
        return (true);
    }

    Core::ProxyType<Web::Response> Probe::Listener::CreateResponse(const uint64_t incomingTime, const uint64_t requestTime)
    {
        Core::ProxyType<Web::Response> response(PluginHost::IFactories::Instance().Response());
        Core::ProxyType<Web::TextBody> body(_textResponseFactory.Element());

        uint32_t delta = static_cast<uint32_t>(Core::NumberType<uint64_t>(Core::Time::Now().Ticks()) - incomingTime);
        uint64_t newTime = requestTime + delta;
        string newBody = Core::NumberType<uint64_t>(newTime).Text();

        if (_model.empty() == false) {
            newBody += (' ' + _model);
        }

        TRACE(Discovery, (delta));

#if defined(SECURITY_LIBRARY_INCLUDED)
        uint8_t base64Buffer[Crypto::HASH_SHA256];

// The signblob returns a HMAC calculated over the data in Base64 format.
#ifdef __DEBUG__
        int length =
#endif
            signBlob(
                newBody.length() * sizeof(TCHAR),
                reinterpret_cast<const unsigned char*>(newBody.c_str()),
                sizeof(base64Buffer),
                base64Buffer);

        ASSERT(length == Crypto::HASH_SHA256);

        response->ContentSignature = Web::Signature(Crypto::HASH_SHA256, base64Buffer);
#endif
        *body = newBody;
        response->Body(body);
        response->ErrorCode = Web::STATUS_OK;
        response->Message = _T("OK");
        response->ApplicationURL = Core::URL(_service->Accessor());

        TRACE(Discovery, (response, Link().RemoteNode()));

        return (response);
    }

    /* virtual*/ uint16_t Probe::Broadcaster::ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
    {

        uint64_t stamp(Core::NumberType<uint64_t>(Core::Time::Now().Ticks()));

        // Here come the responses, see f they are what we expect them to be..
        Web::Response response;
        response.Body(Core::ProxyType<Web::IBody>(_answer));

        response.FromString(string(reinterpret_cast<const char*>(dataFrame), receivedSize));

        if ((response.ApplicationURL.IsSet() == true) && (ValidResponse(response) == true)) {

            uint64_t latency;
            string model;
            size_t length = _answer->find(' ', 0);
            bool secure = ((response.ContentSignature.IsSet() == true) && (response.ContentSignature.Value().Type() == Crypto::HASH_SHA256));

            if (length == string::npos) {
                latency = Core::NumberType<uint64_t>(_answer->c_str(), static_cast<uint32_t>(_answer->length())).Value();
            } else {
                latency = Core::NumberType<uint64_t>(_answer->c_str(), static_cast<uint32_t>(length)).Value();
                model = _answer->substr(length + 1, string::npos);
            }
            latency = (stamp - latency);

            if (latency < 0xFFFFFFFF) {
                _parent.Report(response.ApplicationURL.Value(), static_cast<uint32_t>(latency), model, secure);
            } else {
                TRACE(Discovery, (string(_T("Received a Discovery request with an abnormal latency."))));
            }
        }

        return (receivedSize);
    }

    bool Probe::Broadcaster::ValidResponse(const Web::Response& response) const
    {

#if defined(SECURITY_LIBRARY_INCLUDED)
        if ((response.ContentSignature.IsSet() == true) && (response.ContentSignature.Value().Type() == Crypto::HASH_SHA256)) {
            const string value(*(response.Body<Web::TextBody>()));

            bool valid = (validateBlob(
                              value.length(),
                              reinterpret_cast<const unsigned char*>(value.c_str()),
                              Crypto::HASH_SHA256,
                              response.ContentSignature.Value().Data())
                == 0);
            if (valid == false) {
                TRACE(Discovery, (string(_T("Received an invalid Discovery request."))));
            }
            return (valid);
        }
#else
        DEBUG_VARIABLE(response);
#endif

        return (true);
    }

    void Probe::Report(const Core::URL& remote, const uint32_t latencyinUs, const string& model, const bool secure)
    {
        _adminLock.Lock();
        TRACE(Discovery, (remote, latencyinUs));
        _partners.push_back(Instance(remote, latencyinUs, model, secure));
        _adminLock.Unlock();
    }
}

} // namespace Plugin

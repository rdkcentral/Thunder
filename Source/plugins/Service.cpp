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

#include "Service.h"
#include "Channel.h"

namespace Thunder {

namespace PluginHost {

    PluginHost::Request::Request()
        : Web::Request()
        , _state(INCOMPLETE | SERVICE_CALL)
        , _service()
    {
    }
    /* virtual */ PluginHost::Request::~Request()
    {
    }

    void PluginHost::Request::Clear()
    {
        Web::Request::Clear();
        _state = INCOMPLETE | SERVICE_CALL;

        if (_service.IsValid()) {
            _service.Release();
        }
    }
    void PluginHost::Request::Service(const uint32_t errorCode, const Core::ProxyType<PluginHost::Service>& service, const bool serviceCall)
    {
        ASSERT(_service.IsValid() == false);
        ASSERT(State() == INCOMPLETE);

        uint8_t value = (serviceCall ? SERVICE_CALL : 0);

        if (service.IsValid() == true) {
            _state = COMPLETE | value;
            _service = service;
        } else if (errorCode == Core::ERROR_BAD_REQUEST) {
            _state = OBLIVIOUS | value;
        } else if (errorCode == Core::ERROR_INVALID_SIGNATURE) {
            _state = INVALID_VERSION | value;
        } else if (errorCode == Core::ERROR_UNAVAILABLE) {
            _state = MISSING_CALLSIGN | value;
        }
    }

    /* static */ Core::ProxyType<Core::IDispatch> IShell::Job::Create(IShell* shell, IShell::state toState, IShell::reason why)
    {
        return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<IShell::Job>::Create(shell, toState, why)));
    }

#if THUNDER_RESTFULL_API
    void Service::Notification(const string& message)
    {
        _notifierLock.Lock();

        ASSERT(message.empty() != true);
        {
            std::list<Channel*>::iterator index(_notifiers.begin());

            while (index != _notifiers.end()) {
                (*index)->Submit(message);
                index++;
            }
        }

        _notifierLock.Unlock();
    }
#endif

    void Service::FileToServe(const string& webServiceRequest, Web::Response& response, bool allowUnsafePath)
    {
        Web::MIMETypes result;
        Web::EncodingTypes encoding = Web::ENCODING_UNKNOWN;
        uint16_t offset = static_cast<uint16_t>(_config.WebPrefix().length()) + (_webURLPath.empty() ? 1 : static_cast<uint16_t>(_webURLPath.length()) + 2);
        string fileToService = _webServerFilePath;
        if ((webServiceRequest.length() <= offset) || (Web::MIMETypeAndEncodingForFile(webServiceRequest.substr(offset, -1), fileToService, result, encoding) == false)) {
            Core::ProxyType<Web::FileBody> fileBody(IFactories::Instance().FileBody());

            // No filename gives, be default, we go for the index.html page..
            *fileBody = fileToService + _T("index.html");
            response.ContentType = Web::MIME_HTML;
            response.Body<Web::FileBody>(fileBody);
        } else {
            ASSERT(fileToService.length() >= _webServerFilePath.length());
            bool safePath = true;
            string normalizedPath = Core::File::Normalize(fileToService.substr(_webServerFilePath.length()), safePath);

            if (allowUnsafePath || safePath ) {
                Core::ProxyType<Web::FileBody> fileBody(IFactories::Instance().FileBody());
                *fileBody = fileToService;
                response.ContentType = result;
                if (encoding != Web::ENCODING_UNKNOWN) {
                    response.ContentEncoding = encoding;
                }
                response.Body<Web::FileBody>(fileBody);
            } else {
                response.ErrorCode = Web::STATUS_BAD_REQUEST;
                response.Message = "Invalid Request";
            }
        }
    }

    bool Service::IsWebServerRequest(const string& segment) const
    {
        // Prefix length, no need to compare, that has already been doen, otherwise
        // this call would not be placed.
        uint32_t prefixLength = static_cast<uint32_t>(_config.WebPrefix().length());
        uint32_t webLength = static_cast<uint32_t>(_webURLPath.length());

        return ((_webServerFilePath.empty() == false) && (segment.length() >= prefixLength + webLength) && ((webLength == 0) || (segment.compare(prefixLength + 1, webLength, _webURLPath) == 0)));
    }
}

namespace Plugin {

    /* static */ Core::NodeId Config::IPV4UnicastNode(const string& ifname)
    {
        Core::AdapterIterator adapters;

        Core::IPV4AddressIterator result;

        if (ifname.empty() == false) {
            // Seems we neeed to bind to an interface
            // It might be an interface name, try to resolve it..
            while ((adapters.Next() == true) && (adapters.Name() != ifname)) {
                // Intentionally left empty...
            }

            if (adapters.IsValid() == true) {
                bool found = false;
                Core::IPV4AddressIterator index(adapters.IPV4Addresses());

                while ((found == false) && (index.Next() == true)) {
                    Core::NodeId current(index.Address());
                    if (current.IsMulticast() == false) {
                        result = index;
                        found = (current.IsLocalInterface() == false);
                    }
                }
            }
        } else {
            bool found = false;

            // time to find the public interface
            while ((found == false) && (adapters.Next() == true)) {
                Core::IPV4AddressIterator index(adapters.IPV4Addresses());

                while ((found == false) && (index.Next() == true)) {
                    Core::NodeId current(index.Address());
                    if ((current.IsMulticast() == false) && (current.IsLocalInterface() == false)) {
                        result = index;
                        found = true;
                    }
                }
            }
        }
        return (result.IsValid() ? result.Address() : Core::NodeId());
    }
}
}

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

#ifndef __WEBTRANSFER_H
#define __WEBTRANSFER_H

#include "Module.h"
#include "URL.h"
#include "WebLink.h"
#include "WebSerializer.h"

namespace Thunder {
namespace Web {
    template <typename ELEMENT>
    class SingleElementFactoryType {
    private:
        SingleElementFactoryType(const SingleElementFactoryType<ELEMENT>&) = delete;
        SingleElementFactoryType<ELEMENT>& operator=(const SingleElementFactoryType<ELEMENT>&) = delete;

    public:
        inline SingleElementFactoryType(const uint8_t /* queuSize */)
        {
            _singleElement.AddRef();
        }
        inline ~SingleElementFactoryType()
        {
            _singleElement.CompositRelease();
        }

    public:
        inline Core::ProxyType<ELEMENT> Element()
        {
            return (Core::ProxyType<ELEMENT>(_singleElement));
        }

    private:
        Core::ProxyObject<ELEMENT> _singleElement;
    };

    template <typename LINK, typename FILEBODY>
    class ClientTransferType {
    public:
        enum enumTransferState {
            TRANSFER_IDLE,
            TRANSFER_INFO,
            TRANSFER_UPLOAD,
            TRANSFER_DOWNLOAD
        };

    private:
        class Channel : public WebLinkType<LINK, Web::Response, Web::Request, SingleElementFactoryType<Web::Response>> {
        private:
            static const uint32_t ELEMENTFACTORY_QUEUESIZE = 1;

            typedef WebLinkType<LINK, Web::Response, Web::Request, SingleElementFactoryType<Web::Response>> BaseClass;
            typedef ClientTransferType<LINK, FILEBODY> ThisClass;

        public:
            template <typename... Args>
            Channel(ThisClass& parent, Args&&... args)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, std::forward<Args>(args)...)
                , _parent(parent)
                , _request()
            {
            }
            ~Channel() override
            {
            }

        public:
            uint32_t StartTransfer(const Core::ProxyType<Web::Request>& request)
            {
                ASSERT(_request.IsValid() == false);
                ASSERT(_response.IsValid() == false);

                uint32_t result = Core::ERROR_NONE;

                if (BaseClass::IsOpen() == true) {
                    BaseClass::Submit(request);
                } else {
                    _request = request;
                    result = BaseClass::Open(0);
                }
                return result;
            }
            void Close() {
                BaseClass::Close(Core::infinite);
                BaseClass::Flush();
                Clear();
            }
            inline void Clear() {
                if (_request.IsValid() == true) {
                    _request.Release();
                }
                if (_response.IsValid() == true) {
                    _response.Release();
                }
            }

        private:
            // Notification of a Partial Request received, time to attach a body..
            void LinkBody(Core::ProxyType<Web::Response>& element) override
            {
                _parent.LinkBody(element);
            }

            // Notification of a Request received.
            void Received(Core::ProxyType<Web::Response>& response) override
            {
                // Right we got what we wanted, process it..
                _response = response;

                BaseClass::Close(0);
            }

            // Notification of a Response send.
            void Send(const Core::ProxyType<Web::Request>& request) override
            {
                ASSERT(_request->IsValid() != false);
                ASSERT(_request == request);
                DEBUG_VARIABLE(request);
                // Oke the message is gone, ready for a new one !!
            }

            // Notification of a channel state change..
            void StateChange() override
            {
                if (BaseClass::IsOpen() == true) {
                    ASSERT(_request->IsValid() == true);

                    BaseClass::Submit(_request);
                } else if ((_response.IsValid() == true) || (BaseClass::IsClosed() == true) || (BaseClass::IsSuspended() == true)) {
                    // Close the link and thus the transfer..
                    _parent.EndTransfer(_response);
               }
            }

        private:
            ThisClass& _parent;
            Core::ProxyType<Web::Request> _request;
            Core::ProxyType<Web::Response> _response;
        };

    public:
        ClientTransferType() = delete;
        ClientTransferType(const ClientTransferType<LINK, FILEBODY>& copy) = delete;
        ClientTransferType<LINK, FILEBODY>& operator=(const ClientTransferType<LINK, FILEBODY>& RHS) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        template <typename... Args>
        ClientTransferType(Args&&... args)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, std::forward<Args>(args)...)
        {
            _fileBody.AddRef();
            _request.AddRef();
        }
POP_WARNING()

        virtual ~ClientTransferType()
        {
            _channel.Close();
            _request.CompositRelease();
            _fileBody.CompositRelease();
        }

    public:
        uint32_t CollectInfo(const Core::URL& source)
        {
            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();

            if (_state == TRANSFER_IDLE) {
                result = Core::ERROR_INCORRECT_URL;

                if (source.IsValid() == true) {
                    result = Core::ERROR_COULD_NOT_SET_ADDRESS;

                    if (Setup(source) == true) {
                        result = Core::ERROR_NONE;

                        _state = TRANSFER_INFO;
                        _request.Verb = Web::Request::HTTP_HEAD;
                        _request.Path = '/' + source.Path().Value();
                        _request.Host = source.Host().Value();

                        // Prepare the request for processing
                        result = _channel.StartTransfer(Core::ProxyType<Web::Request>(_request));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Upload(const Core::URL& destination, const Core::File& source)
        {
            uint32_t result = Core::ERROR_INPROGRESS;

            // We need a file that is open and from which we can read !!!
            ASSERT(source.IsOpen() == true);

            _adminLock.Lock();

            if (_state == TRANSFER_IDLE) {
                result = Core::ERROR_INCORRECT_URL;

                if (destination.IsValid() == true) {
                    result = Core::ERROR_COULD_NOT_SET_ADDRESS;

                    if (Setup(destination) == true) {
                        result = Core::ERROR_NONE;

                        // See if we can create a file to store the upload in
                        static_cast<FILEBODY&>(_fileBody) = source;

                        _state = TRANSFER_UPLOAD;
                        _request.Verb = Web::Request::HTTP_PUT;
                        _request.Path = '/' + destination.Path().Value();
                        _request.Host = destination.Host().Value();
                        _request.Body(Core::ProxyType<FILEBODY>(_fileBody));

                        // Maybe we need to add a hash value...
                        _CalculateHash(_request);

                        // Prepare the request for processing
                        result = _channel.StartTransfer(Core::ProxyType<Web::Request>(_request));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        uint32_t Download(const Core::URL& source, Core::File& destination, const uint64_t position)
        {
            uint32_t result = Core::ERROR_INPROGRESS;

            // We need a file that is open and to which we can write, so not READ-ONLY !!!
            ASSERT((destination.IsOpen() == true) && (destination.IsReadOnly() == false));

            _adminLock.Lock();

            if (_state == TRANSFER_IDLE) {
                result = Core::ERROR_INCORRECT_URL;

                if (source.IsValid() == true) {
                    result = Core::ERROR_COULD_NOT_SET_ADDRESS;

                    if (Setup(source) == true) {
                        result = Core::ERROR_NONE;

                        // See if we can create a file to store the download in
                        static_cast<FILEBODY&>(_fileBody) = destination;
                        _fileBody.Position(false, position);

                        _state = TRANSFER_DOWNLOAD;
                        _request.Verb = Web::Request::HTTP_GET;
                        _request.Path = '/' + source.Path().Value();
                        _request.Host = source.Host().Value();

                        if (position) {
                            _request.Range = "bytes=" + std::to_string(position) + '-';
                        }

                        // Prepare the request for processing
                        result = _channel.StartTransfer(Core::ProxyType<Web::Request>(_request));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        inline uint64_t FileSize() const
        {
            return (_fileBody.Core::File::Size());
        }
        inline uint64_t Transferred() const
        {
            return (_fileBody.Position());
        }
        inline void LoadHash(const Crypto::Context& context)
        {
            _LoadHash(context);
        }
        inline void Close()
        {
            _channel.Close();
        }

        virtual bool Setup(const Core::URL& remote) = 0;
        virtual void InfoCollected(const uint32_t result, const Core::ProxyType<Web::Response>& info) = 0;
        virtual void Transferred(const uint32_t result, const FILEBODY& file) = 0;

    protected:
        inline LINK& Link()
        {
            return (_channel.Link());
        }
        inline const LINK& Link() const
        {
            return (_channel.Link());
        }
 
    private:
    

        inline void EndTransfer(const Core::ProxyType<Web::Response>& response)
        {
            uint32_t errorCode = Core::ERROR_NONE;

            // We are done, change state
            _adminLock.Lock();
            if (response.IsValid() == true) {
                if (_state == TRANSFER_DOWNLOAD) {
                    _fileBody.Core::File::LoadFileInfo();
                }

                if (response->ErrorCode == Web::STATUS_NOT_FOUND) {
                    errorCode = Core::ERROR_UNAVAILABLE;
                } else if ((((response->ErrorCode == STATUS_OK) || (response->ErrorCode == STATUS_PARTIAL_CONTENT)) && (_state == TRANSFER_DOWNLOAD)) &&
                           (((Transferred() == 0) && (FileSize() == 0)) || (FileSize() < Transferred()))) {
                    errorCode = Core::ERROR_WRITE_ERROR;
                } else if ((response->ErrorCode == Web::STATUS_UNAUTHORIZED) || 
                          ((_state == TRANSFER_DOWNLOAD) && (_ValidateHash(response->ContentSignature) == false))) {
                    errorCode = Core::ERROR_INCORRECT_HASH;
                } else if (response->ErrorCode == Web::STATUS_REQUEST_RANGE_NOT_SATISFIABLE) {
                    errorCode = Core::ERROR_INVALID_RANGE;
                }
            } else {
                errorCode = Core::ERROR_UNAVAILABLE;
            }

            if (_state == TRANSFER_INFO) {
                InfoCollected(errorCode, response);
            } else {
                Transferred(errorCode, (static_cast<FILEBODY&>(_fileBody)));
            }

            _state = TRANSFER_IDLE;
            if (response.IsValid() == true) {
                response.Release();
            }
            _channel.Clear();
            _adminLock.Unlock();
        }
        // Notification of a Partial Request received, time to attach a body..
        inline void LinkBody(Core::ProxyType<Web::Response>& element)
        {
            if (_fileBody.Exists() == true) {

                element->Body(Core::ProxyType<FILEBODY>(_fileBody));
                _fileBody.Release();
            }
        }

        IS_MEMBER_AVAILABLE(Hash, hasHash);

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _CalculateHash(Web::Request& request)
        {
            uint8_t   buffer[64];
            typename ACTUALFILEBODY::HashType& hash = _fileBody.Hash();
            uint32_t  pos  = _fileBody.Position();
            uint32_t  size = _fileBody.Core::File::Size() - pos;

            hash.Reset();

            // Read all Data to calculate the HASH/HMAC
            while (size > 0) {
                uint16_t chunk = std::min(static_cast<uint16_t>(sizeof(buffer)), static_cast<uint16_t>(size));
                _fileBody.Serialize(buffer, chunk);
                hash.Input(buffer, chunk);
                size -= chunk;
            }

            _fileBody.Position(false, pos);

            const Crypto::EnumHashType type = ACTUALFILEBODY::HashType::Type;
            request.ContentSignature = Signature(type, hash.Result());
        }

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<!hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _CalculateHash()
        {
        }

        template <typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, bool>::type
        _ValidateHash(const Core::OptionalType<Signature>& signature) const
        {
            // See if this is a valid. frame
            typename ACTUALFILEBODY::HashType& hash = const_cast<typename ACTUALFILEBODY::HashType&>(_fileBody.Hash());
            const Crypto::EnumHashType type = ACTUALFILEBODY::HashType::Type;
            return ((signature.IsSet() == false) || (signature.Value().Equal(type, hash.Result()) == true));
        }

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<!hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, bool>::type
        _ValidateHash(const Core::OptionalType<Signature>& signature) const
        {
            return (true);
        }

        IS_MEMBER_AVAILABLE(Hash, hasLoadHash);

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<hasLoadHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _LoadHash(const Crypto::Context& context)
        {
            typename ACTUALFILEBODY::HashType& hash = const_cast<typename ACTUALFILEBODY::HashType&>(_fileBody.Hash());
            hash.Reset();
            hash.Load(context);
        }

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<!hasLoadHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _LoadHash(const Crypto::Context& context)
        {
        }

    private:
        Core::CriticalSection _adminLock;
        enumTransferState _state;
        Core::ProxyObject<Web::Request> _request;
        Core::ProxyObject<FILEBODY> _fileBody;
        Channel _channel;
    };

    template <typename LINK, typename FILEBODY>
    class ServerTransferType : public WebLinkType<LINK, Web::Request, Web::Response, SingleElementFactoryType<Web::Request>> {
    private:
        typedef WebLinkType<LINK, Web::Request, Web::Response, SingleElementFactoryType<Web::Request>> BaseClass;
        typedef ServerTransferType<LINK, FILEBODY> ThisClass;

    public:
        ServerTransferType() = delete;
        ServerTransferType(const ServerTransferType<LINK, FILEBODY>& copy) = delete;
        ServerTransferType<LINK, FILEBODY>& operator=(const ServerTransferType<LINK, FILEBODY>& RHS) = delete;

        template <typename... Args>
        ServerTransferType(const string& pathPrefix, Args&&... args)
            : BaseClass(1, std::forward<Args>(args)...)
            , _pathPrefix(pathPrefix)
            , _fileBody(Core::ProxyType<FILEBODY>::Create())
            , _response(Core::ProxyType<Web::Response>::Create())
        {
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));
        }
        ~ServerTransferType() override
        {
            BaseClass::Close(Core::infinite);
        }

        virtual string Authorize(const Web::Request& request) = 0;

    private:
        // Notification of a Partial Request received, time to attach a body..
        void LinkBody(Core::ProxyType<Web::Request>& element) override
        {
            if (element->Verb == Web::Request::HTTP_PUT) {
                static_cast<FILEBODY&>(*_fileBody) = _pathPrefix + element->Path;

                if (_fileBody->Create() == true) {
                    // Seems we will receive a file.
                    element->Body(_fileBody);
                }
            }
        }

        // Notification of a Request received.
        void Received(Core::ProxyType<Web::Request>& element) override
        {
            // Right we got what we wanted, process it..
            if (element->Verb == Web::Request::HTTP_PUT) {
                if (_fileBody->IsOpen() == false) {
                    _response->ErrorCode = Web::STATUS_NOT_FOUND;
                    _response->Message = _T("File: ") + element->Path + _T(" could not be stored server side.");
                } else {
                    // See if the keys we received correspond.
                    if (_ValidateHash<FILEBODY>(element->ContentSignature) == true) {
                        string message = Authorize(*element);

                        if (message.empty() == true) {
                            _response->ErrorCode = Web::STATUS_OK;
                            _response->Message = _T("File: ") + element->Path + _T(" has been stored server side.");
                        } else {
                            // Somehow we are not Authorzed. Kill it....
                            _response->ErrorCode = Web::STATUS_UNAUTHORIZED;
                            _response->Message = message;
                        }
                    } else {
                        // Delete the file, signature is NOT oke.
                        _response->ErrorCode = Web::STATUS_UNAUTHORIZED;
                        _response->Message = _T("File: ") + element->Path + _T(" has an incorrect signature.");
                    }

                    if (_response->ErrorCode != Web::STATUS_OK) {
                        _fileBody->Destroy();
                    } else {
                        _fileBody->Close();
                    }
                }
            } else if (element->Verb == Web::Request::HTTP_GET) {
                static_cast<FILEBODY&>(*_fileBody) = (_pathPrefix + element->Path);

                if (_fileBody->Exists() == true) {
                    string message = Authorize(*element);

                    if (message.empty() == true) {
                        _CalculateHash(*_response);
                        _response->Body(_fileBody);
                    } else {
                        // Somehow we are not Authorzed. Kill it....
                        _response->ErrorCode = Web::STATUS_UNAUTHORIZED;
                        _response->Message = message;
                    }
                } else {
                    _response->ErrorCode = Web::STATUS_NOT_FOUND;
                    _response->Message = _T("File: ") + element->Path + _T(" was not found server side.");
                }
            }
            else {
                _response->ErrorCode = Web::STATUS_BAD_REQUEST;
                _response->Message = _T("Unknown command received.");
            }
            BaseClass::Submit(_response);
        }

        // Notification of a Response send.
        void Send(const Core::ProxyType<Web::Response>& response) override
        {
            // If we send the message, we are done.
            response.Release();
            BaseClass::Close(0);
        }

        // Notification of a channel state change..
        void StateChange() override
        {
        }

    private:
        IS_MEMBER_AVAILABLE(Hash, hasHash);

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _CalculateHash(Web::Response& request)
        {
            uint8_t   buffer[64];
            typename ACTUALFILEBODY::HashType& hash = _fileBody->Hash();
            uint32_t  pos  = _fileBody->Position();
            std::size_t  size = _fileBody->Size() - pos;

            hash.Reset();

            // Read all Data to calculate the HASH/HMAC
            while (size > 0) {
                uint16_t chunk = std::min(static_cast<uint16_t>(sizeof(buffer)), static_cast<uint16_t>(size));
                _fileBody->Serialize(buffer, chunk);
                hash.Input(buffer, chunk);
                size -= chunk;
            }

            _fileBody->Position(false,pos);
            const Crypto::EnumHashType type = ACTUALFILEBODY::HashType::Type;
            request.ContentSignature = Signature(type, hash.Result());
        }

        template < typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<!hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, void>::type
        _CalculateHash(Web::Response& request)
        {
        }

        template <typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, bool>::type
        _ValidateHash(const Core::OptionalType<Signature>& signature) const
        {
            // See if this is a valid. frame
            typename ACTUALFILEBODY::HashType& hash = _fileBody->Hash();
            const Crypto::EnumHashType type = ACTUALFILEBODY::HashType::Type;
            return ((signature.IsSet() == false) || (signature.Value().Equal(type, hash.Result()) == true));
        }

        template <typename ACTUALFILEBODY = FILEBODY>
        inline typename Core::TypeTraits::enable_if<!hasHash<const ACTUALFILEBODY, const typename ACTUALFILEBODY::HashType&>::value, bool>::type
        _ValidateHash(const Core::OptionalType<Signature>& signature) const
        {
            return (true);
        }

    private:
        const string _pathPrefix;
        Core::ProxyType<FILEBODY> _fileBody;
        Core::ProxyType<Web::Response> _response;
    };
}
} // namespace Solutions.HTTP

#endif

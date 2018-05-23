#ifndef __WEBTRANSFER_H
#define __WEBTRANSFER_H

#include "Module.h"
#include "WebLink.h"
#include "URL.h"
#include "WebSerializer.h"

namespace WPEFramework {
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
            return (Core::ProxyType<ELEMENT>(&_singleElement, &_singleElement));
        }

    private:
        Core::ProxyObject<ELEMENT> _singleElement;
    };

    template <typename LINK, typename FILEBODY>
    class ClientTransferType {
    public:
        enum enumTransferState {
            TRANSFER_IDLE,
            TRANSFER_UPLOAD,
            TRANSFER_DOWNLOAD
        };

    private:
        class Channel : public WebLinkType<LINK, Web::Response, Web::Request, SingleElementFactoryType<Web::Response> > {
        private:
            static const uint32_t ELEMENTFACTORY_QUEUESIZE = 1;

            typedef WebLinkType<LINK, Web::Response, Web::Request, SingleElementFactoryType<Web::Response> > BaseClass;
            typedef ClientTransferType<LINK, FILEBODY> ThisClass;

        public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
            template <typename Arg1>
            Channel(ThisClass& parent, Arg1 arg1)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2, arg3)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2, arg3, arg4)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
                , _request()
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            Channel(ThisClass& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : BaseClass(ELEMENTFACTORY_QUEUESIZE, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _parent(parent)
                , _request()
            {
            }

#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

            virtual ~Channel()
            {
                BaseClass::Close(Core::infinite);
            }

        public:
            void StartTransfer(const Core::ProxyType<Web::Request>& request)
            {
                ASSERT(_request.IsValid() == false);
                ASSERT(_response.IsValid() == false);

                if (BaseClass::IsOpen() == true) {
                    BaseClass::Submit(request);
                }
                else {
                    _request = request;
                    BaseClass::Open(0);
                }
            }

        private:
            // Notification of a Partial Request received, time to attach a body..
            virtual void LinkBody(Core::ProxyType<Web::Response>& element)
            {
                _parent.LinkBody(element);
            }

            // Notification of a Request received.
            virtual void Received(Core::ProxyType<Web::Response>& response)
            {
                // Right we got what we wanted, process it..
                _response = response;

                BaseClass::Close(0);
            }

            // Notification of a Response send.
            virtual void Send(const Core::ProxyType<Web::Request>& request)
            {
                ASSERT(_request.IsValid() != false);
                ASSERT(_request == request);
				DEBUG_VARIABLE(request);
                // Oke the message is gone, ready for a new one !!
                _request.Release();
            }

            // Notification of a channel state change..
            virtual void StateChange()
            {
                if (BaseClass::IsOpen() == true) {
                    ASSERT(_request.IsValid() == true);

                    BaseClass::Submit(_request);
                }
                else if (_response.IsValid() == true) {
                    // Close the link and thus the transfer..
                    _parent.EndTransfer(_response);

                    _response.Release();
                }
            }

        private:
            ThisClass& _parent;
            Core::ProxyType<Web::Request> _request;
            Core::ProxyType<Web::Response> _response;
        };

    private:
        ClientTransferType() = delete;
        ClientTransferType(const ClientTransferType<LINK, FILEBODY>& copy) = delete;
        ClientTransferType<LINK, FILEBODY>& operator=(const ClientTransferType<LINK, FILEBODY>& RHS) = delete;

    public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename Arg1>
        ClientTransferType(Arg1 arg1)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1)
        {
			_request.AddRef();
			_fileBody.AddRef();
        }
        template <typename Arg1, typename Arg2>
        ClientTransferType(Arg1 arg1, Arg2 arg2)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2)
        {
			_request.AddRef();
			_fileBody.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3>
        ClientTransferType(Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2, arg3)
        {
			_request.AddRef();
			_fileBody.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        ClientTransferType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2, arg3, arg4)
        {
			_request.AddRef();
			_fileBody.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        ClientTransferType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
        {
			_request.AddRef();
			_fileBody.AddRef();

        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        ClientTransferType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
        {
			_request.AddRef();
			_fileBody.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        ClientTransferType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _adminLock()
            , _state(TRANSFER_IDLE)
            , _request()
            , _fileBody()
            , _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
        {
			_request.AddRef();
			_fileBody.AddRef();
		}
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

        virtual ~ClientTransferType()
        {
            _request.Clear();
			_request.CompositRelease();
			_fileBody.CompositRelease();
		}

    public:
        inline LINK& Link()
        {
            return (_channel.Link());
        }
        inline const LINK& Link() const
        {
            return (_channel.Link());
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

                        // See if we can create a file to store the download in
                        static_cast<FILEBODY&>(_fileBody) = source;

                        _state = TRANSFER_UPLOAD;
                        _request.Verb = Web::Request::HTTP_PUT;
                        _request.Path = '/' + destination.Path().Value().Text();
                        _request.Body(Core::ProxyType<FILEBODY>(&_fileBody));

                        // Maybe we need to add a hash value...
                        _SerializedHashValue<LINK, FILEBODY>();

                        // Prepare the request for processing
                        _channel.StartTransfer(Core::ProxyType<Web::Request>(&_request, &_request));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Download(const Core::URL& source, Core::File& destination)
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

                        _state = TRANSFER_DOWNLOAD;
                        _request.Verb = Web::Request::HTTP_GET;
                        _request.Path = '/' + source.Path().Value().Text();

                        // Prepare the request for processing
                        _channel.StartTransfer(Core::ProxyType<Web::Request>(&_request, &_request));
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }

        virtual bool Setup(const Core::URL& remote) = 0;
        virtual void Transfered(const uint32_t result, const FILEBODY& file) = 0;

    private:
        inline void EndTransfer(const Core::ProxyType<Web::Response>& response)
        {
            uint32_t errorCode = Core::ERROR_NONE;

            // We are done, change state
            _adminLock.Lock();

            if (response->ErrorCode == Web::STATUS_NOT_FOUND) {
                errorCode = Core::ERROR_UNAVAILABLE;
            }
            else if ((response->ErrorCode == Web::STATUS_UNAUTHORIZED) || ((_state == TRANSFER_DOWNLOAD) && (_DeserializedHashValue<LINK, FILEBODY>(response->ContentSignature) == false))) {
                errorCode = Core::ERROR_INCORRECT_HASH;

            }

            _state = TRANSFER_IDLE;
            Transfered(errorCode, _fileBody);
            _adminLock.Unlock();
        }

        // Notification of a Partial Request received, time to attach a body..
        inline void LinkBody(Core::ProxyType<Web::Response>& element)
        {
            element->Body(Core::ProxyType<FILEBODY>(&_fileBody, &_fileBody));
        }

    private:
        HAS_MEMBER(SerializedHashValue, hasSerializedHashValue);
        HAS_MEMBER(DeserializedHashValue, hasDeserializedHashValue);

        typedef hasSerializedHashValue<FILEBODY, const uint8_t* (FILEBODY::*)() const> TraitSerializedHashValue;
        typedef hasDeserializedHashValue<FILEBODY, const uint8_t* (FILEBODY::*)() const> TraitDeserializedHashValue;

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<ClientTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, void>::type
        _SerializedHashValue()
        {
            _request.ContentSignature = Signature(_fileBody.HashType(), _fileBody.SerializedHashValue());
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<!ClientTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, void>::type
        _SerializedHashValue()
        {
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<ClientTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, bool>::type
        _DeserializedHashValue(const Core::OptionalType<Signature>& signature)
        {
            // See if this is a valid. frame
            return ((signature.IsSet() == false) || (signature.Value().Equal(_fileBody.HashType(), _fileBody.DeserializedHashValue()) == true));
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<!ClientTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, bool>::type
        _DeserializedHashValue(const Core::OptionalType<Signature>&)
        {
            return (true);
        }

    private:
        Core::CriticalSection _adminLock;
        enumTransferState _state;
        Core::ProxyObject<Web::Request> _request;
        Core::ProxyObject<FILEBODY> _fileBody;
        Channel _channel;
    };

    template <typename LINK, typename FILEBODY>
    class ServerTransferType : public WebLinkType<LINK, Web::Request, Web::Response, SingleElementFactoryType<Web::Request> > {
    private:
        typedef WebLinkType<LINK, Web::Request, Web::Response, SingleElementFactoryType<Web::Request> > BaseClass;
        typedef ServerTransferType<LINK, FILEBODY> ThisClass;

    private:
        ServerTransferType() = delete;
        ServerTransferType(const ServerTransferType<LINK, FILEBODY>& copy) = delete;
        ServerTransferType<LINK, FILEBODY>& operator=(const ServerTransferType<LINK, FILEBODY>& RHS) = delete;

    public:
#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif
        template <typename Arg1>
        ServerTransferType(const string& pathPrefix, Arg1 arg1)
            : BaseClass(1, arg1)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
        {
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
        }
        template <typename Arg1, typename Arg2>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2)
            : BaseClass(1, arg1, arg2)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : BaseClass(1, arg1, arg2, arg3)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : BaseClass(1, arg1, arg2, arg3, arg4)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5, arg6)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        ServerTransferType(const string& pathPrefix, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _pathPrefix(pathPrefix)
            , _fileBody()
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
        }
        template <typename Arg1>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1)
            : BaseClass(1, arg1)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2)
            : BaseClass(1, arg1, arg2)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : BaseClass(1, arg1, arg2, arg3)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : BaseClass(1, arg1, arg2, arg3, arg4)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5, arg6)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        ServerTransferType(const string& pathPrefix, const string& hashKey, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : BaseClass(1, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _pathPrefix(pathPrefix)
            , _fileBody(hashKey)
			, _response()
		{
            // Path should be a directory. End with a slash !!!
            ASSERT(_pathPrefix.empty() || (_pathPrefix[_pathPrefix.length() - 1] == '/'));

			_fileBody.AddRef();
			_response.AddRef();
		}
#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

        virtual ~ServerTransferType()
        {
            BaseClass::Close(Core::infinite);

			_fileBody.CompositRelease();
			_response.CompositRelease();
		}

        virtual string Authorize(const Web::Request& request) = 0;

    private:
        // Notification of a Partial Request received, time to attach a body..
        virtual void LinkBody(Core::ProxyType<Web::Request>& element)
        {
            if (element->Verb == Web::Request::HTTP_PUT) {
                static_cast<FILEBODY&>(_fileBody) = _pathPrefix + element->Path;

                if (_fileBody.Create() == true) {
                    // Seems we will receive a file.
                    element->Body(Core::ProxyType<FILEBODY>(&_fileBody));
                }
            }
        }

        // Notification of a Request received.
        virtual void Received(Core::ProxyType<Web::Request>& element)
        {
            // Right we got what we wanted, process it..
            if (element->Verb == Web::Request::HTTP_PUT) {
                if (_fileBody.IsOpen() == false) {
                    _response.ErrorCode = Web::STATUS_NOT_FOUND;
                    _response.Message = _T("File: ") + element->Path + _T(" could not be stored server side.");
                }
                else {
                    // See if the keys we received correspond.
                    if (_DeserializedHashValue<LINK, FILEBODY>(element->ContentSignature) == true) {
                        string message = Authorize(*element);

                        if (message.empty() == true) {
                            _response.ErrorCode = Web::STATUS_OK;
                            _response.Message = _T("File: ") + element->Path + _T(" has been stored server side.");
                        }
                        else {
                            // Somehow we are not Authorzed. Kill it....
                            _response.ErrorCode = Web::STATUS_UNAUTHORIZED;
                            _response.Message = message;
                        }
                    }
                    else {
                        // Delete the file, signature is NOT oke.
                        _response.ErrorCode = Web::STATUS_UNAUTHORIZED;
                        _response.Message = _T("File: ") + element->Path + _T(" has an incorrect signature.");
                    }

                    if (_response.ErrorCode != Web::STATUS_OK) {
                        _fileBody.Destroy();
                    }
                    else {
                        _fileBody.Close();
                    }
                }
            }
            else {
                static_cast<FILEBODY&>(_fileBody) = (_pathPrefix + element->Path);

                if (_fileBody.Exists() == true) {
                    string message = Authorize(*element);

                    if (message.empty() == true) {
                        _response.Body(Core::ProxyType<FILEBODY>(&_fileBody));
                        _SerializedHashValue<LINK, FILEBODY>();
                    }
                    else {
                        // Somehow we are not Authorzed. Kill it....
                        _response.ErrorCode = Web::STATUS_UNAUTHORIZED;
                        _response.Message = message;
                    }
                }
                else {
                    _response.ErrorCode = Web::STATUS_NOT_FOUND;
                    _response.Message = _T("File: ") + element->Path + _T(" was not found server side.");
                }
            }
            BaseClass::Submit(Core::ProxyType<Web::Response>(&_response));
        }

        // Notification of a Response send.
        virtual void Send(const Core::ProxyType<Web::Response>& response)
        {
            // If we send the message, we are done.
            response.Release();
            BaseClass::Close(0);
        }

        // Notification of a channel state change..
        virtual void StateChange()
        {
        }

    private:
        HAS_MEMBER(SerializedHashValue, hasSerializedHashValue);
        HAS_MEMBER(DeserializedHashValue, hasDeserializedHashValue);

        typedef hasSerializedHashValue<FILEBODY, const uint8_t* (FILEBODY::*)() const> TraitSerializedHashValue;
        typedef hasDeserializedHashValue<FILEBODY, const uint8_t* (FILEBODY::*)() const> TraitDeserializedHashValue;

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<ServerTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, void>::type
        _SerializedHashValue()
        {
            _response.ContentSignature = Signature(_fileBody.HashType(), _fileBody.SerializedHashValue());
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<!ServerTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, void>::type
        _SerializedHashValue()
        {
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<ServerTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, bool>::type
        _DeserializedHashValue(const Core::OptionalType<Signature>& signature)
        {
            // See if this is a valid. frame
            return ((signature.IsSet() == true) && (signature.Value().Equal(_fileBody.HashType(), _fileBody.DeserializedHashValue()) == true));
        }

        template <typename ACTUALLINK, typename ACTUALFILEBODY>
        inline typename Core::TypeTraits::enable_if<!ServerTransferType<ACTUALLINK, ACTUALFILEBODY>::TraitSerializedHashValue::value, bool>::type
        _DeserializedHashValue(const Core::OptionalType<Signature>&)
        {
            return (true);
        }

    private:
        const string _pathPrefix;
        Core::ProxyObject<FILEBODY> _fileBody;
        Core::ProxyObject<Web::Response> _response;
    };
}
} // namespace Solutions.HTTP

#endif

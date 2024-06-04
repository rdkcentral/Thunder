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

#ifndef __WEBRESPONSE_H
#define __WEBRESPONSE_H

#include "Module.h"
#include "URL.h"
#include "WebRequest.h"

namespace Thunder {
namespace Web {
    enum WebStatus {
        STATUS_CONTINUE = 100,
        STATUS_SWITCH_PROTOCOL = 101,
        STATUS_OK = 200,
        STATUS_CREATED = 201,
        STATUS_ACCEPTED = 202,
        STATUS_NON_AUTHORATIVE = 203,
        STATUS_NO_CONTENT = 204,
        STATUS_RESET_CONTENT = 205,
        STATUS_PARTIAL_CONTENT = 206,
        STATUS_MULTIPLE_CHOICES = 300,
        STATUS_MOVED_PERMANENTLY = 301,
        STATUS_MOVED_TEMPORARY = 302,
        STATUS_SEE_OTHER = 303,
        STATUS_NOT_MODIFIED = 304,
        STATUS_USE_PROXY = 305,
        STATUS_TEMPORARY_REDIRECT = 307,
        STATUS_BAD_REQUEST = 400,
        STATUS_UNAUTHORIZED = 401,
        STATUS_PAYMENT_REQUIRED = 402,
        STATUS_FORBIDDEN = 403,
        STATUS_NOT_FOUND = 404,
        STATUS_METHOD_NOT_ALLOWED = 405,
        STATUS_NOT_ACCEPTABLE = 406,
        STATUS_PROXY_AUTHENTICATION_REQUIRED = 407,
        STATUS_REQUEST_TIME_OUT = 408,
        STATUS_CONFLICT = 409,
        STATUS_GONE = 410,
        STATUS_LENGTH_RECORD = 411,
        STATUS_PRECONDITION_FAILED = 412,
        STATUS_REQUEST_ENTITY_TOO_LARGE = 413,
        STATUS_REQUEST_URI_TOO_LARGE = 414,
        STATUS_UNSUPPORTED_MEDIA_TYPE = 415,
        STATUS_REQUEST_RANGE_NOT_SATISFIABLE = 416,
        STATUS_EXPECTATION_FAILED = 417,
        STATUS_UNPROCESSABLE_ENTITY = 422,
        STATUS_INTERNAL_SERVER_ERROR = 500,
        STATUS_NOT_IMPLEMENTED = 501,
        STATUS_BAD_GATEWAY = 502,
        STATUS_SERVICE_UNAVAILABLE = 503,
        STATUS_GATEWAY_TIMEOUT = 504,
        STATUS_VERSION_NOT_SUPPORTED = 505

    };

    class EXTERNAL Response {
    public:
        typedef Response BaseElement;

        enum keywords {
            DATE,
            SERVER,
            MODIFIED,
            UPGRADE,
            CONTENT_TYPE,
            CONTENT_LENGTH,
            CONTENT_SIGNATURE,
            CONTENT_ENCODING,
            TRANSFER_ENCODING,
            ACCEPT_RANGE,
            CONNECTION,
            ETAG,
            ACCESS_CONTROL_ALLOW_METHODS,
            ACCESS_CONTROL_ALLOW_ORIGIN,
            ACCESS_CONTROL_ALLOW_HEADERS,
            ACCESS_CONTROL_MAX_AGE,
            ALLOW,
            WEBSOCKET_ACCEPT,
            WEBSOCKET_PROTOCOL,
            LOCATION,
            WAKEUP,
            U_S_N,
            S_T,
            CACHE_CONTROL,
            APPLICATION_URL
        };

        enum upgrade {
            UPGRADE_UNKNOWN,
            UPGRADE_WEBSOCKET

        };

        enum connection {
            CONNECTION_UNKNOWN,
            CONNECTION_UPGRADE,
            CONNECTION_KEEPALIVE,
            CONNECTION_CLOSE
        };

        class EXTERNAL Serializer {
        private:
            enum enumState {
                VERSION = 1,
                ERRORCODE = 2,
                MESSAGE = 3,
                PAIR_KEY = 4,
                PAIR_VALUE = 5,
                BODY = 6,
                REPORT = 7
            };

            const static uint16_t EOL_MARKER = 0x8000;

        public:
            Serializer(const Serializer&) = delete;
            Serializer& operator=(const Serializer&) = delete;

            Serializer()
                : _state(VERSION)
                , _offset(0)
                , _keyIndex(0)
                , _value()
                , _bodyLength(0)
                , _buffer(nullptr)
                , _lock()
                , _current()
            {
            }
            virtual ~Serializer() = default;

        public:
            virtual void Serialized(const Web::Response& element) = 0;

            void Flush()
            {
                _lock.Lock();
                _state = VERSION;
                Web::Response* backup = _current;
                _current = nullptr;
                if (backup != nullptr) {
                    Serialized(*backup);
                }
                _lock.Unlock();
            }

            void Submit(const Web::Response& element)
            {
                _lock.Lock();

                ASSERT(_current == nullptr);

                _current = const_cast<Web::Response*>(&element);

                _lock.Unlock();
            }

            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength);

        private:
            uint16_t _state;
            uint16_t _offset;
            uint8_t _keyIndex;
            string _value;
            uint32_t _bodyLength;
            const TCHAR* _buffer;
            Core::CriticalSection _lock;
            Response* _current;
        };
        class EXTERNAL Deserializer {
        private:
            enum enumState {
                VERSION,
                ERRORCODE,
                MESSAGE,
                PAIR_KEY,
                PAIR_VALUE,
                CHUNK_INIT,
                CHUNK_END,
                BODY_END
            };
            typedef Core::ParserType<Core::TerminatorCarriageReturnLineFeed, Deserializer> Parser;

        public:
            Deserializer(const Deserializer&) = delete;
            Deserializer& operator=(const Deserializer&) = delete;

PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
            Deserializer()
                : _lock()
                , _current()
                , _state(VERSION)
                , _keyWord(Web::Response::keywords::SERVER)
                , _parser(*this)
                , _zlib()
				, _zlibResult(0)
            {
            }
POP_WARNING()
            virtual ~Deserializer() = default;

        public:
            inline void Flush()
            {
                _lock.Lock();

                _state = VERSION;
                _parser.Reset();

                if (_current != nullptr) {
                    Web::Response* element = _current;
                    _current = nullptr;

                    Deserialized(*element);
                }

                _lock.Unlock();
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength)
            {
                _lock.Lock();

                uint16_t usedSize = _parser.Deserialize(stream, maxLength);

                _lock.Unlock();

                return (usedSize);
            }

            // The whole response object is deserialised..
            virtual void Deserialized(Web::Response& element) = 0;

            // We need a response object to be able to fill it with info
            virtual Web::Response* Element() = 0;

            // We reached the body, link a proper body to the response..
            virtual bool LinkBody(Web::Response& response) = 0;

        private:
            friend class Core::ParserType<Core::TerminatorCarriageReturnLineFeed, Deserializer>;

            uint16_t Parse(const uint8_t stream[], const uint16_t maxLength);
            void Parse(const string& buffer, const bool /* quoted */);
            void EndOfLine();
            void EndOfPassThrough();

        private:
            Core::CriticalSection _lock;
            Web::Response* _current;
            enumState _state;
            Web::Response::keywords _keyWord;
            Parser _parser;
            z_stream _zlib;
            uint32_t _zlibResult;
        };

    private:
        Response(const Response&) = delete;
        Response& operator=(const Response&) = delete;

    public:
        Response()
        {
          Clear();
        }
        ~Response()
        {
        }

    public:
        static void ToString(const Response& realObject, string& text)
        {
            uint16_t fillCount = 0;
            bool ready = false;
            class SerializerImpl : public Serializer {
            public:
                SerializerImpl(bool& readyFlag)
                    : Serializer()
                    , _ready(readyFlag)
                {
                }
                ~SerializerImpl() override = default;

            public:
                virtual void Serialized(const Response& /* element */)
                {
                    _ready = true;
                }

            private:
                bool& _ready;

            } serializer(ready);

            // Response an object to e serialized..
            serializer.Submit(realObject);

            // Serialize object
            while (ready == false) {
                uint8_t buffer[1024];
                uint16_t loaded = serializer.Serialize(buffer, sizeof(buffer));

                ASSERT(loaded <= sizeof(buffer));

                fillCount += loaded;

                text += string(reinterpret_cast<char*>(&buffer[0]), loaded);
            }
        }
        inline void ToString(string& text) const
        {
            Response::ToString(*this, text);
        }
        static void FromString(Response& realObject, const string& text)
        {
            class DeserializerImpl : public Deserializer {
            public:
                DeserializerImpl(Web::Response& destination)
                    : Deserializer()
                    , _destination(destination)
                {
                }
                ~DeserializerImpl() override = default;

            public:
                // The whole request object is deserialised..
                virtual void Deserialized(Web::Response& element VARIABLE_IS_NOT_USED)
                {
                }

                // We need a request object to be able to fill it with info
                virtual Web::Response* Element()
                {
                    return (&_destination);
                }

                // We reached the body, link a proper body to the response..
                virtual bool LinkBody(Web::Response& request)
                {
                    return (request.HasBody());
                }

            private:
                Web::Response& _destination;

            } deserializer(realObject);

            // Request an object to e serialized..
            deserializer.Deserialize(reinterpret_cast<const uint8_t*>(text.c_str()), static_cast<uint16_t>(text.length()));
        }
        inline void FromString(const string& text)
        {
            Response::FromString(*this, text);
        }
        inline bool IsValid() const
        {
            return (ErrorCode != static_cast<uint16_t>(~0));
        }
        void Clear()
        {
            _marshalMode = MARSHAL_RAW;
            ErrorCode = Web::STATUS_OK;
            Message.clear();
            MajorVersion = Web::MajorVersion;
            MinorVersion = Web::MinorVersion;
            Date.Clear();
            Connection.Clear();
            Upgrade.Clear();
            Server.Clear();
            Modified.Clear();
            AcceptRange.Clear();
            ETag.Clear();
            ContentType.Clear();
            ContentLength.Clear();
            ContentEncoding.Clear();
            WebSocketAccept.Clear();
            AccessControlOrigin.Clear();
            AccessControlMethod.Clear();
            AccessControlHeaders.Clear();
            AccessControlMaxAge.Clear();
            ContentCharacterSet.Clear();
            Allowed.Clear();
            TransferEncoding.Clear();
            ContentSignature.Clear();
            Location.Clear();
            USN.Clear();
            ST.Clear();
            WakeUp.Clear();
            CacheControl.Clear();
            ApplicationURL.Clear();

            if (_body.IsValid() == true) {
                _body.Release();
            }
        }

        uint16_t ErrorCode;
        string Message;
        uint8_t MajorVersion;
        uint8_t MinorVersion;
        Core::OptionalType<upgrade> Upgrade;
        Core::OptionalType<string> WebSocketAccept;
        Core::OptionalType<MIMETypes> ContentType;
        Core::OptionalType<CharacterTypes> ContentCharacterSet;
        Core::OptionalType<uint32_t> ContentLength;
        Core::OptionalType<Signature> ContentSignature;
        Core::OptionalType<EncodingTypes> ContentEncoding;
        Core::OptionalType<TransferTypes> TransferEncoding;
        Core::OptionalType<Core::Time> Date;
        Core::OptionalType<string> Server;
        Core::OptionalType<Core::Time> Modified;
        Core::OptionalType<uint16_t> Allowed;
        Core::OptionalType<string> AccessControlOrigin;
        Core::OptionalType<uint16_t> AccessControlMethod;
        Core::OptionalType<string> AccessControlHeaders;
        Core::OptionalType<uint32_t> AccessControlMaxAge;
        Core::OptionalType<string> AcceptRange;
        Core::OptionalType<connection> Connection;
        Core::OptionalType<string> ST;
        Core::OptionalType<string> USN;
        Core::OptionalType<string> Location;
        Core::OptionalType<string> WakeUp;
        Core::OptionalType<string> ETag;
        Core::OptionalType<string> WebSocketProtocol;
        Core::OptionalType<string> CacheControl;
        Core::OptionalType<Core::URL> ApplicationURL;

        inline bool HasBody() const
        {
            return (_body.IsValid());
        }
        template <typename BODYTYPE>
        inline void Body(const Core::ProxyType<BODYTYPE>& body)
        {
            _body = Core::ProxyType<IBody>(body);
        }
        template <typename BODYTYPE>
        inline Core::ProxyType<BODYTYPE> Body()
        {
            ASSERT(HasBody() == true);

            return (Core::ProxyType<BODYTYPE>(_body));
        }
        template <typename BODYTYPE>
        inline Core::ProxyType<const BODYTYPE> Body() const
        {
            ASSERT(HasBody() == true);

            return (Core::ProxyType<const BODYTYPE>(_body));
        }
        inline void Mode(const MarshalType mode)
        {
            _marshalMode = mode;
        }
        inline MarshalType Mode() const
        {
            return (_marshalMode);
        }

    private:
        Core::ProxyType<IBody> _body;
        MarshalType _marshalMode;
    };
}
}

#endif // __WEBRESPONSE_H

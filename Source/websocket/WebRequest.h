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

#ifndef __WEBREQUEST_H
#define __WEBREQUEST_H

#include "Module.h"

namespace Thunder {
namespace Web {
    using ProtocolsArray = Core::TokenizedStringList<',', true>;

    static const uint8_t MajorVersion = 1;
    static const uint8_t MinorVersion = 1;

    enum MarshalType {
        MARSHAL_RAW,
        MARSHAL_UPPERCASE
    };

    enum MIMETypes {
        MIME_BINARY,
        MIME_TEXT,
        MIME_HTML,
        MIME_JSON,
        MIME_XML,
        MIME_JS,
        MIME_CSS,
        MIME_IMAGE_TIFF,
        MIME_IMAGE_VND,
        MIME_IMAGE_X_ICON,
        MIME_IMAGE_X_JNG,
        MIME_IMAGE_X_BMP,
        MIME_IMAGE_X_MS_BMP,
        MIME_IMAGE_SVG_XML,
        MIME_IMAGE_WEBP,
        MIME_IMAGE_GIF,
        MIME_IMAGE_JPG,
        MIME_IMAGE_PNG,
        MIME_FONT_TTF,
        MIME_FONT_OPENTYPE,
        MIME_TEXT_XML,
        MIME_APPLICATION_FONT_WOFF,
        MIME_APPLICATION_JAVA_ARCHIVE,
        MIME_APPLICATION_JAVASCRIPT,
        MIME_APPLICATION_ATOM_XML,
        MIME_APPLICATION_RSS_XML,
        MIME_UNKNOWN
    };

    enum EncodingTypes {
        ENCODING_GZIP,
        ENCODING_UNKNOWN
    };

    bool EXTERNAL MIMETypeAndEncodingForFile(const string path, string& fileToService, MIMETypes& mimeType, EncodingTypes& encoding);

    enum TransferTypes {
        TRANSFER_CHUNKED,
        TRANSFER_UNKNOWN
    };

    enum CharacterTypes {
        CHARACTER_UTF8,
        CHARACTER_UNKNOWN
    };

    struct EXTERNAL IBody {
        virtual ~IBody() = default;

        // The Serialize/Deserialize methods mark the start of an upcoming serialization/deserialization
        // of the object. These methods allow for preparation of content to be Serialised or Deserialized.
        // The return value is of the Serialization inidcateds the number of bytes required for the body
        // The reurn value of the Deserialize indicate the number of byes that could by loaded as max.
        virtual uint32_t Serialize() const = 0;
        virtual uint32_t Deserialize() = 0;

        // The End method indicates a completion of the Serialization or Deserialization.
        virtual void End() const = 0;

        // The Serialize and Deserialize methods allow the content to be serialized/deserialized.
        virtual uint16_t Serialize(uint8_t[] /* stream*/, const uint16_t /* maxLength */) const = 0;
        virtual uint16_t Deserialize(const uint8_t[] /* stream*/, const uint16_t /* maxLength */) = 0;
    };

    class EXTERNAL Signature {
    public:
        Signature()
            : _type(Crypto::HASH_MD5)
        {
            ::memset(_hashValue, 0, sizeof(_hashValue));
        }
        Signature(const Crypto::EnumHashType& type, const uint8_t hash[])
            : _type(type)
        {
            ASSERT(_type <= sizeof(_hashValue));
            ::memcpy(_hashValue, hash, _type);
        }
        Signature(const Signature& copy)
            : _type(copy._type)
        {
            ASSERT(_type <= sizeof(_hashValue));
            ::memcpy(_hashValue, copy._hashValue, _type);
        }
        Signature(Signature&& move)
            : _type(std::move(move._type))
        {
            ASSERT(_type <= sizeof(_hashValue));
            ::memcpy(_hashValue, move._hashValue, _type);
            ::memset(move._hashValue, 0, _type);
        }
        ~Signature()
        {
        }

        Signature& operator=(const Signature& RHS)
        {
            _type = RHS._type;
            ::memcpy(_hashValue, RHS._hashValue, _type);

            return (*this);
        }

        Signature& operator=(Signature&& move)
        {
            if (this != &move) {
                _type = std::move(move._type);
                ::memcpy(_hashValue, move._hashValue, _type);
                ::memset(move._hashValue, 0, _type);
            }

            return (*this);
        }

    public:
        bool Equal(const Crypto::EnumHashType& type, const uint8_t value[]) const
        {
            return ((_type == type) && (::memcmp(_hashValue, value, _type) == 0));
        }
        inline bool operator==(const Signature& RHS) const
        {
            return ((RHS._type == _type) && (memcmp(RHS._hashValue, _hashValue, _type) == 0));
        }
        inline bool operator!=(const Signature& RHS) const
        {
            return (!(operator==(RHS)));
        }
        Crypto::EnumHashType Type() const
        {
            return (_type);
        }
        const uint8_t* Data() const
        {
            return (_hashValue);
        }

    private:
        uint8_t _hashValue[64];
        Crypto::EnumHashType _type;
    };

    class EXTERNAL Authorization {
    public:
        enum type {
            BEARER,
            BASIC
        };

    public:
        Authorization()
            : _type(BEARER)
            , _token()
        {
        }
        Authorization(const type& value, const string& token)
            : _type(value)
            , _token(token)
        {
        }
        Authorization(const Authorization& copy)
            : _type(copy._type)
            , _token(copy._token)
        {
        }
        Authorization(Authorization&& move)
            : _type(std::move(move._type))
            , _token(std::move(move._token))
        {
        }
        ~Authorization()
        {
        }

        Authorization& operator=(const Authorization& RHS)
        {
            _type = RHS._type;
            _token = RHS._token;

            return (*this);
        }

        Authorization& operator=(Authorization&& move)
        {
            if (this != &move) {
                _type = std::move(move._type);
                _token = std::move(move._token);
            }

            return (*this);
        }

    public:
        bool Equal(const type& value, const string& token) const
        {
            return ((_type == value) && (token == _token));
        }
        inline bool operator==(const Authorization& RHS) const
        {
            return ((_type == RHS._type) && (_token == RHS._token));
        }
        inline bool operator!=(const Authorization& RHS) const
        {
            return (!(operator==(RHS)));
        }
        type Type() const
        {
            return (_type);
        }
        const string& Token() const
        {
            return (_token);
        }

    private:
        type _type;
        string _token;
    };

    class EXTERNAL Request {
    private:
        static constexpr const TCHAR* DELIMETERS = _T(" ,");

    public:
        typedef Request BaseElement;

        enum keywords {
            HOST,
            UPGRADE,
            CONNECTION,
            ORIGIN,
            ACCEPT,
            ACCEPT_ENCODING,
            USERAGENT,
            CONTENT_TYPE,
            CONTENT_LENGTH,
            CONTENT_SIGNATURE,
            CONTENT_ENCODING,
            TRANSFER_ENCODING,
            ENCODING,
            LANGUAGE,
            ACCESS_CONTROL_REQUEST_METHOD,
            ACCESS_CONTROL_REQUEST_HEADERS,
            WEBSOCKET_KEY,
            WEBSOCKET_PROTOCOL,
            WEBSOCKET_VERSION,
            WEBSOCKET_EXTENSIONS,
            MAN,
            M_X,
            S_T,
			AUTHORIZATION
        };

        enum type {
            HTTP_NONE = 0x0000,
            HTTP_GET = 0x0001,
            HTTP_HEAD = 0x0002,
            HTTP_POST = 0x0004,
            HTTP_PUT = 0x0008,
            HTTP_DELETE = 0x0010,
            HTTP_OPTIONS = 0x0020,
            HTTP_TRACE = 0x0040,
            HTTP_CONNECT = 0x0080,
            HTTP_PATCH = 0x0100,
            HTTP_MSEARCH = 0x0200,
            HTTP_NOTIFY = 0x0400,
            HTTP_UNKNOWN = 0x8000
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
                VERB = 1,
                URL = 2,
                QUERY = 3,
                FRAGMENT = 4,
                VERSION = 5,
                PAIR_KEY = 6,
                PAIR_VALUE = 7,
                BODY = 8,
                REPORT = 9
            };
            const static uint16_t EOL_MARKER = 0x8000;

        public:
            Serializer(const Serializer&) = delete;
            Serializer& operator=(const Serializer&) = delete;

            Serializer()
                : _state(VERB)
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
            virtual void Serialized(const Web::Request& element) = 0;

            void Flush()
            {
                _lock.Lock();
                _state = VERSION;
                Web::Request* backup = _current;
                _current = nullptr;
                if (backup != nullptr) {
                    Serialized(*backup);
                }
                _lock.Unlock();
            }

            void Submit(const Web::Request& element)
            {
                _lock.Lock();

                ASSERT(_current == nullptr);

                _current = const_cast<Request*>(&element);

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
            Request* _current;
        };
        class EXTERNAL Deserializer {
        private:
            enum enumState {
                VERB,
                URL,
                VERSION,
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
                , _state(VERB)
                , _keyWord(Web::Request::keywords::WEBSOCKET_VERSION)
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

                _state = VERB;
                _parser.Reset();

                if (_current != nullptr) {
                    Web::Request* element = _current;
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

            // The whole request object is deserialised..
            virtual void Deserialized(Web::Request& element) = 0;

            // We need a request object to be able to fill it with info
            virtual Web::Request* Element() = 0;

            // We reached the body, link a proper body to the response..
            virtual bool LinkBody(Web::Request& response) = 0;

        private:
            friend class Core::ParserType<Core::TerminatorCarriageReturnLineFeed, Deserializer>;

            uint16_t Parse(const uint8_t stream[], const uint16_t maxLength);
            void Parse(const string& buffer, const bool /* quoted */);
            void EndOfLine();
            void EndOfPassThrough();

        private:
            Core::CriticalSection _lock;
            Web::Request* _current;
            enumState _state;
            Web::Request::keywords _keyWord;
            Parser _parser;
            z_stream _zlib;
            uint32_t _zlibResult;
        };

    private:
        Request(const Request&) = delete;
        Request& operator=(const Request&) = delete;

    public:
        Request()
            : _body()
        {
            Clear();
        }
        ~Request()
        {
        }

    public:
        static const TCHAR* GET;
        static const TCHAR* HEAD;
        static const TCHAR* POST;
        static const TCHAR* PUT;
        static const TCHAR* DELETE;
        static const TCHAR* OPTIONS;
        static const TCHAR* TRACE;
        static const TCHAR* CONNECT;
        static const TCHAR* PATCH;
        static const TCHAR* MSEARCH;
        static const TCHAR* NOTIFY;

        static const TCHAR* ToString(const type value);
        static void ToString(const Request& realObject, string& text)
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
                virtual void Serialized(const Request& /* element */)
                {
                    _ready = true;
                }

            private:
                bool& _ready;

            } serializer(ready);

            // Request an object to e serialized..
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
            Request::ToString(*this, text);
        }
        static void FromString(Request& realObject, const string& text)
        {
            class DeserializerImpl : public Deserializer {
            public:
                DeserializerImpl(Web::Request& destination)
                    : Deserializer()
                    , _destination(destination)
                {
                }
                virtual ~DeserializerImpl() override = default;

            public:
                // The whole request object is deserialised..
                virtual void Deserialized(Web::Request& element VARIABLE_IS_NOT_USED)
                {
                }

                // We need a request object to be able to fill it with info
                virtual Web::Request* Element()
                {
                    return (&_destination);
                }

                // We reached the body, link a proper body to the response..
                virtual bool LinkBody(Web::Request& request)
                {
                    return (request.HasBody());
                }

            private:
                Web::Request& _destination;

            } deserializer(realObject);

            // Request an object to e serialized..
            deserializer.Deserialize(reinterpret_cast<const uint8_t*>(text.c_str()), static_cast<uint16_t>(text.length()));
        }
        inline void FromString(const string& text)
        {
            Request::FromString(*this, text);
        }
        inline bool IsValid() const
        {
            return (Verb != HTTP_UNKNOWN);
        }
        void Clear()
        {
            _marshalMode = MARSHAL_RAW;
            Verb = HTTP_UNKNOWN;
            Path.clear();
            Query.Clear();
            Fragment.Clear();
            MajorVersion = Web::MajorVersion;
            MinorVersion = Web::MinorVersion;
            Origin.Clear();
            Upgrade.Clear();
            Host.Clear();
            Range.Clear();
            Connection.Clear();
            Accept.Clear();
            AcceptEncoding.Clear();
            UserAgent.Clear();
            Encoding.Clear();
            Language.Clear();
            ContentType.Clear();
            ContentLength.Clear();
            ContentEncoding.Clear();
            ContentCharacterSet.Clear();
            AccessControlHeaders.Clear();
            AccessControlMethod.Clear();
            WebSocketKey.Clear();
            WebSocketProtocol.Clear();
            WebSocketVersion.Clear();
            WebSocketExtensions.Clear();
            ContentSignature.Clear();
            TransferEncoding.Clear();
            Man.Clear();
            MX.Clear();
            ST.Clear();
            WebToken.Clear();

            if (_body.IsValid() == true) {
                _body.Release();
            }
        }

        type Verb;
        string Path;
        Core::OptionalType<string> Query;
        Core::OptionalType<string> Fragment;
        uint8_t MajorVersion;
        uint8_t MinorVersion;
        Core::OptionalType<string> Origin;
        Core::OptionalType<upgrade> Upgrade;
        Core::OptionalType<MIMETypes> ContentType;
        Core::OptionalType<CharacterTypes> ContentCharacterSet;
        Core::OptionalType<uint32_t> ContentLength;
        Core::OptionalType<Signature> ContentSignature;
        Core::OptionalType<EncodingTypes> ContentEncoding;
        Core::OptionalType<TransferTypes> TransferEncoding;
        Core::OptionalType<string> Host;
        Core::OptionalType<string> Range;
        Core::OptionalType<connection> Connection;
        Core::OptionalType<string> Accept;
        Core::OptionalType<EncodingTypes> AcceptEncoding;
        Core::OptionalType<string> UserAgent;
        Core::OptionalType<string> Encoding;
        Core::OptionalType<string> Language;
        Core::OptionalType<string> AccessControlHeaders;
        Core::OptionalType<uint16_t> AccessControlMethod;
        Core::OptionalType<string> WebSocketKey;
        Core::OptionalType<ProtocolsArray> WebSocketProtocol;
        Core::OptionalType<uint32_t> WebSocketVersion;
        Core::OptionalType<string> WebSocketExtensions;
        Core::OptionalType<string> Man;
        Core::OptionalType<string> ST;
        Core::OptionalType<uint32_t> MX;
        Core::OptionalType<Authorization> WebToken;

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
        template<typename SCANVALUE>
        static bool ScanForKeyword (const string& buffer, const SCANVALUE value) {
            bool status = false;
            std::size_t start = 0;
            std::size_t end = 0;

            string strValue = Core::EnumerateType<SCANVALUE>(value).Data();
            do {
                end = buffer.find_first_of(DELIMETERS, start);
                if (end - start > 0) {
                    string word = string(buffer, start, end - start);
                    std::transform(word.begin(), word.end(), word.begin(), [](TCHAR c){ return std::toupper(c); } );
                    if (word == strValue) {
                        status = true;
                        break;
                    }
                }
                start = end + 1;
            } while (end != std::string::npos);

            return status;
        }
    private:
        Core::ProxyType<IBody> _body;
        MarshalType _marshalMode;
    };
}
}

#endif // __WEBREQUEST_H

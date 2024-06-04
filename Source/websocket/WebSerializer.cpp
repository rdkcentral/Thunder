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

#include "WebSerializer.h"

namespace Thunder {
namespace Web {

    /* static */ const TCHAR* Request::GET = _T("GET");
    /* static */ const TCHAR* Request::HEAD = _T("HEAD");
    /* static */ const TCHAR* Request::POST = _T("POST");
    /* static */ const TCHAR* Request::PUT = _T("PUT");
    /* static */ const TCHAR* Request::DELETE = _T("DELETE");
    /* static */ const TCHAR* Request::OPTIONS = _T("OPTIONS");
    /* static */ const TCHAR* Request::TRACE = _T("TRACE");
    /* static */ const TCHAR* Request::CONNECT = _T("CONNECT");
    /* static */ const TCHAR* Request::PATCH = _T("PATCH");
    /* static */ const TCHAR* Request::MSEARCH = _T("M-SEARCH");
    /* static */ const TCHAR* Request::NOTIFY = _T("NOTIFY");
}
}

static const TCHAR __UNKNOWN[] = _T("UNKNOWN");
static const TCHAR __WEBSOCKET[] = _T("WEBSOCKET");
static const TCHAR __CONNECTION_UPGRADE[] = _T("UPGRADE");
static const TCHAR __CONNECTION_CLOSE[] = _T("CLOSE");
static const TCHAR __CONNECTION_KEEPALIVE[] = _T("KEEP-ALIVE");
static const TCHAR __ENCODING_GZIP[] = _T("GZIP");

static const TCHAR __HOST[] = _T("HOST:");
static const TCHAR __UPGRADE[] = _T("UPGRADE:");
static const TCHAR __ORIGIN[] = _T("ORIGIN:");
static const TCHAR __CONNECTION[] = _T("CONNECTION:");
static const TCHAR __ACCEPT[] = _T("ACCEPT:");
static const TCHAR __ACCEPT_ENCODING[] = _T("ACCEPT-ENCODING:");
static const TCHAR __USERAGENT[] = _T("USER-AGENT:");
static const TCHAR __CONTENT_TYPE[] = _T("CONTENT-TYPE:");
static const TCHAR __CONTENT_LENGTH[] = _T("CONTENT-LENGTH:");
static const TCHAR __CONTENT_SIGNATURE[] = _T("CONTENT-HMAC:");
static const TCHAR __CONTENT_ENCODING[] = _T("CONTENT-ENCODING:");
static const TCHAR __ENCODING[] = _T("ACCEPT-ENCODING:");
static const TCHAR __LANGUAGE[] = _T("ACCEPT-LANGUAGE:");
static const TCHAR __ACCESS_CONTROL_REQUEST_METHOD[] = _T("ACCESS-CONTROL-REQUEST-METHOD:");
static const TCHAR __ACCESS_CONTROL_REQUEST_HEADERS[] = _T("ACCESS-CONTROL-REQUEST-HEADERS:");
static const TCHAR __TRANSFER_ENCODING[] = _T("TRANSFER-ENCODING:");
static const TCHAR __ST[] = _T("ST:");
static const TCHAR __MAN[] = _T("MAN:");
static const TCHAR __MX[] = _T("MX:");
static const TCHAR __AUTHORIZATION[] = _T("AUTHORIZATION:");

static const TCHAR __DATE[] = _T("DATE:");
static const TCHAR __SERVER[] = _T("SERVER:");
static const TCHAR __MODIFIED[] = _T("LAST-MODIFIED:");
static const TCHAR __ACCEPT_RANGE[] = _T("ACCEPT-RANGES:");
static const TCHAR __RANGE[] = _T("RANGE:");
static const TCHAR __ETAG[] = _T("ETAG:");
static const TCHAR __ALLOW[] = _T("ALLOW:");
static const TCHAR __WEBSOCKET_KEY[] = _T("SEC-WEBSOCKET-KEY:");
static const TCHAR __WEBSOCKET_PROTOCOL[] = _T("SEC-WEBSOCKET-PROTOCOL:");
static const TCHAR __WEBSOCKET_VERSION[] = _T("SEC-WEBSOCKET-VERSION:");
static const TCHAR __WEBSOCKET_ACCEPT[] = _T("SEC-WEBSOCKET-ACCEPT:");
static const TCHAR __WEBSOCKET_EXTENSIONS[] = _T("SEC-WEBSOCKET-EXTENSIONS:");
static const TCHAR __ACCESS_CONTROL_ALLOW_METHODS[] = _T("ACCESS-CONTROL-ALLOW-METHODS:");
static const TCHAR __ACCESS_CONTROL_ALLOW_ORIGIN[] = _T("ACCESS-CONTROL-ALLOW-ORIGIN:");
static const TCHAR __ACCESS_CONTROL_ALLOW_HEADERS[] = _T("ACCESS-CONTROL-ALLOW-HEADERS:");
static const TCHAR __ACCESS_CONTROL_MAX_AGE[] = _T("ACCESS-CONTROL-MAX-AGE:");
static const TCHAR __USN[] = _T("USN:");
static const TCHAR __LOCATION[] = _T("LOCATION:");
static const TCHAR __WAKEUP[] = _T("WAKEUP:");
static const TCHAR __CACHE_CONTROL[] = _T("CACHE-CONTROL:");
static const TCHAR __APPLICATION_URL[] = _T("APPLICATION-URL:");

static const TCHAR __CHARACTER_SET[] = _T("CHARSET=");

#define __TXT(KeyWord) KeyWord, (sizeof(KeyWord) / sizeof(TCHAR)) - 1

namespace Thunder {

ENUM_CONVERSION_BEGIN(Web::MIMETypes)

    { Web::MIME_BINARY, _TXT("application/octet-stream") },
    { Web::MIME_TEXT, _TXT("text/plain") },
    { Web::MIME_HTML, _TXT("text/html") },
    { Web::MIME_JSON, _TXT("application/json") },
    { Web::MIME_XML, _TXT("application/xml") },
    { Web::MIME_JS, _TXT("text/javascript") },
    { Web::MIME_CSS, _TXT("text/css") },
    { Web::MIME_IMAGE_TIFF, _TXT("image/tiff") },
    { Web::MIME_IMAGE_VND, _TXT("image/vnd.wap.wbmp") },
    { Web::MIME_IMAGE_X_ICON, _TXT("image/x-icon") },
    { Web::MIME_IMAGE_X_JNG, _TXT("image/x-jng") },
    { Web::MIME_IMAGE_X_MS_BMP, _TXT("image/x-ms-bmp") },
    { Web::MIME_IMAGE_SVG_XML, _TXT("image/svg+xml") },
    { Web::MIME_IMAGE_WEBP, _TXT("image/webp") },
    { Web::MIME_IMAGE_GIF, _TXT("image/gif") },
    { Web::MIME_IMAGE_JPG, _TXT("image/jpeg") },
    { Web::MIME_IMAGE_PNG, _TXT("image/png") },
    { Web::MIME_FONT_TTF, _TXT("font/ttf") },
    { Web::MIME_FONT_OPENTYPE, _TXT("image/jpeg") },
    { Web::MIME_TEXT_XML, _TXT("text/xml") },
    { Web::MIME_APPLICATION_FONT_WOFF, _TXT("application/font-woff") },
    { Web::MIME_APPLICATION_JAVA_ARCHIVE, _TXT("application/java-archive") },
    { Web::MIME_APPLICATION_JAVASCRIPT, _TXT("application/javascript") },
    { Web::MIME_APPLICATION_ATOM_XML, _TXT("application/atom+xml") },
    { Web::MIME_APPLICATION_RSS_XML, _TXT("application/rss+xml") },
    { Web::MIME_UNKNOWN, _TXT("unknown") },

ENUM_CONVERSION_END(Web::MIMETypes)

ENUM_CONVERSION_BEGIN(Web::EncodingTypes)

    { Web::ENCODING_GZIP, _TXT(__ENCODING_GZIP) },
    { Web::ENCODING_UNKNOWN, _TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::EncodingTypes)

ENUM_CONVERSION_BEGIN(Web::TransferTypes)

    { Web::TRANSFER_CHUNKED, _TXT("chunked") },
    { Web::TRANSFER_UNKNOWN, _TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::TransferTypes)

ENUM_CONVERSION_BEGIN(Web::CharacterTypes)

    { Web::CHARACTER_UTF8, _TXT("utf-8") },
    { Web::CHARACTER_UNKNOWN, _TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::CharacterTypes)

ENUM_CONVERSION_BEGIN(Web::WebStatus)

    { Web::STATUS_CONTINUE, _TXT("CONTINUE") },
    { Web::STATUS_SWITCH_PROTOCOL, _TXT("SWITCH_PROTOCOL") },
    { Web::STATUS_OK, _TXT("OK") },
    { Web::STATUS_CREATED, _TXT("CREATED") },
    { Web::STATUS_ACCEPTED, _TXT("ACCEPTED") },
    { Web::STATUS_NON_AUTHORATIVE, _TXT("NON_AUTHORATIVE") },
    { Web::STATUS_NO_CONTENT, _TXT("NO_CONTENT") },
    { Web::STATUS_RESET_CONTENT, _TXT("RESET_CONTENT") },
    { Web::STATUS_PARTIAL_CONTENT, _TXT("PARTIAL_CONTENT") },
    { Web::STATUS_MULTIPLE_CHOICES, _TXT("MULTIPLE_CHOICES") },
    { Web::STATUS_MOVED_PERMANENTLY, _TXT("MOVED_PERMANENTLY") },
    { Web::STATUS_MOVED_TEMPORARY, _TXT("MOVED_TEMPORARY") },
    { Web::STATUS_SEE_OTHER, _TXT("SEE_OTHER") },
    { Web::STATUS_NOT_MODIFIED, _TXT("NOT_MODIFIED") },
    { Web::STATUS_USE_PROXY, _TXT("USE_PROXY") },
    { Web::STATUS_TEMPORARY_REDIRECT, _TXT("TEMPORARY_REDIRECT") },
    { Web::STATUS_BAD_REQUEST, _TXT("BAD_REQUEST") },
    { Web::STATUS_UNAUTHORIZED, _TXT("UNAUTHORIZED") },
    { Web::STATUS_PAYMENT_REQUIRED, _TXT("PAYMENT_REQUIRED") },
    { Web::STATUS_FORBIDDEN, _TXT("FORBIDDEN") },
    { Web::STATUS_NOT_FOUND, _TXT("NOT_FOUND") },
    { Web::STATUS_METHOD_NOT_ALLOWED, _TXT("METHOD_NOT_ALLOWED") },
    { Web::STATUS_NOT_ACCEPTABLE, _TXT("NOT_ACCEPTABLE") },
    { Web::STATUS_PROXY_AUTHENTICATION_REQUIRED, _TXT("PROXY_AUTHENTICATION_REQUIRED") },
    { Web::STATUS_REQUEST_TIME_OUT, _TXT("REQUEST_TIME_OUT") },
    { Web::STATUS_CONFLICT, _TXT("CONFLICT") },
    { Web::STATUS_GONE, _TXT("GONE") },
    { Web::STATUS_LENGTH_RECORD, _TXT("LENGTH_RECORD") },
    { Web::STATUS_PRECONDITION_FAILED, _TXT("PRECONDITION_FAILED") },
    { Web::STATUS_REQUEST_ENTITY_TOO_LARGE, _TXT("REQUEST_ENTITY_TOO_LARGE") },
    { Web::STATUS_REQUEST_URI_TOO_LARGE, _TXT("REQUEST_URI_TOO_LARGE") },
    { Web::STATUS_UNSUPPORTED_MEDIA_TYPE, _TXT("UNSUPPORTED_MEDIA_TYPE") },
    { Web::STATUS_REQUEST_RANGE_NOT_SATISFIABLE, _TXT("REQUEST_RANGE_NOT_SATISFIABLE") },
    { Web::STATUS_EXPECTATION_FAILED, _TXT("EXPECTATION_FAILED") },
    { Web::STATUS_INTERNAL_SERVER_ERROR, _TXT("INTERNAL_SERVER_ERROR") },
    { Web::STATUS_NOT_IMPLEMENTED, _TXT("NOT_IMPLEMENTED") },
    { Web::STATUS_BAD_GATEWAY, _TXT("NOT_IMPLEMENTED") },
    { Web::STATUS_SERVICE_UNAVAILABLE, _TXT("SERVICE_UNAVAILABLE") },
    { Web::STATUS_GATEWAY_TIMEOUT, _TXT("GATEWAY_TIMEOUT") },
    { Web::STATUS_VERSION_NOT_SUPPORTED, _TXT("VERSION_NOT_SUPPORTED") },

ENUM_CONVERSION_END(Web::WebStatus)

ENUM_CONVERSION_BEGIN(Web::Request::type)

    { Web::Request::HTTP_GET, __TXT(Web::Request::GET) },
    { Web::Request::HTTP_HEAD, __TXT(Web::Request::HEAD) },
    { Web::Request::HTTP_POST, __TXT(Web::Request::POST) },
    { Web::Request::HTTP_PUT, __TXT(Web::Request::PUT) },
    { Web::Request::HTTP_DELETE, __TXT(Web::Request::DELETE) },
    { Web::Request::HTTP_OPTIONS, __TXT(Web::Request::OPTIONS) },
    { Web::Request::HTTP_TRACE, __TXT(Web::Request::TRACE) },
    { Web::Request::HTTP_CONNECT, __TXT(Web::Request::CONNECT) },
    { Web::Request::HTTP_PATCH, __TXT(Web::Request::PATCH) },
    { Web::Request::HTTP_MSEARCH, __TXT(Web::Request::MSEARCH) },
    { Web::Request::HTTP_NOTIFY, __TXT(Web::Request::NOTIFY) },
    { Web::Request::HTTP_UNKNOWN, _TXT("UNKNOWN") },
    { Web::Request::HTTP_NONE, _TXT("NONE") },

ENUM_CONVERSION_END(Web::Request::type)

ENUM_CONVERSION_BEGIN(Web::Request::keywords)

    { Web::Request::HOST, __TXT(__HOST) },
    { Web::Request::UPGRADE, __TXT(__UPGRADE) },
    { Web::Request::ORIGIN, __TXT(__ORIGIN) },
    { Web::Request::CONNECTION, __TXT(__CONNECTION) },
    { Web::Request::ACCEPT, __TXT(__ACCEPT) },
    { Web::Request::ACCEPT_ENCODING, __TXT(__ACCEPT_ENCODING) },
    { Web::Request::USERAGENT, __TXT(__USERAGENT) },
    { Web::Request::CONTENT_TYPE, __TXT(__CONTENT_TYPE) },
    { Web::Request::CONTENT_ENCODING, __TXT(__CONTENT_ENCODING) },
    { Web::Request::CONTENT_LENGTH, __TXT(__CONTENT_LENGTH) },
    { Web::Request::CONTENT_SIGNATURE, __TXT(__CONTENT_SIGNATURE) },
    { Web::Request::TRANSFER_ENCODING, __TXT(__TRANSFER_ENCODING) },
    { Web::Request::ENCODING, __TXT(__ENCODING) },
    { Web::Request::LANGUAGE, __TXT(__LANGUAGE) },
    { Web::Request::ACCESS_CONTROL_REQUEST_METHOD, __TXT(__ACCESS_CONTROL_REQUEST_METHOD) },
    { Web::Request::ACCESS_CONTROL_REQUEST_HEADERS, __TXT(__ACCESS_CONTROL_REQUEST_HEADERS) },
    { Web::Request::WEBSOCKET_KEY, __TXT(__WEBSOCKET_KEY) },
    { Web::Request::WEBSOCKET_PROTOCOL, __TXT(__WEBSOCKET_PROTOCOL) },
    { Web::Request::WEBSOCKET_VERSION, __TXT(__WEBSOCKET_VERSION) },
    { Web::Request::MAN, __TXT(__MAN) },
    { Web::Request::M_X, __TXT(__MX) },
    { Web::Request::S_T, __TXT(__ST) },
    { Web::Request::AUTHORIZATION, __TXT(__AUTHORIZATION) },

ENUM_CONVERSION_END(Web::Request::keywords)

ENUM_CONVERSION_BEGIN(Web::Request::upgrade)

    { Web::Request::UPGRADE_WEBSOCKET, __TXT(__WEBSOCKET) },
    { Web::Request::UPGRADE_UNKNOWN, __TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::Request::upgrade)

ENUM_CONVERSION_BEGIN(Web::Request::connection)

    { Web::Request::CONNECTION_CLOSE, __TXT(__CONNECTION_CLOSE) },
    { Web::Request::CONNECTION_UPGRADE, __TXT(__CONNECTION_UPGRADE) },
    { Web::Request::CONNECTION_KEEPALIVE, __TXT(__CONNECTION_KEEPALIVE) },
    { Web::Request::CONNECTION_UNKNOWN, __TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::Request::connection)

ENUM_CONVERSION_BEGIN(Web::Response::keywords)

    { Web::Response::DATE, __TXT(__DATE) },
    { Web::Response::UPGRADE, __TXT(__UPGRADE) },
    { Web::Response::SERVER, __TXT(__SERVER) },
    { Web::Response::MODIFIED, __TXT(__MODIFIED) },
    { Web::Response::CONTENT_TYPE, __TXT(__CONTENT_TYPE) },
    { Web::Response::CONTENT_LENGTH, __TXT(__CONTENT_LENGTH) },
    { Web::Response::CONTENT_SIGNATURE, __TXT(__CONTENT_SIGNATURE) },
    { Web::Response::CONTENT_ENCODING, __TXT(__CONTENT_ENCODING) },
    { Web::Response::TRANSFER_ENCODING, __TXT(__TRANSFER_ENCODING) },
    { Web::Response::ACCEPT_RANGE, __TXT(__ACCEPT_RANGE) },
    { Web::Response::CONNECTION, __TXT(__CONNECTION) },
    { Web::Response::ETAG, __TXT(__ETAG) },
    { Web::Response::ALLOW, __TXT(__ALLOW) },
    { Web::Response::ACCESS_CONTROL_ALLOW_METHODS, __TXT(__ACCESS_CONTROL_ALLOW_METHODS) },
    { Web::Response::ACCESS_CONTROL_ALLOW_ORIGIN, __TXT(__ACCESS_CONTROL_ALLOW_ORIGIN) },
    { Web::Response::ACCESS_CONTROL_ALLOW_HEADERS, __TXT(__ACCESS_CONTROL_ALLOW_HEADERS) },
    { Web::Response::ACCESS_CONTROL_MAX_AGE, __TXT(__ACCESS_CONTROL_MAX_AGE) },
    { Web::Response::WEBSOCKET_ACCEPT, __TXT(__WEBSOCKET_ACCEPT) },
    { Web::Response::WEBSOCKET_PROTOCOL, __TXT(__WEBSOCKET_PROTOCOL) },
    { Web::Response::LOCATION, __TXT(__LOCATION) },
    { Web::Response::WAKEUP, __TXT(__WAKEUP) },
    { Web::Response::U_S_N, __TXT(__USN) },
    { Web::Response::S_T, __TXT(__ST) },
    { Web::Response::CACHE_CONTROL, __TXT(__CACHE_CONTROL) },
    { Web::Response::APPLICATION_URL, __TXT(__APPLICATION_URL) },

ENUM_CONVERSION_END(Web::Response::keywords)

ENUM_CONVERSION_BEGIN(Web::Response::upgrade)

    { Web::Response::UPGRADE_WEBSOCKET, __TXT(__WEBSOCKET) },
    { Web::Response::UPGRADE_UNKNOWN, __TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::Response::upgrade)

ENUM_CONVERSION_BEGIN(Web::Response::connection)

    { Web::Response::CONNECTION_CLOSE, __TXT(__CONNECTION_CLOSE) },
    { Web::Response::CONNECTION_UPGRADE, __TXT(__CONNECTION_UPGRADE) },
    { Web::Response::CONNECTION_KEEPALIVE, __TXT(__CONNECTION_KEEPALIVE) },
    { Web::Response::CONNECTION_UNKNOWN, __TXT(__UNKNOWN) },

ENUM_CONVERSION_END(Web::Response::connection)

ENUM_CONVERSION_BEGIN(Crypto::EnumHashType)

    { Crypto::HASH_MD5, _TXT("md5") },
    { Crypto::HASH_SHA1, _TXT("sha1") },
    { Crypto::HASH_SHA224, _TXT("sha224") },
    { Crypto::HASH_SHA256, _TXT("sha256") },
    { Crypto::HASH_SHA384, _TXT("sha384") },
    { Crypto::HASH_SHA512, _TXT("sha512") },

ENUM_CONVERSION_END(Crypto::EnumHashType)

ENUM_CONVERSION_BEGIN(Web::Authorization::type)

    { Web::Authorization::BEARER, _TXT("Bearer") },
    { Web::Authorization::BASIC, _TXT("Basic") },

ENUM_CONVERSION_END(Web::Authorization::type)

namespace Web
{
    static const TCHAR DefaultPath[] = _T("/");
    static const TCHAR HTTPKeyWord[] = _T("HTTP/");

    static struct FileExtensionTable {
        Web::MIMETypes type;
        const TCHAR* fileExtension;
        const uint16_t length;
    } extensionLookupTable[] = {
        { Web::MIME_HTML, _TXT("html") },
        { Web::MIME_HTML, _TXT("htm") },
        { Web::MIME_JSON, _TXT("json") },
        { Web::MIME_CSS, _TXT("css") },
        { Web::MIME_IMAGE_TIFF, _TXT("tif") },
        { Web::MIME_IMAGE_TIFF, _TXT("tiff") },
        { Web::MIME_IMAGE_VND, _TXT("wbmp") },
        { Web::MIME_IMAGE_X_ICON, _TXT("ico") },
        { Web::MIME_IMAGE_X_JNG, _TXT("jng") },
        { Web::MIME_IMAGE_X_MS_BMP, _TXT("bmp") },
        { Web::MIME_IMAGE_SVG_XML, _TXT("svg") },
        { Web::MIME_IMAGE_SVG_XML, _TXT("svgz") },
        { Web::MIME_IMAGE_WEBP, _TXT("webp") },
        { Web::MIME_IMAGE_GIF, _TXT("gif") },
        { Web::MIME_IMAGE_JPG, _TXT("jpg") },
        { Web::MIME_IMAGE_JPG, _TXT("jpeg") },
        { Web::MIME_IMAGE_PNG, _TXT("png") },
        { Web::MIME_FONT_TTF, _TXT("ttf") },
        { Web::MIME_FONT_OPENTYPE, _TXT("otp") },
        { Web::MIME_TEXT_XML, _TXT("xml") },
        { Web::MIME_APPLICATION_FONT_WOFF, _TXT("woff") },
        { Web::MIME_APPLICATION_JAVA_ARCHIVE, _TXT("jar") },
        { Web::MIME_APPLICATION_JAVA_ARCHIVE, _TXT("war") },
        { Web::MIME_APPLICATION_JAVA_ARCHIVE, _TXT("e") },
        { Web::MIME_APPLICATION_JAVASCRIPT, _TXT("js") },
        { Web::MIME_APPLICATION_ATOM_XML, _TXT("atom") },
        { Web::MIME_APPLICATION_RSS_XML, _TXT("rss") }
    };

    /* static */ const TCHAR* Request::ToString(const type value)
    {
        return (Core::EnumerateType<type>(value).Data());
    }

    bool EndsWithCaseInsensitive(const string& mainStr, const string& toMatch)
    {
        auto it = toMatch.begin();
        return mainStr.size() >= toMatch.size() &&
                std::all_of(std::next(mainStr.begin(),mainStr.size() - toMatch.size()), mainStr.end(), [&it](const char & character) {
                    return ::tolower(character) == *(it++);
                });
    }

    // Find the correct file to serve
    bool MIMETypeAndEncodingForFile(const string path, string& fileToService, MIMETypes& mimeType, EncodingTypes& encoding)
    {
        mimeType = Web::MIME_UNKNOWN;
        bool filePresent = false;

        if (path.length() != 0) {
            const TCHAR* cpath(path.c_str());
            uint32_t offset(cpath[0] == '/' ? 1 : 0);

            if (cpath[path.length() - 1] == '/') {
                // No filename gives, be default, we go for the index.html page..
                fileToService += &(path[offset]);
            } else {
                // First cut off what we do not need...
                fileToService += &(path[offset]);
                filePresent = true;
            }
        }

        if (filePresent == true) {
            string fileName = fileToService;
            int position = static_cast<int>(fileName.rfind('.', -1));
            if (EndsWithCaseInsensitive(fileName, ".gz")) {
                encoding = EncodingTypes::ENCODING_GZIP;
                fileName.resize(fileName.size() - 3);
                position = static_cast<int>(fileName.rfind('.', -1));
            }
            // See if we have an extension to go on..
            if (position != -1) {
                uint16_t index = 0;

                // Seems we have an extension, what is it
                while ((mimeType == Web::MIME_UNKNOWN) && (index < (sizeof(extensionLookupTable) / sizeof(FileExtensionTable)))) {
                    if (EndsWithCaseInsensitive(fileName, extensionLookupTable[index].fileExtension)) {
                        mimeType = extensionLookupTable[index].type;
                    }

                    index++;
                }
            }
        }
        return (filePresent);
    }

    static Signature ToSignature(const string& input)
    {
        Core::TextFragment inputLine(input);
        Core::OptionalType<Crypto::EnumHashType> hashType;
        Core::OptionalType<Core::TextFragment> hashValue;

        // Convert type and value
        Core::TextParser lineParser(inputLine);

        lineParser.ReadEnumerate<Crypto::EnumHashType, false>(hashType, _T(" \t"));
        lineParser.ReadText(hashValue, _T(" \t"));

        if (hashType.IsSet() && hashValue.IsSet()) {
            uint8_t data[64];
            uint16_t length = sizeof(data);

            // It should be base64 encoded, convert it here
            Core::FromString(hashValue.Value().Text(), data, length);

            if (length == hashType.Value()) {
                return Signature(hashType.Value(), data);
            }
        }

        return (Signature());
    }

    static void FromSignature(const Signature& input, string& signature)
    {
        Core::EnumerateType<Crypto::EnumHashType> enumValue(input.Type());

        signature.clear();

        Core::ToString(input.Data(), input.Type(), false, signature);
        signature = string(enumValue.Data()) + ' ' + signature;
    }

    static Authorization ToAuthorization(const string& input)
    {
        Core::OptionalType<Authorization::type> authorizationType;
        Core::OptionalType<Core::TextFragment> token;
        Core::TextFragment inputLine(input);

        // Convert type and value
        Core::TextParser lineParser(inputLine);

        lineParser.ReadEnumerate<Authorization::type, false>(authorizationType, _T(" \t"));
        lineParser.ReadText(token, _T(" \t"));

        if (authorizationType.IsSet() && token.IsSet()) {
            return (Authorization(authorizationType.Value(), token.Value().Text()));
        }
 
        return (Authorization());
    }

    static void FromAuthorization(const Authorization& input, string& authorization)
    {
        Core::EnumerateType<Authorization::type> enumValue(input.Type());

        authorization = string(enumValue.Data()) + ' ' + input.Token();
    }


    static void ParseContentType(const string& text, Core::OptionalType<MIMETypes>& mime, Core::OptionalType<CharacterTypes>& charType)
    {
        size_t index;
        Core::EnumerateType<MIMETypes> enumValue(text.c_str());

        // See if there is a ';' in the line
        if ((index = text.find(';', 0)) != string::npos) {
            // We need to split, we have more.
            enumValue = Core::EnumerateType<MIMETypes>(Core::TextFragment(text, 0, static_cast<uint32_t>(index)));

            // Check what is behind the colon
            index = text.find_first_not_of(_T(" \t"), index + 1);

            if ((index != string::npos) && (Core::TextFragment(text, static_cast<uint32_t>(index), static_cast<uint32_t>(text.length() - index)) == __CHARACTER_SET)) {
                int start = static_cast<int>(index + sizeof(__CHARACTER_SET));

                // seems like we have character set defined
                Core::EnumerateType<CharacterTypes> myCharType(Core::TextFragment(text, start, static_cast<uint32_t>(text.length() - start)));

                if (myCharType.IsSet() == true) {
                    charType = myCharType.Value();
                } else {
                    charType = CHARACTER_UNKNOWN;
                }
            }
        }

        if (enumValue.IsSet() == true) {
            mime = enumValue.Value();
        } else {
            mime = MIME_UNKNOWN;
        }
    }

    uint16_t Request::Serializer::Serialize(uint8_t stream[], const uint16_t maxLength)
    {
        uint16_t current = 0;

        _lock.Lock();

        if (_state == REPORT) {
            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }
            _buffer = nullptr;
            _state = VERB;
            const Request* backup = _current;
            _current = nullptr;
            Serialized(*backup);
        }

        if (_current != nullptr) {
            while ((current < maxLength) && (_state != REPORT)) {
                while ((current < maxLength) && ((_state & EOL_MARKER) == EOL_MARKER)) {
                    if (_offset == 0) {
                        stream[current++] = '\r';
                        _offset++;
                    } else {
                        stream[current++] = '\n';
                        // We have written what we need to, CLEAR the Marker and continue where we were.
                        _state &= (~EOL_MARKER);
                    }
                }

                // Word complete check and set......
                switch (_state) {
                case VERB: {
                    if (_buffer == nullptr) {
                        Core::EnumerateType<Web::Request::type> textEnum(_current->Verb);
                        if (textEnum.IsSet()) {
                            _buffer = textEnum.Data();
                            _offset = 0;
                        } else {
                            Flush();
                            continue;
                        }
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (current < maxLength) {
                        stream[current++] = ' ';
                        _buffer = nullptr;
                        _state = URL;
                    }
                    break;
                }
                case URL: {
                    if (_buffer == nullptr) {
                        _buffer = (_current->Path.empty() ? DefaultPath : _current->Path.c_str());
                        _offset = 0;
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (current < maxLength) {
                        if ((_current->Query.IsSet() == true) && (_current->Query.Value().empty() == false)) {
                            stream[current++] = '?';
                            _buffer = nullptr;
                            _state = QUERY;
                        } else if ((_current->Fragment.IsSet() == true) && (_current->Fragment.Value().empty() == false)) {
                            stream[current++] = '#';
                            _buffer = nullptr;
                            _state = FRAGMENT;
                        } else {
                            stream[current++] = ' ';
                            _buffer = nullptr;
                            _state = VERSION;
                        }
                    }
                    break;
                }
                case QUERY: {
                    if (_buffer == nullptr) {
                        _buffer = _current->Query.Value().c_str();
                        _offset = 0;
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (current < maxLength) {
                        if ((_current->Fragment.IsSet() == true) && (_current->Fragment.Value().empty() == false)) {
                            stream[current++] = '#';
                            _buffer = nullptr;
                            _state = FRAGMENT;
                        } else {
                            stream[current++] = ' ';
                            _buffer = nullptr;
                            _state = VERSION;
                        }
                    }
                    break;
                }
                case FRAGMENT: {
                    if (_buffer == nullptr) {
                        _buffer = _current->Fragment.Value().c_str();
                        _offset = 0;
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (current < maxLength) {
                        stream[current++] = ' ';
                        _buffer = nullptr;
                        _state = VERSION;
                    }
                    break;
                }
                case VERSION: {
                    if (_buffer == nullptr) {
                        _buffer = HTTPKeyWord;
                        _offset = 0;
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_offset < ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1))) {
                        stream[current++] = _buffer[_offset++];
                    }

                    while ((current < maxLength) && (_state == VERSION)) {
                        if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 0) {
                            // Print Major Version
                            stream[current++] = (_current->MajorVersion + '0');
                            _offset++;
                        } else if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 1) {
                            // Print dot
                            stream[current++] = '.';
                            _offset++;
                        } else if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 2) {
                            // Print Minor Version
                            stream[current++] = (_current->MinorVersion + '0');

                            _state = PAIR_KEY | EOL_MARKER;
                            _keyIndex = static_cast<Request::keywords>(0);
                            _buffer = nullptr;
                            _offset = 0;
                        }
                    }
                    break;
                }
                case PAIR_KEY: {
                    if (_buffer == nullptr) {
                        if ((_keyIndex <= 0) && (_current->Host.IsSet())) {
                            _keyIndex = 1;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __HOST : _T("Host:"));
                            _value = _current->Host.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 1) && (_current->Accept.IsSet() == true)) {
                            _keyIndex = 2;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCEPT : _T("Accept:"));
                            _value = _current->Accept.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 2) && (_current->AcceptEncoding.IsSet() == true)) {
                            Core::EnumerateType<EncodingTypes> enumValue(_current->AcceptEncoding.Value());
                            _keyIndex = 3;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCEPT_ENCODING : _T("Accept-Encoding:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 3) && (_current->Connection.IsSet() == true)) {
                            Core::EnumerateType<Request::connection> enumValue(_current->Connection.Value());
                            _keyIndex = 4;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONNECTION : _T("Connection:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 4) && (_current->Upgrade.IsSet() == true)) {
                            Core::EnumerateType<Request::upgrade> enumValue(_current->Upgrade.Value());
                            _keyIndex = 5;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __UPGRADE : _T("Upgrade:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 5) && (_current->Origin.IsSet() == true)) {
                            _keyIndex = 6;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ORIGIN : _T("Origin:"));
                            _value = _current->Origin.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 6) && (_current->UserAgent.IsSet() == true)) {
                            _keyIndex = 7;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __USERAGENT : _T("User-Agent:"));
                            _value = _current->UserAgent.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 7) && (_current->Encoding.IsSet() == true)) {
                            _keyIndex = 8;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ENCODING : _T("Encoding:"));
                            _value = _current->Encoding.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 8) && (_current->Language.IsSet() == true)) {
                            _keyIndex = 9;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __LANGUAGE : _T("Language:"));
                            _value = _current->Language.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 9) && (_current->WebSocketKey.IsSet() == true)) {
                            _keyIndex = 10;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_KEY : _T("Sec-WebSocket-Key:"));
                            _value = _current->WebSocketKey.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 10) && (_current->WebSocketProtocol.IsSet() == true)) {
                            _keyIndex = 11;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_PROTOCOL : _T("Sec-WebSocket-Protocol:"));
                            _value = _current->WebSocketProtocol.Value().All();
                            _offset = 0;
                        } else if ((_keyIndex <= 11) && (_current->WebSocketVersion.IsSet() == true)) {
                            _keyIndex = 12;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_VERSION : _T("Sec-WebSocket-Version:"));
                            Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_current->WebSocketVersion.Value());
                            number.Serialize(_value);
                            _offset = 0;
                        } else if ((_keyIndex <= 12) && (_current->WebSocketExtensions.IsSet() == true)) {
                            _keyIndex = 13;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_EXTENSIONS : _T("Sec-WebSocket-Extensions:"));
                            _value = _current->WebSocketExtensions.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 13) && (_current->ContentType.IsSet() == true)) {
                            Core::EnumerateType<MIMETypes> enumValue(_current->ContentType.Value());

                            _keyIndex = 14;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_TYPE : _T("Content-Type:"));
                            _value = enumValue.Data();

                            if (_current->ContentCharacterSet.IsSet() == true) {
                                Core::EnumerateType<CharacterTypes> enumValue(_current->ContentCharacterSet.Value());

                                _value += string("; charset=") + enumValue.Data();
                            }

                            _offset = 0;
                        } else if ((_keyIndex <= 14) && (_current->ContentEncoding.IsSet() == true)) {
                            Core::EnumerateType<EncodingTypes> enumValue(_current->ContentEncoding.Value());

                            _keyIndex = 15;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_ENCODING : _T("Content-Encoding:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 15) && (_current->AccessControlHeaders.IsSet() == true)) {
                            _keyIndex = 16;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_REQUEST_HEADERS : _T("Access-Control-Request-Headers:"));
                            _value = _current->AccessControlHeaders.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 16) && (_current->AccessControlMethod.IsSet() == true)) {
                            _keyIndex = 17;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_REQUEST_METHOD : _T("Access-Control-Request-Method:"));
                            _value = _T("");
                            _offset = 0;

                            uint16_t index = 0;
                            const Core::EnumerateConversion<Request::type>* entry = Core::EnumerateType<Request::type>::Entry(index);

                            // Add for each set VERB the keyword...
                            while (entry != nullptr) {
                                if ((entry->value & _current->AccessControlMethod.Value()) != 0) {
                                    // Add this entry to the list of published entries
                                    if (_value.empty() == false) {
                                        _value += ',';
                                    }
                                    _value += entry->name;
                                }
                                entry = Core::EnumerateType<Request::type>::Entry(++index);
                            }
                        } else if ((_keyIndex <= 17) && (_current->TransferEncoding.IsSet() == true)) {
                            Core::EnumerateType<TransferTypes> enumValue(_current->TransferEncoding.Value());

                            _keyIndex = 18;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __TRANSFER_ENCODING : _T("Transfer-Encoding"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 18) && (_current->Man.IsSet() == true)) {
                            _keyIndex = 19;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __MAN : _T("MAN:"));
                            _value = _current->Man.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 19) && (_current->MX.IsSet() == true)) {
                            _keyIndex = 20;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __MX : _T("MX:"));
                            Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_current->MX.Value());
                            number.Serialize(_value);
                            _offset = 0;
                        } else if ((_keyIndex <= 20) && (_current->ST.IsSet() == true)) {
                            _keyIndex = 21;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ST : _T("ST:"));
                            _value = _current->ST.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 21) && (_current->WebToken.IsSet() == true)) {
                            _keyIndex = 22;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __AUTHORIZATION : _T("Authorization:"));
                            FromAuthorization(_current->WebToken.Value(), _value);
                            _offset = 0;
                        } else if ((_keyIndex <= 22) && (((_bodyLength = (_current->_body.IsValid() ? _current->_body->Serialize() : 0)) > 0) || (_current->ContentLength.IsSet() == true) || (!_current->Connection.IsSet()) || (_current->Connection.Value() != Request::CONNECTION_CLOSE))) {
                            _keyIndex = (_bodyLength > 0 ? 23 : 24);

                            Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_bodyLength);
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_LENGTH : _T("Content-Length:"));
                            number.Serialize(_value);
                            _offset = 0;
                        } else if ((_keyIndex <= 23) && (_current->ContentSignature.IsSet() == true)) {
                            _keyIndex = 24;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_SIGNATURE : _T("Content-HMAC:"));
                            FromSignature(_current->ContentSignature.Value(), _value);
                            _offset = 0;
                        } else if ((_keyIndex <= 24) && (_current->Range.IsSet() == true)) {
                            _keyIndex = 25;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __RANGE : _T("Range:"));
                            _value = _current->Range.Value();
                            _offset = 0;
                        }
                    }

                    if (_buffer != nullptr) {
                        // Copy the keyword..
                        while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                            stream[current++] = _buffer[_offset++];
                        }

                        if (current < maxLength) {
                            stream[current++] = ' ';
                            _buffer = nullptr;
                            _state = PAIR_VALUE;
                        }
                    } else {
                        // seems we posted all keywords.. Time for the body..
                        _state = BODY | EOL_MARKER;
                        _offset = 0;
                    }

                    break;
                }
                case PAIR_VALUE: {
                    if (_buffer == nullptr) {
                        _buffer = _value.c_str();
                        _offset = 0;
                    }

                    ASSERT(_buffer != nullptr);

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (_buffer[_offset] == '\0') {
                        _offset = 0;
                        _buffer = nullptr;
                        _state = PAIR_KEY | EOL_MARKER;
                    }
                    break;
                }
                case BODY: {
                    if (_bodyLength != 0) {
                        ASSERT(maxLength >= current);
                        uint32_t size = (static_cast<uint32_t>(maxLength - current) <= _bodyLength ? static_cast<uint32_t>(maxLength - current) : _bodyLength);

                        if (size > 0) {
                            ASSERT(_current->_body.IsValid() == true);

                            _current->_body->Serialize(&(stream[current]), size);
                            _bodyLength -= size;
                            current += size;
                        }
                    }

                    if (_bodyLength == 0) {
                        _state = REPORT;
                    }
                    break;
                }
                case REPORT: {
                    // We should never end up here. Report is on the next call, outside of the loop
                    ASSERT(false);
                    break;
                }
                default: {
                    ASSERT(FALSE);
                    break;
                }
                }
            }
        }

        _lock.Unlock();

        return (current);
    }

    uint16_t Response::Serializer::Serialize(uint8_t stream[], const uint16_t maxLength)
    {
        uint16_t current = 0;

        _lock.Lock();

        if (_state == REPORT) {
            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }
            _buffer = nullptr;
            _state = VERSION;
            const Response* backup = _current;
            _current = nullptr;
            Serialized(*backup);
        }

        if (_current != nullptr) {
            while ((current < maxLength) && (_state != REPORT)) {
                while ((current < maxLength) && ((_state & EOL_MARKER) == EOL_MARKER)) {
                    if (_offset == 0) {
                        stream[current++] = '\r';
                        _offset++;
                    } else {
                        stream[current++] = '\n';
                        // We have written what we need to, CLEAR the Marker and continue where we were.
                        _state &= (~EOL_MARKER);
                    }
                }

                // Word complete check and set......
                switch (_state) {
                case VERSION: {

                    if (_buffer == nullptr) {
                        _buffer = HTTPKeyWord;
                        _offset = 0;
                    }

                    // Copy the keyword..
                    while ((current < maxLength) && (_offset < ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1))) {
                        stream[current++] = _buffer[_offset++];
                    }

                    while ((current < maxLength) && (_state == VERSION)) {
                        if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 0) {
                            // Print Major Version
                            stream[current++] = (_current->MajorVersion + '0');
                            _offset++;
                        } else if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 1) {
                            // Print dot
                            stream[current++] = '.';
                            _offset++;
                        } else if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 2) {
                            // Print Minor Version
                            stream[current++] = (_current->MinorVersion + '0');
                            _offset++;
                        } else if (_offset == ((sizeof(HTTPKeyWord) / sizeof(TCHAR)) - 1) + 3) {
                            // Print space
                            stream[current++] = ' ';
                            _state = ERRORCODE;
                            _buffer = nullptr;
                        }
                    }
                    break;
                }
                case ERRORCODE: {
                    if (_buffer == nullptr) {
                        Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_current->ErrorCode);

                        number.Serialize(_value);

                        // Just use the body as a temeporary buffer :-)
                        _buffer = _value.c_str();
                        _offset = 0;
                    }

                    // Copy the error code..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (_buffer[_offset] == '\0') {
                        _buffer = nullptr;
                        _state = MESSAGE;
                    }

                    break;
                }
                case MESSAGE: {
                    if (current < maxLength) {
                        if (_buffer == nullptr) {
                            if (_current->Message.empty() == true) {
                                Core::EnumerateType<Web::WebStatus> errorCode(_current->ErrorCode);

                                _buffer = errorCode.Data();

                                if (_buffer[0] == '\0') {
                                    _buffer = _T("UNKNOWN");
                                }
                            } else {
                                _buffer = _current->Message.c_str();
                            }

                            // A space before we start !!
                            stream[current++] = ' ';
                            _offset = 0;
                        }

                        ASSERT(_buffer != nullptr);

                        // Copy the keyword..
                        while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                            stream[current++] = _buffer[_offset++];
                        }

                        if (_buffer[_offset] == '\0') {
                            _buffer = nullptr;
                            _offset = 0;
                            _state = PAIR_KEY | EOL_MARKER;
                            _keyIndex = static_cast<Response::keywords>(0);
                        }
                    }
                    break;
                }
                case PAIR_KEY: {
                    if (_buffer == nullptr) {
                        if ((_keyIndex <= 0) && (_current->Server.IsSet())) {
                            _keyIndex = 1;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __SERVER : _T("Server:"));
                            _value = _current->Server.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 1) && (_current->Date.IsSet() == true)) {
                            _keyIndex = 2;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __DATE : _T("Date:"));
                            _value = _current->Date.Value().ToRFC1123(false);
                            _offset = 0;
                        } else if ((_keyIndex <= 2) && (_current->Modified.IsSet() == true)) {
                            _keyIndex = 3;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __MODIFIED : _T("Modified:"));
                            _value = _current->Modified.Value().ToRFC1123(false);
                            _offset = 0;
                        } else if ((_keyIndex <= 3) && (_current->Connection.IsSet() == true)) {
                            Core::EnumerateType<Request::connection> enumValue(_current->Connection.Value());
                            _keyIndex = 4;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONNECTION : _T("Connection:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 4) && (_current->Upgrade.IsSet() == true)) {
                            Core::EnumerateType<Request::upgrade> enumValue(_current->Upgrade.Value());
                            _keyIndex = 5;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __UPGRADE : _T("Upgrade:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 5) && (_current->WebSocketAccept.IsSet() == true)) {
                            _keyIndex = 6;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_ACCEPT : _T("Sec-WebSocket-Accept:"));
                            _value = _current->WebSocketAccept.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 6) && (_current->AcceptRange.IsSet() == true)) {
                            _keyIndex = 7;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCEPT_RANGE : _T("Accept-Range:"));
                            _value = _current->AcceptRange.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 7) && (_current->ETag.IsSet() == true)) {
                            _keyIndex = 8;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ETAG : _T("ETag:"));
                            _value = _current->ETag.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 8) && (_current->WebSocketProtocol.IsSet() == true)) {
                            _keyIndex = 9;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WEBSOCKET_PROTOCOL : _T("Sec-WebSocket-Protocol:"));
                            _value = _current->WebSocketProtocol.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 9) && (_current->Allowed.IsSet() == true)) {
                            _keyIndex = 10;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ALLOW : _T("Allow:"));
                            _value = _T("");
                            _offset = 0;

                            uint16_t index = 0;
                            const Core::EnumerateConversion<Request::type>* entry = Core::EnumerateType<Request::type>::Entry(index);

                            // Add for each set VERB the keyword...
                            while (entry != nullptr) {
                                if ((entry->value & _current->Allowed.Value()) != 0) {
                                    // Add this entry to the list of published entries
                                    if (_value.empty() == false) {
                                        _value += ',';
                                    }
                                    _value += entry->name;
                                }
                                entry = Core::EnumerateType<Request::type>::Entry(++index);
                            }
                        } else if ((_keyIndex <= 10) && (_current->AccessControlHeaders.IsSet() == true)) {
                            _keyIndex = 11;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_ALLOW_HEADERS : _T("Access-Control-Allow-Headers:"));
                            _value = _current->AccessControlHeaders.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 11) && (_current->AccessControlOrigin.IsSet() == true)) {
                            _keyIndex = 12;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_ALLOW_ORIGIN : _T("Access-Control-Allow-Origin:"));
                            _value = _current->AccessControlOrigin.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 12) && (_current->AccessControlMethod.IsSet() == true)) {
                            _keyIndex = 13;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_ALLOW_METHODS : _T("Access-Control-Allow-Methods:"));
                            _value = _T("");
                            _offset = 0;

                            uint16_t index = 0;
                            const Core::EnumerateConversion<Request::type>* entry = Core::EnumerateType<Request::type>::Entry(index);

                            // Add for each set VERB the keyword...
                            while (entry != nullptr) {
                                if ((entry->value & _current->AccessControlMethod.Value()) != 0) {
                                    // Add this entry to the list of published entries
                                    if (_value.empty() == false) {
                                        _value += ',';
                                    }
                                    _value += entry->name;
                                }
                                entry = Core::EnumerateType<Request::type>::Entry(++index);
                            }
                        } else if ((_keyIndex <= 13) && (_current->AccessControlMaxAge.IsSet() == true)) {
                            _keyIndex = 14;

                            Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_current->AccessControlMaxAge.Value());
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ACCESS_CONTROL_MAX_AGE : _T("Access-Control-Max-Age:"));
                            number.Serialize(_value);
                            _offset = 0;
                        } else if ((_keyIndex <= 14) && (_current->ContentType.IsSet() == true)) {
                            Core::EnumerateType<MIMETypes> enumValue(_current->ContentType.Value());

                            _keyIndex = 15;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_TYPE : _T("Content-Type:"));
                            _value = enumValue.Data();
                            if (_current->ContentCharacterSet.IsSet() == true) {
                                Core::EnumerateType<CharacterTypes> enumValue(_current->ContentCharacterSet.Value());

                                _value += string("; charset=") + enumValue.Data();
                            }

                            _offset = 0;
                        } else if ((_keyIndex <= 15) && (_current->ContentEncoding.IsSet() == true)) {
                            Core::EnumerateType<EncodingTypes> enumValue(_current->ContentEncoding.Value());

                            _keyIndex = 16;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_ENCODING : _T("Content-Encoding:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 16) && (_current->TransferEncoding.IsSet() == true)) {
                            Core::EnumerateType<TransferTypes> enumValue(_current->TransferEncoding.Value());

                            _keyIndex = 17;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __TRANSFER_ENCODING : _T("Transfer-Encoding:"));
                            _value = enumValue.Data();
                            _offset = 0;
                        } else if ((_keyIndex <= 17) && (_current->Location.IsSet() == true)) {
                            _keyIndex = 18;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __LOCATION : _T("Location:"));
                            _value = _current->Location.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 18) && (_current->WakeUp.IsSet() == true)) {
                            _keyIndex = 19;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __WAKEUP : _T("Wakeup:"));
                            _value = _current->WakeUp.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 19) && (_current->USN.IsSet() == true)) {
                            _keyIndex = 20;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __USN : _T("USN:"));
                            _value = _current->USN.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 20) && (_current->ST.IsSet() == true)) {
                            _keyIndex = 21;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __ST : _T("ST:"));
                            _value = _current->ST.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 21) && (_current->CacheControl.IsSet() == true)) {
                            _keyIndex = 22;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CACHE_CONTROL : _T("Cache-Control:"));
                            _value = _current->CacheControl.Value();
                            _offset = 0;
                        } else if ((_keyIndex <= 22) && (_current->ApplicationURL.IsSet() == true)) {
                            _keyIndex = 23;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __APPLICATION_URL : _T("Application-URL:"));
                            _value = _current->ApplicationURL.Value().Text();
                            _offset = 0;
                        } else if ((_keyIndex <= 23) && (((_bodyLength = (_current->_body.IsValid() ? _current->_body->Serialize() : 0)) > 0) || (_current->ContentLength.IsSet() == true) || (!_current->Connection.IsSet()) || (_current->Connection.Value() != Response::CONNECTION_CLOSE))) {
                            _keyIndex = (_bodyLength > 0 ? 24 : 25);

                            Core::NumberType<uint32_t, false, BASE_DECIMAL> number(_bodyLength);
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_LENGTH : _T("Content-Length:"));
                            number.Serialize(_value);
                            _offset = 0;
                        } else if ((_keyIndex <= 24) && (_current->ContentSignature.IsSet() == true)) {
                            _keyIndex = 25;
                            _buffer = (_current->Mode() == MARSHAL_UPPERCASE ? __CONTENT_SIGNATURE : _T("Content-HMAC:"));
                            FromSignature(_current->ContentSignature.Value(), _value);
                            _offset = 0;
                        }
                    }

                    if (_buffer != nullptr) {
                        // Copy the keyword..
                        while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                            stream[current++] = _buffer[_offset++];
                        }

                        if (current < maxLength) {
                            stream[current++] = ' ';
                            _buffer = nullptr;
                            _state = PAIR_VALUE;
                        }
                    } else {
                        _offset = 0;
                        // seems we posted all keywords.. Time for the body..
                        _state = BODY | EOL_MARKER;
                    }

                    break;
                }
                case PAIR_VALUE: {
                    if (_buffer == nullptr) {
                        _buffer = _value.c_str();
                        _offset = 0;
                    }

                    ASSERT(_buffer != nullptr);

                    // Copy the keyword..
                    while ((current < maxLength) && (_buffer[_offset] != '\0')) {
                        stream[current++] = _buffer[_offset++];
                    }

                    if (_buffer[_offset] == '\0') {
                        _buffer = nullptr;
                        _offset = 0;
                        _state = PAIR_KEY | EOL_MARKER;
                    }
                    break;
                }
                case BODY: {
                    if (_bodyLength != 0) {
                        ASSERT(maxLength >= current);
                        uint32_t size = (static_cast<uint32_t>(maxLength - current) <= _bodyLength ? static_cast<uint32_t>(maxLength - current) : _bodyLength);

                        if (size > 0) {
                            ASSERT(_current->_body.IsValid() == true);

                            _current->_body->Serialize(&(stream[current]), size);
                            _bodyLength -= size;
                            current += size;
                        }
                    }

                    if (_bodyLength == 0) {
                        _state = REPORT;
                    }
                    break;
                }
                case REPORT: {
                    // We should never end up here. Report is on the next call, outside of the loop
                    ASSERT(false);
                    break;
                }
                default: {
                    ASSERT(FALSE);
                    break;
                }
                }
            }
        }

        _lock.Unlock();

        return (current);
    }

    uint16_t Request::Deserializer::Parse(const uint8_t stream[], const uint16_t maxLength)
    {
        ASSERT(_current != nullptr);

        uint16_t parsed = 0;
        if (_current->_body.IsValid()) {

            // Depending on the ContentEncoding, we need to prepare the data..
            if (_zlibResult == Z_OK) {
                _zlib.avail_in = maxLength;
                _zlib.next_in = const_cast<uint8_t*>(stream);

                /* run inflate() on input until output buffer not full */
                do {
                    uint8_t out[4096];
                    _zlib.avail_out = sizeof(out);
                    _zlib.next_out = out;

                    int ret = inflate(&_zlib, Z_NO_FLUSH);

                    ASSERT(ret != Z_STREAM_ERROR); /* state not clobbered */

                    if ((ret == Z_NEED_DICT) || (ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR)) {
                        inflateEnd(&_zlib);
                        _zlibResult = ret;
                    }

                    parsed = _current->_body->Deserialize(out, static_cast<uint16_t>(sizeof(out) - _zlib.avail_out));

                } while ((_zlib.avail_out == 0) && (_zlibResult == Z_OK));
            } else if (_zlibResult == static_cast<uint32_t>(~0)) {
                parsed = _current->_body->Deserialize(stream, maxLength);
            }
        }

        return parsed;
    }

    void Request::Deserializer::Parse(const string& buffer, const bool /* quoted */)
    {

        _lock.Lock();

        // Word complete check and set......
        switch (_state) {
        case VERB: {
            Core::EnumerateType<Request::type> type(buffer.c_str(), true);
            if ((type.IsSet() == false) || ((_current = Element()) == nullptr)) {
                _parser.FlushLine();
            } else {
                // Seems like we have a hit. Collect a new entry and start setting it.
                _current->Verb = type.Value();
                _parser.CollectWord();
                _state = URL;
            }

            break;
        }
        case URL: {
            // See if there is a '?' mark in this entry
            size_t query = buffer.find('?', 0);
            size_t fragment = buffer.find('#', 0);

            if ((query == string::npos) && (fragment == string::npos)) {
                _current->Path = buffer;
                _current->Query.Clear();
                _current->Fragment.Clear();
            } else if (fragment == string::npos) {
                _current->Path = buffer.substr(0, query);
                _current->Query = buffer.substr(query + 1, buffer.size() - query);
                _current->Fragment.Clear();
            } else if (query == string::npos) {
                _current->Path = buffer.substr(0, fragment);
                _current->Fragment = buffer.substr(fragment + 1, buffer.size() - fragment);
                _current->Query.Clear();
            } else if (query < fragment) {
                _current->Path = buffer.substr(0, query);
                _current->Query = buffer.substr(query + 1, buffer.size() - query);
                _current->Fragment = buffer.substr(fragment + 1, buffer.size() - fragment);
            } else {
                _current->Path = buffer.substr(0, fragment);
                _current->Fragment = buffer.substr(fragment + 1, buffer.size() - fragment);
                _current->Query = buffer.substr(query + 1, buffer.size() - query);
            }

            // It should still be in CollectWord mode.. Continue..
            _state = VERSION;
            break;
        }
        case VERSION: {
            if ((buffer[0] == 'H') && (buffer[1] == 'T') && (buffer[2] == 'T') && (buffer[3] == 'P') && (buffer[4] == '/')) {
                uint8_t number;

                uint32_t usedChars = Core::Unsigned8::Convert(&(buffer.c_str()[5]), 3, number, BASE_DECIMAL);

                if (usedChars > 0) {
                    _current->MajorVersion = number;

                    if (buffer[usedChars + 5] == '.') {
                        usedChars = Core::Unsigned8::Convert(&(buffer.c_str()[5 + 1 + usedChars]), 3, number, BASE_DECIMAL);

                        if (usedChars > 0) {
                            _current->MinorVersion = number;
                        }
                    }
                }

                // Valid, extracted the version numbers..
                _state = PAIR_KEY;
            }
            break;
        }
        case PAIR_KEY: {
            if (buffer.size() == 0) {
                bool chunked = (_current->TransferEncoding.IsSet() == true) && (_current->TransferEncoding.Value() != TRANSFER_UNKNOWN);

                // Empty line means we are starting the BODY
                if (chunked || ((_current->ContentLength.IsSet() == true) && (_current->ContentLength.Value() > 0))) {
                    // Allow the Deserializer to "instantiate"/link the right Body to the response:
                    if (LinkBody(*_current) == true) {
                        _current->Body<Web::IBody>()->Deserialize();
                    }

                    // Depending on the ContentEncoding, we need to prepare the data..
                    if ((_current->ContentEncoding.IsSet()) && (_current->ContentEncoding.Value() != EncodingTypes::ENCODING_UNKNOWN)) {
                        /* allocate inflate state */
                        _zlib.zalloc = nullptr;
                        _zlib.zfree = nullptr;
                        _zlib.opaque = nullptr;
                        _zlib.avail_in = 0;
                        _zlib.next_in = nullptr;
                        _zlibResult = inflateInit2(&_zlib, 16 + MAX_WBITS);
                    } else {
                        _zlibResult = static_cast<uint32_t>(~0);
                    }

                    if (chunked == false) {
                        _parser.PassThrough(_current->ContentLength.Value());
                    } else {
                        _parser.CollectLine();
                        _state = CHUNK_INIT;
                    }
                } else {
                    // There is no body following this according to the length.
                    // Dispatch the Request
                    Deserialized(*_current);
                    _current = nullptr;
                    _state = VERSION;
                }
            } else {
                // See if we recognise this word...
                Core::EnumerateType<Request::keywords> keyWord(buffer.c_str(), true);
                if (keyWord.IsSet() == false) {
                    //TRACE_L1("Could not resolve keyword %s", buffer.c_str());
                    _parser.FlushLine();
                } else {
                    // Seems like we have a hit. Collect a new entry and start setting it.
                    _keyWord = keyWord.Value();
                    _parser.CollectLine();
                    _state = PAIR_VALUE;
                }
            }
            break;
        }
        case PAIR_VALUE: {
            switch (_keyWord) {
            case Request::HOST:
                _current->Host = buffer;
                break;
            case Request::ACCEPT:
                _current->Accept = buffer;
                break;
            case Request::USERAGENT:
                _current->UserAgent = buffer;
                break;
            case Request::ENCODING:
                _current->Encoding = buffer;
                break;
            case Request::LANGUAGE:
                _current->Language = buffer;
                break;
            case Request::ORIGIN:
                _current->Origin = buffer;
                break;
            case Request::WEBSOCKET_PROTOCOL:
                _current->WebSocketProtocol = ProtocolsArray(buffer);
                break;
            case Request::WEBSOCKET_KEY:
                _current->WebSocketKey = buffer;
                break;
            case Request::WEBSOCKET_EXTENSIONS:
                _current->WebSocketExtensions = buffer;
                break;
            case Request::ACCESS_CONTROL_REQUEST_HEADERS:
                _current->AccessControlHeaders = buffer;
                break;
            case Request::MAN:
                _current->Man = buffer;
                break;
            case Request::S_T:
                _current->ST = buffer;
                break;
            case Request::M_X:
                _current->MX = Core::NumberType<uint32_t>(buffer.c_str(), static_cast<uint32_t>(buffer.length())).Value();
                break;
            case Request::AUTHORIZATION:
                _current->WebToken = ToAuthorization(buffer);
                break;
            case Request::CONTENT_SIGNATURE:
                _current->ContentSignature = ToSignature(buffer);
                break;
            case Request::CONTENT_TYPE:
                ParseContentType(buffer, _current->ContentType, _current->ContentCharacterSet);
                break;
            case Request::CONTENT_ENCODING: {
                Core::EnumerateType<EncodingTypes> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->ContentEncoding = enumValue.Value();
                } else {
                    _current->ContentEncoding = ENCODING_UNKNOWN;
                }
                break;
            }
            case Request::ACCEPT_ENCODING: {
                // We only allow for GZIP, right now, so see if it is an allowed format, if so, use it.
                Core::TextSegmentIterator entries(Core::TextFragment(buffer), true, ',');

                while (entries.Next() != false) {
                    if (entries.Current().EqualText(__ENCODING_GZIP, 0, ((sizeof(__ENCODING_GZIP) / sizeof(TCHAR)) - 1), false) == true) {
                        _current->AcceptEncoding = ENCODING_GZIP;
                    }
                }
                break;
            }
            case Request::TRANSFER_ENCODING: {
                Core::EnumerateType<TransferTypes> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->TransferEncoding = enumValue.Value();
                } else {
                    _current->TransferEncoding = TRANSFER_UNKNOWN;
                }
                break;
            }
            case Request::CONNECTION: {
                Core::EnumerateType<Request::connection> enumValue(buffer.c_str(), false);
                if (enumValue.IsSet() == true) {
                    _current->Connection = enumValue.Value();
                } else if (Request::ScanForKeyword(buffer, Request::connection::CONNECTION_UPGRADE) == true) {
                    _current->Connection = Request::connection::CONNECTION_UPGRADE;
                } else {
                    _current->Connection = Request::CONNECTION_UNKNOWN;
                }
                break;
            }
            case Request::UPGRADE: {
                Core::EnumerateType<Request::upgrade> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->Upgrade = enumValue.Value();
                } else {
                    _current->Upgrade = Request::UPGRADE_UNKNOWN;
                }
                break;
            }
            case Request::WEBSOCKET_VERSION: {
                uint32_t number = 0;

                if (Core::Unsigned32::Convert(buffer.c_str(), static_cast<uint32_t>(buffer.size()), number, BASE_DECIMAL) > 0) {
                    _current->WebSocketVersion = number;
                }
                break;
            }
            case Request::CONTENT_LENGTH: {
                uint32_t number = 0;

                if (Core::Unsigned32::Convert(buffer.c_str(), static_cast<uint32_t>(buffer.size()), number, BASE_DECIMAL) > 0) {
                    _current->ContentLength = number;
                }
                break;
            }
            case Request::ACCESS_CONTROL_REQUEST_METHOD: {
                uint16_t value = 0;
                Core::TextSegmentIterator index(Core::TextFragment(buffer), true, ',');
                while (index.Next()) {
                    Core::EnumerateType<Request::type> enumerate(index.Current().Text().c_str(), false);

                    if (enumerate.IsSet() == true) {
                        value |= enumerate.Value();
                    }
                }

                _current->AccessControlMethod = value;

                break;
            }
            }
            break;
        }
        case CHUNK_INIT: {
            uint32_t chunkedSize = Core::NumberType<uint32_t, false, BASE_HEXADECIMAL>(Core::TextFragment(buffer));
            if (chunkedSize == 0) {
                _state = BODY_END;
                _parser.FlushLine();
            } else {
                _parser.PassThrough(chunkedSize);
            }
            break;
        }
        case CHUNK_END:
        case BODY_END:
            break;
        }

        _lock.Unlock();
    }

    void Request::Deserializer::EndOfPassThrough()
    {
        // Check if we are onchunked mode
        if ((_current->TransferEncoding.IsSet() == true) && (_current->TransferEncoding.Value() == TRANSFER_CHUNKED)) {
            _state = CHUNK_END;
            _parser.FlushLine();
        } else {
            if ((_current->ContentEncoding.IsSet()) && (_current->ContentEncoding.Value() != EncodingTypes::ENCODING_UNKNOWN)) {
                /* clean up and return */
                (void)inflateEnd(&_zlib);
            }

            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }

            // There is no body following this according to the length.
            // Dispatch the Request
            Deserialized(*_current);
            _current = nullptr;
            _parser.CollectWord(Parser::UPPERCASE);
            _state = VERB;
        }
    }

    void Request::Deserializer::EndOfLine()
    {
        switch (_state) {
        case VERB:
        case URL:
        case VERSION: {
            // This is an error. Reset.
            _state = VERB;
            _current = nullptr;
            _parser.CollectWord(Parser::UPPERCASE);
            break;
        }
        case PAIR_VALUE: {
            _state = PAIR_KEY;
            _parser.CollectWord(':', Parser::UPPERCASE);
            break;
        }
        case PAIR_KEY: {
            _parser.CollectWord(':', Parser::UPPERCASE);
            break;
        }
        case CHUNK_END: {
            _state = CHUNK_INIT;
            _parser.CollectLine();
            break;
        }
        case BODY_END: {
            if ((_current->ContentEncoding.IsSet()) && (_current->ContentEncoding.Value() != EncodingTypes::ENCODING_UNKNOWN)) {
                /* clean up and return */
                (void)inflateEnd(&_zlib);
            }

            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }

            // There is no body following this according to the length.
            // Dispatch the Request
            Deserialized(*_current);
            _current = nullptr;
            _parser.CollectWord(Parser::UPPERCASE);
            _state = VERB;
            break;
        }
        case CHUNK_INIT: {
            break;
        }
        default: {
            ASSERT(false);
        }
        }
    }

    uint16_t Response::Deserializer::Parse(const uint8_t stream[], const uint16_t maxLength)
    {
        ASSERT(_current != nullptr);

        uint16_t parsed = 0;
        if (_current->_body.IsValid()) {

            // Depending on the ContentEncoding, we need to prepare the data..
            if (_zlibResult == Z_OK) {
                _zlib.avail_in = maxLength;
                _zlib.next_in = const_cast<uint8_t*>(stream);

                /* run inflate() on input until output buffer not full */
                do {
                    uint8_t out[4096];
                    _zlib.avail_out = sizeof(out);
                    _zlib.next_out = out;

                    int ret = inflate(&_zlib, Z_NO_FLUSH);

                    ASSERT(ret != Z_STREAM_ERROR); /* state not clobbered */

                    if ((ret == Z_NEED_DICT) || (ret == Z_DATA_ERROR) || (ret == Z_MEM_ERROR)) {
                        inflateEnd(&_zlib);
                        _zlibResult = ret;
                    }

                    parsed = _current->_body->Deserialize(out, static_cast<uint16_t>(sizeof(out) - _zlib.avail_out));

                } while ((_zlib.avail_out == 0) && (_zlibResult == Z_OK));
            } else if (_zlibResult == static_cast<uint32_t>(~0)) {
                parsed = _current->_body->Deserialize(stream, maxLength);
            }
        }

        return parsed;
    }

    void Response::Deserializer::Parse(const string& buffer, const bool /* quoted */)
    {
        _lock.Lock();

        // Word complete check and set......
        switch (_state) {
        case VERSION: {
            if ((buffer[0] == 'H') && (buffer[1] == 'T') && (buffer[2] == 'T') && (buffer[3] == 'P') && (buffer[4] == '/')) {
                uint8_t number;

                uint32_t usedChars = Core::Unsigned8::Convert(&(buffer.c_str()[5]), 3, number, BASE_DECIMAL);

                _current = Element();

                if (usedChars > 0) {
                    _current->MajorVersion = number;

                    if (buffer[usedChars + 5] == '.') {
                        usedChars = Core::Unsigned8::Convert(&(buffer.c_str()[5 + 1 + usedChars]), 3, number, BASE_DECIMAL);

                        if (usedChars > 0) {
                            _current->MinorVersion = number;
                        }
                    }
                }

                // Valild, extracted the version numbers..
                _state = ERRORCODE;
            }

            break;
        }
        case ERRORCODE: {
            Core::NumberType<uint16_t, false, BASE_DECIMAL> translatedCode(buffer.c_str(), static_cast<uint32_t>(buffer.size()), BASE_DECIMAL);
            _current->ErrorCode = translatedCode;
            _state = MESSAGE;
            _parser.CollectLine();
            break;
        }
        case MESSAGE: {
            _current->Message = buffer;
            _state = PAIR_KEY;
            break;
        }
        case PAIR_KEY: {
            if (buffer.size() == 0) {
                bool chunked = (_current->TransferEncoding.IsSet() == true) && (_current->TransferEncoding.Value() != TRANSFER_UNKNOWN);

                // Empty line means we are starting the BODY
                if (chunked || ((_current->ContentLength.IsSet() == true) && (_current->ContentLength.Value() > 0))) {
                    // Allow the Deserializer to "instantiate"/link the right Body to the response:
                    bool hasBody = LinkBody(*_current);
                    if (hasBody == true) {
                        _current->Body<Web::IBody>()->Deserialize();
                    }

                    // Depending on the ContentEncoding, we need to prepare the data..
                    if ((_current->ContentEncoding.IsSet()) && (_current->ContentEncoding.Value() != EncodingTypes::ENCODING_UNKNOWN)) {
                        /* allocate inflate state */
                        _zlib.zalloc = nullptr;
                        _zlib.zfree = nullptr;
                        _zlib.opaque = nullptr;
                        _zlib.avail_in = 0;
                        _zlib.next_in = nullptr;
                        _zlibResult = inflateInit2(&_zlib, 16 + MAX_WBITS);
                    } else {
                        _zlibResult = static_cast<uint32_t>(~0);
                    }

                    if (hasBody == true) {
                        if (chunked == false) {
                            _parser.PassThrough(_current->ContentLength.Value());
                        } else {
                            _parser.CollectLine();
                            _state = CHUNK_INIT;
                        }
                    } else {
                        _state = BODY_END;
                    }
                } else {
                    // There is no body following this according to the length.
                    // Dispatch the Response
                    Deserialized(*_current);
                    _current = nullptr;
                    _parser.Reset();
                    _state = VERSION;
                }
                break;
            } else {
                // See if we recognise this word...
                Core::EnumerateType<Response::keywords> keyWord(buffer.c_str(), true);
                if (keyWord.IsSet() == false) {
                    //TRACE_L1("Could not resolve keyword %s", buffer.c_str());
                    _parser.FlushLine();
                } else {
                    // Seems like we have a hit. Collect a new entry and start setting it.
                    _keyWord = keyWord.Value();
                    _parser.CollectLine();
                    _state = PAIR_VALUE;
                }
            }
            break;
        }
        case PAIR_VALUE: {
            switch (_keyWord) {
            case Response::ACCEPT_RANGE:
                _current->AcceptRange = buffer;
                break;
            case Response::ACCESS_CONTROL_ALLOW_ORIGIN:
                _current->AccessControlOrigin = buffer;
                break;
            case Response::ACCESS_CONTROL_ALLOW_HEADERS:
                _current->AccessControlHeaders = buffer;
                break;
            case Response::ETAG:
                _current->ETag = buffer;
                break;
            case Response::SERVER:
                _current->Server = buffer;
                break;
            case Response::WEBSOCKET_ACCEPT:
                _current->WebSocketAccept = buffer;
                break;
            case Response::WEBSOCKET_PROTOCOL:
                _current->WebSocketProtocol = buffer;
                break;
            case Response::CONTENT_SIGNATURE:
                _current->ContentSignature = ToSignature(buffer);
                break;
            case Response::LOCATION:
                _current->Location = buffer;
                break;
            case Response::WAKEUP:
                _current->WakeUp = buffer;
                break;
            case Response::U_S_N:
                _current->USN = buffer;
                break;
            case Response::S_T:
                _current->ST = buffer;
                break;
            case Response::APPLICATION_URL:
                _current->ApplicationURL = Core::URL(buffer);
                break;
            case Response::CACHE_CONTROL:
                _current->CacheControl = buffer;
                break;
            case Response::CONTENT_TYPE:
                ParseContentType(buffer, _current->ContentType, _current->ContentCharacterSet);
                break;
            case Response::CONTENT_ENCODING: {
                Core::EnumerateType<EncodingTypes> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->ContentEncoding = enumValue.Value();
                } else {
                    _current->ContentEncoding = ENCODING_UNKNOWN;
                }
                break;
            }
            case Response::TRANSFER_ENCODING: {
                Core::EnumerateType<TransferTypes> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->TransferEncoding = enumValue.Value();
                } else {
                    _current->TransferEncoding = TRANSFER_UNKNOWN;
                }
                break;
            }
            case Response::ALLOW: {
                uint16_t value = 0;
                Core::TextSegmentIterator index(Core::TextFragment(buffer), true, ',');
                while (index.Next()) {
                    Core::EnumerateType<Request::type> enumerate(index.Current().Text().c_str(), false);

                    if (enumerate.IsSet() == true) {
                        value |= enumerate.Value();
                    }
                }

                _current->Allowed = value;

                break;
            }
            case Response::ACCESS_CONTROL_ALLOW_METHODS: {
                uint16_t value = 0;
                Core::TextSegmentIterator index(Core::TextFragment(buffer), true, ',');
                while (index.Next()) {
                    Core::EnumerateType<Request::type> enumerate(index.Current().Text().c_str(), false);

                    if (enumerate.IsSet() == true) {
                        value |= enumerate.Value();
                    }
                }

                _current->AccessControlMethod = value;

                break;
            }
            case Response::CONNECTION: {
                Core::EnumerateType<Response::connection> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->Connection = enumValue.Value();
                } else {
                    _current->Connection = Response::CONNECTION_UNKNOWN;
                }
                break;
            }
            case Response::UPGRADE: {
                Core::EnumerateType<Response::upgrade> enumValue(buffer.c_str(), false);

                if (enumValue.IsSet() == true) {
                    _current->Upgrade = enumValue.Value();
                } else {
                    _current->Upgrade = Response::UPGRADE_UNKNOWN;
                }
                break;
            }
            case Response::DATE: {
                Core::Time time;

                if (time.FromString(buffer, true) == true) {
                    _current->Date = time;
                }

                break;
            }
            case Response::MODIFIED: {
                Core::Time time;

                if (time.FromString(buffer, true) == true) {
                    _current->Date = time;
                }

                break;
            }
            case Response::CONTENT_LENGTH: {
                uint32_t number = 0;

                if (Core::Unsigned32::Convert(buffer.c_str(), static_cast<uint32_t>(buffer.size()), number, BASE_DECIMAL) > 0) {
                    _current->ContentLength = number;
                }
                break;
            }
            case Response::ACCESS_CONTROL_MAX_AGE: {
                uint32_t number = 0;

                if (Core::Unsigned32::Convert(buffer.c_str(), static_cast<uint32_t>(buffer.size()), number, BASE_DECIMAL) > 0) {
                    _current->AccessControlMaxAge = number;
                }
                break;
            }
            }
            break;
        }
        case CHUNK_INIT: {
            uint32_t chunkedSize = Core::NumberType<uint32_t>(Core::TextFragment(buffer), NumberBase::BASE_HEXADECIMAL);
            if (chunkedSize == 0) {
                _parser.FlushLine();
                _state = BODY_END;
            } else {
                _parser.PassThrough(chunkedSize);
            }
            break;
        }
        case CHUNK_END:
        case BODY_END:
            break;
        }

        _lock.Unlock();
    }

    void Response::Deserializer::EndOfPassThrough()
    {
        // Check if we are onchunked mode
        if ((_current->TransferEncoding.IsSet() == true) && (_current->TransferEncoding.Value() == TRANSFER_CHUNKED)) {
            _state = CHUNK_END;
            _parser.FlushLine();
        } else {
            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }

            // There is no body following this according to the length.
            // Dispatch the Response
            Deserialized(*_current);
            _current = nullptr;
            _parser.Reset();
            _state = VERSION;
        }
    }

    void Response::Deserializer::EndOfLine()
    {
        switch (_state) {
        case VERSION: {
            // Stay where you are, nothing to be done, we expect a beginning
            break;
        }
        case ERRORCODE: {
            // This is an error. Reset.
            _state = VERSION;
            _current = nullptr;
            break;
        }
        case MESSAGE: {
            // Thats fine than the error code was the last item..
            _current->Message.clear();
            _state = PAIR_KEY;
            _parser.CollectWord(':', Parser::UPPERCASE);
            break;
        }
        case CHUNK_END: {
            _state = CHUNK_INIT;
            _parser.CollectLine();
            break;
        }
        case PAIR_VALUE: {
            _state = PAIR_KEY;
            _parser.CollectWord(':', Parser::UPPERCASE);
            break;
        }
        case PAIR_KEY: {
            _parser.CollectWord(':', Parser::UPPERCASE);
            break;
        }
        case BODY_END: {
            if ((_current->ContentEncoding.IsSet()) && (_current->ContentEncoding.Value() != EncodingTypes::ENCODING_UNKNOWN)) {
                /* clean up and return */
                (void)inflateEnd(&_zlib);
            }

            if (_current->_body.IsValid() == true) {
                _current->_body->End();
            }

            // There is no body following this according to the length.
            // Dispatch the Response
            Deserialized(*_current);
            _current = nullptr;
            _parser.Reset();
            _state = VERSION;
            break;
        }
        case CHUNK_INIT: {
            break;
        }
        default:
            ASSERT(false);
        }
    }
}
}

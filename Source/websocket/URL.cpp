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

#include "URL.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(URL::SchemeType)

    { URL::SCHEME_FILE, _TXT("file") },
    { URL::SCHEME_HTTP, _TXT("http") },
    { URL::SCHEME_HTTPS, _TXT("https") },
    { URL::SCHEME_FTP, _TXT("ftp") },
    { URL::SCHEME_NTP, _TXT("ntp") },
    { URL::SCHEME_WS, _TXT("ws") },
    { URL::SCHEME_WSS, _TXT("wss") },
    { URL::SCHEME_UNKNOWN, _TXT("????") },

ENUM_CONVERSION_END(URL::SchemeType)

namespace Core
{
    /* Returns a url-encoded version of source */
    /* static */ uint16_t URL::Encode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength)
    {
        static char hex[] = "0123456789abcdef";
        uint16_t srcLength = sourceLength;
        uint16_t dstLength = destinationLength;

        while ((*source != '\0') && (srcLength != 0) && (dstLength >= 3)) {
            TCHAR current = *source++;

            if ((isalnum(current) != 0) || (current == '-') || (current == '_') || (current == '.') || (current == '~')) {
                *destination++ = current;
                dstLength--;
            } else if (current == ' ') {
                *destination++ = '+';
                dstLength--;
            } else {
                *destination++ = '%';
                *destination++ = hex[(current >> 4) & 0x0F];
                *destination++ = hex[(current & 0x0F)];
                dstLength -= 3;
            }

            srcLength--;
        }

        if (dstLength != 0) {
            *destination = '\0';
        }

        return (destinationLength - dstLength);
    }

    /* Returns a url-decoded version of source */
    /* static */ uint16_t URL::Decode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength)
    {
        uint16_t srcLength = sourceLength;
        uint16_t dstLength = destinationLength;

        while ((*source != '\0') && (srcLength != 0) && (dstLength != 0)) {
            TCHAR current = *source++;

            if (current == '%') {
                if ((source[0] != '\0') && (source[1] != '\0')) {
                    *destination++ = (((isdigit(source[0]) ? (source[0] - '0') : (tolower(source[0]) - 'a' + 10)) & 0x0F) << 4) | ((isdigit(source[1]) ? (source[1] - '0') : (tolower(source[1]) - 'a' + 10)) & 0x0F);
                    source += 2;
                    srcLength -= 3;
                }
            } else if (current == '+') {
                *destination++ = ' ';
                srcLength--;
            } else {
                *destination++ = current;
                srcLength--;
            }

            dstLength--;
        }

        if (dstLength != 0) {
            *destination = '\0';
        }

        return (destinationLength - dstLength);
    }

    /* static */ uint16_t URL::Base64Encode(const uint8_t* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength, const bool padding)
    {
        static const TCHAR base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                            "abcdefghijklmnopqrstuvwxyz"
                                            "0123456789-_";

        uint8_t state = 0;
        uint16_t index = 0;
        uint8_t lastStuff = 0;
        uint16_t result = 0;

        while ((result < destinationLength) && (index < sourceLength)) {
            if (state == 0) {
                destination[result++] = base64_chars[((source[index] & 0xFC) >> 2)];
                lastStuff = ((source[index] & 0x03) << 4);
                state = 1;
            } else if (state == 1) {
                destination[result++] = base64_chars[(((source[index] & 0xF0) >> 4) | lastStuff)];
                lastStuff = ((source[index] & 0x0F) << 2);
                state = 2;
            } else if (state == 2) {
                destination[result++] = base64_chars[(((source[index] & 0xC0) >> 6) | lastStuff)];
                if (result < destinationLength) {
					destination[result++] = base64_chars[(source[index] & 0x3F)];
				}
                state = 0;
            }
            index++;
        }

        if ((state != 0) && (result < destinationLength)) {
            destination[result++] = base64_chars[lastStuff];

            if (padding == true) {
                if (result < destinationLength) {
                    destination[result++] = '.';
				}
                if ((state == 2) && (result < destinationLength)) {
                    destination[result++] = '.';
                }
            }
        }

        return (result);
    }

    /* static */ uint16_t URL::Base64Decode(const TCHAR* source, const uint16_t sourceLength, uint8_t* destination, const uint16_t destinationLength, const TCHAR* ignoreList)
    {
        uint8_t state = 0;
        uint16_t index = 0;
        uint16_t filler = 0;
        uint8_t lastStuff = 0;

        while ((index < sourceLength) && (filler < destinationLength)) {
            uint8_t converted;
            TCHAR current = source[index++];

            if ((current >= 'A') && (current <= 'Z')) {
                converted = (current - 'A');
            } else if ((current >= 'a') && (current <= 'z')) {
                converted = (current - 'a' + 26);
            } else if ((current >= '0') && (current <= '9')) {
                converted = (current - '0' + 52);
            } else if (current == '-') {
                converted = 62;
            } else if (current == '_') {
                converted = 63;
            } else if ((ignoreList != nullptr) && (::strchr(ignoreList, current) != nullptr)) {
                continue;
            } else {
                break;
            }

            if (state == 0) {
                lastStuff = converted << 2;
                state = 1;
            } else if (state == 1) {
                destination[filler++] = (((converted & 0x30) >> 4) | lastStuff);
                lastStuff = ((converted & 0x0F) << 4);
                state = 2;
            } else if (state == 2) {
                destination[filler++] = (((converted & 0x3C) >> 2) | lastStuff);
                lastStuff = ((converted & 0x03) << 6);
                state = 3;
            } else if (state == 3) {
                destination[filler++] = ((converted & 0x3F) | lastStuff);
                state = 0;
            }
        }

        return (filler);
    }

    inline static uint32_t CopyFragment(TCHAR* destination, const uint32_t maxLength, uint32_t index, const string& data)
    {
        const uint32_t count = (data.length() > static_cast<unsigned int>(maxLength) ? maxLength : static_cast<uint32_t>(data.length()));

        ::memcpy(&(destination[index]), data.c_str(), (count * sizeof(TCHAR)));

        return (index + count);
    }

    inline static uint32_t FindNext(const TextFragment& url, const uint32_t start, const uint32_t end) 
    {
        uint32_t result = start;
        uint32_t offset = url.ForwardFind(url[result], result + 1);

        while (offset < end) {
            result = offset;
            offset = url.ForwardFind(url[result], result + 1);
        }
        return (result);
    }

    //
    //  scheme://username:password@example.com:8888/this/is/a/path/to/file.html?name=ferret#nose
    //  \____/   \________________/\_________/ \__/\__________________________/ \_________/ \__/
    //     |           |               |       |                 |                   |       |
    //  scheme      userinfo        hostname  port              path               query    hash
    //
    void URL::Parse(const TextFragment& urlStr)
    {
        uint32_t offset = 0;
        uint32_t length = urlStr.Length();

        // find he first part, the scheme
        if (((offset = urlStr.ForwardFind(':', 0)) >= length) || (urlStr[offset+1] != '/') || (urlStr[offset+2] != '/')) {
            _scheme = SCHEME_UNKNOWN;
        }
        else {
            // determine the Schema
            Core::EnumerateType<SchemeType> value (Core::TextFragment(urlStr, 0, offset));

            if (value.IsSet() == false) {
                _scheme = SCHEME_UNKNOWN;
            }
            else {
                _scheme = value.Value();
            }

            offset += 3;

            // The delimiter, '://' is found now see if we recognize the scheme..
            uint32_t atsign = urlStr.ForwardFind('@', offset);
            uint32_t path = urlStr.ForwardFind('/', offset);
            uint32_t query = urlStr.ForwardFind('?', offset); 
            uint32_t ref = urlStr.ForwardFind('#', offset); 

            // It could be that there is  no path,or query and onl a ref. Determine the marker by finding the
            // minum index value we have..
            uint32_t marker = std::min(std::min(std::min(path, query), ref), length);

            if ((atsign < length) && (atsign < marker)) {
                // the password might contain a '@' if so, the last '@' sign is the one that 
                // signals the start of the hostname portion
                uint32_t hostname = FindNext(urlStr, atsign, marker);
                uint32_t user = urlStr.ForwardFind(':', offset);

                // find he first part, the scheme
                if (user < hostname) {
                    _username = Core::TextFragment(urlStr, offset, user - offset).Text();
                    _password = Core::TextFragment(urlStr, user + 1, hostname - user - 1).Text();
                }
 
                ParseDomain(Core::TextFragment(urlStr, atsign + 1, marker - atsign - 1));
            }
            else {
                ParseDomain(Core::TextFragment(urlStr, offset, marker - offset));
            }

            if (marker < length) {

                // Check for a path
                if (path < length) {
                    marker = std::min(std::min(query, ref), length);
                    _path = Core::TextFragment(urlStr, path + 1, marker - path - 1).Text();
                }

                if (query < length) {
                    marker = std::min(ref, length);
                    _query = Core::TextFragment(urlStr, query + 1, marker - query - 1).Text();
                }

                if (ref < length) {
                    _ref = Core::TextFragment(urlStr, ref + 1, length - ref - 1).Text();
                }
            }
        }
    }

    void URL::ParseDomain(const Core::TextFragment& url)
    {
        // Finde the last ':', if it is in there..
        uint32_t port = url.ForwardFind(':', 0);
        uint32_t length = url.Length();

        if (port >= length) {
            // Only a domain name
            _host = url.Text();
        }
        else {
            port = FindNext(url, port, length);
            _host = Core::TextFragment(url, 0, port).Text();
            _port = Core::NumberType<uint16_t>(Core::TextFragment(url, port + 1, length - port - 1)).Value();
        }
    }

    string URL::Text() const
    {
        string result;

        if (IsValid() == true) {
            uint32_t url_len = MaximumURLLength;
            TCHAR* url = reinterpret_cast<TCHAR*>(alloca((url_len + 1) * sizeof(TCHAR)));
             
            uint32_t index = CopyFragment(url, url_len, 0, string(Core::EnumerateType<URL::SchemeType>(_scheme).Data()));

            index = CopyFragment(url, url_len, index, string(_T("://")));

            if (_username.IsSet()) {
                index = CopyFragment(url, url_len, index, _username.Value());

                if ((_password.IsSet()) && (index < url_len)) {
                    url[index] = ':';
                    index = CopyFragment(url, url_len, index + 1, _password.Value());
                }

                url[index] = '@';
                index += 1;
            }

            if (_host.IsSet()) {
                index = CopyFragment(url, url_len, index, _host.Value());

                if ((_port.IsSet()) && (index < url_len)) {
                    url[index] = ':';
                    index = CopyFragment(url, url_len, index + 1, Core::Unsigned16(_port.Value()).Text());
                }
            }

            url[index] = '/';
            index += 1;

            if (_path.IsSet()) {
                index = CopyFragment(url, url_len, index, _path.Value());
            }

            if ((_query.IsSet()) && (index < url_len)) {
                url[index] = '?';
                index = CopyFragment(url, url_len, index + 1, _query.Value());
            }
            if ((_ref.IsSet()) && (index < url_len)) {
                url[index] = '#';
                index = CopyFragment(url, url_len, index + 1, _ref.Value());
            }

            if (index < url_len) {
                url[index] = '\0';
            }

            result = string(url);
        }

        return (result);
    }


}
} // namespace Core

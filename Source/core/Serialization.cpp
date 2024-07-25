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
 
#include "Serialization.h"

namespace Thunder {
namespace Core {
PUSH_WARNING(DISABLE_WARNING_DEPRECATED_USE)

#ifndef __CORE_NO_WCHAR_SUPPORT__
    void ToString(const wchar_t realString[], std::string& result)
    {
#if defined(__WINDOWS__) || defined(__LINUX__)

        int requiredSize = static_cast<int>(::wcstombs(nullptr, realString, 0));
#ifdef __WINDOWS__
        char* convertedText = static_cast<char*>(::_alloca((requiredSize + 1) * sizeof(char)));
#else
        char* convertedText = static_cast<char*>(alloca((requiredSize + 1) * sizeof(char)));
#endif

#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        size_t size =
#endif
#endif
            ::wcstombs(convertedText, realString, requiredSize + 1);
#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        ASSERT(size != (size_t)(-1));
#endif
#endif

        result = std::string(convertedText);
#endif
    }

    void ToString(const char realString[], std::wstring& result)
    {
        int requiredSize = static_cast<int>(::mbstowcs(nullptr, realString, 0));

#if defined(__WINDOWS__) || defined(__LINUX__)

#ifdef __WINDOWS__
        wchar_t* convertedText = static_cast<wchar_t*>(::_alloca((requiredSize + 1) * sizeof(wchar_t)));
#else
        wchar_t* convertedText = static_cast<wchar_t*>(alloca((requiredSize + 1) * sizeof(wchar_t)));
#endif

#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        size_t size =
#endif
#endif
            ::mbstowcs(convertedText, realString, requiredSize + 1);

#endif

#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        ASSERT(size != (size_t)(-1));
#endif
#endif

        result = std::wstring(convertedText);
    }

    string ToString(const wchar_t realString[], unsigned int length)
    {
#ifdef _UNICODE

        return (string(realString, length));

#else

        int requiredSize = static_cast<int>(::wcstombs(nullptr, realString, length));

#ifdef __WINDOWS__

        char* convertedText = static_cast<char*>(::_alloca(requiredSize + 1));

#if _TRACE_LEVEL > 0
        size_t size =
#endif
            ::wcstombs(convertedText, realString, requiredSize + 1);

#else

        char* convertedText = static_cast<char*>(::alloca(requiredSize + 1));

#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        size_t size =
#endif
#endif
            ::wcstombs(convertedText, realString, requiredSize + 1);

#endif

#if _TRACE_LEVEL > 0
#ifdef __DEBUG__
        ASSERT(size != (size_t)(-1));
#endif
#endif

        return (string(convertedText));

#endif // _UNICODE
    }

#endif // __CORE_NO_WCHAR_SUPPORT__

    string ToString(const char realString[], unsigned int length)
    {
#ifdef _UNICODE

#if defined(__WINDOWS__) || defined(__LINUX__)

        int requiredSize = ::mbstowcs(nullptr, realString, length);
        wchar_t* convertedText = static_cast<wchar_t*>(::_alloca((requiredSize + 1) * 2));

#if _TRACE_LEVEL > 0
        size_t size =
#endif
            ::mbstowcs(convertedText, realString, requiredSize + 1);

#if _TRACE_LEVEL > 0
        ASSERT(size != (size_t)(-1));
#endif

        return (string(convertedText));
#endif

#else

        return (string(realString, length));

#endif // _UNICODE
    }

POP_WARNING()

    static const TCHAR hex_chars[] = "0123456789abcdef";

    void EXTERNAL ToHexString(const uint8_t object[], const uint32_t length, string& result, const TCHAR delimiter)
    {
        ASSERT(object != nullptr);

        uint32_t index = static_cast<uint32_t>(result.length());
        result.resize(index + (length * 2) + (delimiter == '\0' ? 0 : (length - 1)));

        result[1] = hex_chars[object[0] & 0xF];

        for (uint32_t i = 0, j = index; i < length; i++) {
            if ((delimiter != '\0') && (i > 0)) {
                result[j++] = delimiter;
            }
            result[j++] = hex_chars[object[i] >> 4];
            result[j++] = hex_chars[object[i] & 0xF];
        }
    }

    uint32_t EXTERNAL FromHexString(const string& hexString, uint8_t* object, const uint32_t maxLength, const TCHAR delimiter)
    {
        ASSERT(object != nullptr || maxLength == 0); 
        uint8_t highNibble;
        uint8_t lowNibble;
        uint32_t bufferIndex = 0, strIndex = 0;

        // assume first character is 0 if length is odd. 
        if ((delimiter == '\0') && (hexString.length() % 2 == 1)) {
            lowNibble = FromHexDigits(hexString[strIndex++]);
            object[bufferIndex++] = lowNibble;
        }

        while ((bufferIndex < maxLength) && (strIndex < hexString.length())) {
            if (delimiter == '\0') {
                highNibble = FromHexDigits(hexString[strIndex++]);
                lowNibble = FromHexDigits(hexString[strIndex++]);
            }
            else {
                uint8_t nibble = FromHexDigits(hexString[strIndex++]);
                if (hexString[strIndex] == delimiter) {
                    highNibble = 0;
                    lowNibble = nibble;
                    ++strIndex;
                }
                else {
                    highNibble = nibble;
                    lowNibble = FromHexDigits(hexString[strIndex++]);
                    if (hexString[strIndex] == delimiter) {
                        ++strIndex;
                    }
                }
            }

            object[bufferIndex++] = (highNibble << 4) + lowNibble; 
        }

        // return buffer length
        return bufferIndex;
    }

    static const TCHAR base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

    void ToString(const uint8_t object[], const uint32_t length, const bool padding, string& result)
    {
        uint8_t state = 0;
        uint32_t index = 0;
        uint8_t lastStuff = 0;

        while (index < length) {
            if (state == 0) {
                result += base64_chars[((object[index] & 0xFC) >> 2)];
                lastStuff = ((object[index] & 0x03) << 4);
                state = 1;
            } else if (state == 1) {
                result += base64_chars[(((object[index] & 0xF0) >> 4) | lastStuff)];
                lastStuff = ((object[index] & 0x0F) << 2);
                state = 2;
            } else if (state == 2) {
                result += base64_chars[(((object[index] & 0xC0) >> 6) | lastStuff)];
                result += base64_chars[(object[index] & 0x3F)];
                state = 0;
            }
            index++;
        }

        if (state != 0) {
            result += base64_chars[lastStuff];

            if (padding == true) {
                if (state == 1) {
                    result += _T("==");
                } else {
                    result += _T("=");
                }
            }
        }
    }

    uint32_t FromString(const string& newValue, uint8_t object[], uint32_t& length, const TCHAR* ignoreList)
    {
        uint8_t state = 0;
        uint32_t index = 0;
        uint32_t filler = 0;
        uint8_t lastStuff = 0;

        while ((index < newValue.size()) && (filler < length)) {
            uint8_t converted;
            TCHAR current = newValue[index++];

            if ((current >= 'A') && (current <= 'Z')) {
                converted = (current - 'A');
            } else if ((current >= 'a') && (current <= 'z')) {
                converted = (current - 'a' + 26);
            } else if ((current >= '0') && (current <= '9')) {
                converted = (current - '0' + 52);
            } else if (current == '+') {
                converted = 62;
            } else if (current == '/') {
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
                object[filler++] = (((converted & 0x30) >> 4) | lastStuff);
                lastStuff = ((converted & 0x0F) << 4);
                state = 2;
            } else if (state == 2) {
                object[filler++] = (((converted & 0x3C) >> 2) | lastStuff);
                lastStuff = ((converted & 0x03) << 6);
                state = 3;
            } else if (state == 3) {
                object[filler++] = ((converted & 0x3F) | lastStuff);
                state = 0;
            }
        }

        length = filler;

        return (index);
    }

    uint16_t FromString(const string& newValue, uint8_t object[], uint16_t& length, const TCHAR* ignoreList)
    {
        uint32_t tempLength = length;
        const uint16_t result = FromString(newValue, object, tempLength, ignoreList);
        length = static_cast<uint16_t>(tempLength);
        return (result);
    }

    bool CodePointToUTF16(const uint32_t codePoint, uint16_t& lowPart, uint16_t& highPart) {

        bool translated = true;

        // Do we need to adapt the CodePoint for UTF16 ?
        if (codePoint <= 0xFFFF) {
            // Nope no need to re-encode, just send as is..
            lowPart = codePoint;
            highPart = 0;
        }
        else {
            // Yes it is bigger than 16 bits..
            uint32_t adjustedCodePoint = codePoint - 0x10000;

            // According to the specification the code point can not exceed 20 bits than..
            if (adjustedCodePoint > 0xFFFFF) {
                lowPart = 0x20; // It becomes a SPACE
                highPart = 0x00;
                translated = false;
            }
            else {
                // Time to re-encode the HigPart and the LowPart..
                lowPart = (adjustedCodePoint & 0x3FF) | 0xDC00;
                highPart = ((adjustedCodePoint >> 10) & 0x3FF) | 0xD800;
            }
        }

        return (translated);
    }
    bool UTF16ToCodePoint(const uint16_t lowPart, const uint16_t highPart, uint32_t& codePoint) {
        bool translated = true;
        if (highPart == 0) {
            codePoint = lowPart;
        }
        else if (((lowPart & 0xFC00) == 0xDC00) && ((highPart & 0xFC00) == 0xD800)) {
            codePoint = ((lowPart & 0x03FF) | ((highPart & 0x03FF) << 10)) + 0x10000;
        }
        else {
            codePoint = 0x20; // It becomes a SPACE
            translated = false;
        }

        return (translated);
    }

    int8_t ToCodePoint(const TCHAR* data, const uint8_t length,  uint32_t& codePoint) {

        bool invalid = false;
        #ifdef _UNICODE
        static_assert(sizeof(TCHAR) != sizeof(char), "UTF16 to code point needs an implementation")
        #else
        uint32_t header = static_cast<uint16_t>(*data & 0xFF);
        uint8_t following = (header < 0xC0 ? 0 :
            header < 0xE0 ? 1 :
            header < 0xF0 ? 2 :
            header < 0xF8 ? 3 :
            header < 0xFC ? 4 : 5);

        // Get the bits of the indicator (ranges from 7 bits to 1)
        if (following == 0) {
            codePoint = header & 0x7F;
        }
        else {
            codePoint = header & ((1 << (7 - following)) - 1);

            // all right shit in the other bits..
            for (uint8_t index = 1; (index <= following) && (index <= length); index++) {
                codePoint = (codePoint << 6) | (data[index] & 0x3F);
                invalid = invalid | ((data[index] & 0xC0) != 0x80);
            }
        }
        #endif

        return (invalid ? -(following + 1) : (following + 1));
    }
    int8_t FromCodePoint(uint32_t codePoint, TCHAR* data, const uint8_t length) {
        #ifdef _UNICODE
        static_assert(sizeof(TCHAR) != sizeof(char), "UTF16 to code point needs an implementation")
        #else
        uint8_t following = (codePoint <= 0x0000007F ? 0 :
            codePoint <= 0x000007FF ? 1 :
            codePoint <= 0x0000FFFF ? 2 :
            codePoint <= 0x001FFFFF ? 3 :
            codePoint <= 0x03FFFFFF ? 4 : 5);
        uint32_t shifter = codePoint;

        // Get the bits of the indicator (ranges from 7 bits to 1)
        if (following == 0) {
            *data = (codePoint & 0x7F);
        }
        else {
            // Just start shiftin out all easy bits..
            for (uint8_t index = following; index > 0; index--) {
                if (index < length) {
                    data[index] = (shifter & 0x3F) | 0x80;
                }
                shifter = shifter >> 6;
            }

            // Now contruct a proper preamble..
            data[0] = (~((1 << (7 - following)) - 1)) | (shifter & 0x3F);
        }
        #endif

        return (following + 1);
    }

    // If we are going to mark the length of a string by quotes, make 
    // sure that the internal quotes are escaped...
    string EXTERNAL ToQuotedString(const TCHAR quote, const string& input) {
        string result;
        result += quote;
        for (auto entry : input) {
            if (entry == quote) {
                result += '\\';
            }
            result += entry;
        }
        result += quote;
        return (result);
    }

}
} // namespace Core

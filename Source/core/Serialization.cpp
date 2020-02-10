 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

namespace WPEFramework {
namespace Core {
#ifdef __WINDOWS__
#pragma warning(disable : 4996)
#endif

#ifndef __NO_WCHAR_SUPPORT__
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

#endif // __NO_WCHAR_SUPPORT__

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

#ifdef __WINDOWS__
#pragma warning(default : 4996)
#endif

    static const TCHAR hex_chars[] = "0123456789abcdef";

    void EXTERNAL ToHexString(const uint8_t object[], const uint16_t length, string& result)
    {
        ASSERT(object != nullptr);

        uint16_t index = static_cast<uint16_t>(result.length());
        result.resize(index + (length * 2));

        result[1] = hex_chars[object[0] & 0xF];

        for (uint16_t i = 0, j = index; i < length; i++) {
            result[j++] = hex_chars[object[i] >> 4];
            result[j++] = hex_chars[object[i] & 0xF];
        }
    }

    uint16_t EXTERNAL FromHexString(const string& hexString, uint8_t* object, const uint16_t maxLength) {
        ASSERT(object != nullptr || maxLength == 0); 
        uint8_t highNibble;
        uint8_t lowNibble;
        uint16_t bufferIndex = 0, strIndex = 0;

        // assume first character is 0 if length is odd. 
        if (hexString.length() % 2 == 1) {
            lowNibble = FromHexDigits(hexString[strIndex++]);
            object[bufferIndex++] = lowNibble;
        }

        while ((bufferIndex < maxLength) && (strIndex < hexString.length())) {
            highNibble = FromHexDigits(hexString[strIndex++]);
            lowNibble = FromHexDigits(hexString[strIndex++]);

            object[bufferIndex++] = (highNibble << 4) + lowNibble; 
        }

        // return buffer length
        return bufferIndex;
    }

    static const TCHAR base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

    void ToString(const uint8_t object[], const uint16_t length, const bool padding, string& result)
    {
        uint8_t state = 0;
        uint16_t index = 0;
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

    uint16_t FromString(const string& newValue, uint8_t object[], uint16_t& length, const TCHAR* ignoreList)
    {
        uint8_t state = 0;
        uint16_t index = 0;
        uint16_t filler = 0;
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

        if ((state != 0) && (filler < length)) {
            object[filler++] = lastStuff;
        
		}
        length = filler;

        return (index);
    }
}
} // namespace Core

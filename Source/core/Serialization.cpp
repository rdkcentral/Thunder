#include "Serialization.h"

namespace WPEFramework {
namespace Core {
#ifdef __WIN32__
#pragma warning(disable : 4996)
#endif

#ifndef __NO_WCHAR_SUPPORT__
    void ToString(const wchar_t realString[], std::string& result)
    {
#if defined(__WIN32__) || defined(__LINUX__)

        int requiredSize = static_cast<int>(::wcstombs(nullptr, realString, 0));
#ifdef __WIN32__
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

#if defined(__WIN32__) || defined(__LINUX__)

#ifdef __WIN32__
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

    const string ToString(const wchar_t realString[], unsigned int length)
    {
#ifdef _UNICODE

        return (string(realString, length));

#else

        int requiredSize = static_cast<int>(::wcstombs(nullptr, realString, length));

#ifdef __WIN32__

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

    const string ToString(const char realString[], unsigned int length)
    {
#ifdef _UNICODE

#if defined(__WIN32__) || defined(__LINUX__)

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

#ifdef __WIN32__
#pragma warning(default : 4996)
#endif

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
            }
            else if (state == 1) {
                result += base64_chars[(((object[index] & 0xF0) >> 4) | lastStuff)];
                lastStuff = ((object[index] & 0x0F) << 2);
                state = 2;
            }
            else if (state == 2) {
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
                }
                else {
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
            }
            else if ((current >= 'a') && (current <= 'z')) {
                converted = (current - 'a' + 26);
            }
            else if ((current >= '0') && (current <= '9')) {
                converted = (current - '0' + 52);
            }
            else if (current == '+') {
                converted = 62;
            }
            else if (current == '/') {
                converted = 63;
            }
            else if ((ignoreList != nullptr) && (::strchr(ignoreList, current) != nullptr)) {
                continue;
            }
            else {
                break;
            }

            if (state == 0) {
                lastStuff = converted << 2;
                state = 1;
            }
            else if (state == 1) {
                object[filler++] = (((converted & 0x30) >> 4) | lastStuff);
                lastStuff = ((converted & 0x0F) << 4);
                state = 2;
            }
            else if (state == 2) {
                object[filler++] = (((converted & 0x3C) >> 2) | lastStuff);
                lastStuff = ((converted & 0x03) << 6);
                state = 3;
            }
            else if (state == 3) {
                object[filler++] = ((converted & 0x3F) | lastStuff);
                state = 0;
            }
        }
        length = filler;

        return (index);
    }
}
} // namespace Core

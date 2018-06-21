#ifndef __SERIALIZATION_H
#define __SERIALIZATION_H

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"
#include "Number.h"

namespace WPEFramework {
namespace Core {
#ifdef _UNICODE
    inline int ToCharacter(const char* character, TCHAR converted[], unsigned int count)
#else
    inline int ToCharacter(const char* character, TCHAR converted[], unsigned int /* count */)
#endif
    {
#ifdef _UNICODE
#ifdef __LINUX__
        return (::mbstowcs(connverted, character, count));
#else
        return (::mbtowc(converted, character, count));
#endif
#else
        converted[0] = *character;
        return (1);
#endif
    }

#ifndef __NO_WCHAR_SUPPORT__
    inline int ToCharacter(const wchar_t* character, TCHAR converted[], unsigned int /* DUMMY JUST TO HAVE THE SAME IF */)
    {
#ifdef _UNICODE
        converted[0] = *character;
        return (1);
#else
#ifdef __LINUX__
        mbstate_t result;
        return (::wcsrtombs(converted, &character, wcwidth(*character), &result));
#else
#pragma warning(disable : 4996)
        return (::wctomb(converted, character[0]));
#pragma warning(default : 4996)
#endif
#endif
    }
    EXTERNAL const string ToString(const wchar_t realstring[], const unsigned int length);
    EXTERNAL void ToString(const wchar_t realstring[], std::string& result);
    EXTERNAL void ToString(const char realstring[], std::wstring& result);

    inline void ToString(const wchar_t realstring[], std::wstring& result)
    {
        result = realstring;
    }

    inline const string ToString(const wchar_t realstring[])
    {
#ifdef _UNICODE
        return (std::wstring(realstring));
#else
        std::string result;
        ToString(realstring, result);
        return (result);
#endif
    }
#endif // __NO_WCHAR_SUPPORT__

    EXTERNAL const string ToString(const char realstring[], const unsigned int length);

    inline void ToString(const char realstring[], std::string& result)
    {
        result = realstring;
    }

    inline string ToString(const char realstring[])
    {
#ifdef _UNICODE
        std::wstring result;
        ToString(realstring, result);
        return (result);
#else
        return (std::string(realstring));
#endif
    }

    inline std::string ToString(const string& realstring)
    {
#ifdef _UNICODE
        std::string result;
        ToString(realstring.c_str(), result);
        return (result);
#else
        return (realstring);
#endif
    }

    // Specific instantiation of several Serialization functions.
    //------------------------------------------------------------------------
    // Serialize: UINT8
    //------------------------------------------------------------------------
    inline string ToString(const uint8_t& object)
    {
        return (Core::NumberType<uint8_t, false>(object).Text());
    }

    inline bool FromString(const string& newValue, uint8_t& object)
    {
        return (NumberType<uint8_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: SINT8
    //------------------------------------------------------------------------
    inline string ToString(const int8_t& object)
    {
        return (Core::NumberType<int8_t, true>(object).Text());
    }

    inline bool FromString(const string& newValue, int8_t& object)
    {
        return (NumberType<int8_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: UINT16
    //------------------------------------------------------------------------
    inline string ToString(const uint16_t& object)
    {
        return (Core::NumberType<uint16_t, false>(object).Text());
    }

    inline bool FromString(const string& newValue, uint16_t& object)
    {
        return (NumberType<uint16_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: SINT16
    //------------------------------------------------------------------------
    inline string ToString(const int16_t& object)
    {
        return (Core::NumberType<int16_t, true>(object).Text());
    }

    inline bool FromString(const string& newValue, int16_t& object)
    {
        return (NumberType<int16_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: UINT32
    //------------------------------------------------------------------------
    inline string ToString(const uint32_t& object)
    {
        return (Core::NumberType<uint32_t, false>(object).Text());
    }

    inline bool FromString(const string& newValue, uint32_t& object)
    {
        return (NumberType<uint32_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: SINT32
    //------------------------------------------------------------------------
    inline string ToString(const int32_t& object)
    {
        return (Core::NumberType<int32_t, true>(object).Text());
    }

    inline bool FromString(const string& newValue, int32_t& object)
    {
        return (NumberType<int32_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: UINT64
    //------------------------------------------------------------------------
    inline string ToString(const uint64_t& object)
    {
        return (Core::NumberType<uint64_t, false>(object).Text());
    }

    inline bool FromString(const string& newValue, uint64_t& object)
    {
        return (NumberType<uint64_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

    //------------------------------------------------------------------------
    // Serialize: SINT64
    //------------------------------------------------------------------------
    inline string ToString(const int64_t& object)
    {
        return (Core::NumberType<int64_t, true>(object).Text());
    }

    inline bool FromString(const string& newValue, int64_t& object)
    {
        return (NumberType<int64_t>::Convert(newValue.c_str(), static_cast<uint32_t>(newValue.length()), object, BASE_UNKNOWN) == newValue.length());
    }

	//------------------------------------------------------------------------
	// Serialize: boolean
	//------------------------------------------------------------------------
	inline string ToString(const bool object, const bool uppercase = false)
	{
		return (uppercase ? (object ? _T("TRUE") : _T("FALSE")) : (object ? _T("true") : _T("false")));
	}

	inline bool FromString(const string& newValue, bool& object)
	{
		if (newValue.length() == 1) {
			TCHAR value = ::toupper(newValue[0]);
			if ((value == '0') || (value == 'F')) {
				object = false;
				return (true);
			}
			else if ((value == '1') || (value == 'T')) {
				object = true;
				return (true);
			}
		}
		else if (newValue.length() == 4) {
			
			if (_tcsnicmp(_T("TRUE"), newValue.c_str(), 4) == 0) {
				object = true;
				return (true);
			}
		}
		else if (newValue.length() == 5) {
			if (_tcsnicmp(_T("FALSE"), newValue.c_str(), 5) == 0) {
				object = false;
				return (true);
			}
		}

		return (false);
	}

	//------------------------------------------------------------------------
    // Serialize: Base64
    //------------------------------------------------------------------------
    void EXTERNAL ToString(const uint8_t object[], const uint16_t length, const bool padding, string& result);

    uint16_t EXTERNAL FromString(const string& newValue, uint8_t object[], uint16_t& length, const TCHAR* ignoreList = nullptr);

    namespace Serialize {
        template <typename TEXTTERMINATOR, typename HANDLER>
        class ParserType {
        private:
            ParserType();
            ParserType(const ParserType<TEXTTERMINATOR, HANDLER>&);
            ParserType<TEXTTERMINATOR, HANDLER>& operator=(const ParserType<TEXTTERMINATOR, HANDLER>&);

        public:
            enum ParseState {
                LOADED = 0x01,
                QUOTED = 0x02,
                CLOSE_WHITESPACE = 0x04,
                CLOSE_LINE = 0x08,
                QUOTE_ACTIVE = 0x10
            };

            ParserType(HANDLER& parent)
                : _state(0)
                , _buffer()
                , _parent(parent)
                , _terminator()
            {
            }
            ~ParserType()
            {
            }

        public:
            void SubmitWord(const TextFragment& dataSet)
            {
                ASSERT((_state & LOADED) != LOADED);
                _locator = 0;
                _state |= LOADED | CLOSE_WHITESPACE;
                _state &= (~CLOSE_LINE);
                _buffer = dataSet;
            }
            void SubmitLine(const TextFragment& dataSet)
            {
                ASSERT((_state & LOADED) != LOADED);
                _locator = 0;
                _state |= LOADED | CLOSE_LINE;
                _state &= (~CLOSE_WHITESPACE);
                _buffer = dataSet;
            }
            void SubmitPart(const TextFragment& dataSet)
            {
                ASSERT((_state & LOADED) != LOADED);
                _locator = 0;
                _state |= LOADED;
                _state &= (~CLOSE_WHITESPACE | CLOSE_LINE);
                _buffer = dataSet;
            }
            inline void Quote()
            {
                _state |= QUOTED;
            }
            uint16_t Serialize(const uint8_t[] /*stream*/, const uint16_t maxLength)
            {
                uint16_t serializedBytes = 0;

                while (((_state & LOADED) == LOADED) && (serializedBytes < maxLength)) {
                    _parent.Serialized();
                }

                return (serializedBytes);
            }

        private:
            uint8_t _state;
            uint32_t _locator;
            TextFragment _buffer;
            HANDLER& _parent;
            TEXTTERMINATOR _terminator;
        };
    }
}
} // namespace Core::Serialization

#endif

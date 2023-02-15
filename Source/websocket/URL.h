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

#pragma once

#include "Module.h"

namespace WPEFramework {

namespace Core {

    class EXTERNAL URL {
    private:
        static constexpr uint32_t MaximumURLLength = 2048;

    public:
        typedef enum {
            SCHEME_FILE,
            SCHEME_HTTP,
            SCHEME_HTTPS,
            SCHEME_FTP,
            SCHEME_NTP,
            SCHEME_WS,
            SCHEME_WSS,
            SCHEME_UNKNOWN

        } SchemeType;

        class KeyValue {
        public:
            enum class status : uint8_t {
                UNAVAILABLE = 0,
                KEY_ONLY    = 1,
                KEY_VALUE   = 3
            };

        public:
            KeyValue() = delete;

            KeyValue(const string& data)
                : _data(data) {
            }
            KeyValue(const Core::TextFragment& data)
                : _data(data) {
            }
            KeyValue(KeyValue&& move)
                : _data(move._data) {
            }
            KeyValue(const KeyValue& copy)
                : _data(copy._data) {
            }
            ~KeyValue() = default;

            KeyValue& operator= (const KeyValue& RHS) {
                _data = RHS._data;
                return (*this);
            }

        public:
            // Please use the HasKey method...
            DEPRECATED inline bool Exists(const TCHAR key[], const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                return (LoadKey(key, result, caseSensitive) != status::UNAVAILABLE);
            }
            inline status HasKey(const TCHAR key[], const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                return (LoadKey(key, result, caseSensitive));
            }
            // By definition is this method case sensitive..
            inline const Core::TextFragment operator[](const TCHAR key[]) const
            {
                Core::TextFragment result;
                LoadKey(key, result, true);
                return (result);
            }
            inline const Core::TextFragment Value(const TCHAR key[], const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                LoadKey(key, result, caseSensitive);
                return (result);
            }
            template <typename NUMBERTYPE>
            NUMBERTYPE Number(const TCHAR key[], NUMBERTYPE defaultValue = Core::NumberType<NUMBERTYPE>::Max(), const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                if (LoadKey(key, result, caseSensitive) == status::KEY_VALUE) {
                    return (Core::NumberType<NUMBERTYPE>(result).Value());
                }
                return (defaultValue);
            }
            template <typename ENUMERATE>
            ENUMERATE Enumerate(const TCHAR key[], ENUMERATE defaultValue, const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                if (LoadKey(key, result, caseSensitive) == status::KEY_VALUE) {
                    return (Core::EnumerateType<ENUMERATE>(result).Value());
                }
                return (defaultValue);
            }
            inline bool Boolean(const TCHAR key[], const bool defaultValue, const bool caseSensitive = true) const
            {
                Core::TextFragment result;
                if (LoadKey(key, result, caseSensitive) == status::KEY_VALUE) {
                    if (result.Length() == 1) {
                        TCHAR value = ::toupper(*(result.Data()));
                        if ((value == '0') || (value == 'F')) {
                            return (false);
                        } else if ((value == '1') || (value == 'T')) {
                            return (true);
                        }
                    } else if (result.Length() == 4) {
                        if (result.EqualText(_T("TRUE"), 0, 4, false) == true) {
                            return (true);
                        }
                    } else if (result.Length() == 5) {
                        if (result.EqualText(_T("FALSE"), 0, 5, false) == true) {
                            return (false);
                        }
                    }
                }
                return (defaultValue);
            }

        private:
            status LoadKey(const TCHAR key[], Core::TextFragment& data, const bool caseSensitive) const
            {
                status current = status::UNAVAILABLE;
                const Core::TextFragment comparator(key, static_cast<uint32_t>(strlen(key)));
                Core::TextSegmentIterator options(_data, false, '&');

                while ((options.Next() == true) && (current == status::UNAVAILABLE)) {
                    Core::TextSegmentIterator keyvalue(options.Current(), false, '=');

                    if (keyvalue.Next() == true) {
                        if (keyvalue.Current().EqualText(comparator, caseSensitive) == true) {
                            if (keyvalue.Next() == true) {
                                current = status::KEY_VALUE;
                                data = keyvalue.Current();
                            } else {
                                current = status::KEY_ONLY;
                                data.Clear();
                            }
                        }
                    }
                }
                return (current);
            }

        private:
            Core::TextFragment _data;
        };

    public:
        URL()
            : _scheme(SCHEME_UNKNOWN)
            , _username()
            , _password()
            , _host()
            , _port()
            , _path()
            , _query()
            , _ref()
        {
        }
        explicit URL(const SchemeType type)
            : _scheme(type)
            , _username()
            , _password()
            , _host()
            , _port()
            , _path()
            , _query()
            , _ref()
        {
        }
        explicit URL(const TCHAR urlStr[])
            : _scheme(SCHEME_UNKNOWN)
            , _username()
            , _password()
            , _host()
            , _port()
            , _path()
            , _query()
            , _ref()
        {
            Parse(Core::TextFragment(urlStr));
        }
        explicit URL(const string& urlStr)
            : _scheme(SCHEME_UNKNOWN)
            , _username()
            , _password()
            , _host()
            , _port()
            , _path()
            , _query()
            , _ref()
        {
            Parse(Core::TextFragment(urlStr));
        }
        explicit URL(const Core::TextFragment& text)
            : _scheme(SCHEME_UNKNOWN)
            , _username()
            , _password()
            , _host()
            , _port()
            , _path()
            , _query()
            , _ref()
        {
            Parse(text);
        }
        URL(const URL& copy)
            : _scheme(copy._scheme)
            , _username(copy._username)
            , _password(copy._password)
            , _host(copy._host)
            , _port(copy._port)
            , _path(copy._path)
            , _query(copy._query)
            , _ref(copy._ref)
        {
        }
        ~URL()
        {
        }

    public:
        static uint16_t Port(const SchemeType type) {

            switch (type) {
            case SCHEME_HTTP:    return(80);    break;
            case SCHEME_HTTPS:   return(443);   break;
            case SCHEME_FTP:     return(21);    break;
            case SCHEME_NTP:     return(123);   break;
            case SCHEME_WS:      return(80);    break;
            case SCHEME_WSS:     return(443);   break;
            case SCHEME_UNKNOWN: return(0);     break;
            case SCHEME_FILE:    return(0);     break;
            default:             ASSERT(false); break;
            }

            return(0);
        }
        inline URL& operator=(const URL& copy)
        {
            _scheme = copy._scheme;
            _username = copy._username;
            _password = copy._password;
            _host = copy._host;
            _port = copy._port;
            _path = copy._path;
            _query = copy._query;
            _ref = copy._ref;

            return (*this);
        }

        inline bool IsValid() const
        {
            return (_scheme != SCHEME_UNKNOWN);
        }

        SchemeType Type() const 
        {
            return (_scheme);
        }

        inline const Core::OptionalType<string>& UserName() const
        {
            return (_username);
        }

        inline const Core::OptionalType<string>& Password() const
        {
            return (_password);
        }

        inline const Core::OptionalType<string>& Host() const
        {
            return (_host);
        }

        inline const Core::OptionalType<unsigned short>& Port() const
        {
            return (_port);
        }

        inline const Core::OptionalType<string>& Path() const
        {
            return (_path);
        }

        inline const Core::OptionalType<string>& Query() const
        {
            return (_query);
        }

        inline const Core::OptionalType<string>& Ref() const
        {
            return (_ref);
        }

        inline void UserName(const Core::OptionalType<string>& value)
        {
            _username = value;
        }

        inline void Password(const Core::OptionalType<string>& value)
        {
            _password = value;
        }

        inline void Host(const Core::OptionalType<string>& value)
        {
            _host = value;
        }

        inline void Port(const Core::OptionalType<unsigned short> port)
        {
            _port = port;
        }

        inline void Path(const Core::OptionalType<string>& value)
        {
            _path = value;
        }

        inline void Query(const Core::OptionalType<string>& value)
        {
            _query = value;
        }

        inline void Ref(const Core::OptionalType<string>& value)
        {
            _ref = value;
        }

        string Text() const;

        bool IsDomain(const TCHAR domain[], const unsigned int length) const
        {
            bool result = false;

            if ((_host.IsSet() == true) && (_host.Value().length() >= length)) {
                uint32_t offset = static_cast<uint32_t>(_host.Value().length()) - length;

                if ((offset == 0) || (_host.Value()[(offset - 1)] == '.')) {
                    uint32_t index = 0;
                    while ((index < length) && (tolower(domain[index]) == _host.Value()[index + offset])) {
                        index++;
                    }
                    result = (index == length);
                }
            }
            return (result);
        }
        static uint16_t Encode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength);
        static uint16_t Decode(const TCHAR* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength);
        static uint16_t Base64Encode(const uint8_t* source, const uint16_t sourceLength, TCHAR* destination, const uint16_t destinationLength, const bool padding = false);
        static uint16_t Base64Decode(const TCHAR* source, const uint16_t sourceLength, uint8_t* destination, const uint16_t destinationLength, const TCHAR* ignoreList = nullptr);

    private:
        void Parse(const Core::TextFragment& url);
        void ParseDomain(const Core::TextFragment& url);

    private:
        SchemeType _scheme;
        Core::OptionalType<string> _username;
        Core::OptionalType<string> _password;
        Core::OptionalType<string> _host;
        Core::OptionalType<unsigned short> _port;
        Core::OptionalType<string> _path;
        Core::OptionalType<string> _query;
        Core::OptionalType<string> _ref;
    };

}
} // namespace Core

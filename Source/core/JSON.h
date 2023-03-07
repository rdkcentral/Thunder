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

#ifndef __JSON_H
#define __JSON_H

#include <map>
#include <vector>

#include "Enumerate.h"
#include "FileSystem.h"
#include "Number.h"
#include "Portability.h"
#include "Proxy.h"
#include "Serialization.h"
#include "TextFragment.h"
#include "TypeTraits.h"

namespace WPEFramework {

namespace Core {

    namespace JSON {

        struct EXTERNAL Error {
            explicit Error(string&& message)
                : _message(std::move(message))
                , _context()
                , _pos(0)
            {
            }

            Error(const Error&) = default;
            Error(Error&&) = default;
            Error& operator=(const Error&) = default;
            Error& operator=(Error&&) = default;

            string Message() const { return _message; }
            string Context() const { return _context; }
            size_t Position() const { return _pos; }

            // Unfortunately top most element has broader context than the one rising an error this is why this
            // is splited and not made mandatory upon creation.
            void Context(const char json[], size_t jsonLength, size_t pos)
            {
                size_t contextLength = std::min(kContextMaxLength, std::min(jsonLength, pos));
                std::string context{ &json[pos - contextLength], &json[pos] };
                _context.swap(context);
                _pos = pos;
            }

        private:
            friend class OptionalType<Error>;
            Error()
                : _message()
                , _context()
                , _pos(0)
            {
            }

            static constexpr size_t kContextMaxLength = 80;

            string _message;
            string _context;
            size_t _pos;
        };

        string EXTERNAL ErrorDisplayMessage(const Error& err);

        struct EXTERNAL IElement {

            static TCHAR NullTag[5];
            static TCHAR TrueTag[5];
            static TCHAR FalseTag[6];

            virtual ~IElement() = default;

            template <typename INSTANCEOBJECT>
            static bool ToString(const INSTANCEOBJECT& realObject, string& text)
            {
                char buffer[1024];
                uint16_t loaded;
                uint32_t offset = 0;

                text.clear();

                // Serialize object
                do {
                    loaded = static_cast<const IElement&>(realObject).Serialize(buffer, sizeof(buffer), offset);

                    ASSERT(loaded <= sizeof(buffer));
                    DEBUG_VARIABLE(loaded);

                    text += string(buffer, loaded);

                } while ((offset != 0) && (loaded == sizeof(buffer)));

                return (offset == 0);
            }

            template <typename INSTANCEOBJECT>
            static bool FromString(const string& text, INSTANCEOBJECT& realObject)
            {
                Core::OptionalType<Error> error;
                return FromString(text, realObject, error);
            }

            template <typename INSTANCEOBJECT>
            static bool FromString(const string& text, INSTANCEOBJECT& realObject, Core::OptionalType<Error>& error)
            {
                uint32_t offset  = 0;
                uint32_t handled = 0;
                uint32_t size    = static_cast<uint32_t>(text.length());

                realObject.Clear();

                while (handled < size) {

                    uint16_t payload = static_cast<uint16_t>(std::min((size - handled) + 1, static_cast<uint32_t>(0xFFFF)));

                    // Deserialize object
                    uint16_t loaded = static_cast<IElement&>(realObject).Deserialize(&(text.c_str()[handled]), payload, offset, error);

                    ASSERT(loaded <= payload);
                    DEBUG_VARIABLE(loaded);

                    if (loaded == 0) {
                        break;
                    }
                    handled += loaded;
                }

                if (((offset != 0) || (handled < size)) && (error.IsSet() == false)) {
                    error = Error{ "Malformed JSON. Missing closing quotes or brackets" };
                    realObject.Clear();
                }

                if (error.IsSet() == true) {
                    TRACE_L1("Parsing failed: %s", ErrorDisplayMessage(error.Value()).c_str());
                    realObject.Clear();
                }

                return (error.IsSet() == false);
            }

            inline bool ToString(string& text) const
            {
                return (Core::JSON::IElement::ToString(*this, text));
            }

            inline bool FromString(const string& text)
            {
                Core::OptionalType<Error> error;
                return FromString(text, error);
            }

            inline bool FromString(const string& text, Core::OptionalType<Error>& error)
            {
                return (Core::JSON::IElement::FromString(text, *this, error));
            }

            template <typename INSTANCEOBJECT>
            static bool ToFile(Core::File& fileObject, const INSTANCEOBJECT& realObject)
            {
                bool completed = false;

                if (fileObject.IsOpen()) {

                    char buffer[1024];
                    uint16_t loaded;
                    uint32_t offset = 0;

                    // Serialize object
                    do {
                        loaded = static_cast<const IElement&>(realObject).Serialize(buffer, sizeof(buffer), offset);

                        ASSERT(loaded <= sizeof(buffer));

                    } while ((fileObject.Write(reinterpret_cast<const uint8_t*>(buffer), loaded) == loaded) && (loaded == sizeof(buffer)) && (offset != 0));

                    completed = (offset == 0);
                }

                return (completed);
            }

            template <typename INSTANCEOBJECT>
            static bool FromFile(Core::File& fileObject, INSTANCEOBJECT& realObject)
            {
                Core::OptionalType<Error> error;
                return FromFile(fileObject, realObject, error);
            }

            template <typename INSTANCEOBJECT>
            static bool FromFile(Core::File& fileObject, INSTANCEOBJECT& realObject, Core::OptionalType<Error>& error)
            {
                if (fileObject.IsOpen()) {

                    char buffer[1024];
                    uint16_t readBytes;
                    uint16_t loaded;
                    uint32_t offset = 0;

                    realObject.Clear();

                    // Serialize object
                    do {
                        readBytes = static_cast<uint16_t>(fileObject.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer)));

                        if (readBytes == 0) {
                            loaded = ~0;
                        } else {
                            loaded = static_cast<IElement&>(realObject).Deserialize(buffer, sizeof(buffer), offset, error);

                            ASSERT(loaded <= readBytes);

                            if (loaded != readBytes) {
                                fileObject.Position(true, -(readBytes - loaded));
                            }
                        }

                    } while ((loaded == readBytes) && (offset != 0));

                    if (offset != 0 && error.IsSet() == false) {
                        error = Error{ "Malformed JSON. Missing closing quotes or brackets" };
                        realObject.Clear();
                    }
                }

                if (error.IsSet() == true) {
                    TRACE_L1("Parsing failed with %s", ErrorDisplayMessage(error.Value()).c_str());
                }

                return (error.IsSet() == false);
            }

            bool ToFile(Core::File& fileObject) const
            {
                return (Core::JSON::IElement::ToFile(fileObject, *this));
            }

            bool FromFile(Core::File& fileObject)
            {
                Core::OptionalType<Error> error;
                return FromFile(fileObject, error);
            }

            bool FromFile(Core::File& fileObject, Core::OptionalType<Error>& error)
            {
                return (Core::JSON::IElement::FromFile(fileObject, *this, error));
            }

            // JSON Serialization interface
            // --------------------------------------------------------------------------------
            virtual void Clear() = 0;
            virtual bool IsSet() const = 0;
            virtual bool IsNull() const = 0;
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const = 0;
            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset)
            {
                Core::OptionalType<Error> error;
                uint16_t loaded = Deserialize(stream, maxLength, offset, error);

                if (error.IsSet() == true) {
                    Clear();
                    error.Value().Context(stream, maxLength, loaded);
                    TRACE_L1("Parsing failed: %s", ErrorDisplayMessage(error.Value()).c_str());
                }

                return loaded;
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) = 0;
        };

        struct EXTERNAL IMessagePack {
            virtual ~IMessagePack() = default;

            static constexpr uint8_t NullValue = 0xC0;

            template <typename INSTANCEOBJECT>
            static bool ToBuffer(std::vector<uint8_t>& stream, const INSTANCEOBJECT& realObject)
            {
                uint8_t buffer[1024];
                uint16_t loaded;
                uint32_t offset = 0;

                stream.clear();
                // Serialize object
                do {
                    loaded = static_cast<const IMessagePack&>(realObject).Serialize(buffer, sizeof(buffer), offset);

                    ASSERT(loaded <= sizeof(buffer));
                    DEBUG_VARIABLE(loaded);

                    stream.reserve(stream.size() + loaded);
                    stream.insert(stream.end(), buffer, buffer + loaded);
                } while ((offset != 0) && (loaded == sizeof(buffer)));

                return (offset == 0);
            }
            template <typename INSTANCEOBJECT>
            static bool FromBuffer(const std::vector<uint8_t>& stream, INSTANCEOBJECT& realObject)
            {
                uint32_t offset = 0;
                uint32_t handled = 0;
                uint32_t size = static_cast<uint32_t>(stream.size());

                realObject.Clear();

                while (size != handled) {
                        uint16_t partial = static_cast<uint16_t>(std::min(size - handled, static_cast<uint32_t>(0xFFFF)));

                        // Deserialize object
                        uint16_t loaded = static_cast<IMessagePack&>(realObject).Deserialize(&(stream[handled]), partial, offset);

                        ASSERT(loaded <= partial);
                        DEBUG_VARIABLE(loaded);
                        handled += loaded;
                }

                if (offset) {
                    realObject.Clear();
                }

                return (offset == 0);
            }

            template <typename INSTANCEOBJECT>
            static bool ToFile(Core::File& fileObject, const INSTANCEOBJECT& realObject)
            {
                bool completed = false;

                if (fileObject.IsOpen()) {

                    uint8_t buffer[1024];
                    uint16_t loaded;
                    uint32_t offset = 0;

                    // Serialize object
                    do {
                        loaded = static_cast<const IMessagePack&>(realObject).Serialize(buffer, sizeof(buffer), offset);

                        ASSERT(loaded <= sizeof(buffer));

                    } while ((fileObject.Write(reinterpret_cast<const uint8_t*>(buffer), loaded) == loaded) && (loaded == sizeof(buffer)) && (offset != 0));

                    completed = (offset == 0);
                }

                return (completed);
            }

            template <typename INSTANCEOBJECT>
            static bool FromFile(Core::File& fileObject, INSTANCEOBJECT& realObject)
            {
                bool completed = false;

                Core::OptionalType<Error> error;
                if (fileObject.IsOpen()) {

                    uint8_t buffer[1024];
                    uint16_t readBytes;
                    uint16_t loaded;
                    uint32_t offset = 0;

                    realObject.Clear();

                    // Serialize object
                    do {
                        readBytes = static_cast<uint16_t>(fileObject.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer)));

                        if (readBytes == 0) {
                            loaded = ~0;
                        } else {
                            loaded = static_cast<IMessagePack&>(realObject).Deserialize(buffer, sizeof(buffer), offset);

                            ASSERT(loaded <= readBytes);

                            if (loaded != readBytes) {
                                fileObject.Position(true, -(readBytes - loaded));
                            }
                        }

                    } while ((loaded == readBytes) && (offset != 0));

                    if (offset != 0) {
                        realObject.Clear();
                    }
                    completed = (offset == 0);
                }

                return completed;
            }

            bool ToBuffer(std::vector<uint8_t>& stream) const
            {
                return (Core::JSON::IMessagePack::ToBuffer(stream, *this));
            }

            bool FromBuffer(const std::vector<uint8_t>& stream)
            {
                return (Core::JSON::IMessagePack::FromBuffer(stream, *this));
            }

            bool ToFile(Core::File& fileObject) const
            {
                return (Core::JSON::IMessagePack::ToFile(fileObject, *this));
            }

            bool FromFile(Core::File& fileObject)
            {
                return (Core::JSON::IMessagePack::FromFile(fileObject, *this));
            }

            // JSON Serialization interface
            // --------------------------------------------------------------------------------
            virtual void Clear() = 0;
            virtual bool IsSet() const = 0;
            virtual bool IsNull() const = 0;
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const = 0;
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) = 0;
        };

        enum class ValueValidity : int8_t {
            IS_NULL,
            UNKNOWN,
            INVALID,
            VALID
        };

        static ValueValidity IsNullValue(const char stream[], const uint16_t maxLength, uint32_t& offset, uint16_t& loaded)
        {
            ValueValidity validity = ValueValidity::INVALID;
            const size_t nullTagLen = strlen(IElement::NullTag);
            ASSERT(offset < nullTagLen);
            while (offset < nullTagLen) {
                if (loaded + 1 == maxLength) {
                    validity = ValueValidity::UNKNOWN;
                    break;
                }
                if (stream[loaded++] != IElement::NullTag[offset++]) {
                    offset = 0;
                    break;
                }
            }

            if (offset == nullTagLen)
                validity = ValueValidity::IS_NULL;

            return validity;
        }

        template <class TYPE, bool SIGNED, const NumberBase BASETYPE>
        class NumberType : public IElement, public IMessagePack {
        private:
            enum modes {
                OCTAL = 0x008,
                DECIMAL = 0x00A,
                HEXADECIMAL = 0x010,
                QUOTED = 0x020,
                SET = 0x040,
                ERROR = 0x080,
                NEGATIVE = 0x100,
                UNDEFINED = 0x200
            };

        public:
            NumberType()
                : _set(0)
                , _value(0)
                , _default(0)
            {
            }
            NumberType(const TYPE Value, const bool set = false)
                : _set(set ? SET : 0)
                , _value(Value)
                , _default(Value)
            {
            }
            NumberType(NumberType<TYPE, SIGNED, BASETYPE>&& move)
                : _set(std::move(move._set))
                , _value(std::move(move._value))
                , _default(std::move(move._default))
            {
            }
            NumberType(const NumberType<TYPE, SIGNED, BASETYPE>& copy)
                : _set(copy._set)
                , _value(copy._value)
                , _default(copy._default)
            {
            }
            ~NumberType() override = default;

            NumberType<TYPE, SIGNED, BASETYPE>& operator=(NumberType<TYPE, SIGNED, BASETYPE>&& move)
            {
                _value = std::move(move._value);
                _set = std::move(move._set);

                return (*this);
            }

            NumberType<TYPE, SIGNED, BASETYPE>& operator=(const NumberType<TYPE, SIGNED, BASETYPE>& RHS)
            {
                _value = RHS._value;
                _set = RHS._set;

                return (*this);
            }

            NumberType<TYPE, SIGNED, BASETYPE>& operator=(const TYPE& RHS)
            {
                _value = RHS;
                _set = SET;

                return (*this);
            }

            inline TYPE Default() const
            {
                return _default;
            }

            inline TYPE Value() const
            {
                return ((_set & SET) != 0 ? _value : _default);
            }

            inline operator TYPE() const
            {
                return Value();
            }

            void Null(const bool enabled)
            {
                if (enabled == true)
                    _set |= (SET | UNDEFINED);
                else
                    _set &= ~(SET | UNDEFINED);
            }

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return ((_set & SET) != 0);
            }

            bool IsNull() const override
            {
                return ((_set & UNDEFINED) != 0);
            }

            void Clear() override
            {
                _set = 0;
                _value = 0;
            }

            // IElement iface:
            // If this should be serialized/deserialized, it is indicated by a MinSize > 0)
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                ASSERT(maxLength > 0);

                while ((offset < 4) && (loaded < maxLength)) {

                    if ((_set & UNDEFINED) != 0) {
                        stream[loaded++] = IElement::NullTag[offset++];
                        if (offset == 4) {
                            offset = 0;
                            break;
                        }
                    } else if (BASETYPE == BASE_DECIMAL) {
                        if ((SIGNED == true) && (_value < 0)) {
                            stream[loaded++] = '-';
                        }
                        offset = 4;
                    } else if (BASETYPE == BASE_OCTAL) {
                        if (offset == 0) {
                            stream[loaded++] = '\"';
                            offset = 1;
                        } else if (offset == 1) {
                            if ((SIGNED == true) && (_value < 0)) {
                                stream[loaded++] = '-';
                            }
                            offset = 2;
                        } else if (offset == 2) {
                            stream[loaded++] = '0';
                            offset = 4;
                        }
                    } else if (BASETYPE == BASE_HEXADECIMAL) {
                        if (offset == 0) {
                            stream[loaded++] = '\"';
                            offset = 1;
                        } else if (offset == 1) {
                            if ((SIGNED == true) && (_value < 0)) {
                                stream[loaded++] = '-';
                            }
                            offset = 2;
                        } else if (offset == 2) {
                            stream[loaded++] = '0';
                            offset = 3;
                        } else if (offset == 3) {
                            stream[loaded++] = 'x';
                            offset = 4;
                        }
                    }
                }

                if (((_set & UNDEFINED) == 0) && (loaded < maxLength)) {
                    loaded += Convert(&(stream[loaded]), (maxLength - loaded), offset, TemplateIntToType<SIGNED>());
                }

                if ((offset != 0) && (loaded < maxLength)) {
                    stream[loaded++] = '\"';
                    offset = 0;
                }

                return (loaded);
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;

                // Peamble investigation, determine the right flags..
                while ((offset < 4) && (loaded < maxLength)) {
                    if (offset == 0) {
                        _value = 0;
                        _set = 0;

                        if (stream[loaded] == '\"') {
                            _set = QUOTED;
                            offset++;
                        } else if (stream[loaded] == '-') {
                            _set = NEGATIVE | DECIMAL;
                            offset = 4;
                        } else if (isdigit(stream[loaded])) {
                            _set = DECIMAL;
                            _value = (stream[loaded] - '0');
                            offset = 4;
                        } else if (stream[loaded] == 'n') {
                            _set = UNDEFINED;
                            offset = 1;
                        } else {
                            error = Error{ "Unsupported character \"" + std::string(1, stream[loaded]) + "\" in a number" };
                            ++loaded;
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 1) {
                        ASSERT(_set == QUOTED || _set == UNDEFINED);
                        if (stream[loaded] == '0') {
                            offset = 2;
                        } else if (stream[loaded] == '-') {
                            _set |= NEGATIVE;
                            offset = 2;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set |= DECIMAL;
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'u')) {
                            offset = 2;
                        } else if ((stream[loaded] == '\"') && ((_set & QUOTED) != 0)) {
                            offset = 4;
                            --loaded;
                        } else {
                            error = Error{ "Unsupported character \"" + std::string(1, stream[loaded]) + "\" in a number" };
                            ++loaded;
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 2) {
                        if (stream[loaded] == '0') {
                            offset = 3;
                        } else if (::toupper(stream[loaded]) == 'X') {
                            offset = 4;
                            _set |= HEXADECIMAL;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set |= (_set & NEGATIVE ? DECIMAL : OCTAL);
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'l')) {
                            offset = 3;
                        } else if (stream[loaded] == '\"' && ((_set & QUOTED) != 0)) {
                            offset = 4;
                            --loaded;
                        } else {
                            error = Error{ "Unsupported character \"" + std::string(1, stream[loaded]) + "\" in a number" };
                            ++loaded;
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 3) {
                        if (::toupper(stream[loaded]) == 'X') {
                            offset = 4;
                            _set |= HEXADECIMAL;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set |= OCTAL;
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'l')) {
                            offset = 4;
                        } else if (stream[loaded] == '\"' && ((_set & QUOTED) != 0)) {
                            offset = 4;
                            --loaded;
                        } else {
                            error = Error{ "Unsupported character \"" + std::string(1, stream[loaded]) + "\" in a number" };
                            ++loaded;
                            _set = ERROR;
                            offset = 4;
                        }
                    }
                    loaded++;
                }

                bool completed = ((_set & (ERROR|UNDEFINED)) != 0);

                while ((loaded < maxLength) && (completed == false)) {
                    if (isdigit(stream[loaded])) {
                        _value *= (_set & 0x1F);
                        _value += (stream[loaded] - '0');
                        loaded++;
                    } else if (isxdigit(stream[loaded])) {
                        _value *= 16;
                        _value += (::toupper(stream[loaded]) - 'A') + 10;
                        loaded++;
                    } else if (((_set & QUOTED) != 0) && (stream[loaded] == '\"')) {
                        completed = true;
                        loaded++;
                    } else if (((_set & QUOTED) == 0) && (::isspace(stream[loaded]) || (stream[loaded] == '\0') || (stream[loaded] == ',') || (stream[loaded] == '}') || (stream[loaded] == ']'))) {
                        completed = true;
                    } else {
                        // Oopsie daisy, error, computer says *NO*
                        error = Error{ "Unsupported character \"" + std::string(1, stream[loaded]) + "\" in a number" };
                        ++loaded;
                        _set |= ERROR;
                        completed = true;
                    }
                }

                if ((_set & (ERROR | QUOTED)) == (ERROR | QUOTED)) {
                    while ((loaded < maxLength) && (offset != 0)) {
                        if (stream[loaded++] == '\"') {
                            offset = 0;
                        }
                    }
                } else if ( (completed == true) && (offset >= 4) ) {
                    if (_set & NEGATIVE) {
                        _value *= -1;
                    }
                    _set |= SET;
                    offset = 0;
                }

                return (loaded);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                if ((_set & UNDEFINED) != 0) {
                    stream[0] = IMessagePack::NullValue;
                    return (1);
                }
                return (Convert(stream, maxLength, offset, TemplateIntToType<SIGNED>()));
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint8_t loaded = 0;
                if (offset == 0) {
                    // First byte depicts a lot. Find out what we need to read
                    _value = 0;
                    uint8_t header = stream[loaded++];

                    if (header == IMessagePack::NullValue) {
                        _set = UNDEFINED;
                    } else if ((header >= 0xCC) && (header <= 0xCF)) {
                        _set = (1 << (header - 0xCC)) << 12;
                        offset = 1;
                    } else if ((header >= 0xD0) && (header <= 0xD3)) {
                        _set = (1 << (header - 0xD0)) << 12;
                        offset = 1;
                    } else if ((header & 0x80) == 0) {
                        _value = (header & 0x7F);
                    } else if ((header & 0xE0) == 0xE0) {
                        _value = (header & 0x0F);
                        _set = NEGATIVE;
                    } else {
                        _set = ERROR;
                    }
                }

                while ((loaded < maxLength) && (offset != 0)) {
                    _value = _value << 8;
                    _value += stream[loaded++];
                    offset = (offset == ((_set >> 12) & 0xF) ? 0 : offset + 1);
                }

                if (_value != 0) {
                    _set |= SET;
                }
                return (loaded);
            }

        private:
            uint16_t Convert(char stream[], const uint16_t maxLength, uint32_t& offset, const TYPE serialize) const
            {
                uint8_t parsed = 4;
                uint16_t loaded = 0;
                TYPE divider = 1;
                TYPE value = (serialize / BASETYPE);

                while (divider <= value) {
                    divider *= BASETYPE;
                }

                value = serialize;

                while ((divider > 0) && (loaded < maxLength)) {
                    if (parsed >= offset) {
                        uint8_t digit = static_cast<uint8_t>(value / divider);
                        if ((BASETYPE != BASE_HEXADECIMAL) || (digit < 10)) {
                            stream[loaded++] = static_cast<char>('0' + digit);
                        } else {
                            stream[loaded++] = static_cast<char>('A' - 10 + digit);
                        }
                        offset++;
                    }
                    parsed++;
                    value %= divider;
                    divider /= BASETYPE;
                }

                if ((BASETYPE == BASE_DECIMAL) && (loaded < maxLength)) {
                    offset = 0;
                }

                return (loaded);
            }

            uint16_t Convert(char stream[], const uint16_t maxLength, uint32_t& offset, const TemplateIntToType<false>& /* For compile time diffrentiation */) const
            {
                return (Convert(stream, maxLength, offset, _value));
            }

            uint16_t Convert(char stream[], const uint16_t maxLength, uint32_t& offset, const TemplateIntToType<true>& /* For c ompile time diffrentiation */) const
            {
                return (Convert(stream, maxLength, offset, ::abs(_value)));
            }

            uint16_t Convert(uint8_t stream[], const uint16_t maxLength, uint32_t& offset, const TemplateIntToType<false>& /* For compile time diffrentiation */) const
            {
                uint8_t loaded = 0;
                uint8_t bytes = (_value <= 0x7F ? 0 : _value < 0xFF ? 1 : _value < 0xFFFF ? 2 : _value < 0xFFFFFFFF ? 4 : 8);

                if (offset == 0) {
                    if (bytes == 0) {
                        if (_value != 0) {
                            stream[loaded++] = static_cast<uint8_t>(_value);
                        } else {
                            stream[loaded++] = IMessagePack::NullValue;
                        }
                    } else {
                        switch (bytes) {
                        case 1:
                            stream[loaded++] = 0xCC;
                            break;
                        case 2:
                            stream[loaded++] = 0xCD;
                            break;
                        case 4:
                            stream[loaded++] = 0xCE;
                            break;
                        case 8:
                            stream[loaded++] = 0xCF;
                            break;
                        default:
                            ASSERT(false);
                        }
                        offset = 1;
                    }
                }

                while ((loaded < maxLength) && (offset != 0)) {
                    TYPE value = _value >> (8 * (bytes - offset));

                    stream[loaded++] = static_cast<uint8_t>(value & 0xFF);
                    offset = (offset == bytes ? 0 : offset + 1);
                }

                return (loaded);
            }

            uint16_t Convert(uint8_t stream[], const uint16_t maxLength, uint32_t& offset, const TemplateIntToType<true>& /* For c ompile time diffrentiation */) const
            {
                uint8_t loaded = 0;
                uint8_t bytes = (((_value < 16) && (_value > -15)) ? 0 : ((_value < 128) && (_value > -127)) ? 1 : ((_value < 32767) && (_value > -32766)) ? 2 : ((_value < 2147483647) && (_value > -2147483646)) ? 4 : 8);

                if (offset == 0) {
                    if (bytes == 0) {
                        stream[loaded++] = (_value & 0x1F) | 0xE0;
                    } else {
                        switch (bytes) {
                        case 1:
                            stream[loaded++] = 0xD0;
                            break;
                        case 2:
                            stream[loaded++] = 0xD1;
                            break;
                        case 4:
                            stream[loaded++] = 0xD2;
                            break;
                        case 8:
                            stream[loaded++] = 0xD3;
                            break;
                        default:
                            ASSERT(false);
                        }
                        offset = 1;
                    }
                }

                while ((loaded < maxLength) && (offset != 0)) {
                    TYPE value = _value >> (8 * (bytes - offset));

                    stream[loaded++] = static_cast<uint8_t>(value & 0xFF);
                    offset = (offset == bytes ? 0 : offset + 1);
                }

                return (loaded);
            }

        private:
            uint16_t _set;
            TYPE _value;
            TYPE _default;
        };

        typedef NumberType<uint8_t, false, BASE_DECIMAL> DecUInt8;
        typedef NumberType<int8_t, true, BASE_DECIMAL> DecSInt8;
        typedef NumberType<uint16_t, false, BASE_DECIMAL> DecUInt16;
        typedef NumberType<int16_t, true, BASE_DECIMAL> DecSInt16;
        typedef NumberType<uint32_t, false, BASE_DECIMAL> DecUInt32;
        typedef NumberType<int32_t, true, BASE_DECIMAL> DecSInt32;
        typedef NumberType<uint64_t, false, BASE_DECIMAL> DecUInt64;
        typedef NumberType<int64_t, true, BASE_DECIMAL> DecSInt64;
        typedef NumberType<uint8_t, false, BASE_HEXADECIMAL> HexUInt8;
        typedef NumberType<int8_t, true, BASE_HEXADECIMAL> HexSInt8;
        typedef NumberType<uint16_t, false, BASE_HEXADECIMAL> HexUInt16;
        typedef NumberType<int16_t, true, BASE_HEXADECIMAL> HexSInt16;
        typedef NumberType<uint32_t, false, BASE_HEXADECIMAL> HexUInt32;
        typedef NumberType<int32_t, true, BASE_HEXADECIMAL> HexSInt32;
        typedef NumberType<uint64_t, false, BASE_HEXADECIMAL> HexUInt64;
        typedef NumberType<int64_t, true, BASE_HEXADECIMAL> HexSInt64;
        typedef NumberType<uint8_t, false, BASE_OCTAL> OctUInt8;
        typedef NumberType<int8_t, true, BASE_OCTAL> OctSInt8;
        typedef NumberType<uint16_t, false, BASE_OCTAL> OctUInt16;
        typedef NumberType<int16_t, true, BASE_OCTAL> OctSInt16;
        typedef NumberType<uint32_t, false, BASE_OCTAL> OctUInt32;
        typedef NumberType<int32_t, true, BASE_OCTAL> OctSInt32;
        typedef NumberType<uint64_t, false, BASE_OCTAL> OctUInt64;
        typedef NumberType<int64_t, true, BASE_OCTAL> OctSInt64;

        typedef NumberType<Core::instance_id, false, BASE_HEXADECIMAL> InstanceId;
        typedef InstanceId Pointer;

        template <class TYPE>
        class FloatType : public IElement, public IMessagePack {
        private:
            enum modes {
                QUOTED = 0x020,
                SET = 0x040,
                ERROR = 0x080,
                NEGATIVE = 0x100,
                UNDEFINED = 0x200
            };

        public:
            FloatType()
                : _set(0)
                , _value(0.0)
                , _default(0.0)
                , _strValue()
            {
            }
            FloatType(const TYPE Value, const bool set = false)
                : _set(set ? SET : 0)
                , _value(Value)
                , _default(Value)
                , _strValue()
            {
            }
            FloatType(FloatType<TYPE>&& move)
                : _set(std::move(move._set))
                , _value(std::move(move._value))
                , _default(std::move(move._default))
                , _strValue()
            {
            }
            FloatType(const FloatType<TYPE>& copy)
                : _set(copy._set)
                , _value(copy._value)
                , _default(copy._default)
                , _strValue()
            {
            }
            ~FloatType() override = default;

            FloatType<TYPE>& operator=(FloatType<TYPE>&& move)
            {
                _value = std::move(move._value);
                _set = std::move(move._set);

                return (*this);
            }

            FloatType<TYPE>& operator=(const FloatType<TYPE>& RHS)
            {
                _value = RHS._value;
                _set = RHS._set;

                return (*this);
            }

            FloatType<TYPE>& operator=(const TYPE& RHS)
            {
                _value = RHS;
                _set = SET;

                return (*this);
            }

            inline TYPE Default() const
            {
                return _default;
            }

            inline TYPE Value() const
            {
                return ((_set & SET) != 0 ? _value : _default);
            }

            inline operator TYPE() const
            {
                return Value();
            }

            void Null(const bool enabled)
            {
                if (enabled == true)
                    _set |= (UNDEFINED|SET);
                else
                    _set &= ~(UNDEFINED|SET);
            }

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return ((_set & SET) != 0);
            }

            bool IsNull() const override
            {
                return ((_set & UNDEFINED) != 0);
            }

            void Clear() override
            {
                _set = 0;
                _value = 0;
            }

            // IElement iface:
            // If this should be serialized/deserialized, it is indicated by a MinSize > 0)
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                ASSERT(maxLength > 0);

                if ((_set & UNDEFINED) != 0 ||
                    std::isinf(_value) ||
                    std::isnan(_value))
                {
                    ASSERT(offset < (sizeof(IElement::NullTag) - 1));
                    loaded = std::min(static_cast<uint16_t>((sizeof(IElement::NullTag) - 1) - offset), maxLength);
                    ::memcpy(stream, &(IElement::NullTag[offset]), loaded);
                    offset = (((offset + loaded) == (sizeof(IElement::NullTag) - 1)) ? 0 : offset + loaded);
                }
                else
                {
                    loaded += Convert(stream, maxLength, offset);
                }

                return loaded;
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    _value = 0;
                    _set = 0;
                    _strValue.clear();
                }

                if ((stream[loaded] == '\"') && ((_set & QUOTED) == 0)) {
                    _set = QUOTED;
                    offset++;
                    loaded++;
                }

                bool completed = false;

                while ((loaded < maxLength) && (completed == false)) {

                    if (((_set & QUOTED) != 0) && (stream[loaded] == '\"')) {
                        completed = true;
                        loaded++;
                        offset++;
                        _set &= ~QUOTED;
                    } else if ( (((_set & QUOTED) == 0) && (::isspace(stream[loaded]))) || (stream[loaded] == '\0') ||
                               (stream[loaded] == ',') || (stream[loaded] == '}') || (stream[loaded] == ']') ) {
                        completed = true;
                    } else {
                        _strValue += stream[loaded++];
                        offset++;
                    }
                }

                if (completed == true) {

                    if (_strValue == IElement::NullTag) {
                        _set |= (SET|UNDEFINED);

                    } else {
                        TYPE val;
                        char* end;
                        if (std::is_same<float,TYPE>::value) {
                            val = std::strtof(_strValue.c_str(), &end);
                        } else {
                            val = static_cast<TYPE>(std::strtod(_strValue.c_str(), &end));
                        }

                        if (end == _strValue.c_str()) {
                            error = Error{ "Error converting \"" + _strValue + "\" to a float/double" };
                            _set = ERROR;
                        } else {
                            _value = val;
                            _set |= SET;
                        }

                        _strValue.clear();
                    }
                    offset = 0;
                }

                return loaded;
            }

            // IMessagePack iface:
            // Refer to https://github.com/msgpack/msgpack/blob/master/spec.md#float-format-family
            // for MessagePack format for float.
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                if ((_set & UNDEFINED) != 0 ||
                    std::isinf(_value) ||
                    std::isnan(_value))
                {
                    stream[0] = IMessagePack::NullValue;
                    return (1);
                }

                uint16_t loaded = 0;
                uint8_t bytes = std::is_same<float,TYPE>::value ? 4 : 8;

                if (offset == 0) {
                    switch (bytes) {
                    case 4:
                        stream[loaded++] = 0xCA;
                        break;
                    case 8:
                        stream[loaded++] = 0xCB;
                        break;
                    default:
                        ASSERT(false);
                    }
                }

                uint8_t* val = (uint8_t*) &_value;

                while ((loaded < maxLength) && (bytes != 0)) {
                    stream[loaded++] = val[bytes - 1];
                    bytes--;
                }

                return loaded;
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t loaded = 0;
                int bytes = 0;
                if (offset == 0) {
                    // First byte depicts a lot. Find out what we need to read
                    _value = 0;
                    uint8_t header = stream[loaded++];

                    if (header == IMessagePack::NullValue) {
                        _set = UNDEFINED;
                    } else if (header == 0xCA) {
                        bytes = 4;
                    } else if (header == 0xCB) {
                        bytes = 8;
                    } else {
                        _set = ERROR;
                    }
                }

                uint8_t* val = (uint8_t*) &_value;

                while ((loaded < maxLength) && (bytes != 0)) {
                    val[bytes - 1] = stream[loaded++];
                    bytes--;
                }

                if (_value != 0) {
                    _set |= SET;
                }
                return loaded;
            }

        private:
            uint16_t Convert(char stream[], const uint16_t maxLength, uint32_t& offset) const
            {
                uint16_t loaded = 0;

                if (_strValue.empty() == true) {
                    char str[16];
                    std::sprintf(str, "%g", _value);
                    const_cast<FloatType*>(this)->_strValue = str;
                }

                while ((loaded < maxLength) && (offset < _strValue.size())) {
                    stream[loaded] = _strValue[offset];
                    loaded++;
                    offset++;
                }
                if (offset == _strValue.size()) {
                    offset = 0;
                    const_cast<FloatType*>(this)->_strValue.clear();
                }

                return loaded;
            }


        private:
            uint16_t _set;
            TYPE _value;
            TYPE _default;
            std::string _strValue;
        };

        typedef FloatType<float> Float;
        typedef FloatType<double> Double;

        class EXTERNAL Boolean : public IElement, public IMessagePack {
        private:
            static constexpr uint8_t None = 0x00;
            static constexpr uint8_t ValueBit = 0x01;
            static constexpr uint8_t DefaultBit = 0x02;
            static constexpr uint8_t SetBit = 0x04;
            static constexpr uint8_t DeserializeBit = 0x08;
            static constexpr uint8_t ErrorBit = 0x10;
            static constexpr uint8_t NullBit = 0x20;

        public:
            Boolean()
                : _value(None)
            {
            }

            Boolean(const bool Value)
                : _value(Value ? DefaultBit : None)
            {
            }

            Boolean(Boolean&& move)
                : _value(std::move(move._value))
            {
            }

            Boolean(const Boolean& copy)
                : _value(copy._value)
            {
            }

            ~Boolean() override = default;

            Boolean& operator=(Boolean&& move)
            {
                // Do not overwrite the default, if not set...copy if set
                _value = (move._value & (SetBit | ValueBit)) | ((move._value & (SetBit)) ? (move._value & DefaultBit) : (_value & DefaultBit));

                return (*this);
            }

            Boolean& operator=(const Boolean& RHS)
            {
                // Do not overwrite the default, if not set...copy if set
                _value = (RHS._value & (SetBit | ValueBit)) | ((RHS._value & (SetBit)) ? (RHS._value & DefaultBit) : (_value & DefaultBit));

                return (*this);
            }

            Boolean& operator=(const bool& RHS)
            {
                // Do not overwrite the default
                _value = (RHS ? (SetBit | ValueBit) : SetBit) | (_value & DefaultBit);

                return (*this);
            }

            inline bool Value() const
            {
                return ((_value & SetBit) != 0 ? (_value & ValueBit) != 0 : (_value & DefaultBit) != 0);
            }

            inline bool Default() const
            {
                return (_value & DefaultBit) != 0;
            }

            inline operator bool() const
            {
                return Value();
            }

            void Null(const bool enabled)
            {
                if (enabled == true)
                    _value |= (SetBit|NullBit);
                else
                    _value &= ~(SetBit|NullBit);
            }

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return ((_value & SetBit) != 0);
            }

            bool IsNull() const override
            {
                return ((_value & NullBit) != 0);
            }

            void Clear() override
            {
                _value = (_value & DefaultBit);
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;
                if ((_value & NullBit) != 0) {
                    while ((loaded < maxLength) && (offset < 4)) {
                        stream[loaded++] = NullTag[offset++];
                    }
                    if (offset == 4) {
                        offset = 0;
                    }
                } else if (Value() == true) {
                    while ((loaded < maxLength) && (offset < (sizeof(IElement::TrueTag) - 1))) {
                        stream[loaded++] = IElement::TrueTag[offset++];
                    }
                    if (offset == (sizeof(IElement::TrueTag) - 1)) {
                        offset = 0;
                    }
                } else {
                    while ((loaded < maxLength) && (offset < (sizeof(IElement::FalseTag) - 1))) {
                        stream[loaded++] = IElement::FalseTag[offset++];
                    }
                    if (offset == (sizeof(IElement::FalseTag) - 1)) {
                        offset = 0;
                    }
                }
                return (loaded);
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>&) override
            {
                uint16_t loaded = 0;
                static constexpr char trueBuffer[] = "true";
                static constexpr char falseBuffer[] = "false";

                if (offset == 0) {
                    if (stream[0] == trueBuffer[0]) {
                        _value = DeserializeBit | (_value & DefaultBit);
                        offset = 1;
                        loaded = 1;
                    } else if (stream[0] == falseBuffer[0]) {
                        _value = (_value & DefaultBit);
                        offset = 1;
                        loaded = 1;
                    } else if (stream[0] == IElement::NullTag[0]) {
                        offset = 1;
                        _value = NullBit | (_value & DefaultBit);
                        loaded = 1;
                    } else if (stream[0] == '0') {
                        _value = SetBit | (_value & DefaultBit);
                        loaded = 1;
                    } else if (stream[0] == '1') {
                        _value = SetBit | ValueBit | (_value & DefaultBit);
                        loaded = 1;
                    } else {
                        _value = ErrorBit | (_value & DefaultBit);
                        offset = 0;
                    }
                }

                if (offset > 0) {
                    uint8_t length = (_value & NullBit ? sizeof(IElement::NullTag) : _value & DeserializeBit ? sizeof(IElement::TrueTag) : sizeof(IElement::FalseTag)) - 1;
                    const char* buffer = (_value & NullBit ? IElement::NullTag : _value & DeserializeBit ? IElement::TrueTag : IElement::FalseTag);

                    while ((loaded < maxLength) && (offset < length) && ((_value & ErrorBit) == 0)) {
                        if (stream[loaded] != buffer[offset]) {
                            _value = ErrorBit | (_value & DefaultBit);
                        } else {
                            offset++;
                            loaded++;
                        }
                    }

                    if ((offset == length) || ((_value & ErrorBit) != 0)) {
                        offset = 0;
                        _value |= SetBit | ((_value & (ErrorBit | DeserializeBit | NullBit)) == DeserializeBit ? ValueBit : 0);
                    }
                }
                return (loaded);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t VARIABLE_IS_NOT_USED maxLength, uint32_t& offset) const override
            {
                ASSERT (maxLength >= 1);

                if ((_value & NullBit) != 0) {
                    stream[offset] = IMessagePack::NullValue;
                } else if ((_value & ValueBit) != 0) {
                    stream[offset] = 0xC3;
                } else {
                    stream[offset] = 0xC2;
                }
                return (1);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t VARIABLE_IS_NOT_USED maxLength, uint32_t& offset) override
            {
                ASSERT (maxLength >= 1);

                if ((stream[0] == IMessagePack::NullValue) != 0) {
                    _value = NullBit;
                } else if ((stream[offset] == 0xC3) != 0) {
                    _value = ValueBit | SetBit;
                } else if ((stream[offset] == 0xC2) != 0) {
                    _value = SetBit;
                } else {
                    _value = ErrorBit;
                }

                return (1);
            }

        private:
            uint8_t _value;
        };

        class EXTERNAL String : public IElement, public IMessagePack {
        private:
            static constexpr uint16_t FlagMask = 0xFC00;
            static constexpr uint16_t EscapeFoundBit = 0x0400;
            static constexpr uint16_t SpecialSequenceBit = 0x0800;
            static constexpr uint16_t QuotedSerializeBit = 0x1000;
            static constexpr uint16_t QuoteFoundBit = 0x2000;
            static constexpr uint16_t NullBit = 0x4000;
            static constexpr uint16_t SetBit = 0x8000;

            enum class ScopeBracket : uint8_t {
                CURLY_BRACKET = 0,
                SQUARE_BRACKET = 1
            };

        public:
            explicit String(const bool quoted = true)
                : _default()
                , _value()
                , _storage(0)
                , _flagsAndCounters(quoted ? QuotedSerializeBit : 0)
            {
            }

            explicit String(const string& Value, const bool quoted = true)
                : _default()
                , _value()
                , _storage(0)
                , _flagsAndCounters(quoted ? QuotedSerializeBit : 0)
            {
                Core::ToString(Value.c_str(), _default);
            }

            explicit String(const char Value[], const bool quoted = true)
                : _default()
                , _value()
                , _storage(0)
                , _flagsAndCounters(quoted ? QuotedSerializeBit : 0)
            {
                Core::ToString(Value, _default);
            }

#ifndef __CORE_NO_WCHAR_SUPPORT__
            explicit String(const wchar_t Value[], const bool quoted = true)
                : _default()
                , _value()
                , _storage(0)
                , _flagsAndCounters(quoted ? QuotedSerializeBit : 0)
            {
                Core::ToString(Value, _default);
            }
#endif // __CORE_NO_WCHAR_SUPPORT__

            String(String&& move)
                : _default(std::move(move._default))
                , _value(std::move(move._value))
                , _storage(std::move(move._storage))
                , _flagsAndCounters(std::move(move._flagsAndCounters))
            {
            }

            String(const String& copy)
                : _default(copy._default)
                , _value(copy._value)
                , _storage(copy._storage)
                , _flagsAndCounters(copy._flagsAndCounters)
            {
            }

            ~String() override = default;

            String& operator=(const string& RHS)
            {
                Core::ToString(RHS.c_str(), _value);
                _flagsAndCounters |= SetBit;

                return (*this);
            }

            String& operator=(const char RHS[])
            {
                Core::ToString(RHS, _value);
                _flagsAndCounters |= SetBit;

                return (*this);
            }

#ifndef __CORE_NO_WCHAR_SUPPORT__
            String& operator=(const wchar_t RHS[])
            {
                Core::ToString(RHS, _value);
                _flagsAndCounters |= SetBit;

                return (*this);
            }
#endif // __CORE_NO_WCHAR_SUPPORT__

            String& operator=(String&& move)
            {
                _default = std::move(move._default);
                _value = std::move(move._value);
                _flagsAndCounters = std::move(move._flagsAndCounters);

                return (*this);
            }

            String& operator=(const String& RHS)
            {
                _default = RHS._default;
                _value = RHS._value;
                _flagsAndCounters = RHS._flagsAndCounters;

                return (*this);
            }

            inline bool operator==(const String& RHS) const
            {
                return (Value() == RHS.Value());
            }

            inline bool operator!=(const String& RHS) const
            {
                return (!operator==(RHS));
            }

            inline bool operator==(const char RHS[]) const
            {
                return (Value() == RHS);
            }

            inline bool operator!=(const char RHS[]) const
            {
                return (!operator==(RHS));
            }

#ifndef __CORE_NO_WCHAR_SUPPORT__
            inline bool operator==(const wchar_t RHS[]) const
            {
                std::string comparator;
                Core::ToString(RHS, comparator);
                return (Value() == comparator);
            }

            inline bool operator!=(const wchar_t RHS[]) const
            {
                return (!operator==(RHS));
            }
#endif // __CORE_NO_WCHAR_SUPPORT__

            inline bool operator<(const String& RHS) const
            {
                return (Value() < RHS.Value());
            }

            inline bool operator>(const String& RHS) const
            {
                return (Value() > RHS.Value());
            }

            inline bool operator>=(const String& RHS) const
            {
                return (!operator<(RHS));
            }

            inline bool operator<=(const String& RHS) const
            {
                return (!operator>(RHS));
            }

            inline const string Value() const
            {
                if ((_flagsAndCounters & (SetBit | QuoteFoundBit | QuotedSerializeBit)) == (SetBit | QuoteFoundBit)) {
                    return (Core::ToQuotedString('\"', _value));
                }
                return (((_flagsAndCounters & (SetBit | NullBit)) == SetBit) ? Core::ToString(_value.c_str()) : Core::ToString(_default.c_str()));
            }

            inline const string& Default() const
            {
                return (_default);
            }

            inline operator const string() const
            {
                return (Value());
            }

            void Null(const bool enabled)
            {
                if (enabled == true) {
                    _flagsAndCounters |= (NullBit | SetBit);
                    _value = IElement::NullTag;
                }
                else {
                    _flagsAndCounters &= ~(NullBit | SetBit);
                    _value.clear();
                }
            }

            // IElement iface:
            bool IsNull() const override
            {
                return (_flagsAndCounters & NullBit) != 0;
            }

            bool IsSet() const override
            {
                return ((_flagsAndCounters & SetBit) != 0);
            }

            void Clear() override
            {
                _flagsAndCounters = (_flagsAndCounters & QuotedSerializeBit);
                _value.clear();
            }

            inline bool IsQuoted() const
            {
                return (((_flagsAndCounters & NullBit) == 0) && ((_flagsAndCounters & (QuotedSerializeBit | QuoteFoundBit)) != 0));
            }

            inline void SetQuoted(const bool enable)
            {
                if (enable == true) {
                    _flagsAndCounters |= QuotedSerializeBit;
                }
                else {
                    _flagsAndCounters &= (~QuotedSerializeBit);
                }
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t result = 0;

                ASSERT(maxLength > 0);

                if ((_flagsAndCounters & SetBit) != 0) {
                    bool isQuoted = IsQuoted();
                    if (offset == 0)  {
                        if (isQuoted == true) {
                            // We always start with a quote or Block marker
                            stream[result++] = '\"';
                        }
                        offset = 1;
                        _flagsAndCounters &= (FlagMask ^ (SpecialSequenceBit|EscapeFoundBit));
                    }

                    uint32_t length = static_cast<uint32_t>(_value.length()) - (offset - 1);

                    while ((result < maxLength) && (length > 0)) {
                        const uint16_t current = static_cast<uint16_t>((_value[offset - 1]) & 0xFF);

                        // See if this is a printable character
                        if ((isQuoted == false) || ((::isprint(current)) && (current != '\"') && (current != '\\') && (current != '/')) ) {
                            stream[result++] = static_cast<TCHAR>(current);
                            length--;
                            offset++;
                        }
                        else if ((_flagsAndCounters & SpecialSequenceBit) == 0) {
                            // We need to escape these..
                            stream[result++] = '\\';
                            _flagsAndCounters |= SpecialSequenceBit;
                        }
                        else if ((_flagsAndCounters & 0xFF) == 0x00) {
                            // Check if it is a single character drop or a \u
                            switch (current) {
                            case 0x08: stream[result++] = 'b'; break;
                            case 0x09: stream[result++] = 't'; break;
                            case 0x0a: stream[result++] = 'n'; break;
                            case 0x0c: stream[result++] = 'f'; break;
                            case 0x0d: stream[result++] = 'r'; break;
                            case '\\': stream[result++] = '\\'; break;
                            case '/': stream[result++] = '/'; break;
                            case '"': stream[result++] = '"'; break;
                            default: {
                                uint16_t lowPart, highPart;
                                int8_t codeSize = ToCodePoint(&(_value[offset - 1]), length, _storage);

                                if (codeSize < 0) {
                                    // Oops it is a bad code thingy, Skip it..
                                    // TODO: report an error
                                    codeSize = -codeSize;
                                }

                                ASSERT(codeSize <= 7);

                                if (CodePointToUTF16(_storage, lowPart, highPart) == false) {
                                    // Oops we have a bad transaltion of the code point
                                    // TODO: report an error
                                }

                                _storage = (highPart << 16) | lowPart;

                                // Oke start processing an escape squence and remember how many bytes we jump if
                                // we are completed, start at 2 index now as we already wrote /u
                                _flagsAndCounters |= ((codeSize & 0x07) << 3) | 0x02;

                                stream[result++] = 'u';
                                break;
                            }
                            }

                            // If all has been writeen it is time to move back to the "copying situation...
                            if ((_flagsAndCounters & 0xFF) == 0x00) {
                                _flagsAndCounters ^= SpecialSequenceBit;
                                length--;
                                offset++;
                            }
                        }
                        else {
                            if ((_flagsAndCounters & 0x7) < 0x2) {
                                stream[result++] = ((_flagsAndCounters & 0x07) == 0 ? '\\' : 'u');
                            }
                            else {
                                uint8_t part;

                                // Write out the CodePoint....
                                if (_storage > 0xFFFF) {
                                    // First write the Most Significant part
                                    part = (_storage >> (16 + ((5 - (_flagsAndCounters & 0x07)) * 4))) & 0x0F;
                                }
                                else {
                                    part = (_storage >> ((5 - (_flagsAndCounters & 0x07)) * 4)) & 0x0F;
                                }
                                stream[result++] = (part > 9 ? 'A' + (part - 10) : '0' + part);
                            }

                            _flagsAndCounters += 1;

                            if ((_flagsAndCounters & 0x7) == 6) {
                                // Oke we flushed a HEX value of 4 digits, lets determine the next step..
                                if (_storage > 0xFFFF) {
                                    _storage = (_storage & 0xFFFF);
                                    _flagsAndCounters &= (FlagMask | 0xF8);
                                }
                                else {
                                    // We are done ! Move on, strange character has been handled and converted
                                    uint8_t skip = ((_flagsAndCounters >> 3) & 0x07);
                                    length -= skip;
                                    offset += skip;

                                    _flagsAndCounters &= (FlagMask ^ SpecialSequenceBit);
                                }
                            }
                        }
                    }

                    if (length == 0) {
                        // And we close with a quote..
                        if (isQuoted == false) {
                            offset = 0;
                        }
                        else if (result < maxLength) {
                            stream[result++] = '\"';
                            offset = 0;
                        }
                    }
                }

                return (result);
            }
            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                bool finished = false;
                uint16_t result = 0;
                ASSERT(maxLength > 0);

                if (offset == 0) {
                    _value.clear();
                    _flagsAndCounters &= (FlagMask ^ (SpecialSequenceBit|EscapeFoundBit|QuoteFoundBit));
                    _storage = 0;
                    if (stream[result] == '\"') {
                        result++;
                        _flagsAndCounters |= QuoteFoundBit;
                    }
                    offset = 1;
                }

                // Might be that the last character we added was a
                while ((result < maxLength) && (finished == false)) {

                    TCHAR current = stream[result];

                    // What are we deserializing a string, or an opaque JSON object!!!
                    if ((_flagsAndCounters & QuoteFoundBit) == 0) {
                        // It's an opaque structure, so *no* decoding required. Leave as is !
                        if (current == '{') {
                            if (InScope(ScopeBracket::CURLY_BRACKET) == false) {
                                error = Error{ "Opaque object nesting too deep" };
                            }
                        }
                        else if (current == '[') {
                            if (InScope(ScopeBracket::SQUARE_BRACKET) == false) {
                                error = Error{ "Opaque object nesting too deep" };
                            }
                        }
                        else if ((_flagsAndCounters & 0x1F) == 0) {
                            // If we did not open an object, the only thing we allow are whitespaces as they can
                            // always be dropped!
                            finished = (((_flagsAndCounters & EscapeFoundBit) == 0) && ((current == ',') || (current == '}') || (current == ']') || (current == '\0')));
                        }
                        else if (current == '}') {
                            if (OutScope(ScopeBracket::CURLY_BRACKET) == false) {
                                error = Error{ "Expected \"]\" but got \"}\" in opaque object" };
                            }
                        }
                        else if (current == ']') {
                            if (OutScope(ScopeBracket::SQUARE_BRACKET) == false) {
                                error = Error{ "Expected \"}\" but got \"]\" in opaque object" };
                            }
                        }

                        if (finished == false) {
                            // Write the amount we possibly can..
                            _value += current;

                            if ((current == '\"') && ((_value.empty() == true) || (_value[_value.length() - 1] != '\\'))) {
                                // Oke we are going to enetr a Serialized thingy... lets be opaque from here on
                                _flagsAndCounters ^= EscapeFoundBit;
                            }

                            result++;
                        }
                    }
                    // Since it is a "real" string translate back all escaped stuff.. are we in an unescaping mode?
                    else if ((_flagsAndCounters & SpecialSequenceBit) == 0x00) {
                        // Nope we are not, so see if we need to start it and otherwise, just copy...
                        if (current == '\\') {
                            // And we need to start it.
                            _flagsAndCounters |= SpecialSequenceBit;
                        }
                        else if (current == '\"') {
                            // We are done! leave this element.
                            finished = true;
                        }
                        else {
                            // Just copy, we and onto the next;
                            _value += current;
                        }
                        result++;
                    }
                    else if ((_flagsAndCounters & 0xFF) == 0x00) {

                        if (current == 'u') {
                            _flagsAndCounters |= 0x4;
                        }
                        else {
                            // We are in a string mode, so we need to decode. Decode what we receive..
                            switch (current) {
                            case '\"': _value += '\"'; break;
                            case '\\': _value += '\\'; break;
                            case '/':  _value += '/';  break;
                            case 'b':  _value += static_cast<TCHAR>(0x08); break;
                            case 't':  _value += static_cast<TCHAR>(0x09); break;
                            case 'n':  _value += static_cast<TCHAR>(0x0a); break;
                            case 'f':  _value += static_cast<TCHAR>(0x0c); break;
                            case 'r':  _value += static_cast<TCHAR>(0x0d); break;
                            default:
                                error = Error{ "unknown escaping code." };
                                break;
                            }
                            _flagsAndCounters ^= SpecialSequenceBit;
                        }
                        result++;
                    }
                    else {
                        // If we end up here, we are actually gathering unicode values to be decoded.
                        _flagsAndCounters--;

                        if (::isxdigit(current) == false) {
                            error = Error{ "the unescaping of the u requires hexadecimal characters" };
                        }
                        else {
                            _storage = (_storage << 4) | ((::isdigit(current) ? current - '0' : 10 + (::toupper(current) - 'A')) & 0xF);
                            result++;
                            if ((_flagsAndCounters & 0xFF) == 0x00) {
                                _flagsAndCounters ^= SpecialSequenceBit;

                                // Examine the codePoint, if ot is a pair ot not..
                                if ( (_storage >= 0xFFFF) || ((_storage & 0xFC00) != 0xD800) ) {

                                    // We have a full monty, 2 x UTF16 to be translated :-)
                                    uint32_t codePoint;
                                    TCHAR buffer[6];

                                    UTF16ToCodePoint((_storage & 0xFFFF), ((_storage >> 16) & 0xFFFF), codePoint);

                                    // Seems like we have a pending code point to be added, before we add anaything elese :-)
                                    int8_t bytes = FromCodePoint(codePoint, buffer, sizeof(buffer));

                                    if (bytes <= 0) {
                                        error = Error{ "There is no valid codepoint defined." };
                                    }
                                    else {
                                        _value += string(buffer, bytes);
                                    }
                                    _storage = 0;
                                }
                            }
                        }
                    }
                }

                if ( (finished == false) && (error.IsSet() == false) ) {
                    offset += static_cast<uint32_t>(_value.length()) ;
                } else {
                    offset = 0;
                    _flagsAndCounters |= (_value == IElement::NullTag ? NullBit|SetBit : SetBit);

                    if ((_flagsAndCounters & QuoteFoundBit) == 0) {
                        // Right-trim the non-string value, it's always left-trimmed already
                        _value.erase(std::find_if(_value.rbegin(), _value.rend(), [](const unsigned char ch) { return (!std::isspace(ch)); }).base(), _value.end());
                    }
                }

                return (result);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    if ((_flagsAndCounters & NullBit) != 0) {
                        stream[loaded++] = IMessagePack::NullValue;
                    } else if (_value.length() <= 31) {
                        _storage = 1;
                        stream[loaded++] = static_cast<uint8_t>(_value.length() | 0xA0);
                        offset++;
                    } else if (_value.length() <= 0xFF) {
                        _storage = 2;
                        stream[loaded++] = 0xD9;
                        offset++;
                    } else if (_value.length() <= 0xFFFF) {
                        _storage = 3;
                        stream[loaded++] = 0xDA;
                        offset++;
                    } else {
                        stream[loaded++] = IMessagePack::NullValue;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset < (_storage & 0x0F))) {
                        stream[loaded++] = static_cast<uint8_t>((_value.length() >> (8 * ( (_storage & 0x0F) - offset - 1))) & 0xFF);
                        offset++;
                    }

                    uint16_t copied = 0;
                    while ((loaded < maxLength) && (offset != 0)) {
                        copied = static_cast<uint16_t>(_value.copy(reinterpret_cast<char*>(&stream[loaded]), (maxLength - loaded), offset - (_storage & 0x0F)));
                        offset += copied;
                        loaded += copied;
                        if ((_storage & 0x0F) != 0) {
                           offset -= (_storage & 0x0F);
                           _storage = 0;
                        }

                        if (offset >= _value.length()) {
                            offset = 0;
                        }
                    }
                }

                return (loaded);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    _value.clear();
                    if (stream[loaded] == IMessagePack::NullValue) {
                        _flagsAndCounters |= NullBit;
                        loaded++;
                    } else if ((stream[loaded] & 0xA0) == 0xA0) {
                        _storage = stream[loaded] & 0x1F;
                        offset = 3;
                        loaded++;
                    } else if (stream[loaded] == 0xD9) {
                        _storage = 0;
                        offset = 2;
                        loaded++;
                    } else if (stream[loaded] == 0xDA) {
                        _storage = 0;
                        offset = 1;
                        loaded++;
                    } else {
                        loaded = maxLength;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset < 3)) {
                        _storage = (_storage << 8) + stream[loaded++];
                        offset++;
                    }

                    while ((loaded < maxLength) && ((offset - 3) < static_cast<uint16_t>(_storage))) {
                        _value += static_cast<char>(stream[loaded++]);
                        offset++;
                    }

                    if ((offset >= 3) && (static_cast<uint16_t>(offset - 3) == _storage)) {
                        offset = 0;
                        _flagsAndCounters |= ((_flagsAndCounters & QuoteFoundBit) ? SetBit : (_value == NullTag ? NullBit : SetBit));
                    }
                }

                return (loaded);
            }

        private:
            bool InScope(const ScopeBracket mode) {
                bool added = false;
                uint8_t depth = (_flagsAndCounters & 0x1F);

                if ( ((depth != 0) || (_value.empty() == true)) && ((depth + 1) <= 31) ) {
                    _storage <<= 1;
                    _storage |= static_cast<uint8_t>(mode);
                    _flagsAndCounters++;
                    added = true;
                }
                return (added);
            }
            bool OutScope(const ScopeBracket mode) {
                bool succcesfull = false;
                ScopeBracket bracket = static_cast<ScopeBracket>(_storage & 0x1);
                uint8_t depth = (_flagsAndCounters & 0x1F);
                if ((depth > 0) && (bracket == mode)) {
                    _storage >>= 1;
                    _flagsAndCounters--;
                    succcesfull = true;
                }
                return (succcesfull);
            }


        private:
            std::string _default;
            std::string _value;

            mutable uint32_t _storage;

            // The value stores the following BITS:
            // | 8 | 2 |  3  |  3  |
            //   F   U   LLL   III
            // Where:
            // F are flags bits (See statics at the beginning of the class)
            // U is unused
            // L Length of the bytes under process (8)
            // I Index to the written bytes (8).
            mutable uint16_t _flagsAndCounters;
        };

        class EXTERNAL Buffer : public IElement, public IMessagePack {
        private:
            enum modus {
                SET = 0x20,
                ERROR = 0x40,
                UNDEFINED = 0x80
            };

        public:
            Buffer()
                : _state(0)
                , _lastStuff(0)
                , _index(0)
                , _length(0)
                , _maxLength(255)
                , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxLength)))
            {
            }

            Buffer(Buffer&& move)
                : _state(std::move(move._state))
                , _lastStuff(std::move(move._lastStuff))
                , _index(std::move(move._index))
                , _length(std::move(move._length))
                , _maxLength(std::move(move._maxLength))
                , _buffer(std::move(move._buffer))
            {
                move._buffer = nullptr;
            }

            Buffer(const Buffer& copy)
                : _state(copy._state)
                , _lastStuff(copy._lastStuff)
                , _index(copy._index)
                , _length(copy._length)
                , _maxLength(copy._maxLength)
                , _buffer(reinterpret_cast<uint8_t*>(::malloc(_maxLength)))
            {
                ASSERT(_length <= _maxLength);
                ::memcpy(_buffer, copy._buffer, _length);
            }

           ~Buffer() override
            {
                if (_buffer != nullptr) {
                    ::free(_buffer);
                }
            }

            Buffer& operator= (Buffer&& move) {
                _state = std::move(move._state);
                _lastStuff = std::move(move._lastStuff);
                _index = std::move(move._index);
                _length = std::move(move._length);
                _maxLength = std::move(move._maxLength);
                _buffer = std::move(move._buffer);

                move._buffer = nullptr;

                return (*this);
            }

            Buffer& operator=(const Buffer& copy) {
                _state = copy._state;
                _lastStuff = copy._lastStuff;
                _index = copy._index;
                _length = copy._length;
                _maxLength = copy._maxLength;
                _buffer = reinterpret_cast<uint8_t*>(::malloc(_maxLength));

                ASSERT(_length <= _maxLength);
                ::memcpy(_buffer, copy._buffer, _length);

                return (*this);
            }

            void Null(const bool enabled)
            {
                if (enabled == true)
                    _state |= UNDEFINED;
                else
                    _state &= ~UNDEFINED;
            }

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return ((_length > 0) && ((_state & SET) != 0));
            }

            bool IsNull() const
            {
                return ((_state & UNDEFINED) != 0);
            }

            void Clear() override
            {
                _state = 0;
                _length = 0;
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                static const TCHAR base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                    "abcdefghijklmnopqrstuvwxyz"
                                                    "0123456789+/";

                uint16_t loaded = 0;

                if (offset == 0) {
                    _state = 0;
                    _index = 0;
                    _lastStuff = 0;
                    offset = 1;
                    stream[loaded++] = ((_state & UNDEFINED) == 0 ? '\"' : 'n');
                }

                if ((_state & UNDEFINED) == 0) {
                    while ((loaded < maxLength) && (offset < 4)) {
                        stream[loaded++] = IElement::NullTag[offset++];
                    }
                    if (offset == 4) {
                        offset = 0;
                    }
                } else {
                    while ((loaded < maxLength) && (_index < _length)) {
                        if (_state == 0) {
                            stream[loaded++] = base64_chars[((_buffer[_index] & 0xFC) >> 2)];
                            _lastStuff = ((_buffer[_index] & 0x03) << 4);
                            _state = 1;
                            _index++;
                        } else if (_state == 1) {
                            stream[loaded++] += base64_chars[(((_buffer[_index] & 0xF0) >> 4) | _lastStuff)];
                            _lastStuff = ((_buffer[_index] & 0x0F) << 2);
                            _index++;
                            _state = 2;
                        } else if (_state == 2) {
                            stream[loaded++] += base64_chars[(((_buffer[_index] & 0xC0) >> 6) | _lastStuff)];
                            _state = 3;
                        } else {
                            ASSERT(_state == 3);
                            stream[loaded++] += base64_chars[(_buffer[_index] & 0x3F)];
                            _state = 0;
                            _index++;
                        }
                    }

                    if ((loaded < maxLength) && (_index == _length)) {
                        if (_state != 0) {
                            stream[loaded++] = base64_chars[_lastStuff];
                            _state = 0;
                        }
                        if (loaded < maxLength) {
                            stream[loaded++] = '\"';
                            offset = 0;
                        }
                    }
                }
                return (loaded);
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    _state = 0xFF;
                    _index = 0;
                    _lastStuff = 0;
                    _length = 0;
                    offset = 1;
                }

                if (_state == 0xFF) {

                    while ((loaded < maxLength) && ((stream[loaded] != '\"') && (stream[loaded] != 'n'))) {
                        loaded++;
                    }

                    if (loaded < maxLength) {

                        loaded++;
                        _state = (stream[loaded] == 'n' ? UNDEFINED : 0);
                    }
                }

                if (_state != 0xFF) {
                    if ((_state & UNDEFINED) != 0) {
                        while ((loaded < maxLength) && (offset != 0) && (offset < 4)) {
                            if (stream[loaded] != IElement::NullTag[offset]) {
                                error = Error{ "Only base64 characters or null supported." };
                                _state = ERROR;
                                offset = 0;
                            } else {
                                offset++;
                                loaded++;
                            }
                        }
                    } else if ((_state & SET) == 0) {
                        while (loaded < maxLength) {
                            uint8_t converted;
                            TCHAR current = stream[loaded++];

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
                            } else if (::isspace(current)) {
                                continue;
                            } else if (current == '\"') {
                                _state |= SET;
                                break;
                            } else {
                                error = Error{ "Only base64 characters or null supported." };
                                _state = ERROR;
                                offset = 0;
                                break;
                            }

                            // See if we can still add a character
                            if (_index == _maxLength) {
                                uint16_t maxLength = (2 * _maxLength);
                                uint8_t* newBuffer = reinterpret_cast<uint8_t*>(::realloc(_buffer, maxLength));
                                if (newBuffer != nullptr) {
                                    _buffer = newBuffer;
                                    _maxLength = maxLength;
                                }
                            }

                            if (_index >= _maxLength) {
                                TRACE_L1("Out of memory !!!!");
                            }
                            else {
                                if (_state == 0) {
                                    _lastStuff = converted << 2;
                                    _state = 1;
                                }
                                else if (_state == 1) {
                                    _buffer[_index++] = (((converted & 0x30) >> 4) | _lastStuff);
                                    _lastStuff = ((converted & 0x0F) << 4);
                                    _state = 2;
                                }
                                else if (_state == 2) {
                                    _buffer[_index++] = (((converted & 0x3C) >> 2) | _lastStuff);
                                    _lastStuff = ((converted & 0x03) << 6);
                                    _state = 3;
                                }
                                else if (_state == 3) {
                                    _buffer[_index++] = ((converted & 0x3F) | _lastStuff);
                                    _state = 0;
                                }

                            }
                        }

                        if (((_state & SET) == SET) && ((_state & 0xF) != 0)) {

                            if (_index == _maxLength) {
                                uint16_t maxLength = (_maxLength + 0xFF);
                                uint8_t* newBuffer = reinterpret_cast<uint8_t*>(::realloc(_buffer, maxLength));
                                if (newBuffer != nullptr) {
                                    _buffer = newBuffer;
                                    _maxLength = maxLength;
                                }
                            }
                            if (_index < _maxLength) {
                                _buffer[_index++] = _lastStuff;
                            }
                            else {
                                TRACE_L1("Out of Memory!!!!");
                            }
                        }
                    }
                }

                return (loaded);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    if ((_state & UNDEFINED) != 0) {
                        stream[loaded++] = IMessagePack::NullValue;
                    } else if (_length <= 0xFF) {
                        _index = 0;
                        stream[loaded++] = 0xC4;
                        offset = 2;
                    } else {
                        _index = 0;
                        stream[loaded++] = 0xC5;
                        offset = 1;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset < 3)) {
                        stream[loaded++] = static_cast<uint8_t>((_length >> (8 * (2 - offset))) & 0xFF);
                        offset++;
                    }

                    while ((loaded < maxLength) && (_index < _length)) {
                        stream[loaded++] = _buffer[_index++];
                    }
                    offset = (_index == _length);
                }

                return (loaded);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    _state = 0;
                    _length = 0;
                    _index = 0;

                    if (stream[loaded] == IMessagePack::NullValue) {
                        _state = UNDEFINED;
                        loaded++;
                    } else if (stream[loaded] == 0xC4) {
                        offset = 2;
                        loaded++;
                    } else if (stream[loaded] == 0xC5) {
                        offset = 1;
                        loaded++;
                    } else {
                        _state = ERROR;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset < 3)) {
                        _length = (_length << 8) + stream[loaded++];
                        offset++;
                    }

                    if (offset == 3) {
                        if (_length > _maxLength) {
                            _maxLength = _length;
                            ::free(_buffer);
                            _buffer = reinterpret_cast<uint8_t*>(::malloc(_maxLength));
                        }
                        offset = 4;
                    }

                    while ((loaded < maxLength) && (_index < _length)) {
                        _buffer[_index++] = stream[loaded++];
                    }

                    if (_index == _length) {
                        offset = 0;
                    }
                }

                return (loaded);
            }

        private:
            mutable uint8_t _state;
            mutable uint8_t _lastStuff;
            mutable uint16_t _index;
            uint16_t _length;
            uint16_t _maxLength;
            uint8_t* _buffer;
        };

        template <typename ENUMERATE>
        class EnumType : public IElement, public IMessagePack {
        private:
            enum status {
                SET = 0x01,
                ERROR = 0x02,
                UNDEFINED = 0x04
            };

        public:
            EnumType()
                : _state(0)
                , _value()
                , _default(static_cast<ENUMERATE>(0))
            {
            }

            EnumType(const ENUMERATE Value)
                : _state(0)
                , _value()
                , _default(Value)
            {
            }

            EnumType(EnumType<ENUMERATE>&& move)
                : _state(std::move(move._state))
                , _value(std::move(move._value))
                , _default(std::move(move._default))
            {
            }

            EnumType(const EnumType<ENUMERATE>& copy)
                : _state(copy._state)
                , _value(copy._value)
                , _default(copy._default)
            {
            }

            ~EnumType() override = default;

            EnumType<ENUMERATE>& operator=(EnumType<ENUMERATE>&& move)
            {
                _value = std::move(move._value);
                _state = std::move(move._state);
                return (*this);
            }

            EnumType<ENUMERATE>& operator=(const EnumType<ENUMERATE>& RHS)
            {
                _value = RHS._value;
                _state = RHS._state;
                return (*this);
            }

            EnumType<ENUMERATE>& operator=(const ENUMERATE& RHS)
            {
                _value = RHS;
                _state = SET;

                return (*this);
            }

            inline const ENUMERATE Default() const
            {
                return (_default);
            }

            inline const ENUMERATE Value() const
            {
                return (((_state & (SET | UNDEFINED)) == SET) ? _value : _default);
            }

            inline operator const ENUMERATE() const
            {
                return Value();
            }

            const TCHAR* Data() const
            {
                return (Core::EnumerateType<ENUMERATE>(Value()).Data());
            }

            void Null(const bool enabled)
            {
                if (enabled == true)
                    _state |= (UNDEFINED|SET);
                else
                    _state &= ~(UNDEFINED|SET);
            }

            bool IsSet() const override
            {
                return ((_state & SET) != 0);
            }

            bool IsNull() const override
            {
                return ((_state & UNDEFINED) != 0);
            }

            void Clear() override
            {
                _state = 0;
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                if (offset == 0) {
                    if ((_state & UNDEFINED) != 0) {
                        _parser.Null(true);
                    } else {
                        _parser = Core::EnumerateType<ENUMERATE>(Value()).Data();
                    }
                }
                return (static_cast<const IElement&>(_parser).Serialize(stream, maxLength, offset));
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t result = static_cast<IElement&>(_parser).Deserialize(stream, maxLength, offset, error);

                if (offset == 0) {

                    if (_parser.IsNull() == true) {
                        _state = (UNDEFINED|SET);
                    } else if (_parser.IsSet() == true) {
                        // Looks like we parsed the value. Lets see if we can find it..
                        Core::EnumerateType<ENUMERATE> converted(_parser.Value().c_str(), false);

                        if (converted.IsSet() == true) {
                            _value = converted.Value();
                            _state = SET;
                        } else {
                            _state = (SET|UNDEFINED);
                            TRACE_L1(_T("Unknown enum value: %s"), _parser.Value().c_str());
                            error = Error{ "Unknown enum value: " +  _parser.Value()};
                        }
                    } else {
                        error = Error{ "Invalid enum" };
                        _state = ERROR;
                    }
                }

                return (result);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    if ((_state & UNDEFINED) != 0) {
                        stream[loaded++] = IMessagePack::NullValue;
                    } else {
                        _package = static_cast<uint32_t>(Value());
                    }
                }

                return (loaded == 0 ? static_cast<const IMessagePack&>(_package).Serialize(stream, maxLength, offset) : loaded);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t result = 0;

                if ((offset == 0) && (stream[0] == IMessagePack::NullValue)) {
                    _state = UNDEFINED;
                    result = 1;
                } else {
                    result = static_cast<IMessagePack&>(_package).Deserialize(stream, maxLength, offset);
                }

                if ((offset == 0) && (_state != UNDEFINED)) {
                    if (_package.IsSet() == true) {
                        _value = static_cast<ENUMERATE>(_package.Value());
                    } else {
                        _state = ERROR;
                    }
                }

                return (result);
            }

        private:
            uint8_t _state;
            ENUMERATE _value;
            ENUMERATE _default;
            mutable String _parser;
            mutable NumberType<uint32_t, FALSE, BASE_HEXADECIMAL> _package;
        };

        template <typename ELEMENT>
        class ArrayType : public IElement, public IMessagePack {
        private:
            enum modus : uint8_t {
                ERROR = 0x80,
                SET = 0x20,
                UNDEFINED = 0x40
            };

            static constexpr uint16_t FIND_MARKER = 0;
            static constexpr uint16_t BEGIN_MARKER = 1;
            static constexpr uint16_t END_MARKER = 2;
            static constexpr uint16_t SKIP_BEFORE = 3;
            static constexpr uint16_t SKIP_AFTER = 4;
            static constexpr uint16_t PARSE = 5;

        public:
            template <typename ARRAYELEMENT>
            class ConstIteratorType {
            private:
                typedef std::list<ARRAYELEMENT> ArrayContainer;
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            public:
                ConstIteratorType()
                    : _container(nullptr)
                    , _iterator()
                    , _state(AT_BEGINNING)
                {
                }

                ConstIteratorType(const ArrayContainer& container)
                    : _container(&container)
                    , _iterator(_container->begin())
                    , _state(AT_BEGINNING)
                {
                }

                ConstIteratorType(ConstIteratorType<ARRAYELEMENT>&& move)
                    : _container(std::move(move._container))
                    , _iterator(std::move(move._iterator))
                    , _state(std::move(move._state))
                {
                }

                ConstIteratorType(const ConstIteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }

                ~ConstIteratorType() = default;

                ConstIteratorType<ARRAYELEMENT>& operator=(ConstIteratorType<ARRAYELEMENT>&& move)
                {
                    _container = std::move(move._container);
                    _iterator = std::move(move._iterator);
                    _state = std::move(move._state);

                    return (*this);
                }

                ConstIteratorType<ARRAYELEMENT>& operator=(const ConstIteratorType<ARRAYELEMENT>& RHS)
                {
                    _container = RHS._container;
                    _iterator = RHS._iterator;
                    _state = RHS._state;

                    return (*this);
                }

            public:
                inline bool IsValid() const
                {
                    return (_state == AT_ELEMENT);
                }

                inline bool Reset(const uint32_t index = ~0)
                {
                    _state = AT_BEGINNING;

                    if (_container != nullptr) {
                        _iterator = _container->begin();

                        if (index != static_cast<uint32_t>(~0)) {
                            uint32_t position = (index + 1);
                            while (position > 0) {
                                while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                    _iterator++;
                                }

                                if (_iterator == _container->end()) {
                                    position = 0;
                                }
                                else if (--position != 0) {
                                    _iterator++;
                                }
                            }
                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    }

                    return (_state == AT_ELEMENT);
                }

                inline bool Next()
                {
                    if (_container != nullptr) {
                        if (_state != AT_END) {
                            if (_state != AT_BEGINNING) {
                                _iterator++;
                            }

                            while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                _iterator++;
                            }

                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    } else {
                        _state = AT_END;
                    }
                    return (_state == AT_ELEMENT);
                }

                const ARRAYELEMENT& Current() const
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator);
                }

                inline uint32_t Count() const
                {
                    return (_container == nullptr ? 0 : static_cast<uint32_t>(_container->size()));
                }

            private:
                const ArrayContainer* _container;
                typename ArrayContainer::const_iterator _iterator;
                State _state;
            };

            template <typename ARRAYELEMENT>
            class IteratorType {
            private:
                typedef std::list<ARRAYELEMENT> ArrayContainer;
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            public:
                IteratorType()
                    : _container(nullptr)
                    , _iterator()
                    , _state(AT_BEGINNING)
                {
                }

                IteratorType(ArrayContainer& container)
                    : _container(&container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }

                IteratorType(IteratorType<ARRAYELEMENT>&& move)
                    : _container(std::move(move._container))
                    , _iterator(std::move(move._iterator))
                    , _state(std::move(move._state))
                {
                }

                IteratorType(const IteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }

                ~IteratorType() = default;

                IteratorType<ARRAYELEMENT>& operator=(IteratorType<ARRAYELEMENT>&& move)
                {
                    _container = std::move(move._container);
                    _iterator = std::move(move._iterator);
                    _state = std::move(move._state);

                    return (*this);
                }

                IteratorType<ARRAYELEMENT>& operator=(const IteratorType<ARRAYELEMENT>& RHS)
                {
                    _container = RHS._container;
                    _iterator = RHS._iterator;
                    _state = RHS._state;

                    return (*this);
                }

            public:
                inline bool IsValid() const
                {
                    return (_state == AT_ELEMENT);
                }

                bool Reset(const uint32_t index = ~0)
                {
                    _state = AT_BEGINNING;

                    if (_container != nullptr) {
                        _iterator = _container->begin();
                        if (index != static_cast<uint32_t>(~0)) {
                            uint32_t position = (index + 1);
                            while (position > 0) {
                                while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                    _iterator++;
                                }

                                if (_iterator == _container->end()) {
                                    position = 0;
                                }
                                else if (--position != 0) {
                                    _iterator++;
                                }
                            }
                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    }

                    return (_state == AT_ELEMENT);
                }

                bool Next()
                {
                    if (_container != nullptr) {
                        if (_state != AT_END) {
                            if (_state != AT_BEGINNING) {
                                _iterator++;
                            }

                            while ((_iterator != _container->end()) && (_iterator->IsSet() == false)) {
                                _iterator++;
                            }

                            _state = (_iterator != _container->end() ? AT_ELEMENT : AT_END);
                        }
                    } else {
                        _state = AT_END;
                    }
                    return (_state == AT_ELEMENT);
                }

                IElement* Element()
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (&(*_iterator));
                }

                ARRAYELEMENT& Current()
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator);
                }

                inline uint32_t Count() const
                {
                    return (_container == nullptr ? 0 : static_cast<uint32_t>(_container->size()));
                }

            private:
                ArrayContainer* _container;
                typename ArrayContainer::iterator _iterator;
                State _state;
            };

            typedef IteratorType<ELEMENT> Iterator;
            typedef ConstIteratorType<ELEMENT> ConstIterator;

        public:
            ArrayType()
                : _state(0)
                , _count(0)
                , _data()
                , _iterator(_data)
            {
            }

            ArrayType(ArrayType<ELEMENT>&& move)
                : _state(std::move(move._state))
                , _count(std::move(move._count))
                , _data(std::move(move._data))
                , _iterator(std::move(move._iterator))
            {
            }

            ArrayType(const ArrayType<ELEMENT>& copy)
                : _state(copy._state)
                , _count(copy._count)
                , _data(copy._data)
                , _iterator(_data)
            {
            }

            ~ArrayType() override = default;

            ArrayType<ELEMENT>& operator=(ArrayType<ELEMENT>&& move)
            {
                _state = std::move(move._state);
                _data = std::move(move._data);
                _iterator = IteratorType<ELEMENT>(_data);

                return (*this);
            }

            ArrayType<ELEMENT>& operator=(const ArrayType<ELEMENT>& RHS)
            {
                _state = RHS._state;
                _data = RHS._data;
                _iterator = IteratorType<ELEMENT>(_data);

                return (*this);
            }

        public:
            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return ( (Length() > 0) || ((_state & modus::SET) != 0) );
            }

            bool IsNull() const override
            {
                //TODO: Implement null for Arrays
                return ((_state & UNDEFINED) != 0);
            }

            void Set(const bool enabled)
            {
                if (enabled == true) {
                    _state |= (modus::SET);
                }
                else {
                    _state &= (~modus::SET);
                }
            }

            void Clear() override
            {
                _state = 0;
                _data.clear();
            }

            inline uint16_t Length() const
            {
                return static_cast<uint16_t>(_data.size());
            }

            inline ELEMENT& Add()
            {
                _data.push_back(ELEMENT());

                return (_data.back());
            }

            inline ELEMENT& Add(const ELEMENT& element)
            {
                _data.push_back(element);

                return (_data.back());
            }

            ELEMENT& operator[](const uint32_t index)
            {
                uint32_t skip = index;
                ASSERT(index < Length());

                typename std::list<ELEMENT>::iterator locator = _data.begin();

                while (skip != 0) {
                    locator++;
                    skip--;
                }

                ASSERT(locator != _data.end());

                return (*locator);
            }

            const ELEMENT& operator[](const uint32_t index) const
            {
                uint32_t skip = index;
                ASSERT(index < Length());

                typename std::list<ELEMENT>::const_iterator locator = _data.begin();

                while (skip != 0) {
                    locator++;
                    skip--;
                }

                ASSERT(locator != _data.end());

                return (*locator);
            }

            const ELEMENT& Get(const uint32_t index) const
            {
                return operator[](index);
            }

            inline Iterator Elements()
            {
                return (Iterator(_data));
            }

            inline ConstIterator Elements() const
            {
                return (ConstIterator(_data));
            }

            inline operator string() const
            {
                string result;
                ToString(result);
                return (result);
            }

            inline ArrayType<ELEMENT>& operator=(const string& RHS)
            {
                FromString(RHS);
                return (*this);
            }

            template<typename ENUM, typename std::enable_if<std::is_same<ELEMENT, EnumType<ENUM>>::value, int>::type = 0>
            inline ArrayType<ELEMENT>& operator=(const ENUM& RHS)
            {
                using T = typename std::underlying_type<ENUM>::type;
                T value(static_cast<T>(RHS));
                T bit(1);

                Clear();

                while (value != 0) {
                    if ((value & bit) != 0) {
                        Add() = static_cast<ENUM>(bit);
                        value &= ~bit;
                    }

                    bit <<= 1;
                }

                return (*this);
            }

            template<typename ENUM, typename std::enable_if<std::is_same<ELEMENT, EnumType<ENUM>>::value, int>::type = 0>
            inline operator const ENUM() const
            {
                using T = typename std::underlying_type<ENUM>::type;
                T value{};

                for (const EnumType<ENUM>& item : _data) {
                    if (item.IsSet() == true) {
                        const T& element = static_cast<const T>(item.Value());
                        ASSERT((element == 0) || ((element & (element - 1)) == 0));

                        value |= element;
                    }
                }

                return (static_cast<const ENUM>(value));
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                if (offset == FIND_MARKER) {
                    _iterator.Reset();
                    stream[loaded++] = '[';
                    offset = (_iterator.Next() == false ? ~0 : PARSE);
                } else if (offset == END_MARKER) {
                    offset = ~0;
                }
                while ((loaded < maxLength) && (offset != static_cast<uint32_t>(~0))) {
                    if (offset >= PARSE) {
                        offset -= PARSE;
                        loaded += static_cast<const IElement&>(_iterator.Current()).Serialize(&(stream[loaded]), maxLength - loaded, offset);
                        offset = (offset != FIND_MARKER ? offset + PARSE : (_iterator.Next() == true ? BEGIN_MARKER : ~0));
                    } else if (offset == BEGIN_MARKER) {
                        stream[loaded++] = ',';
                        offset = PARSE;
                    }
                }
                if (offset == static_cast<uint32_t>(~0)) {
                    if (loaded < maxLength) {
                        stream[loaded++] = ']';
                        offset = FIND_MARKER;
                    } else {
                        offset = END_MARKER;
                    }
                }

                return (loaded);
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;
                // Run till we find opening bracket..
                if (offset == FIND_MARKER) {
                    while ((loaded < maxLength) && ::isspace(stream[loaded])) {
                        loaded++;
                    }
                }

                if (loaded == maxLength) {
                    offset = FIND_MARKER;
                } else if (offset == FIND_MARKER) {
                    ValueValidity valid = stream[loaded] != '[' ? IsNullValue(stream, maxLength, offset, loaded) : ValueValidity::VALID;
                    offset = FIND_MARKER;
                    switch (valid) {
                    default:
                        // fall through
                    case ValueValidity::UNKNOWN:
                        break;
                    case ValueValidity::IS_NULL:
                        _state = UNDEFINED;
                        break;
                    case ValueValidity::INVALID:
                        error = Error{ "Invalid value.\"null\" or \"[\" expected." };
                        break;
                    case ValueValidity::VALID:
                        offset = SKIP_BEFORE;
                        loaded++;
                        break;
                    }
                }

                while ((offset != FIND_MARKER) && (loaded < maxLength)) {
                    if ((offset == SKIP_BEFORE) || (offset == SKIP_AFTER)) {
                        // Run till we find a character not a whitespace..
                        while ((loaded < maxLength) && (::isspace(stream[loaded]))) {
                            loaded++;
                        }

                        if (loaded < maxLength) {
                            switch (stream[loaded]) {
                            case ']':
                                offset = FIND_MARKER;
                                loaded++;
                                break;
                            case ',':
                                if (offset == SKIP_BEFORE) {
                                    _state = ERROR;
                                    error = Error{ "Expected new element, \",\" found." };
                                    offset = FIND_MARKER;
                                } else {
                                    offset = SKIP_BEFORE;
                                }
                                loaded++;
                                break;
                            default:
                                if (offset == SKIP_AFTER) {
                                    error = Error{ "Unexpected character \"" + std::string(1, stream[loaded]) + "\". Expected either \",\" or \"]\"" };
                                    offset = FIND_MARKER;
                                    ++loaded;
                                } else {
                                    offset = PARSE;
                                    _data.push_back(ELEMENT());
                                }
                                break;
                            }
                        }
                    }

                    if (offset >= PARSE) {
                        offset = (offset - PARSE);
                        loaded += static_cast<IElement&>(_data.back()).Deserialize(&(stream[loaded]), maxLength - loaded, offset, error);
                        offset = (offset == FIND_MARKER ? SKIP_AFTER : offset + PARSE);
                    }

                    if (error.IsSet() == true)
                        break;
                }

                return (loaded);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    _iterator.Reset();
                    if (_data.size() <= 15) {
                        stream[loaded++] = (0x90 | static_cast<uint8_t>(_data.size()));
                        if (_data.size() > 0) {
                            offset = PARSE;
                        }
                    } else {
                        stream[loaded++] = 0xDC;
                        offset = 1;
                    }
                    _iterator.Next();
                }
                while ((loaded < maxLength) && (offset > 0) && (offset < PARSE)) {
                    if (offset == 1) {
                        stream[loaded++] = (_data.size() >> 8) & 0xFF;
                        offset = 2;
                    } else if (offset == 2) {
                        stream[loaded++] = _data.size() & 0xFF;
                        offset = PARSE;
                    }
                }
                while ((loaded < maxLength) && (offset >= PARSE)) {
                    offset -= PARSE;
                    loaded += static_cast<const IMessagePack&>(_iterator.Current()).Serialize(&(stream[loaded]), maxLength - loaded, offset);
                    offset += PARSE;
                    if ((offset == PARSE) && (_iterator.Next() != true)) {
                        offset = 0;
                    }
                }

                return (loaded);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    if (stream[0] == IMessagePack::NullValue) {
                        _state = UNDEFINED;
                        loaded = 1;
                    } else if ((stream[0] & 0xF0) == 0x90) {
                        _count = (stream[0] & 0x0F);
                        offset = PARSE;
                    } else if (stream[0] & 0xDC) {
                        offset = 1;
                    }
                }

                while ((loaded < maxLength) && (offset > 0) && (offset < PARSE)) {
                    if (offset == 1) {
                        _count = (_count << 8) | stream[loaded++];
                        offset = 2;
                    } else if (offset == 2) {
                        _count = (_count << 8) | stream[loaded++];
                        offset = PARSE;
                    }
                }

                while ((loaded < maxLength) && (offset >= PARSE)) {

                    if (offset == PARSE) {
                        if (_count > 0) {
                            _count--;
                            _data.emplace_back(ELEMENT());
                        } else {
                            offset = 0;
                        }
                    }
                    if (offset >= PARSE) {
                        offset -= PARSE;
                        loaded += static_cast<IMessagePack&>(_data.back()).Deserialize(stream, maxLength, offset);
                        offset += PARSE;
                    }
                }

                return (loaded);
            }

        private:
            uint8_t _state;
            uint16_t _count;
            std::list<ELEMENT> _data;
            mutable IteratorType<ELEMENT> _iterator;
        };

        class EXTERNAL Container : public IElement, public IMessagePack {
        private:
            enum modus : uint8_t {
                ERROR = 0x80,
                UNDEFINED = 0x40
            };

            static constexpr uint16_t FIND_MARKER = 0;
            static constexpr uint16_t BEGIN_MARKER = 1;
            static constexpr uint16_t END_MARKER = 2;
            static constexpr uint16_t SKIP_BEFORE = 3;
            static constexpr uint16_t SKIP_BEFORE_VALUE = 4;
            static constexpr uint16_t SKIP_AFTER = 5;
            static constexpr uint16_t SKIP_AFTER_KEY = 6;
            static constexpr uint16_t PARSE = 7;

            typedef std::pair<const TCHAR*, IElement*> JSONLabelValue;
            typedef std::list<JSONLabelValue> JSONElementList;

            class Iterator {
            private:
                enum State {
                    AT_BEGINNING,
                    AT_ELEMENT,
                    AT_END
                };

            private:
                Iterator();
                Iterator(Iterator&&);
                Iterator(const Iterator&);
                Iterator& operator=(Iterator&&);
                Iterator& operator=(const Iterator&);

            public:
                Iterator(Container& parent, JSONElementList& container)
                    : _parent(parent)
                    , _container(container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }

                ~Iterator() = default;

            public:
                void Reset()
                {
                    _iterator = _container.begin();
                    _state = AT_BEGINNING;
                }

                bool Next()
                {
                    if (_state != AT_END) {
                        if (_state != AT_BEGINNING) {
                            _iterator++;
                        }

                        _state = (_iterator != _container.end() ? AT_ELEMENT : AT_END);
                    }
                    return (_state == AT_ELEMENT);
                }

                const TCHAR* Label() const
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator).first;
                }

                IElement* Element()
                {
                    ASSERT(_state == AT_ELEMENT);
                    ASSERT((*_iterator).second != nullptr);

                    return ((*_iterator).second);
                }

            private:
                Container& _parent;
                JSONElementList& _container;
                JSONElementList::iterator _iterator;
                State _state;
            };

        public:
            Container(Container&&) = delete;
            Container(const Container&) = delete;
            Container& operator=(Container&&) = delete;
            Container& operator=(const Container&) = delete;

            Container()
                : _state(0)
                , _count(0)
                , _data()
                , _iterator()
                , _fieldName(true)
            {
                ::memset(&_current, 0, sizeof(_current));
            }
            ~Container() override = default;

        public:
            bool HasLabel(const string& label) const
            {
                JSONElementList::const_iterator index(_data.begin());

                while ((index != _data.end()) && (index->first != label)) {
                    index++;
                }

                return (index != _data.end());
            }

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                JSONElementList::const_iterator index = _data.begin();
                // As long as we did not find a set element, continue..
                while ((index != _data.end()) && (index->second->IsSet() == false)) {
                    index++;
                }

                return (index != _data.end());
            }

            bool IsNull() const override
            {
                // TODO: Implement null for conrtainers
                return ((_state & UNDEFINED) != 0);
            }

            void Clear() override
            {
                JSONElementList::const_iterator index = _data.begin();

                // As long as we did not find a set element, continue..
                while (index != _data.end()) {
                    index->second->Clear();
                    index++;
                }
            }

            void Add(const TCHAR label[], IElement* element)
            {
                _data.push_back(JSONLabelValue(label, element));
            }

            void Remove(const TCHAR label[])
            {
                JSONElementList::iterator index(_data.begin());

                while ((index != _data.end()) && (index->first != label)) {
                    index++;
                }

                if (index != _data.end()) {
                    _data.erase(index);
                }
            }

            // IElement iface:
            uint16_t Serialize(char stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                if (offset == FIND_MARKER) {
                    _iterator = _data.begin();
                    stream[loaded++] = '{';

                    offset = (_iterator == _data.end() ? ~0 : ((_iterator->second->IsSet() == false) && (FindNext() == false)) ? ~0 : BEGIN_MARKER);
                    if (offset == BEGIN_MARKER) {
                        _fieldName = string(_iterator->first);
                        _current.json = &_fieldName;
                        offset = PARSE;
                    }
                } else if (offset == END_MARKER) {
                    offset = ~0;
                }

                while ((loaded < maxLength) && (offset != static_cast<uint32_t>(~0))) {
                    if (offset >= PARSE) {
                        offset -= PARSE;
                        loaded += _current.json->Serialize(&(stream[loaded]), maxLength - loaded, offset);
                        offset = (offset == FIND_MARKER ? BEGIN_MARKER : offset + PARSE);
                    } else if (offset == BEGIN_MARKER) {
                        if (_current.json == &_fieldName) {
                            stream[loaded++] = ':';
                            _current.json = _iterator->second;
                            offset = PARSE;
                        } else {
                            if (FindNext() != false) {
                                stream[loaded++] = ',';
                                _fieldName = string(_iterator->first);
                                _current.json = &_fieldName;
                                offset = PARSE;
                            } else {
                                offset = ~0;
                            }
                        }
                    }
                }
                if (offset == static_cast<uint32_t>(~0)) {
                    if (loaded < maxLength) {
                        stream[loaded++] = '}';
                        offset = FIND_MARKER;
                        _fieldName.Clear();
                    } else {
                        offset = END_MARKER;
                    }
                }

                return (loaded);
            }

            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;
                // Run till we find opening bracket..
                if (offset == FIND_MARKER) {
                    while ((loaded < maxLength) && (::isspace(stream[loaded]))) {
                        loaded++;
                    }
                }

                if (loaded == maxLength) {
                    offset = FIND_MARKER;
                } else if (offset == FIND_MARKER) {
                    ValueValidity valid = stream[loaded] != '{' ? IsNullValue(stream, maxLength, offset, loaded) : ValueValidity::VALID;
                    offset = FIND_MARKER;
                    switch (valid) {
                    default:
                        // fall through
                    case ValueValidity::UNKNOWN:
                        break;
                    case ValueValidity::IS_NULL:
                        _state = UNDEFINED;
                        break;
                    case ValueValidity::INVALID:
                        error = Error{ "Invalid value.\"null\" or \"{\" expected." };
                        break;
                    case ValueValidity::VALID:
                        loaded++;
                        _fieldName.Clear();
                        offset = SKIP_BEFORE;
                        break;
                    }
                }

                while ((offset != FIND_MARKER) && (loaded < maxLength)) {
                    if ((offset == SKIP_BEFORE) || (offset == SKIP_AFTER) || offset == SKIP_BEFORE_VALUE || offset == SKIP_AFTER_KEY) {
                        // Run till we find a character not a whitespace..
                        while ((loaded < maxLength) && (::isspace(stream[loaded]))) {
                            loaded++;
                        }

                        if (loaded < maxLength) {
                            switch (stream[loaded]) {
                            case '}':
                                if (offset == SKIP_BEFORE && !_data.empty()) {
                                    _state = ERROR;
                                    error = Error{ "Expected new element, \"}\" found." };
                                } else if (offset == SKIP_BEFORE_VALUE || offset == SKIP_AFTER_KEY) {
                                    _state = ERROR;
                                    error = Error{ "Expected value, \"}\" found." };
                                }
                                offset = FIND_MARKER;
                                loaded++;
                                break;
                            case ',':
                                if (offset == SKIP_BEFORE) {
                                    _state = ERROR;
                                    error = Error{ "Expected new element \",\" found." };
                                    offset = FIND_MARKER;
                                } else if (offset == SKIP_BEFORE_VALUE || offset == SKIP_AFTER_KEY) {
                                    _state = ERROR;
                                    error = Error{ "Expected value, \",\" found." };
                                    offset = FIND_MARKER;
                                } else {
                                    offset = SKIP_BEFORE;
                                }
                                loaded++;
                                break;
                            case ':':
                                if (offset == SKIP_BEFORE || offset == SKIP_BEFORE_VALUE) {
                                    _state = ERROR;
                                    error = Error{ "Expected " + std::string{ offset == SKIP_BEFORE_VALUE ? "value" : "new element" } + ", \":\" found." };
                                    offset = FIND_MARKER;
                                } else if (_fieldName.IsSet() == false) {
                                    _state = ERROR;
                                    error = Error{ "Expected \"}\" or \",\", \":\" found." };
                                    offset = FIND_MARKER;
                                } else {
                                    offset = SKIP_BEFORE_VALUE;
                                }
                                loaded++;
                                break;
                            default:
                                if (_fieldName.IsSet() == true) {
                                    if (_current.json != nullptr) {
                                        _state = ERROR;
                                        // This is not a critical error. It happens when config contains more/different
                                        // things as the one "registered".
                                        // error = Error{"Internal parser error."};
                                        // offset = 0;
                                        // break;
                                    } else if (offset != SKIP_BEFORE_VALUE) {
                                        _state = ERROR;
                                        error = Error{ "Colon expected." };
                                        offset = FIND_MARKER;
                                        ++loaded;
                                        break;
                                    }
                                    _current.json = Find(_fieldName.Value().c_str());

                                    _fieldName.Clear();

                                    if (_current.json == nullptr) {
                                        _current.json = &_fieldName;
                                    }
                                } else {
                                    if (offset == SKIP_AFTER || offset == SKIP_AFTER_KEY) {
                                        _state = ERROR;
                                        error = Error{ "Expected either \",\" or \"}\", \"" + std::string(1, stream[loaded]) + "\" found." };
                                        offset = FIND_MARKER;
                                        ++loaded;
                                        break;
                                    }
                                    _current.json = nullptr;
                                }
                                offset = PARSE;
                                break;
                            }
                        }
                    }

                    if (offset >= PARSE) {
                        offset = (offset - PARSE);
                        uint16_t skip = SKIP_AFTER;
                        if (_current.json == nullptr) {
                            loaded += static_cast<IElement&>(_fieldName).Deserialize(&(stream[loaded]), maxLength - loaded, offset, error);
                            if (_fieldName.IsQuoted() == false) {
                                error = Error{ "Key must be properly quoted." };
                            }
                            skip = SKIP_AFTER_KEY;
                        } else {
                            loaded += _current.json->Deserialize(&(stream[loaded]), maxLength - loaded, offset, error);
                        }
                        offset = (offset == FIND_MARKER ? skip : offset + PARSE);
                    }

                    if (error.IsSet() == true)
                        break;
                }

                // This is done for containers only using the fact the top most JSON element is a container.
                // This make sure the parsing error at any level results in an empty C++ objects and context
                // is as full as possible.
                if (error.IsSet() == true) {
                    Clear();
                    error.Value().Context(stream, maxLength, loaded);
                }

                return (loaded);
            }

            // IMessagePack iface:
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint32_t& offset) const override
            {
                uint16_t loaded = 0;

                uint16_t elementSize = Size();
                if (offset == 0) {
                    _iterator = _data.begin();
                    if (elementSize <= 15) {
                        stream[loaded++] = (0x80 | static_cast<uint8_t>(Size()));
                        if (_iterator != _data.end()) {
                            offset = PARSE;
                        }
                    } else {
                        stream[loaded++] = 0xDE;
                        offset = 1;
                    }
                    if (offset != 0) {
                        if ((_iterator->second->IsSet() == false) && (FindNext() == false)) {
                            offset = 0;
                        } else {
                            _fieldName = string(_iterator->first);
                        }
                    }
                }
                while ((loaded < maxLength) && (offset > 0) && (offset < PARSE)) {
                    if (offset == 1) {
                        stream[loaded++] = (elementSize >> 8) & 0xFF;
                        offset = 2;
                    } else if (offset == 2) {
                        stream[loaded++] = elementSize & 0xFF;
                        offset = PARSE;
                    }
                }
                while ((loaded < maxLength) && (offset >= PARSE)) {
                    offset -= PARSE;
                    if (_fieldName.IsSet() == true) {
                        loaded += static_cast<const IMessagePack&>(_fieldName).Serialize(&(stream[loaded]), maxLength - loaded, offset);
                        if (offset == 0) {
                            _fieldName.Clear();
                        }
                        offset += PARSE;
                    } else {
                        const IMessagePack* element = dynamic_cast<const IMessagePack*>(_iterator->second);
                        if (element != nullptr) {
                            loaded += element->Serialize(&(stream[loaded]), maxLength - loaded, offset);
                            if (offset == 0) {
                                _fieldName.Clear();
                            }
                        } else {
                            stream[loaded++] = IMessagePack::NullValue;
                        }
                        offset += PARSE;
                        if (offset == PARSE) {
                            if (FindNext() != false) {
                                _fieldName = string(_iterator->first);
                            } else {
                               offset = 0;
                               _fieldName.Clear();
                            }
                        }
                    }
                }

                return (loaded);
            }

            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint32_t& offset) override
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    if (stream[0] == IMessagePack::NullValue) {
                        _state = UNDEFINED;
                    } else if ((stream[0] & 0x80) == 0x80) {
                        _count = (stream[0] & 0x0F);
                        offset = (_count > 0 ? PARSE : 0);
                    } else if (stream[0] & 0xDE) {
                        offset = 1;
                    }
                    loaded = 1;
                }

                while ((loaded < maxLength) && (offset > 0) && (offset < PARSE)) {
                    if (offset == 1) {
                        _count = (_count << 8) | stream[loaded++];
                        offset = 2;
                    } else if (offset == 2) {
                        _count = (_count << 8) | stream[loaded++];
                        offset = (_count > 0 ? PARSE : 0);
                    }
                }

                while ((loaded < maxLength) && (offset >= PARSE)) {

                    if (_fieldName.IsSet() == true) {
                        if (_current.pack != nullptr) {
                            offset -= PARSE;
                            loaded += _current.pack->Deserialize(&stream[loaded], maxLength - loaded, offset);
                            offset += PARSE;
                            if (offset == PARSE) {
                                _fieldName.Clear();
                            // Seems like another field is completed. Reduce the count
                                _count--;
                                if (_count == 0) {
                                    offset = 0;
                                }
                            }
                        } else {
                            _current.pack = dynamic_cast<IMessagePack*>(Find(_fieldName.Value().c_str()));
                            if (_current.pack == nullptr) {
                                _current.pack = &(static_cast<IMessagePack&>(_fieldName));
                            }
                        }
                    } else {
                        offset -= PARSE;
                        loaded += static_cast<IMessagePack&>(_fieldName).Deserialize(&stream[loaded], maxLength - loaded, offset);
                        offset += PARSE;
                        _current.pack = nullptr;
                    }
                }

                return (loaded);
            }

        protected:
            void Reset()
            {
                _data.clear();
            }

            IElement* Find(const char label[])
            {
                IElement* result = nullptr;

                JSONElementList::iterator index = _data.begin();

                while ((index != _data.end()) && (strcmp(label, index->first) != 0)) {
                    index++;
                }

                if (index != _data.end()) {
                    result = index->second;
                }
                else if (Request(label) == true) {
                    index = _data.end();

                    while ((result == nullptr) && (index != _data.begin())) {
                        index--;
                        if (strcmp(label, index->first) == 0) {
                            result = index->second;
                        }
                    }
                }
                return (result);
            }

            bool FindNext() const
            {
                _iterator++;
                while ((_iterator != _data.end()) && (_iterator->second->IsSet() == false)) {
                    _iterator++;
                }
                return (_iterator != _data.end());
            }

            uint16_t Size() const
            {
                uint16_t count = 0;
                if (_data.size() > 0) {
                    JSONElementList::const_iterator iterator = _data.begin();
                    while (iterator != _data.end()) {
                        if (iterator->second->IsSet() != false) {
                            count++;
                        }
                        iterator++;
                    }
                }
                return count;
            }

            virtual bool Request(const TCHAR [])
            {
                return (false);
            }

        private:
            uint8_t _state;
            uint16_t _count;
            union {
                mutable IElement* json;
                mutable IMessagePack* pack;
            } _current;
            JSONElementList _data;
            mutable JSONElementList::const_iterator _iterator;
            mutable String _fieldName;
        };

        class VariantContainer;

        class EXTERNAL Variant : public JSON::String {
        public:
            enum class type {
                EMPTY,
                BOOLEAN,
                NUMBER,
                STRING,
                ARRAY,
                OBJECT,
                FLOAT,
                DOUBLE
            };

        public:
            Variant()
                : JSON::String(false)
                , _type(type::EMPTY)
            {
                String::operator=("null");
            }

            Variant(const int32_t value)
                : JSON::String(false)
                , _type(type::NUMBER)
            {
                String::operator=(Core::NumberType<int32_t, true, NumberBase::BASE_DECIMAL>(value).Text());
            }

            Variant(const int64_t value)
                : JSON::String(false)
                , _type(type::NUMBER)
            {
                String::operator=(Core::NumberType<int64_t, true, NumberBase::BASE_DECIMAL>(value).Text());
            }

            Variant(const uint32_t value)
                : JSON::String(false)
                , _type(type::NUMBER)
            {
                String::operator=(Core::NumberType<uint32_t, false, NumberBase::BASE_DECIMAL>(value).Text());
            }

            Variant(const uint64_t value)
                : JSON::String(false)
                , _type(type::NUMBER)
            {
                String::operator=(Core::NumberType<uint64_t, false, NumberBase::BASE_DECIMAL>(value).Text());
            }

            Variant(const float value)
                : JSON::String(false)
                , _type(type::FLOAT)
            {
                string result;
                JSON::Float(value).ToString(result);
                String::operator=(result);
            }

            Variant(const double value)
                : JSON::String(false)
                , _type(type::DOUBLE)
            {
                string result;
                JSON::Double(value).ToString(result);
                String::operator=(result);
            }

            Variant(const bool value)
                : JSON::String(false)
                , _type(type::BOOLEAN)
            {
                String::operator=(value ? _T("true") : _T("false"));
            }

            Variant(const string& text)
                : JSON::String(true)
                , _type(type::STRING)
            {
                String::operator=(text);
            }

            Variant(const TCHAR* text)
                : JSON::String(true)
                , _type(type::STRING)
            {
                String::operator=(text);
            }

            Variant(Variant&& move)
                : JSON::String(std::move(move))
                , _type(std::move(move._type))
            {
            }
            Variant(const Variant& copy)
                : JSON::String(copy)
                , _type(copy._type)
            {
            }


            Variant(const VariantContainer& object);

            ~Variant() override = default;

            Variant& operator=(Variant&& move)
            {
                JSON::String::operator=(std::move(move));
                _type = std::move(move._type);
                return (*this);
            }

            Variant& operator=(const Variant& RHS)
            {
                JSON::String::operator=(RHS);
                _type = RHS._type;
                return (*this);
            }

        public:
            type Content() const
            {
                return _type;
            }

            bool Boolean() const
            {
                bool result = false;
                if (_type == type::BOOLEAN) {
                    result = (Value() == "true");
                }
                return result;
            }

            int64_t Number() const
            {
                int64_t result = 0;
                if (_type == type::NUMBER) {
                    result = Core::NumberType<int64_t>(Value().c_str(), static_cast<uint32_t>(Value().length()));
                } else if (_type == type::FLOAT) {
                    result = static_cast<int64_t>(Float());
                } else if (_type == type::DOUBLE) {
                    result = static_cast<int64_t>(Double());
                }
                return result;
            }

            float Float() const
            {
                float result = 0.0f;
                if (_type == type::NUMBER) {
                    result = static_cast<float>(Number());
                } else if (_type == type::FLOAT) {
                    JSON::Float value;
                    if (value.FromString(Value())) {
                        result = value.Value();
                    }
                } else if (_type == type::DOUBLE) {
                    result = static_cast<float>(Double());
                }
                return result;
            }

            double Double() const
            {
                double result = 0.0;
                if (_type == type::NUMBER) {
                    result = static_cast<double>(Number());
                } else if (_type == type::FLOAT) {
                    result = static_cast<double>(Float());
                } else if (_type == type::DOUBLE) {
                    JSON::Double value;
                    if (value.FromString(Value())) {
                        result = value.Value();
                    }
                }
                return result;
            }

            const string String() const
            {
                return Value();
            }

            ArrayType<Variant> Array() const
            {
                ArrayType<Variant> result;

                result.FromString(Value());

                return result;
            }

            inline VariantContainer Object() const;

            void Boolean(const bool value)
            {
                _type = type::BOOLEAN;
                String::SetQuoted(false);
                String::operator=(value ? _T("true") : _T("false"));
            }

            template <typename TYPE>
            void Number(const TYPE value)
            {
                _type = type::NUMBER;
                String::SetQuoted(false);
                String::operator=(Core::NumberType<TYPE>(value).Text());
            }

            void Number(const float value)
            {
                _type = type::FLOAT;
                String::SetQuoted(false);
                string result;
                JSON::Float(value).ToString(result);
                String::operator=(result);
            }

            void Number(const double value)
            {
                _type = type::DOUBLE;
                String::SetQuoted(false);
                string result;
                JSON::Double(value).ToString(result);
                String::operator=(result);
            }

            void String(const TCHAR* value)
            {
                _type = type::STRING;
                String::SetQuoted(true);
                String::operator=(value);
            }

            void Array(const ArrayType<Variant>& value)
            {
                _type = type::ARRAY;
                string data;
                value.ToString(data);
                String::SetQuoted(false);
                String::operator=(data);
            }

            inline void Object(const VariantContainer& object);

            template <typename VALUE>
            Variant& operator=(const VALUE& value)
            {
                Number(value);
                return (*this);
            }

            Variant& operator=(const bool& value)
            {
                Boolean(value);
                return (*this);
            }

            Variant& operator=(const string& value)
            {
                String(value.c_str());
                return (*this);
            }

            Variant& operator=(const TCHAR value[])
            {
                String(value);
                return (*this);
            }

            Variant& operator=(const ArrayType<Variant>& value)
            {
                Array(value);
                return (*this);
            }

            Variant& operator=(const VariantContainer& value)
            {
                Object(value);
                return (*this);
            }

            inline void ToString(string& result) const;

            // IElement and IMessagePack iface:
            bool IsSet() const override
            {
                return String::IsSet();
            }

            string GetDebugString(const TCHAR name[], int indent = 0, int arrayIndex = -1) const;

        private:
            // IElement iface:
            uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override;

            static uint16_t FindEndOfScope(const char stream[], uint16_t maxLength)
            {
                ASSERT(maxLength > 0 && (stream[0] == '{' || stream[0] == '['));
                char charOpen = stream[0];
                char charClose = charOpen == '{' ? '}' : ']';
                uint16_t stack = 1;
                uint16_t endIndex = 0;
                bool insideQuotes = false;
                for (uint16_t i = 1; i < maxLength; ++i) {
                    if ((stream[i] == '\"') && ((stream[i - 1] != '\\') || ((stream[i - 1] == '\\') && (stream[i - 2] == '\\')))) {
                        insideQuotes = !insideQuotes;
                    }
                    if (!insideQuotes) {
                        if (stream[i] == charClose)
                            stack--;
                        else if (stream[i] == charOpen)
                            stack++;
                        if (stack == 0) {
                            endIndex = i;
                            break;
                        }
                    }
                }
                return endIndex;
            }

        private:
            type _type;
        };

        class EXTERNAL VariantContainer : public Container {
        private:
            using Elements = std::unordered_map<string, WPEFramework::Core::JSON::Variant>;

        public:
            class Iterator {
            public:
                Iterator()
                    : _container(nullptr)
                    , _index()
                    , _start(true)
                {
                }

                Iterator(const Elements& container)
                    : _container(&container)
                    , _index(_container->begin())
                    , _start(true)
                {
                }

                Iterator(Iterator&& move)
                    : _container(std::move(move._container))
                    , _index(std::move(move._index))
                    , _start(std::move(move._start))
                {
                    if (_container != nullptr) {
                        _index = _container->begin();
                    }
                }

                Iterator(const Iterator& copy)
                    : _container(copy._container)
                    , _index()
                    , _start(true)
                {
                    if (_container != nullptr) {
                        _index = _container->begin();
                    }
                }

                ~Iterator() = default;

                Iterator& operator=(Iterator&& move)
                {
                    _container = std::move(move._container);
                    _index = std::move(move._index);
                    _start = std::move(move._start);

                    return (*this);
                }

                Iterator& operator=(const Iterator& rhs)
                {
                    _container = rhs._container;
                    _index = rhs._index;
                    _start = rhs._start;

                    return (*this);
                }


            public:
                bool IsValid() const
                {
                    return ((_container != nullptr) && (_start == false) && (_index != _container->end()));
                }

                void Reset()
                {
                    if (_container != nullptr) {
                        _start = true;
                        _index = _container->begin();
                    }
                }

                bool Next()
                {
                    if (_container != nullptr) {
                        if (_start == true) {
                            _start = false;
                        } else if (_index != _container->end()) {
                            _index++;
                        }
                        return (_index != _container->end());
                    }

                    return (false);
                }

                const TCHAR* Label() const
                {
                    return (_index->first.c_str());
                }

                const JSON::Variant& Current() const
                {
                    return (_index->second);
                }

            private:
                const Elements* _container;
                Elements::const_iterator _index;
                bool _start;
            };

        public:
            VariantContainer()
                : Container()
                , _elements()
            {
            }

            VariantContainer(const TCHAR serialized[])
                : Container()
                , _elements()
            {
                Container::FromString(serialized);
            }

            VariantContainer(const string& serialized)
                : Container()
                , _elements()
            {
                Container::FromString(serialized);
            }

            VariantContainer(VariantContainer&& move)
                : Container()
                , _elements(std::move(move._elements))
            {
                Elements::iterator index(_elements.begin());

                while (index != _elements.end()) {
                    ASSERT (HasLabel(index->first.c_str()));
                    Container::Add(index->first.c_str(), &(index->second));
                    index++;
                }
            }

            VariantContainer(const VariantContainer& copy)
                : Container()
                , _elements(copy._elements)
            {
                Elements::iterator index(_elements.begin());

                while (index != _elements.end()) {
                   if (copy.HasLabel(index->first.c_str())) {
                        Container::Add(index->first.c_str(), &(index->second));
                    }
                    index++;
                }
            }

            VariantContainer(const Elements& values)
                : Container()
            {
                Elements::const_iterator index(values.begin());

                while (index != values.end()) {
                    Elements::iterator index = _elements.emplace(std::piecewise_construct,
                        std::forward_as_tuple(index->first),
                        std::forward_as_tuple(index->second)).first;
                    Container::Add(index->first.c_str(), &(index->second));
                    index++;
                }
            }

            ~VariantContainer() override = default;

        public:
            VariantContainer& operator=(const VariantContainer& rhs)
            {
                // combine the labels
                Elements::iterator index(_elements.begin());

                // First copy all existing ones over, if the rhs does not have a value, remove the entry.
                while (index != _elements.end()) {
                    // Check if the rhs, has these..
                    Elements::const_iterator rhs_index(rhs.Find(index->first.c_str()));

                    if (rhs_index != rhs._elements.end()) {
                        // This is a valid element, copy the value..
                        index->second = rhs_index->second;
                        index++;
                    } else {
                        // This element does not exist on the other side..
                        Container::Remove(index->first.c_str());
                        index = _elements.erase(index);
                    }
                }

                Elements::const_iterator rhs_index(rhs._elements.begin());

                // Now add the ones we are missing from the RHS
                while (rhs_index != rhs._elements.end()) {
                    if (Find(rhs_index->first.c_str()) == _elements.end()) {
                        index = _elements.emplace(std::piecewise_construct,
                            std::forward_as_tuple(rhs_index->first),
                            std::forward_as_tuple(rhs_index->second)).first;
                        Container::Add(index->first.c_str(), &index->second);
                    }
                    rhs_index++;
                }

                return (*this);
            }



            void Set(const TCHAR fieldName[], const JSON::Variant& value)
            {
                Elements::iterator index(Find(fieldName));
                if (index != _elements.end()) {
                    index->second = value;
                } else {
                    index = _elements.emplace(std::piecewise_construct,
                        std::forward_as_tuple(fieldName),
                        std::forward_as_tuple(value)).first;
                    Container::Add(index->first.c_str(), &(index->second));
                }
            }

            Variant Get(const TCHAR fieldName[]) const
            {
                JSON::Variant result;

                Elements::const_iterator index(Find(fieldName));

                if (index != _elements.end()) {
                    result = index->second;
                }

                return (result);
            }

            JSON::Variant& operator[](const TCHAR fieldName[])
            {
                Elements::iterator index(Find(fieldName));

                if (index == _elements.end()) {
                    index = _elements.emplace(std::piecewise_construct,
                        std::forward_as_tuple(fieldName),
                        std::forward_as_tuple()).first;
                    Container::Add(index->first.c_str(), &(index->second));
                }

                return (index->second);
            }

            const JSON::Variant& operator[](const TCHAR fieldName[]) const
            {
                static const JSON::Variant emptyVariant;

                Elements::const_iterator index(Find(fieldName));

                return (index == _elements.end() ? emptyVariant : index->second);
            }

            bool HasLabel(const TCHAR labelName[]) const
            {
                return (Find(labelName) != _elements.end());
            }

            Iterator Variants() const
            {
                return (Iterator(_elements));
            }

            void Clear()
            {
                Reset();
                _elements.clear();
            }
            string GetDebugString(int indent = 0) const;

        private:
            Elements::iterator Find(const TCHAR fieldName[])
            {
                return (_elements.find(fieldName));
            }

            Elements::const_iterator Find(const TCHAR fieldName[]) const
            {
                return (_elements.find(fieldName));
            }

            // Container iface:
            bool Request(const TCHAR label[]) override
            {
                // Whetever comes in and has no counter part, we need to create a Variant for it, so
                // it can be filled.
                Elements::iterator  index = _elements.emplace(std::piecewise_construct,
                    std::forward_as_tuple(label),
                    std::forward_as_tuple()).first;

                Container::Add(index->first.c_str(), &(index->second));

                return (true);
            }

        private:
            Elements _elements;
        };

        inline Variant::Variant(const VariantContainer& object)
            : JSON::String(false)
            , _type(type::OBJECT)
        {
            string value;
            object.ToString(value);
            String::operator=(value);
        }

        inline void Variant::Object(const VariantContainer& object)
        {
            _type = type::OBJECT;
            string value;
            object.ToString(value);
            String::operator=(value);
        }

        inline VariantContainer Variant::Object() const
        {
            VariantContainer result;
            result.FromString(Value());
            return (result);
        }

        inline uint16_t Variant::Deserialize(const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error)
        {
            uint16_t result = 0;
            if (stream[0] == '{' || stream[0] == '[') {
                uint16_t endIndex = FindEndOfScope(stream, maxLength);
                if (endIndex > 0 && endIndex < maxLength) {
                    result = endIndex + 1;
                    SetQuoted(false);
                    string str(stream, endIndex + 1);
                    if (stream[0] == '{') {
                        _type = type::OBJECT;
                        String::operator=(str);
                    } else {
                        _type = type::ARRAY;
                        String::operator=(str);
                    }
                }
            }
            if (result == 0) {
                result = String::Deserialize(stream, maxLength, offset, error);

                _type = type::STRING;

                // If we are complete, try to guess what it was that we received...
                if (offset == 0) {
                    bool quoted = IsQuoted();
                    SetQuoted(quoted);
                    // If it is not quoted, it can be a boolean or a number...
                    if (quoted == false) {
                        if ((Value() == _T("true")) || (Value() == _T("false"))) {
                            _type = type::BOOLEAN;
                        } else if (IsNull() == false) {
                            _type = type::NUMBER;
                        }
                    }
                }
            }
            return (result);
        }

        template <uint16_t SIZE, typename INSTANCEOBJECT>
        class Tester {
        private:
            using ThisClass = Tester<SIZE, INSTANCEOBJECT>;

        public:
            Tester(Tester<SIZE, INSTANCEOBJECT>&&) = delete;
            Tester(const Tester<SIZE, INSTANCEOBJECT>&) = delete;
            Tester<SIZE, INSTANCEOBJECT>& operator=(Tester<SIZE, INSTANCEOBJECT>&&) = delete;
            Tester<SIZE, INSTANCEOBJECT>& operator=(const Tester<SIZE, INSTANCEOBJECT>&) = delete;

            Tester() = default;
            ~Tester() = default;

            bool FromString(const string& value, Core::ProxyType<INSTANCEOBJECT>& receptor)
            {
                uint16_t fillCount = 0;
                uint32_t offset = 0;
                uint16_t size, loaded;

                receptor->Clear();
                Core::OptionalType<Error> error;

                do {
                    size = static_cast<uint16_t>((value.size() - fillCount) < SIZE ? (value.size() - fillCount) : SIZE);

                    // Prepare the deserialize buffer
                    memcpy(_buffer, &(value.data()[fillCount]), size);

                    fillCount += size;

                    loaded = static_cast<IElement&>(*receptor).Deserialize(_buffer, size, offset, error);

                    ASSERT(loaded <= size);

                } while ((loaded == size) && (offset != 0));

                return (error.IsSet() == false);
            }

            bool ToString(const Core::ProxyType<INSTANCEOBJECT>& receptor, string& value)
            {
                uint32_t offset = 0;
                uint16_t loaded;

                // Serialize object
                do {

                    loaded = static_cast<Core::JSON::IElement&>(*receptor).Serialize(_buffer, SIZE, offset);

                    ASSERT(loaded <= SIZE);

                    value += string(_buffer, loaded);

                } while ((loaded == SIZE) && (offset != 0));

                return (offset == 0);
            }

        private:
            char _buffer[SIZE];
        };

        template <typename JSONOBJECT>
        class LabelType : public JSONOBJECT {
        public:
            LabelType(LabelType<JSONOBJECT>&&) = delete;
            LabelType(const LabelType<JSONOBJECT>&) = delete;
            LabelType<JSONOBJECT>& operator=(LabelType<JSONOBJECT>&&) = delete;
            LabelType<JSONOBJECT>& operator=(const LabelType<JSONOBJECT>&) = delete;

            LabelType()
                : JSONOBJECT()
            {
            }
            ~LabelType() override = default;

        public:
            static const char* Id()
            {
                return (__ID<JSONOBJECT>());
            }
            virtual const char* Label() const
            {
                return (LabelType<JSONOBJECT>::Id());
            }
            void Clear() {
                __Clear();
            }

        private:
            IS_MEMBER_AVAILABLE(Id, hasID);

            template <typename TYPE = JSONOBJECT>
            static inline typename Core::TypeTraits::enable_if<hasID<TYPE, const char*>::value, const char*>::type __ID()
            {
                return (JSONOBJECT::Id());
            }

            template <typename TYPE = JSONOBJECT>
            static inline typename Core::TypeTraits::enable_if<!hasID<TYPE, const char*>::value, const char*>::type __ID()
            {
                static std::string className = (Core::ClassNameOnly(typeid(JSONOBJECT).name()).Text());

                return (className.c_str());
            }

            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Clear, hasClear);

            template <typename TYPE = JSONOBJECT>
            inline typename Core::TypeTraits::enable_if<hasID<TYPE, void>::value, void>::type
                __Clear()
            {
                TYPE::Clear();
            }
            template <typename TYPE = JSONOBJECT>
            inline typename Core::TypeTraits::enable_if<!hasID<TYPE, void>::value, void>::type
                __Clear()
            {
            }
        };

    } // namespace JSON
} // namespace Core
} // namespace WPEFramework

using JsonObject = WPEFramework::Core::JSON::VariantContainer;
using JsonValue = WPEFramework::Core::JSON::Variant;
using JsonArray = WPEFramework::Core::JSON::ArrayType<JsonValue>;

#endif // __JSON_H

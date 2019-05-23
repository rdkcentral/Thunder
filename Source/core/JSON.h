#ifndef __JSON_H
#define __JSON_H

#include <map>

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

        struct EXTERNAL IElement {

            static char NullTag[];

            virtual ~IElement() {}

            template <typename INSTANCEOBJECT>
            static bool ToString(const INSTANCEOBJECT& realObject, string& text)
            {
                char buffer[1024];
                uint16_t loaded;
                uint16_t offset = 0;

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
                uint16_t offset = 0;

                realObject.Clear();

                if (text.empty() == false) {
                    // Deserialize object
                    uint16_t loaded = static_cast<IElement&>(realObject).Deserialize(text.c_str(), static_cast<uint16_t>(text.length()), offset);

                    ASSERT(loaded <= text.length());
                    DEBUG_VARIABLE(loaded);
                }

                return (offset == 0);
            }

            inline bool ToString(string& text) const
            {
                return (Core::JSON::IElement::ToString(*this, text));
            }
            inline bool FromString(const string& text)
            {
                return (Core::JSON::IElement::FromString(text, *this));
            }

            template <typename INSTANCEOBJECT>
            static bool ToFile(Core::File& fileObject, const INSTANCEOBJECT& realObject)
            {
                bool completed = false;

                if (fileObject.IsOpen()) {

                    char buffer[1024];
                    uint16_t loaded;
                    uint16_t offset = 0;

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
                bool completed = false;

                if (fileObject.IsOpen()) {

                    char buffer[1024];
                    uint16_t readBytes;
                    uint16_t loaded;
                    uint16_t offset = 0;

                    realObject.Clear();

                    // Serialize object
                    do {
                        readBytes = static_cast<uint16_t>(fileObject.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer)));

						if (readBytes == 0) {
                            loaded = ~0;
						} else {
                            loaded = static_cast<IElement&>(realObject).Deserialize(buffer, sizeof(buffer), offset);

                            ASSERT(loaded <= readBytes);

                            if (loaded != readBytes) {
                                fileObject.Position(true, -(readBytes - loaded));
                            }
                        }

                    } while ((loaded == readBytes) && (offset != 0));

                    completed = (offset == 0);
                }

                return (completed);
            }

            bool ToFile(Core::File& fileObject) const
            {
                return (Core::JSON::IElement::ToFile(fileObject, *this));
            }
            bool FromFile(Core::File& fileObject)
            {
                return (Core::JSON::IElement::FromFile(fileObject, *this));
            }

            // JSON Serialization interface
            // --------------------------------------------------------------------------------
            virtual void Clear() = 0;
            virtual bool IsSet() const = 0;

            virtual uint16_t Serialize(char Stream[], const uint16_t MaxLength, uint16_t& offset) const = 0;
            virtual uint16_t Deserialize(const char Stream[], const uint16_t MaxLength, uint16_t& offset) = 0;
        };

        struct EXTERNAL IMessagePack {
            virtual ~IMessagePack() {}

            // JSON Serialization interface
            // --------------------------------------------------------------------------------
            virtual void Clear() = 0;
            virtual bool IsSet() const = 0;

            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint16_t& offset) const = 0;
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint16_t& offset) = 0;
        };

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
            NumberType(const NumberType<TYPE, SIGNED, BASETYPE>& copy)
                : _set(copy._set)
                , _value(copy._value)
                , _default(copy._default)
            {
            }
            virtual ~NumberType()
            {
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
                return _value;
            }
            bool Null() const
            {
                return ((_set & UNDEFINED) != 0);
            }
            void Null(const bool enabled)
            {
                if (enabled == true)
                    _set |= UNDEFINED;
                else
                    _set &= ~UNDEFINED;
            }
            virtual bool IsSet() const override
            {
                return ((_set & (SET | UNDEFINED)) != 0);
            }
            virtual void Clear() override
            {
                _set = 0;
                _value = 0;
            }

        private:
            // If this should be serialized/deserialized, it is indicated by a MinSize > 0)
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const
            {
                uint16_t loaded = 0;

                ASSERT(maxLength > 0);

                while ((offset < 4) && (loaded < maxLength)) {

                    if ((_set & UNDEFINED) != 0) {
                        stream[loaded++] = IElement::NullTag[offset++];
                        if (offset == 4) {
                            offset = 0;
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

                if (loaded < maxLength) {
                    loaded += Convert(&(stream[loaded]), (maxLength - loaded), offset, TemplateIntToType<SIGNED>());
                }
                if ((offset != 0) && (loaded < maxLength)) {
                    stream[loaded++] = '\"';
                    offset = 0;
                }

                return (loaded);
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset)
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    // We are starting, see what the current char is
                    _value = 0;
                    _set = 0;
                }
                while ((offset < 4) && (loaded < maxLength)) {
                    if (offset == 0) {
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
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 1) {
                        ASSERT(_set = QUOTED);
                        if (stream[loaded] == '0') {
                            offset = 2;
                        } else if (stream[loaded] == '-') {
                            _set |= NEGATIVE;
                            offset = 3;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set |= DECIMAL;
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'u')) {
                            offset = 2;
                        } else {
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 2) {
                        ASSERT(_set = QUOTED);
                        if (stream[loaded] == '0') {
                            ASSERT(_set & NEGATIVE);
                            offset = 3;
                        } else if (::toupper(stream[loaded]) == 'X') {
                            offset = 4;
                            _set |= HEXADECIMAL;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set = (_set & NEGATIVE ? DECIMAL : OCTAL);
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'l')) {
                            offset = 3;
                        } else {
                            _set = ERROR;
                            offset = 4;
                        }
                    } else if (offset == 3) {
                        ASSERT(_set & QUOTED);
                        ASSERT(_set & NEGATIVE);
                        if (::toupper(stream[loaded]) == 'X') {
                            offset = 4;
                            _set |= HEXADECIMAL;
                        } else if (isdigit(stream[loaded])) {
                            _value = (stream[loaded] - '0');
                            _set |= OCTAL;
                            offset = 4;
                        } else if (((_set & UNDEFINED) != 0) && (stream[loaded] == 'l')) {
                            offset = 4;
                        } else {
                            _set = ERROR;
                            offset = 4;
                        }
                    }
                    loaded++;
                }

                bool completed = ((_set & ERROR) != 0);

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
                    } else if (((_set & QUOTED) == 0) && ((stream[loaded] == ',') || stream[loaded] == '}' || stream[loaded] == ']')) {
                        completed = true;
                    } else {
                        // Oopsie daisy, error, computer says *NO*
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
                } else if (completed == true) {
                    if (_set & NEGATIVE) {
                        _value *= -1;
                    }
                    _set |= SET;
                    offset = 0;
                }

                return (loaded);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint16_t& offset) const override
            {
                if ((_set & UNDEFINED) != 0) {
                    stream[0] = 0xC0;
                    return (1);
                }
                return (Convert(stream, maxLength, offset, TemplateIntToType<SIGNED>()));
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint16_t& offset)
            {
                uint8_t loaded = 0;
                if (offset == 0) {
                    // First byte depicts a lot. Find out what we need to read
                    _value = 0;
                    uint8_t header = stream[loaded++];

                    if (header == 0xC0) {
                        _set = UNDEFINED;
                    } else if ((header >= 0xCC) && (header <= 0xCF)) {
                        _set = (1 < (header - 0xCC)) << 12;
                        offset = 1;
                    } else if ((header >= 0xD0) && (header <= 0xD3)) {
                        _set = (1 << (header - 0xD0)) << 12;
                        offset = 1;
                    } else if ((header & 0x80) == 0) {
                        _value = (header & 0x7F);
                    } else if ((header & 0xE0) == 0xE0) {
                        _value = (header & 0x0F);
                    } else {
                        _set = ERROR;
                    }
                }

                while ((loaded < maxLength) && (offset != 0)) {
                    _value = _value << 8;
                    _value += stream[loaded++];
                    offset = (offset == ((_set >> 12) & 0xF) ? 0 : offset + 1);
                }
                return (loaded);
            }
            uint16_t Convert(char stream[], const uint16_t maxLength, uint16_t& offset, const TYPE serialize) const
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
            uint16_t Convert(char stream[], const uint16_t maxLength, uint16_t& offset, const TemplateIntToType<false>& /* For compile time diffrentiation */) const
            {
                return (Convert(stream, maxLength, offset, _value));
            }
            uint16_t Convert(char stream[], const uint16_t maxLength, uint16_t& offset, const TemplateIntToType<true>& /* For c ompile time diffrentiation */) const
            {
                return (Convert(stream, maxLength, offset, ::abs(_value)));
            }
            uint16_t Convert(uint8_t stream[], const uint16_t maxLength, uint16_t& offset, const TemplateIntToType<false>& /* For compile time diffrentiation */) const
            {
                uint8_t loaded = 0;
                uint8_t bytes = (_value <= 0x7F ? 0 : _value < 0xFF ? 1 : _value < 0xFFFF ? 2 : _value < 0xFFFFFFFF ? 4 : 8);

                if (offset == 0) {
                    if (bytes == 0) {
                        stream[loaded++] = static_cast<uint8_t>(_value);
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
            uint16_t Convert(uint8_t stream[], const uint16_t maxLength, uint16_t& offset, const TemplateIntToType<true>& /* For c ompile time diffrentiation */) const
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
            Boolean(const Boolean& copy)
                : _value(copy._value)
            {
            }
            ~Boolean()
            {
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
                return (_value & ValueBit);
            }
            bool Null() const
            {
                return ((_value & NullBit) != 0);
            }
            void Null(const bool enabled)
            {
                if (enabled == true)
                    _value |= NullBit;
                else
                    _value &= ~NullBit;
            }
            virtual bool IsSet() const override
            {
                return ((_value & (SetBit | NullBit)) != 0);
            }
            virtual void Clear() override
            {
                _value = (_value & DefaultBit);
            }

        private:
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const
            {
                static constexpr char trueBuffer[] = "true";
                static constexpr char falseBuffer[] = "false";

                uint16_t loaded = 0;
                if ((_value & NullBit) != 0) {
                    while ((loaded < maxLength) && (offset < 4)) {
                        stream[loaded++] = NullTag[offset++];
                    }
                    if (offset == 4) {
                        offset = 0;
                    }
                } else if (Value() == true) {
                    while ((loaded < maxLength) && (offset < (sizeof(trueBuffer) - 1))) {
                        stream[loaded++] = trueBuffer[offset++];
                    }
                    if (offset == (sizeof(trueBuffer) - 1)) {
                        offset = 0;
                    }
                } else {
                    while ((loaded < maxLength) && (offset < (sizeof(falseBuffer) - 1))) {
                        stream[loaded++] = falseBuffer[offset++];
                    }
                    if (offset == (sizeof(falseBuffer) - 1)) {
                        offset = 0;
                    }
                }
                return (loaded);
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset)
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
                    uint8_t length = (_value & DeserializeBit ? sizeof(trueBuffer) : sizeof(falseBuffer)) - 1;
                    const char* buffer = (_value & DeserializeBit ? trueBuffer : falseBuffer);

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
                        _value |= SetBit | ((_value & (ErrorBit | DeserializeBit)) == DeserializeBit ? ValueBit : 0);
                    }
                }
                return (loaded);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint16_t& offset) const override
            {
                if ((_value & NullBit) != 0) {
                    stream[0] = 0xC0;
                } else if ((_value & ValueBit) != 0) {
                    stream[0] = 0xC3;
                } else {
                    stream[0] = 0xC2;
                }
                return (1);
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint16_t& offset) override
            {
                if ((stream[0] == 0xC0) != 0) {
                    _value = NullBit;
                } else if ((stream[0] == 0xC3) != 0) {
                    _value = ValueBit | SetBit;
                } else if ((stream[0] == 0xC2) != 0) {
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
            static constexpr uint32_t None = 0x00000000;
            static constexpr uint32_t ScopeMask = 0x0FFFFFFF;
            static constexpr uint32_t QuotedSerializeBit = 0x80000000;
            static constexpr uint32_t SetBit = 0x40000000;
            static constexpr uint32_t QuoteFoundBit = 0x20000000;
            static constexpr uint32_t NullBit = 0x10000000;

        public:
            String(const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
            }
            explicit String(const string& Value, const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value.c_str(), _default);
            }
            explicit String(const char Value[], const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value, _default);
            }
#ifndef __NO_WCHAR_SUPPORT__
            explicit String(const wchar_t Value[], const bool quoted = true)
                : _default()
                , _scopeCount(quoted ? QuotedSerializeBit : None)
                , _value()
            {
                Core::ToString(Value, _default);
            }
#endif // __NO_WCHAR_SUPPORT__
            String(const String& copy)
                : _default(copy._default)
                , _scopeCount(copy._scopeCount & (QuotedSerializeBit | SetBit))
                , _value(copy._value)
            {
            }
            virtual ~String()
            {
            }

            String& operator=(const string& RHS)
            {
                Core::ToString(RHS.c_str(), _value);
                _scopeCount |= SetBit;

                return (*this);
            }
            String& operator=(const char RHS[])
            {
                Core::ToString(RHS, _value);
                _scopeCount |= SetBit;

                return (*this);
            }
#ifndef __NO_WCHAR_SUPPORT__
            String& operator=(const wchar_t RHS[])
            {
                Core::ToString(RHS, _value);
                _scopeCount |= SetBit;

                return (*this);
            }
#endif // __NO_WCHAR_SUPPORT__
            String& operator=(const String& RHS)
            {
                _default = RHS._default;
                _value = RHS._value;
                _scopeCount = (RHS._scopeCount & ~QuotedSerializeBit) | (_scopeCount & QuotedSerializeBit);

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
#ifndef __NO_WCHAR_SUPPORT__
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

#endif // __NO_WCHAR_SUPPORT__
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
                if ((_scopeCount & (SetBit | QuoteFoundBit | QuotedSerializeBit)) == (SetBit|QuoteFoundBit)) {
					return('\"' + Core::ToString(_value.c_str()) + '\"');
				}
                return (((_scopeCount & (SetBit | NullBit)) == SetBit) ? Core::ToString(_value.c_str()) : Core::ToString(_default.c_str()));
            }
            inline const string& Default() const
            {
                return (_default);
            }
            bool Null() const
            {
                return (_scopeCount & NullBit) != 0;
            }
            void Null(const bool enabled)
            {
                if (enabled == true)
                    _scopeCount |= NullBit;
                else
                    _scopeCount &= ~NullBit;
            }
            // IElement interface methods
            virtual bool IsSet() const override
            {
                return ((_scopeCount & (SetBit|NullBit)) != 0);
            }
            virtual void Clear() override
            {
                _scopeCount = (_scopeCount & QuotedSerializeBit);
            }
            inline bool IsQuoted() const
            {
                return ((_scopeCount & (QuotedSerializeBit | QuoteFoundBit)) != 0);
            }
            inline void SetQuoted(const bool enable)
            {
                if (enable == true) {
                    _scopeCount |= QuoteFoundBit;
                } else {
                    _scopeCount &= (~QuoteFoundBit);
                }
            }

        protected:
            inline bool MatchLastCharacter(const string& str, char ch) const
            {
                return (str.length() > 0) && (str[str.length() - 1] == ch);
            }
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const override
            {
                bool quoted = IsQuoted();
                uint16_t result = 0;

                ASSERT(maxLength > 0);

                if ((quoted == false) || ((_scopeCount & NullBit) != 0)) {
                    std::string source((_value.empty() || (_scopeCount & NullBit)) ? NullTag : _value);
                    result = static_cast<uint16_t>(source.copy(stream, maxLength - result, offset));
                    offset = (result < maxLength ? 0 : offset + result);
                } else {
                    if (offset == 0) {
                        // We always start with a quote or Block marker
                        stream[result++] = '\"';
                        offset = 1;
                        _unaccountedCount = 0;
                    }

                    uint16_t length = static_cast<uint16_t>(_value.length()) - (offset - 1);
                    if (length > 0) {
                        const TCHAR* source = &(_value[offset - 1]);
                        offset += length;

                        while ((result < maxLength) && (length > 0)) {

                            // See where we are and add...
                            if ((*source != '\"') || (_unaccountedCount == 1)) {
                                _unaccountedCount = 0;
                                stream[result++] = *source++;
                                length--;
                            } else {
                                // this we need to escape...
                                stream[result++] = '\\';
                                _unaccountedCount = 1;
                            }
                        }
                    }

                    if (result == maxLength) {
                        offset -= length;
                    } else {
                        // And we close with a quote..
                        stream[result++] = '\"';
                        offset = 0;
                    }
                }

                return (result);
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset) override
            {
                bool finished = false;
                uint16_t result = 0;
                ASSERT(maxLength > 0);

                if (offset == 0) {
                    // We got a quote, start recording..
                    _value.clear();
                    _scopeCount &= QuotedSerializeBit;
                    if (stream[result] != '\"') {
                        _unaccountedCount = 0;
                    } else {
                        result++;
                        _scopeCount |= (QuoteFoundBit | 1);
                        _unaccountedCount = 1;
                    }
                }

                bool escapedSequence = MatchLastCharacter(_value, '\\');

                // Might be that the last character we added was a
                while ((result < maxLength) && (finished == false)) {

                    TCHAR current = stream[result];

                    if (escapedSequence == false) {
                        if ((current == '{') || (current == '[')) {
                            _scopeCount++;
                        } else if ((current == '}') || (current == ']')) {
                            if ((_scopeCount & ScopeMask) > 0) {
                                _scopeCount--;
                            } else {
                                finished = true;
                            }
                        } else if ((current == '\"') && ((_scopeCount & (ScopeMask | QuoteFoundBit)) == (QuoteFoundBit | 1))) {
                            result++;
                            finished = true;
                        } else if ((_scopeCount & ScopeMask) == 0) {
                            finished = ((current == ',') || (current == ' ') || (current == '\t'));
                        }
                    }

                    if (finished == false) {

                        if ((escapedSequence == true) && (current == '\"')) {
                            _value[_value.length() - 1] = current;
                            _unaccountedCount++;
                        } else {
                            // Write the amount we possibly can..
                            _value += current;
                        }

                        escapedSequence = (current == '\\');

                        // Move on to the next position
                        result++;
                    }
                }

                if (finished == false) {
                    offset = static_cast<uint16_t>(_value.length()) + _unaccountedCount;
                } else {
                    offset = 0;
                    _scopeCount |= ((_scopeCount & QuoteFoundBit) ? SetBit : (_value == NullTag ? NullBit : SetBit));
                }

                return (result);
            }
            virtual uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, uint16_t& offset) const override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    if ((_scopeCount & NullBit) == 0) {
                        stream[loaded++] = 0xC0;
                    } else if (_value.length() <= 31) {
                        _unaccountedCount = 0;
                        stream[loaded++] = static_cast<uint8_t>(_value.length() | 0xA0);
                        offset++;
                    } else if (_value.length() <= 0xFF) {
                        _unaccountedCount = 1;
                        stream[loaded++] = 0xD9;
                        offset++;
                    } else if (_value.length() <= 0xFFFF) {
                        _unaccountedCount = 2;
                        stream[loaded++] = 0xDA;
                        offset++;
                    } else {
                        stream[loaded++] = 0xC0;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset <= _unaccountedCount)) {
                        stream[loaded++] = static_cast<uint8_t>((_value.length() >> (8 * (_unaccountedCount - offset))) & 0xFF);
                    }

                    while ((loaded < maxLength) && (offset == 0)) {
                        loaded += static_cast<uint16_t>(_value.copy(reinterpret_cast<char*>(&stream[loaded]), (maxLength - loaded), offset - _unaccountedCount));
                        offset += loaded;

                        if (offset - _unaccountedCount >= _value.length()) {
                            offset = 0;
                        }
                    }
                }

                return (loaded);
            }
            virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, uint16_t& offset) override
            {
                uint16_t loaded = 0;
                if (offset == 0) {
                    if (stream[loaded] == 0xC0) {
                        _scopeCount |= NullBit;
                        loaded++;
                    } else if (stream[loaded] == 0xA0) {
                        _unaccountedCount = stream[loaded] & 0x1F;
                        offset = 3;
                        loaded++;
                    } else if (stream[loaded] == 0xD9) {
                        _unaccountedCount = 0;
                        offset = 2;
                        loaded++;
                    } else if (stream[loaded] == 0xDA) {
                        _unaccountedCount = 0;
                        offset = 1;
                        loaded++;
                    }
                }

                if (offset != 0) {
                    while ((loaded < maxLength) && (offset < 3)) {
                        _unaccountedCount = (_unaccountedCount << 8) + stream[loaded++];
                        offset++;
                    }

                    while ((loaded < maxLength) && ((offset - 3) < static_cast<uint16_t>(_unaccountedCount))) {
                        _value += static_cast<char>(stream[loaded++]);
                        offset++;
                    }

		    if ((offset > 3) && (static_cast<uint16_t>(offset - 3) == _unaccountedCount)) {
                      offset = 0;
                    }
                }

                return (loaded);
            }

        private:
            std::string _default;
            uint32_t _scopeCount;
            mutable uint32_t _unaccountedCount;
            std::string _value;
        };

        template <typename ENUMERATE>
        class EnumType : public IElement {
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
            EnumType(const EnumType<ENUMERATE>& copy)
                : _state(copy._state)
                , _value(copy._value)
                , _default(copy._default)
            {
            }
            virtual ~EnumType()
            {
            }

            EnumType<ENUMERATE>& operator=(const EnumType<ENUMERATE>& RHS)
            {
                _value = RHS._value;
                _state = SET;
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
                return _value;
            }
            const TCHAR* Data() const
            {
                return (Core::EnumerateType<ENUMERATE>(Value()).Data());
            }
            bool Null() const
            {
                return ((_state & UNDEFINED) != 0);
            }
            void Null(const bool enabled)
            {
                if (enabled == true)
                    _state |= UNDEFINED;
                else
                    _state &= ~UNDEFINED;
            }
            virtual bool IsSet() const override
            {
                return ((_state & SET) != 0);
            }
            virtual void Clear() override
            {
                _state = 0;
            }

        private:
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const
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
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset) override
            {
                uint16_t result = static_cast<IElement&>(_parser).Deserialize(stream, maxLength, offset);

                if (offset == 0) {

                    if (_parser.Null() == true) {
                        _state = UNDEFINED;
                    } else if (_parser.IsSet() == true) {
                        // Looks like we parsed the value. Lets see if we can find it..
                        Core::EnumerateType<ENUMERATE> converted(_parser.Value().c_str(), false);

                        if (converted.IsSet() == true) {
                            _value = converted.Value();
                            _state = SET;
                        } else {
                            _state = ERROR;
                        }
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
        };

        template <typename ELEMENT>
        class ArrayType : public IElement {
        private:
            enum mode : uint8_t {
                ERROR = 0x80
            };

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
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }
                ConstIteratorType(const ConstIteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }
                ~ConstIteratorType()
                {
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
                void Reset()
                {
                    if (_container != nullptr) {
                        _iterator = _container->begin();
                    }
                    _state = AT_BEGINNING;
                }
                virtual bool Next()
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
                    return (_container == nullptr ? 0 : _container->size());
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
                IteratorType(const IteratorType<ARRAYELEMENT>& copy)
                    : _container(copy._container)
                    , _iterator(copy._iterator)
                    , _state(copy._state)
                {
                }
                virtual ~IteratorType()
                {
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
                void Reset()
                {
                    if (_container != nullptr) {
                        _iterator = _container->begin();
                    }
                    _state = AT_BEGINNING;
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
                void AddElement()
                {
                    // This can only be called if there is a container..
                    ASSERT(_container != nullptr);

                    if ((_container->size() == 0) || (_container->back().IsSet() == true)) {
                        _container->push_back(ARRAYELEMENT());
                        _state = AT_ELEMENT;
                        _iterator = _container->end();
                        _iterator--;
                    }
                }
                void Clear()
                {
                    // This can only be called if there is a container..
                    ASSERT(_container != nullptr);

                    if (_container != nullptr) {
                        _container->clear();
                    }
                }
                ARRAYELEMENT& Current()
                {
                    ASSERT(_state == AT_ELEMENT);

                    return (*_iterator);
                }
                inline uint32_t Count() const
                {
                    return (_container == nullptr ? 0 : _container->size());
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
                : _data()
                , _iterator(_data)
            {
            }
            ArrayType(const ArrayType<ELEMENT>& copy)
                : _data(copy._data)
                , _iterator(_data)
            {
            }
            virtual ~ArrayType()
            {
            }

        public:
            virtual bool IsSet() const override
            {
                return (Length() > 0);
            }
            virtual void Clear() override
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
            ArrayType<ELEMENT>& operator=(const ArrayType<ELEMENT>& RHS)
            {
                _state = RHS._state;
                _data = RHS._data;
                _iterator = IteratorType<ELEMENT>(_data);

                return (*this);
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

        private:
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    _iterator.Reset();
                    stream[loaded++] = '[';
                    offset = (_iterator.Next() == false ? ~0 : PARSE);
                }
                while ((loaded < maxLength) && (offset != static_cast<uint16_t>(~0))) {
                    if (offset >= PARSE) {
                        offset -= PARSE;
                        loaded += static_cast<const IElement&>(_iterator.Current()).Serialize(&(stream[loaded]), maxLength - loaded, offset);
                        offset = (offset != 0 ? offset + PARSE : (_iterator.Next() == true ? BEGIN_MARKER : ~0));
                    } else if (offset == BEGIN_MARKER) {
                        stream[loaded++] = ',';
                        offset = PARSE;
                    }
                }
                if ((offset == static_cast<uint16_t>(~0)) && (loaded < maxLength)) {
                    stream[loaded++] = ']';
                    offset = 0;
                }

                return (loaded);
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset)
            {
                uint16_t loaded = 0;
                if ((offset == 0) || (offset == BEGIN_MARKER)) {
                    // Run till we find opening bracket..
                    while ((loaded < maxLength) && (stream[loaded] != '[')) {
                        loaded++;
                    }
                    if (loaded == maxLength) {
                        offset = BEGIN_MARKER;
                    } else {
                        offset = SKIP_BEFORE;
                        loaded++;
                    }
                }

                while ((offset != 0) && (loaded < maxLength)) {
                    if ((offset == SKIP_BEFORE) || (offset == SKIP_AFTER)) {
                        // Run till we find a character not a whitespace..
                        while ((loaded < maxLength) && (::isspace(stream[loaded]))) {
                            loaded++;
                        }

                        if (loaded < maxLength) {
                            switch (stream[loaded]) {
                            case ']':
                                offset = 0;
                                loaded++;
                                break;
                            case ',':
                                if (offset == SKIP_BEFORE) {
                                    _state = ERROR;
                                } else {
                                    offset = SKIP_BEFORE;
                                }
                                loaded++;
                                break;
                            default:
                                offset = PARSE;
                                _data.push_back(ELEMENT());
                                break;
                            }
                        }
                    }
                    if (offset >= PARSE) {
                        offset = (offset - PARSE);
                        loaded += static_cast<IElement&>(_data.back()).Deserialize(&(stream[loaded]), maxLength - loaded, offset);
                        offset = (offset == 0 ? SKIP_AFTER : offset + PARSE);
                    }
                }

                return (loaded);
            }

        private:
            uint8_t _state;
            std::list<ELEMENT> _data;
            mutable IteratorType<ELEMENT> _iterator;
        };

        class EXTERNAL Container : public IElement {
        private:
            enum mode : uint8_t {
                ERROR = 0x80
            };

            static constexpr uint16_t BEGIN_MARKER = 1;
            static constexpr uint16_t END_MARKER = 2;
            static constexpr uint16_t SKIP_BEFORE = 3;
            static constexpr uint16_t SKIP_AFTER = 4;
            static constexpr uint16_t PARSE = 5;

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
                Iterator(const Iterator& copy);
                Iterator& operator=(const Iterator& RHS);

            public:
                Iterator(Container& parent, JSONElementList& container)
                    : _parent(parent)
                    , _container(container)
                    , _iterator(container.begin())
                    , _state(AT_BEGINNING)
                {
                }
                virtual ~Iterator()
                {
                }

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
            Container(const Container& copy) = delete;
            Container& operator=(const Container& RHS) = delete;
            Container()
                : _state(0)
                , _current(nullptr)
                , _data()
                , _iterator()
                , _fieldName(true)
            {
            }
            virtual ~Container()
            {
            }

        public:
            bool HasLabel(const string& label) const
            {
                JSONElementList::const_iterator index(_data.begin());

                while ((index != _data.end()) && (index->first != label)) {
                    index++;
                }

                return (index != _data.end());
            }

            virtual bool IsSet() const override
            {
                JSONElementList::const_iterator index = _data.begin();

                // As long as we did not find a set element, continue..
                while ((index != _data.end()) && (index->second->IsSet() == false)) {
                    index++;
                }

                return (index != _data.end());
            }
            virtual void Clear() override
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

        private:
            virtual uint16_t Serialize(char stream[], const uint16_t maxLength, uint16_t& offset) const
            {
                uint16_t loaded = 0;

                if (offset == 0) {
                    _iterator = _data.begin();
                    stream[loaded++] = '{';

                    offset = (_iterator == _data.end() ? ~0 : ((_iterator->second->IsSet() == false) && (FindNext() == false)) ? ~0 : BEGIN_MARKER);
                    if (offset == BEGIN_MARKER) {
                        _fieldName = string(_iterator->first);
                        _current = &_fieldName;
                        offset = PARSE;
                    }
                }
                while ((loaded < maxLength) && (offset != static_cast<uint16_t>(~0))) {
                    if (offset >= PARSE) {
                        offset -= PARSE;
                        loaded += _current->Serialize(&(stream[loaded]), maxLength - loaded, offset);
                        offset = (offset == 0 ? BEGIN_MARKER : offset + PARSE);
                    } else if (offset == BEGIN_MARKER) {
                        if (_current == &_fieldName) {
                            stream[loaded++] = ':';
                            _current = _iterator->second;
                            offset = PARSE;
                        } else {
                            if (FindNext() != false) {
                                stream[loaded++] = ',';
                                _fieldName = string(_iterator->first);
                                _current = &_fieldName;
                                offset = PARSE;
                            } else {
                                offset = ~0;
                            }
                        }
                    }
                }
                if ((offset == static_cast<uint16_t>(~0)) && (loaded < maxLength)) {
                    stream[loaded++] = '}';
                    offset = 0;
                }

                return (loaded);
            }
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset)
            {
                uint16_t loaded = 0;
                if ((offset == 0) || (offset == BEGIN_MARKER)) {
                    // Run till we find opening bracket..
                    while ((loaded < maxLength) && (stream[loaded] != '{')) {
                        loaded++;
                    }

                    if (stream[loaded] != '{') {
                        offset = BEGIN_MARKER;
                    } else {
                        loaded++;
                        _fieldName.Clear();
                        offset = SKIP_BEFORE;
                    }
                }

                while ((offset != 0) && (loaded < maxLength)) {
                    if ((offset == SKIP_BEFORE) || (offset == SKIP_AFTER)) {
                        // Run till we find a character not a whitespace..
                        while ((loaded < maxLength) && (::isspace(stream[loaded]))) {
                            loaded++;
                        }

                        if (loaded < maxLength) {
                            switch (stream[loaded]) {
                            case '}':
                                offset = 0;
                                loaded++;
                                break;
                            case ',':
                                if (offset == SKIP_BEFORE) {
                                    _state = ERROR;
                                } else {
                                    offset = SKIP_BEFORE;
                                }
                                loaded++;
                                break;
                            case ':':
                                if (offset == SKIP_BEFORE) {
                                    _state = ERROR;
                                } else {
                                    offset = SKIP_BEFORE;
                                }
                                loaded++;
                                break;
                            default:
                                offset = PARSE;
                                if (_fieldName.IsSet() == true) {
                                    if (_current != nullptr) {
                                        _state = ERROR;
                                    }
                                    _current = Find(_fieldName.Value().c_str());

                                    _fieldName.Clear();

                                    if (_current == nullptr) {
                                        _current = &_fieldName;
                                    }
                                } else {
                                    _current = nullptr;
                                }
                                break;
                            }
                        }
                    }
                    if (offset >= PARSE) {
                        offset = (offset - PARSE);
                        if (_current == nullptr) {
                            loaded += static_cast<IElement&>(_fieldName).Deserialize(&(stream[loaded]), maxLength - loaded, offset);
                        } else {
                            loaded += _current->Deserialize(&(stream[loaded]), maxLength - loaded, offset);
                        }
                        offset = (offset == 0 ? SKIP_AFTER : offset + PARSE);
                    }
                }

                return (loaded);
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
                if (Request(label) == true) {
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
            virtual bool Request(const TCHAR label[])
            {
                return (false);
            }

        private:
            uint8_t _state;
            mutable IElement* _current;
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
                OBJECT
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
            Variant(const ArrayType<Variant>& array)
                : JSON::String(false)
                , _type(type::ARRAY)
                , _array(Core::ProxyType<ArrayType<Variant>>::Create())
            {
                *_array = array;
            }
            //Variant(const VariantContainer& object);
            Variant(const Variant& copy)
                : JSON::String(copy)
                , _type(copy._type)
                , _array(copy._array)
                , _object(copy._object)
            {
            }
            inline Variant(const VariantContainer& object);
            virtual ~Variant()
            {
            }
            Variant& operator=(const Variant& RHS)
            {
                JSON::String::operator=(RHS);
                _type = RHS._type;
                _array = RHS._array;
                _object = RHS._object;
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
                }
                return result;
            }
            const TCHAR* String() const
            {
                if (_type == type::STRING) {
                    const_cast<Variant*>(this)->_string = Value();
                }
                return _string.c_str();
            }
            const ArrayType<Variant>& Array() const
            {
                if (_type == type::ARRAY) {
                    return *_array;
                } else {
                    static ArrayType<Variant> empty;
                    return empty;
                }
            }
            const VariantContainer& Object() const;
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
            void String(const TCHAR* value)
            {
                _type = type::STRING;
                String::SetQuoted(true);
                String::operator=(value);
            }
            void Array(const ArrayType<Variant>& array)
            {
                _type = type::ARRAY;
                _array = Core::ProxyType<ArrayType<Variant>>::Create();
                *_array = array;
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
            const JSON::Variant& operator[](uint32_t index) const;
            const JSON::Variant& operator[](const TCHAR fieldName[]) const;
            inline void ToString(string& result) const;
            virtual bool IsSet() const override
            {
                if (_type == type::ARRAY)
                    return _array->IsSet();
                else if (_type == type::OBJECT)
                    return true;
                else
                    return String::IsSet();
            }
            string GetDebugString(const TCHAR name[], int indent = 0, int arrayIndex = -1) const;

        private:
            virtual uint16_t Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset) override;
            static uint16_t FindEndOfScope(const char stream[], uint16_t maxLength)
            {
                ASSERT(maxLength > 0 && (stream[0] == '{' || stream[0] == '['));
                char charOpen = stream[0];
                char charClose = charOpen == '{' ? '}' : ']';
                uint16_t stack = 1;
                uint16_t endIndex = 0;
                bool insideQuotes = false;
                for (uint16_t i = 1; i < maxLength; ++i) {
                    if (stream[i] == '\"') {
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
            string _string;
            Core::ProxyType<ArrayType<Variant>> _array;
            Core::ProxyType<VariantContainer> _object;
        };

        class EXTERNAL VariantContainer : public Container {
        private:
            typedef std::list<std::pair<string, WPEFramework::Core::JSON::Variant>> Elements;

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
                Iterator(const Iterator& copy)
                    : _container(copy._container)
                    , _index()
                    , _start(true)
                {
                    if (_container != nullptr) {
                        _index = _container->begin();
                    }
                }
                ~Iterator()
                {
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
            VariantContainer(const VariantContainer& copy)
                : Container()
                , _elements(copy._elements)
            {
                Elements::iterator index(_elements.begin());

                while (index != _elements.end()) {
                    if (copy.HasLabel(index->first.c_str()))
                        Container::Add(index->first.c_str(), &(index->second));
                    index++;
                }
            }
            VariantContainer(const Elements& values)
                : Container()
            {
                Elements::const_iterator index(values.begin());

                while (index != values.end()) {
                    _elements.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(index->first),
                        std::forward_as_tuple(index->second));
                    Container::Add(_elements.back().first.c_str(), &(_elements.back().second));
                    index++;
                }
            }
            virtual ~VariantContainer()
            {
            }

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
                        _elements.emplace_back(std::piecewise_construct,
                            std::forward_as_tuple(rhs_index->first),
                            std::forward_as_tuple(rhs_index->second));
                        Container::Add(_elements.back().first.c_str(), &(_elements.back().second));
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
                    _elements.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(fieldName),
                        std::forward_as_tuple(value));
                    Container::Add(_elements.back().first.c_str(), &(_elements.back().second));
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
                    _elements.emplace_back(std::piecewise_construct,
                        std::forward_as_tuple(fieldName),
                        std::forward_as_tuple());
                    Container::Add(_elements.back().first.c_str(), &(_elements.back().second));
                    index = _elements.end();
                    index--;
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
            string GetDebugString(int indent = 0) const;

        private:
            Elements::iterator Find(const TCHAR fieldName[])
            {
                Elements::iterator index(_elements.begin());
                while ((index != _elements.end()) && (index->first != fieldName)) {
                    index++;
                }
                return (index);
            }
            Elements::const_iterator Find(const TCHAR fieldName[]) const
            {
                Elements::const_iterator index(_elements.begin());
                while ((index != _elements.end()) && (index->first != fieldName)) {
                    index++;
                }
                return (index);
            }
            virtual bool Request(const TCHAR label[])
            {
                // Whetever comes in and has no counter part, we need to create a Variant for it, so
                // it can be filled.
                _elements.emplace_back(std::piecewise_construct,
                    std::forward_as_tuple(label),
                    std::forward_as_tuple());

                Container::Add(_elements.back().first.c_str(), &(_elements.back().second));

                return (true);
            }

        private:
            Elements _elements;
        };
        inline Variant::Variant(const VariantContainer& object)
            : JSON::String(false)
            , _type(type::OBJECT)
            , _object(Core::ProxyType<VariantContainer>::Create())
        {
            *_object = object;
        }
        inline void Variant::Object(const VariantContainer& object)
        {
            _type = type::OBJECT;
            _object = Core::ProxyType<VariantContainer>::Create();
            *_object = object;
        }
        inline const VariantContainer& Variant::Object() const
        {
            if (_type == type::OBJECT) {
                return *_object;
            } else {
                static VariantContainer empty;
                return empty;
            }
        }
        inline const JSON::Variant& Variant::operator[](uint32_t index) const
        {
            if (_type == type::ARRAY) {
                ASSERT(index < _array->Length());
                return _array->operator[](index);
            } else {
                static Variant empty;
                return empty;
            }
        }
        inline const JSON::Variant& Variant::operator[](const TCHAR fieldName[]) const
        {
            if (_type == type::OBJECT) {
                return _object->operator[](fieldName);
            } else {
                static Variant empty;
                return empty;
            }
        }
        inline void Variant::ToString(string& result) const
        {
            if (_type == type::ARRAY)
                _array->ToString(result);
            else if (_type == type::OBJECT)
                _object->ToString(result);
            else
                String::ToString(result);
        }
        inline uint16_t Variant::Deserialize(const char stream[], const uint16_t maxLength, uint16_t& offset)
        {
            uint16_t result = 0;
            if (stream[0] == '{' || stream[0] == '[') {
                uint16_t endIndex = FindEndOfScope(stream, maxLength);
                if (endIndex > 0 && endIndex < maxLength) {
                    result = endIndex + 1;
                    string str(stream, endIndex + 1);
                    if (stream[0] == '{') {
                        VariantContainer object;
                        object.FromString(str);
                        Object(object);
                    } else {
                        ArrayType<Variant> array;
                        array.FromString(str);
                        Array(array);
                    }
                }
            } else {
                result = String::Deserialize(stream, maxLength, offset);

                _type = type::STRING;

                // If we are complete, try to guess what it was that we received...
                if (offset == 0) {
                    bool quoted = IsQuoted();
                    SetQuoted(quoted);
                    // If it is not quoted, it can be a boolean or a number...
                    if (quoted == false) {
                        if ((Value() == _T("true")) || (Value() == _T("false"))) {
                            _type = type::BOOLEAN;
                        } else {
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
            typedef Tester<SIZE, INSTANCEOBJECT> ThisClass;

        private:
            Tester(const Tester<SIZE, INSTANCEOBJECT>&) = delete;
            Tester<SIZE, INSTANCEOBJECT>& operator=(const Tester<SIZE, INSTANCEOBJECT>&) = delete;

        public:
            Tester()
            {
            }
            ~Tester()
            {
            }

            bool FromString(const string& value, Core::ProxyType<INSTANCEOBJECT>& receptor)
            {
                uint16_t fillCount = 0;
                uint16_t offset = 0;
                uint16_t size, loaded;

                receptor->Clear();

                do {
                    size = static_cast<uint16_t>((value.size() - fillCount) < SIZE ? (value.size() - fillCount) : SIZE);

                    // Prepare the deserialize buffer
                    memcpy(_buffer, &(value.data()[fillCount]), size);

                    fillCount += size;

                    loaded = static_cast<IElement&>(*receptor).Deserialize(_buffer, size, offset);

                    ASSERT(loaded <= size);

                } while ((loaded == size) && (offset != 0));

                return (offset == 0);
            }

            bool ToString(const Core::ProxyType<INSTANCEOBJECT>& receptor, string& value)
            {
                uint16_t offset = 0;
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
    } // namespace JSON
} // namespace Core
} // namespace WPEFramework

using JsonObject = WPEFramework::Core::JSON::VariantContainer;
using JsonValue = WPEFramework::Core::JSON::Variant;
using JsonArray = WPEFramework::Core::JSON::ArrayType<JsonValue>;

#endif // __JSON_H

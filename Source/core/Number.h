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
 
#ifndef __NUMBER_H
#define __NUMBER_H

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"
#include "TypeTraits.h"

namespace Thunder {
namespace Core {
    extern "C" {
    EXTERNAL unsigned char FromDigits(const TCHAR element);
    EXTERNAL unsigned char FromHexDigits(const TCHAR element);
    EXTERNAL unsigned char FromBase64(const TCHAR element);
    EXTERNAL unsigned char FromDirect(const TCHAR element);
    EXTERNAL TCHAR ToDigits(const unsigned char element);
    EXTERNAL TCHAR ToHexDigits(const unsigned char element);
    EXTERNAL TCHAR ToBase64(const unsigned char element);
    EXTERNAL TCHAR ToDirect(const unsigned char element);
    }

    template <class TYPE, bool SIGNED = (TypeTraits::sign<TYPE>::Signed == 1), const NumberBase BASETYPE = BASE_UNKNOWN>
    class NumberType {
    public:
        NumberType()
            : m_Value(0)
        {
        }
        NumberType(
            const TYPE Value)
            : m_Value(Value)
        {
        }
        NumberType(
            const TextFragment& text,
            const NumberBase Type = BASETYPE)
            : m_Value(0)
        {
            NumberType<TYPE, SIGNED, BASETYPE>::Convert(text.Data(), text.Length(), m_Value, Type);
        }
        NumberType(
            const TCHAR Value[],
            const uint32_t Length,
            const NumberBase Type = BASETYPE)
            : m_Value(0)
        {
            NumberType<TYPE, SIGNED, BASETYPE>::Convert(Value, Length, m_Value, Type);
        }
        NumberType(
            const NumberType<TYPE, SIGNED, BASETYPE>& rhs)
            : m_Value(rhs.m_Value)
        {
        }
        NumberType(
            NumberType<TYPE, SIGNED, BASETYPE>&& move)
            : m_Value(std::move(move.m_Value))
        {
        }
        ~NumberType()
        {
        }

        inline NumberType<TYPE, SIGNED, BASETYPE>&
        operator=(
            const NumberType<TYPE, SIGNED, BASETYPE>& Value)
        {
            return (NumberType<TYPE, SIGNED, BASETYPE>::operator=(Value.m_Value));
        }
        inline NumberType<TYPE, SIGNED, BASETYPE>&
        operator=(
            NumberType<TYPE, SIGNED, BASETYPE>&& move)
        {
            if (this != &move) {
                m_Value = std::move(move.m_Value);
            }
            return *this;
        }
        inline NumberType<TYPE, SIGNED, BASETYPE>&
        operator=(const TYPE Value)
        {
            m_Value = Value;

            return (*this);
        }

    public:
        inline static uint8_t ToNetwork(uint8_t input)
        {
            return (input);
        }
        inline static uint8_t FromNetwork(uint8_t input)
        {
            return (input);
        }
        inline static int8_t ToNetwork(int8_t input)
        {
            return (input);
        }
        inline static int8_t FromNetwork(int8_t input)
        {
            return (input);
        }
        inline static uint16_t ToNetwork(uint16_t input)
        {
            return (htons(input));
        }
        inline static uint16_t FromNetwork(uint16_t input)
        {
            return (ntohs(input));
        }
        inline static int16_t ToNetwork(int16_t input)
        {
            return (htons(input));
        }
        inline static int16_t FromNetwork(int16_t input)
        {
            return (ntohs(input));
        }
        inline static uint32_t ToNetwork(uint32_t input)
        {
            return (htonl(input));
        }
        inline static uint32_t FromNetwork(uint32_t input)
        {
            return (ntohl(input));
        }
        inline static int32_t ToNetwork(int32_t input)
        {
            return (htonl(input));
        }
        inline static int32_t FromNetwork(int32_t input)
        {
            return (ntohl(input));
        }
        inline static uint64_t ToNetwork(uint64_t input)
        {
#ifdef LITTLE_ENDIAN_PLATFORM
            return (input);
#else
            ASSERT(false); //TODO: To be implemented
            return (input);
#endif
        }
        inline static uint64_t FromNetwork(uint64_t input)
        {
#ifdef LITTLE_ENDIAN_PLATFORM
            return (input);
#else
            ASSERT(false); //TODO: To be implemented
            return (input);
#endif
        }
        inline static int64_t ToNetwork(int64_t input)
        {
#ifdef LITTLE_ENDIAN_PLATFORM
            return (input);
#else
            ASSERT(false); //TODO: To be implemented
            return (input);
#endif
        }
        inline static int64_t FromNetwork(int64_t input)
        {
#ifdef LITTLE_ENDIAN_PLATFORM
            return (input);
#else
            ASSERT(false); //TODO: To be implemented
            return (input);
#endif
        }
        static uint8_t MaxSize()
        {
            if (BASETYPE == BASE_HEXADECIMAL) {
                return ((sizeof(TYPE) * 2) + (SIGNED == true ? 1 : 0) + 2 /* Number prefix 0x */ + 1 /* Closing null char */);
            } else if (BASETYPE == BASE_OCTAL) {
                return ((sizeof(TYPE) * 4) + (SIGNED == true ? 1 : 0) + 1 /* Number prefix 0 */ + 1 /* Closing null char */);
            } else {
                return ((sizeof(TYPE) * 3) + (SIGNED == true ? 1 : 0) + 1 /* Closing null char */);
            }
        }
        static uint32_t Convert(
            const wchar_t* value,
            const uint32_t length,
            TYPE& number,
            const NumberBase formatting)
        {
            return (Convert<TYPE>(value, length, number, formatting, TemplateIntToType<SIGNED>()));
        }
        static uint32_t Convert(
            const char* value,
            const uint32_t length,
            TYPE& number,
            const NumberBase formatting)
        {
            return (Convert<TYPE>(value, length, number, formatting, TemplateIntToType<SIGNED>()));
        }

        operator string() const
        {
            return (NumberType<TYPE, SIGNED, BASETYPE>::Text());
        }
        string Text() const
        {
            // Max size needed to dreate
            TCHAR Buffer[36];
            uint16_t Index = FillBuffer(Buffer, sizeof(Buffer), BASETYPE);

            return (&Buffer[Index]);
        }
        uint16_t Serialize(std::string& buffer) const
        {
            char Buffer[36];
            uint16_t Index = FillBuffer(Buffer, sizeof(Buffer), BASETYPE);
            uint16_t Result = (sizeof(Buffer) - Index - 1 /* Do not account for the closing char */);

            // Move it to the actual buffer
            buffer = std::string(&Buffer[Index], Result);

            return (Result);
        }

#ifndef __CORE_NO_WCHAR_SUPPORT__
        uint16_t Serialize(std::wstring& buffer)
        {
            wchar_t Buffer[36];
            uint16_t Index = FillBuffer(Buffer, sizeof(Buffer), BASETYPE);
            uint16_t Result = (sizeof(Buffer) - Index - 1 /* Do not account for the closing char */);

            // Move it to the actual buffer
            buffer = std::wstring(&Buffer[Index], Result);

            return (Result);
        }
        uint16_t Deserialize(const std::wstring& buffer)
        {
            return (Convert(buffer.data(), buffer.length(), m_Value, BASETYPE));
        }
#endif

        uint16_t Deserialize(const std::string& buffer)
        {
            return (Convert(buffer.data(), static_cast<uint32_t>(buffer.length()), m_Value, BASETYPE));
        }
        inline TYPE& Value()
        {
            return (m_Value);
        }
        inline const TYPE& Value() const
        {
            return (m_Value);
        }
        inline operator TYPE() const
        {
            return (m_Value);
        }
        inline bool operator==(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value == rhs.m_Value);
        }
        inline bool operator!=(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value != rhs.m_Value);
        }
        inline bool operator<=(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value <= rhs.m_Value);
        }
        inline bool operator>=(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value >= rhs.m_Value);
        }
        inline bool operator<(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value < rhs.m_Value);
        }
        inline bool operator>(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (m_Value > rhs.m_Value);
        }

        inline NumberType<TYPE, SIGNED> operator+(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value + rhs.m_Value));
        }
        inline NumberType<TYPE, SIGNED>& operator+=(const NumberType<TYPE, SIGNED>& rhs)
        {
            m_Value += rhs.m_Value;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator-(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value - rhs.m_Value));
        }
        inline NumberType<TYPE, SIGNED>& operator-=(const NumberType<TYPE, SIGNED>& rhs)
        {
            m_Value -= rhs.m_Value;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator*(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value * rhs.m_Value));
        }
        inline NumberType<TYPE, SIGNED>& operator*=(const NumberType<TYPE, SIGNED>& rhs)
        {
            m_Value *= rhs.m_Value;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator/(const NumberType<TYPE, SIGNED>& rhs) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value / rhs.m_Value));
        }
        inline NumberType<TYPE, SIGNED>& operator/=(const NumberType<TYPE, SIGNED>& rhs)
        {
            m_Value /= rhs.m_Value;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator+(const TYPE Number) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value + Number));
        }
        inline NumberType<TYPE, SIGNED>& operator+=(const TYPE Number)
        {
            m_Value += Number;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator-(const TYPE Number) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value - Number));
        }
        inline NumberType<TYPE, SIGNED>& operator-=(const TYPE Number)
        {
            m_Value -= Number;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator*(const TYPE Number) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value * Number));
        }
        inline NumberType<TYPE, SIGNED>& operator*=(const TYPE Number)
        {
            m_Value *= Number;
            return (*this);
        }
        inline NumberType<TYPE, SIGNED> operator/(const TYPE Number) const
        {
            return (NumberType<TYPE, SIGNED>(m_Value / Number));
        }
        inline NumberType<TYPE, SIGNED>& operator/=(const TYPE Number)
        {
            m_Value /= Number;
            return (*this);
        }

        static const TYPE Min()
        {
            return (TypedMin(TemplateIntToType<SIGNED>()));
        }
        static const TYPE Max()
        {
            return (TypedMax(TemplateIntToType<SIGNED>()));
        }
        inline bool Negative() const
        {
            return (TypedNegative(TemplateIntToType<SIGNED>()));
        }
        inline const TYPE Abs() const
        {
            return (TypedAbs(TemplateIntToType<SIGNED>()));
        }

    private:
        uint16_t FillBuffer(char* buffer, const uint16_t maxLength, const NumberBase BaseType) const
        {
            TCHAR* Location = &buffer[maxLength - 1];
            TYPE Value = NumberType<TYPE, SIGNED>(m_Value).Abs();
            uint16_t Index = maxLength - 1 /* closing character */ - (Negative() ? 1 : 0) - (BaseType == BASE_OCTAL ? 1 : (BaseType == BASE_HEXADECIMAL ? 2 : 0));
            uint8_t Divider = (BaseType == BASE_UNKNOWN ? BASE_DECIMAL : BaseType);

            // Close it with a terminating character!!
            *Location-- = '\0';

            // Convert the number to a string
            do {
                uint8_t newDigit = (Value % Divider);
                if (newDigit < 10) {
                    *Location-- = static_cast<wchar_t>(newDigit + '0');
                } else {
                    *Location-- = static_cast<wchar_t>(newDigit - 10 + 'A');
                }
                Value = Value / Divider;
                Index--;

            } while ((Value != 0) && (Index > 0));

            if ((BaseType == BASE_OCTAL) || (BaseType == BASE_HEXADECIMAL)) {
                if (BaseType == BASE_HEXADECIMAL) {
                    *Location-- = 'x';
                }
                *Location-- = '0';
            }

            if (Negative()) {
                *Location = '-';
            }

            return (Index);
        }
        uint16_t FillBuffer(wchar_t* buffer, const uint16_t maxLength, const NumberBase BaseType)
        {
            wchar_t* Location = &buffer[maxLength];
            TYPE Value = NumberType<TYPE, SIGNED>(m_Value).Abs();
            uint16_t Index = maxLength - (Negative() ? 1 : 0) - (BaseType == BASE_OCTAL ? 1 : (BaseType == BASE_HEXADECIMAL ? 2 : 0));
            uint8_t Divider = (BaseType == BASE_UNKNOWN ? BASE_DECIMAL : BaseType);

            // Close it with a terminating character!!
            *Location-- = '\0';

            // Convert the number to a string
            do {
                uint8_t newDigit = (Value % Divider);
                if (newDigit < 10) {
                    *Location-- = static_cast<wchar_t>(newDigit + '0');
                } else {
                    *Location-- = static_cast<wchar_t>(newDigit - 10 + 'A');
                }
                Value = Value / Divider;
                Index--;

            } while ((Value != 0) && (Index > 0));

            if ((BaseType == BASE_OCTAL) || (BaseType == BASE_HEXADECIMAL)) {
                if (BaseType == BASE_HEXADECIMAL) {
                    *Location-- = 'x';
                }
                *Location-- = '0';
            }

            if (Negative()) {
                *Location = '-';
            }

            return (Index);
        }
        template <typename NUMBER>
        static uint32_t
        Convert(
            const wchar_t* Start,
            const uint32_t MaxLength,
            NUMBER& Value,
            const NumberBase Type,
            const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            /* Do the conversion from string to the proper number. */
            bool Success = true;
            const wchar_t* Text = Start;
            bool Sign = false;
            NumberBase Base = Type;
            NUMBER Max = NUMBER_MAX_SIGNED(NUMBER);
            uint32_t ItemsLeft = MaxLength;

            // We start at 0
            Value = 0;

            // Convert the number until we reach the 0 character.
            while ((ItemsLeft != 0) && (Success == true) && (*Text != '\0')) {
                if ((Value == 0) && (*Text == '0') && (Base == BASE_UNKNOWN)) {
                    // Base change, move over to an OCTAL conversion
                    Base = BASE_OCTAL;
                } else if ((Value == 0) && (toupper(*Text) == 'X') && ((Base == BASE_OCTAL) || (Base == BASE_HEXADECIMAL))) {
                    // Base change, move over to an HEXADECIMAL conversion
                    Base = BASE_HEXADECIMAL;
                } else if ((Value == 0) && ((*Text == '+') || ((*Text == '-')) || (*Text == ' ') || (*Text == '\t') || (*Text == '0'))) {
                    // Is it a sign change ???
                    if (*Text == '-') {
                        Sign = true;
                        Max = NUMBER_MIN_SIGNED(NUMBER);
                    }
                } else {
                    if (Base == BASE_UNKNOWN) {
                        Base = BASE_DECIMAL;
                    }

                    if ((*Text >= '0') && (*Text <= '7')) {
                        if (Sign) {
                            int8_t Digit = ('0' - *Text);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = (*Text - '0');

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else if ((*Text >= '8') && (*Text <= '9') && (Base != BASE_OCTAL)) {
                        if (Sign) {
                            int8_t Digit = ('0' - *Text);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = (*Text - '0');

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else if ((toupper(*Text) >= 'A') && (toupper(*Text) <= 'F') && (Base == BASE_HEXADECIMAL)) {
                        if (Sign) {
                            int8_t Digit = static_cast<int8_t>('A' - toupper(*Text) - 10);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = static_cast<int8_t>(toupper(*Text) - 'A' + 10);

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else {
                        Success = false;
                    }
                }

                // Get the next character in line.
                if (Success) {
                    Text++;
                    ItemsLeft--;
                }
            }

            return (MaxLength - ItemsLeft);
        }

        template <typename NUMBER>
        static uint32_t
        Convert(
            const wchar_t* Start,
            const uint32_t MaxLength,
            NUMBER& Value,
            const NumberBase Type,
            const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            /* Do the conversion from string to the proper number. */
            bool Success = true;
            const wchar_t* Text = Start;
            NumberBase Base = Type;
            NUMBER Max = NUMBER_MAX_UNSIGNED(NUMBER);
            uint32_t ItemsLeft = MaxLength;

            // We start at 0
            Value = 0;

            // Convert the number until we reach the 0 character.
            while ((ItemsLeft != 0) && (Success == true) && (*Text != '\0')) {
                if ((Value == 0) && (*Text == '0') && (Base == BASE_UNKNOWN)) {
                    // Base change, move over to an OCTAL conversion
                    Base = BASE_OCTAL;
                } else if ((Value == 0) && (toupper(*Text) == 'X') && ((Base == BASE_OCTAL) || (Base == BASE_HEXADECIMAL))) {
                    // Base change, move over to an HEXADECIMAL conversion
                    Base = BASE_HEXADECIMAL;
                } else if ((Value == 0) && ((*Text == '+') || (*Text == ' ') || (*Text == '\t') || (*Text == '0'))) {
                    // Skip all shit and other white spaces
                } else {
                    if (Base == BASE_UNKNOWN) {
                        Base = BASE_DECIMAL;
                    }

                    if ((*Text >= '0') && (*Text <= '7')) {
                        uint8_t Digit = (*Text - '0');

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else if ((*Text >= '8') && (*Text <= '9') && (Base != BASE_OCTAL)) {
                        uint8_t Digit = (*Text - '0');

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else if ((toupper(*Text) >= 'A') && (toupper(*Text) <= 'F') && (Base == BASE_HEXADECIMAL)) {
                        uint8_t Digit = static_cast<uint8_t>(toupper(*Text) - 'A' + 10);

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else {
                        Success = false;
                    }
                }

                // Get the next character in line.
                if (Success) {
                    Text++;
                    ItemsLeft--;
                }
            }

            return (MaxLength - ItemsLeft);
        }
        template <typename NUMBER>
        static uint32_t
        Convert(
            const char* Start,
            const uint32_t MaxLength,
            NUMBER& Value,
            const NumberBase Type,
            const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            /* Do the conversion from string to the proper number. */
            bool Success = true;
            const char* Text = Start;
            bool Sign = false;
            NumberBase Base = Type;
            NUMBER Max = NUMBER_MAX_SIGNED(NUMBER);
            uint32_t ItemsLeft = MaxLength;

            // We start at 0
            Value = 0;

            // Convert the number until we reach the 0 character.
            while ((ItemsLeft != 0) && (Success == true) && (*Text != '\0')) {
                if ((Value == 0) && (*Text == '0') && (Base == BASE_UNKNOWN)) {
                    // Base change, move over to an OCTAL conversion
                    Base = BASE_OCTAL;
                } else if ((Value == 0) && (toupper(*Text) == 'X') && ((Base == BASE_OCTAL) || (Base == BASE_HEXADECIMAL))) {
                    // Base change, move over to an HEXADECIMAL conversion
                    Base = BASE_HEXADECIMAL;
                } else if ((Value == 0) && ((*Text == '+') || ((*Text == '-')) || (*Text == ' ') || (*Text == '\t') || (*Text == '0'))) {
                    // Is it a sign change ???
                    if (*Text == '-') {
                        Sign = true;
                        Max = NUMBER_MIN_SIGNED(NUMBER);
                    }
                } else {
                    if (Base == BASE_UNKNOWN) {
                        Base = BASE_DECIMAL;
                    }

                    if ((*Text >= '0') && (*Text <= '7')) {
                        if (Sign) {
                            int8_t Digit = ('0' - *Text);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = (*Text - '0');

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else if ((*Text >= '8') && (*Text <= '9') && (Base != BASE_OCTAL)) {
                        if (Sign) {
                            int8_t Digit = ('0' - *Text);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = (*Text - '0');

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else if ((toupper(*Text) >= 'A') && (toupper(*Text) <= 'F') && (Base == BASE_HEXADECIMAL)) {
                        if (Sign) {
                            int8_t Digit = static_cast<int8_t>('A' - toupper(*Text) - 10);

                            if ((Value >= (Max / Base)) && (Digit >= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        } else {
                            int8_t Digit = static_cast<int8_t>(toupper(*Text) - 'A' + 10);

                            if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                                Value = (Value * Base) + Digit;
                            } else {
                                Success = false;
                            }
                        }
                    } else {
                        Success = false;
                    }
                }

                // Get the next character in line.
                if (Success) {
                    Text++;
                    ItemsLeft--;
                }
            }

            return (MaxLength - ItemsLeft);
        }

        template <typename NUMBER>
        static uint32_t
        Convert(
            const char* Start,
            const uint32_t MaxLength,
            NUMBER& Value,
            const NumberBase Type,
            const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            /* Do the conversion from string to the proper number. */
            bool Success = true;
            const char* Text = Start;
            NumberBase Base = Type;
            NUMBER Max = NUMBER_MAX_UNSIGNED(NUMBER);
            uint32_t ItemsLeft = MaxLength;

            // We start at 0
            Value = 0;

            // Convert the number until we reach the 0 character.
            while ((ItemsLeft != 0) && (Success == true) && (*Text != '\0')) {
                if ((Value == 0) && (*Text == '0') && (Base == BASE_UNKNOWN)) {
                    // Base change, move over to an OCTAL conversion
                    Base = BASE_OCTAL;
                } else if ((Value == 0) && (toupper(*Text) == 'X') && ((Base == BASE_OCTAL) || (Base == BASE_HEXADECIMAL))) {
                    // Base change, move over to an HEXADECIMAL conversion
                    Base = BASE_HEXADECIMAL;
                } else if ((Value == 0) && ((*Text == '+') || (*Text == ' ') || (*Text == '\t') || (*Text == '0'))) {
                    // Skip all shit and other white spaces
                } else {
                    if (Base == BASE_UNKNOWN) {
                        Base = BASE_DECIMAL;
                    }

                    if ((*Text >= '0') && (*Text <= '7')) {
                        uint8_t Digit = (*Text - '0');

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else if ((*Text >= '8') && (*Text <= '9') && (Base != BASE_OCTAL)) {
                        uint8_t Digit = (*Text - '0');

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else if ((toupper(*Text) >= 'A') && (toupper(*Text) <= 'F') && (Base == BASE_HEXADECIMAL)) {
                        uint8_t Digit = static_cast<uint8_t>(toupper(*Text) - 'A' + 10);

                        if ((Value <= (Max / Base)) && (Digit <= (Max - (Value * Base)))) {
                            Value = (Value * static_cast<uint8_t>(Base)) + Digit;
                        } else {
                            Success = false;
                        }
                    } else {
                        Success = false;
                    }
                }

                // Get the next character in line.
                if (Success) {
                    Text++;
                    ItemsLeft--;
                }
            }

            return (MaxLength - ItemsLeft);
        }
        inline const TYPE TypedAbs(const TemplateIntToType<true>& /* For compile time diffrentiation */) const
        {
            return (m_Value < 0 ? -m_Value : m_Value);
        }
        inline const TYPE TypedAbs(const TemplateIntToType<false>& /* For compile time diffrentiation */) const
        {
            return (m_Value);
        }
        inline bool TypedNegative(const TemplateIntToType<true>& /* For compile time diffrentiation */) const
        {
            return (m_Value < 0);
        }
        inline bool TypedNegative(const TemplateIntToType<false>& /* For compile time diffrentiation */) const
        {
            return (false);
        }
        static const TYPE TypedMin(const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            return (NUMBER_MIN_UNSIGNED(TYPE));
        }
        static const TYPE TypedMax(const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            return (NUMBER_MAX_UNSIGNED(TYPE));
        }
        static const TYPE TypedMin(const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            return (NUMBER_MIN_SIGNED(TYPE));
        }
        static const TYPE TypedMax(const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            return (NUMBER_MAX_SIGNED(TYPE));
        }

    private:
        TYPE m_Value;
    };

    class EXTERNAL Fractional {
    public:
        Fractional();
        Fractional(const int32_t& integer, const uint32_t& remainder = 0);
        Fractional(const Fractional& copy);
        Fractional(Fractional&& move);
        virtual ~Fractional();

        Fractional& operator=(const Fractional& RHS);
        Fractional& operator=(Fractional&& move);

    public:
        template <typename FLOATINGPOINTTYPE>
        FLOATINGPOINTTYPE Composit() const
        {
            uint16_t count = static_cast<uint16_t>(log10((float)m_Remainder)) + 1;
            uint32_t base = static_cast<uint32_t>(pow((float)10, count));

            return (static_cast<FLOATINGPOINTTYPE>(m_Integer) + static_cast<FLOATINGPOINTTYPE>(m_Remainder / base));
        }

        inline int32_t Integer() const
        {
            return (m_Integer);
        }

        inline uint32_t Remainder() const
        {
            return (m_Remainder);
        }

        bool operator==(const Fractional& RHS) const;
        bool operator!=(const Fractional& RHS) const;
        bool operator>=(const Fractional& RHS) const;
        bool operator<=(const Fractional& RHS) const;
        bool operator>(const Fractional& RHS) const;
        bool operator<(const Fractional& RHS) const;
        string Text(const uint8_t decimalPlaces) const;

    private:
        int32_t m_Integer;
        uint32_t m_Remainder;
    };

    template <bool SIGNED>
    class NumberType<Fractional, SIGNED> {
    public:
        static uint32_t Convert(
            const TCHAR* value,
            const uint32_t length,
            Fractional& number,
            const NumberBase formatting)
        {
            int32_t integer;
            uint32_t result = NumberType<int32_t, true>::Convert(value, length, integer, formatting);
            number = Fractional(integer, 0);

            return (result);
        }

        static const Fractional Min()
        {
            return (TypedMin(TemplateIntToType<SIGNED>()));
        }
        static const Fractional Max()
        {
            return (Fractional(NUMBER_MAX_SIGNED(int32_t), NUMBER_MAX_UNSIGNED(uint32_t)));
        }

    private:
        static const Fractional TypedMin(const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            return (Fractional(0, 0));
        }
        static const Fractional TypedMin(const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            return (Fractional(NUMBER_MIN_SIGNED(int32_t), NUMBER_MAX_UNSIGNED(uint32_t)));
        }
    };

    typedef NumberType<uint8_t, false> Unsigned8;
    typedef NumberType<int8_t, true> Signed8;
    typedef NumberType<uint16_t, false> Unsigned16;
    typedef NumberType<int16_t, true> Signed16;
    typedef NumberType<uint32_t, false> Unsigned32;
    typedef NumberType<int32_t, true> Signed32;
    typedef NumberType<uint64_t, false> Unsigned64;
    typedef NumberType<int64_t, true> Signed64;


    // BitArray

    template<uint8_t MAXBITS, typename DERIVED>
    class BitArrayBaseType  {
    public:
        static_assert(MAXBITS <= 64, "MAXBITS is too big");
        using T = typename std::conditional<MAXBITS <= 8, std::uint8_t,
                        typename std::conditional<MAXBITS <= 16, std::uint16_t,
                                typename std::conditional<MAXBITS <= 32, std::uint32_t, std::uint64_t>::type>::type>::type;

        uint8_t MaxSize() const
        {
            return (MAXBITS);
        }
        void Set(uint8_t index)
        {
            ASSERT(index < _Derived()->Size());
            _value |= ((1 << index) & ((1 << _Derived()->Size()) - 1));
        }
        void Clr(uint8_t index)
        {
            ASSERT(index < _Derived()->Size());
            _value &= ~(1 << index);
        }
        bool IsSet(uint8_t index) const
        {
            ASSERT(index < _Derived()->Size());
            return (_value & (1 << index));
        }
        bool Empty() const
        {
            return (_value == 0);
        }
        bool Full() const
        {
            return (_value == ((1 << _Derived()->Size()) - 1));
        }
        void Reset(T initial = 0)
        {
            _value = (initial & ((1 << _Derived()->Size()) - 1));
        }
        uint8_t Find() const
        {
            for (uint8_t i = 0; i < _Derived()->Size(); i++) {
                if (!IsSet(i)) {
                    return i;
                }
            }
            return (~0);
        }

    private:
        DERIVED* _Derived() { return static_cast<DERIVED*>(this); }
        const DERIVED* _Derived() const { return static_cast<const DERIVED*>(this); }

    protected:
        BitArrayBaseType()
        { /* leave construction to the deriving class */ }

        T _value;
    };

    template<uint8_t MAXBITS>
    class BitArrayType : public BitArrayBaseType<MAXBITS,BitArrayType<MAXBITS>>  {
    public:
        using BASE = BitArrayBaseType<MAXBITS,BitArrayType<MAXBITS>>;
        using T = typename BASE::T;

        BitArrayType(T initial = 0)
        {
            BASE::Reset(initial);
        }
        uint8_t Size() const
        {
            return (BASE::MaxSize());
        }
    };

    template<uint8_t MAXBITS>
    class BitArrayFlexType : public BitArrayBaseType<MAXBITS,BitArrayFlexType<MAXBITS>>  {
    public:
        using BASE = BitArrayBaseType<MAXBITS,BitArrayFlexType<MAXBITS>>;
        using T = typename BASE::T;

        BitArrayFlexType(uint8_t size = 0, T initial = 0)
        {
            Reset(size);
        }
        uint8_t Size() const
        {
            return _size;
        }
        void Reset(uint8_t size = 0, T initial = 0) /* shadows */
        {
            ASSERT(size <= BASE::MaxSize());
            _size = (size != 0? size : BASE::MaxSize());
            BASE::Reset(initial);
        }

    private:
        uint8_t _size;
    };

}
} // namespace Core

#endif // __NUMBER_H

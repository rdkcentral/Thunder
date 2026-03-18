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

// note does not include Module.h for now as that also would drag in Portability.h (and we do not need Module.h here for now)

#include "Module.h"
#include "Trace.h"

#include <limits>
#include <cfenv>

namespace Thunder {
namespace ExtraNumberDefinitions {

#if !defined(__STDCPP_FLOAT16_T__)
      #define __STDC_WANT_IEC_60559_TYPES__
      #include <cfloat>

      #if (defined(__GCC__) || defined(__GNUG__)) && (defined(__i386__) || defined(__amd64__)) && defined(__FLT16_MIN__) && defined(__FLT16_MAX__)
            using float16_t = _Float16;
      #elif (defined(__GCC__) || defined(__GNUG__)) && (defined(__arm__) || defined(__aarch64__)) && defined(__FLT16_MIN__) && defined(__FLT16_MAX__)
            using float16_t = __fp16;
      #else
            using float16_t = float; // Fallback
      #endif

#else
      #include <stdfloat>

      using float16_t = std::float16_t
#endif

} // ExtraNumberDefinitions
} // Thunder

namespace Thunder {
namespace Core {

    class SInt24 {
    public:
        static constexpr uint8_t SizeOf = 3;
        static constexpr uint32_t Max = 0x007FFFFF;
        static constexpr int32_t Min = 0xFF800000;

        using InternalType = int32_t;

        static constexpr int32_t SignificantBitsBitMask = 0x00FFFFFF;

        SInt24()
            : _value{ 0 }
        {}
        SInt24(const SInt24&) = default;
        SInt24(SInt24&&) = default;
        ~SInt24() = default;

        SInt24& operator=(const SInt24&) = default;
        SInt24& operator=(SInt24&&) = default;

        template <typename T = int32_t>
        T AsSInt24() const
        {
            return ( static_cast<T>(_value & SignificantBitsBitMask) );
        }

        bool Overflowed() const {
            return ( _value == std::numeric_limits<InternalType>::max() );
        }

        // Arithmetic
        // ----------

        // Unary plus
        SInt24 operator+() const
        {
            return ( *this );
        }

        // Unary minus
        SInt24 operator-() const
        {
            return ( SInt24(-_value) );
        }

        // Addition
        SInt24 operator+(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false
                         || RHS.Overflowed() != false
                         || // Negative overflow
                            (   _value < 0
                              && RHS._value < 0
                              && (_value < (SInt24::Min - RHS._value))
                            )
                         || // Positive overflow
                            (   _value > 0
                              && RHS._value > 0
                              && (_value > (static_cast<InternalType>(SInt24::Max) - RHS._value))
                            )
                       )
                     ?
                       SInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       SInt24{ _value + RHS._value }
                   );
        }

        // Subtraction
        SInt24 operator-(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false
                         || RHS.Overflowed() != false
                         || // Negative overflow
                            (    _value < 0
                              && RHS._value > 0
                              && (_value < (SInt24::Min + RHS._value))
                            )
                         || // Positive overflow
                            (    _value > 0
                              && RHS._value < 0
                              && (_value > (static_cast<InternalType>(SInt24::Max) + RHS._value))
                            )
                       )
                     ?
                       SInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       SInt24{ _value - RHS._value }
                   );
        }

        // Multipication
        SInt24 operator*(const SInt24& RHS) const
        {
            return (   (
                            Overflowed() != false
                         || RHS.Overflowed() != false
                         || ( // Both positive or both negative
                                 (    _value > 0
                                   && RHS._value > 0
                                   && _value > (static_cast<InternalType>(SInt24::Max) / (RHS._value))
                                 )
                              ||
                                 (    _value < 0
                                   && RHS._value < 0
                                      // Unequal range integer; avoid result division equals 0
                                   && ( _value < RHS._value
                                        ?
                                          (RHS._value) < (static_cast<InternalType>(SInt24::Max) / _value)
                                        :
                                          _value < (static_cast<InternalType>(SInt24::Max) / (RHS._value))
                                      )
                                 )
                            )
                         || ( // One positive and one negative
                                 (
                                      _value > 0
                                   && RHS._value < 0
                                   && (static_cast<InternalType>(SInt24::Min) / RHS._value) < (_value)
                                 )
                              || (
                                      _value < 0
                                   && RHS._value > 0
                                   && (static_cast<InternalType>(SInt24::Min) / _value) < (RHS._value)
                                 )
                            )
                       )
                     ?
                       SInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       SInt24{ _value * RHS._value }
                   );
        }

        // Division
        SInt24 operator/(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value == 0
                       ) 
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value / RHS._value }
                   );
        }

        // Remainder
        SInt24 operator%(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value == 0
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value % RHS._value }
                   );
        }

        // BitWise NOT
        SInt24 operator~() const
        {
            return (   Overflowed() != false 
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ ~_value }
                   );
        }

        // BitWise AND
        SInt24 operator&(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value & RHS._value }
                   );
        }

        // BitWise OR
        SInt24 operator|(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value | RHS._value }
                   );
        }

        // BitWise XOR
        SInt24 operator^(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value ^ RHS._value }
                   );
        }

        // BitWise left shift
        SInt24 operator<<(const SInt24& RHS) const
        {
            using UnsignedInternalType = std::make_unsigned<InternalType>::type;

            static_assert(   std::is_unsigned<decltype(SInt24::Max)>::value != false
                          && sizeof(UnsignedInternalType) >= sizeof(InternalType)
                          , ""
            );

            UnsignedInternalType result{ static_cast<UnsignedInternalType>(_value) << std::min(static_cast<UnsignedInternalType>(RHS._value), static_cast<UnsignedInternalType>(23)) };

            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value < 0
                         || RHS._value >= 24
                         || _value < 0
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ result & SignificantBitsBitMask }
                   );
        }

        // BitWise right shift
        SInt24 operator>>(const SInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value < 0
                         || RHS._value >= 24
                         // Implementation defined, here defined as overflow
                         || _value < 0
                       )
                     ? SInt24{ std::numeric_limits<InternalType>::max() }
                     : SInt24{ _value >> RHS._value }
                   );
        }

        // Logical
        // -------

        // Negation
        bool operator!() const
        {
            return ( !_value );
        }

        // AND
        bool operator&&(const SInt24& RHS) const
        {
            return ( _value && RHS._value );
        }

        // inclusive OR
        bool operator||(const SInt24& RHS) const
        {
            return ( _value || RHS._value );
        }

        // Assignment
        // ----------

        // single assignment
//        SInt24& operator=(const SInt24& RHS) = default;

        // addition assignment
        SInt24& operator+=(const SInt24& RHS)
        {
            (*this) = ((*this) + RHS);

            return ( *this );
        }

        // subtraction assignment
        SInt24& operator-=(const SInt24& RHS)
        {
            (*this) = ((*this) - RHS);

            return ( *this );
        }

        // multiplication assignment
        SInt24& operator*=(const SInt24& RHS)
        {
            (*this) = ((*this) * RHS);

            return ( *this );
        }

        // division assignment
        SInt24& operator/=(const SInt24& RHS)
        {
            (*this) = ((*this) / RHS);

            return ( *this );
        }

        // remainder assignment
        SInt24& operator%=(const SInt24& RHS)
        {
            (*this) = ((*this) % RHS);

            return ( *this );
        }

        // bitwise AND assignment
        SInt24& operator&=(const SInt24& RHS)
        {
            (*this) = ((*this) & RHS);

            return ( *this );
        }

        // bitwise OR assignment
        SInt24& operator|=(const SInt24& RHS)
        {
            (*this) = ((*this) | RHS);

            return ( *this );
        }

        // bitwise XOR assignment
        SInt24& operator^=(const SInt24& RHS)
        {
            (*this) = ((*this) ^ RHS);

            return ( *this );
        }

        // bitwise left shift assignment
        SInt24& operator<<=(const SInt24& RHS)
        {
            (*this) = ((*this) << RHS);

            return ( *this );
        }

        // bitwise right shift assignment
        SInt24& operator>>=(const SInt24& RHS)
        {
            (*this) = ((*this) >> RHS);

            return ( *this );
        }

        // increment and decrement
        // -----------------------

        // pre-increment
        SInt24& operator++()
        {
            (*this) += SInt24(1);

            return ( *this );
        }

        // pre-decrement
        SInt24& operator--()
        {
            (*this) -= SInt24(1);

            return ( *this );
        }

        // post-increment
        SInt24 operator++(int)
        {
            InternalType value{ _value };

            /* SInt24& */ ++(*this);

            return ( SInt24(value) );
        }

        // post-decrement
        SInt24 operator--(int)
        {
            InternalType value{ _value };

            /* SInt24& */ --(*this); 

            return ( SInt24(value) );
        }

        // comparison
        // ----------

        // Equal to
        bool operator==(const SInt24& RHS) const
        {
            return ( ((*this) != RHS) != true );
        }

        // Not equal to
        bool operator!=(const SInt24& RHS) const
        {
            return (    ((*this) < RHS) != false
                     || ((*this) > RHS) != false
                   );
        }

        // Less than
        bool operator<(const SInt24& RHS) const
        {
             ASSERT(   Overflowed() != true
                   && RHS.Overflowed() != true
            );

            return ( _value  < RHS._value );
        }

        // Greater than
        bool operator>(const SInt24& RHS) const
        {
             ASSERT(   Overflowed() != true
                   && RHS.Overflowed() != true
            );

            return ( _value > RHS._value );
        }

        // Less than or equal to
        bool operator<=(const SInt24& RHS) const
        {
            return ( ((*this) < RHS) || ((*this) == RHS) );
        }

        // Greater than or equal to
        bool operator>=(const SInt24& RHS) const
        {
            return ( ((*this) > RHS) || ((*this) == RHS) );
        }

        // other
        // -----

        // comma
        SInt24& operator,(SInt24& RHS)
        {
            return RHS;
        }

        // Conversion constructor, integral
        template<typename T, typename std::enable_if<std::is_integral<T>::value != false, bool>::type = true>
        SInt24(const T value)
            : SInt24{}
        {
            bool overflow = !( (    sizeof(T) >= sizeof(InternalType)
                                 && (   (    std::is_unsigned<T>::value != false 
                                          && value <= static_cast<T>(SInt24::Max)
                                        )
                                     || (    std::is_signed<T>::value != false
                                          && value >= static_cast<T>(SInt24::Min)
                                          && value <= static_cast<T>(SInt24::Max)
                                        )
                                    )
                               )
                               ||
                               (    sizeof(T) < sizeof(InternalType)
                                 && (   (    std::is_unsigned<T>::value != false 
                                          && static_cast<InternalType>(value) <= static_cast<InternalType>(SInt24::Max)
                                        )
                                     || (    std::is_signed<T>::value != false
                                          && static_cast<InternalType>(value) >= SInt24::Min
                                          && static_cast<InternalType>(value) <= static_cast<InternalType>(SInt24::Max)
                                        )
                                    )
                               )
                             );

            ASSERT(overflow != true);

            _value = ( overflow != false ? std::numeric_limits<InternalType>::max() : static_cast<InternalType>(value) );
        }

        // Conversion, assignment, integral
        template<typename T>
        typename std::enable_if<std::is_integral<T>::value != false, SInt24>::type
        operator=(const T value)
        {
            return (*this) = SInt24{ value };
        }

        // Conversion, integral
        template<typename T, typename std::enable_if<std::is_integral<T>::value != false, bool>::type = true>
        operator T() const
        {
            ASSERT(   Overflowed() != true
                   && ( (    sizeof(T) >= sizeof(InternalType)
                          && ( (    std::is_unsigned<T>::value != false 
                                 && _value >= 0
                                 && static_cast<T>(_value) <= std::numeric_limits<T>::max()
                               )
                               ||
                               (    std::is_signed<T>::value != false
                                 && static_cast<T>(_value) >= std::numeric_limits<T>::lowest()
                                 && static_cast<T>(_value) <= std::numeric_limits<T>::max()
                               )
                             )
                        )
                        ||
                        (
                             sizeof(T) < sizeof(InternalType)
                          && ( (    std::is_unsigned<T>::value != false 
                                 && _value >= 0
                                 && _value <= static_cast<InternalType>(std::numeric_limits<T>::max())
                               )
                               ||
                               (    std::is_signed<T>::value != false
                                 && _value >= static_cast<InternalType>(std::numeric_limits<T>::lowest())
                                 && _value <= static_cast<InternalType>(std::numeric_limits<T>::max())
                               )
                             )
                        )
                     )
            );

            return ( static_cast<T>(_value) );
        }

        explicit operator bool() const
        {
            return ( static_cast<bool>(_value) );
        }

        // Conversion, floating point
        template<typename T, typename std::enable_if<(std::is_floating_point<T>::value != false || std::is_same<T, ::Thunder::ExtraNumberDefinitions::float16_t>::value != false) && FE_ALL_EXCEPT != 0, bool>::type = true>
        operator T() const
        {
            ASSERT(Overflowed() != true);

            constexpr int floatingPointErrorMask = FE_ALL_EXCEPT; // FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW | additional implementation defined

            VARIABLE_IS_NOT_USED int exception = feclearexcept(floatingPointErrorMask);

            T result{ static_cast<T>(_value) };

            // FE_INEXACT may never be raised
            ASSERT(   exception == 0
                   && fetestexcept(floatingPointErrorMask) == 0
            );

            return ( result );
        }

#ifdef _0
        // Function call
        template<itypename... Args>
        R operator()(Args... args)()
        {}
#endif

        // Arithmetic, addition
        template<typename T>
        SInt24 operator +(const T value) const
        {
            return ( (*this) + SInt24{ value } );
        }

        // Arithmetic, subtraction
        template<typename T>
        SInt24 operator-(const T value) const
        {
            return ( (*this) - SInt24{ value } );
        }

        // Arithmetic, multiplication
        template<typename T>
        SInt24 operator*(const T value) const
        {
            return ( (*this) * SInt24{ value } );
        }

        // Arithmetic, divsion
        template<typename T>
        SInt24 operator/(const T value) const
        {
            return ( (*this) / SInt24{ value } );
        }

        // Arithmetic, remainder
         template<typename T>
        SInt24 operator%(const T value) const
        {
            return ( (*this) % SInt24{ value } );
        }

        // Arithmetic, bitwise AND
        template<typename T>
        SInt24 operator&(const T value) const
        {
            return ( (*this) & SInt24{ value } );
        }

        // Arithmetic, bitwise OR
        template<typename T>
        SInt24 operator|(const T value) const
        {
            return ( (*this) | SInt24{ value } );
        }

        // Arithmetic, bitwise XOR
        template<typename T>
        SInt24 operator^(const T value) const
        {
            return ( (*this) ^ SInt24{ value } );
        }

        // Arithmetic, bitwise left shift
        template<typename T>
        SInt24 operator<<(const T value) const
        {
            return ( (*this) << SInt24{ value } );
        }

        // Arithmetic, bitwise right shift
        template<typename T>
        SInt24 operator>>(const T value) const
        {
            return ( (*this) >> SInt24{ value } );
        }

        // Logical, AND
        template<typename T>
        bool operator&&(const T value) const
        {
            return ( (*this) && SInt24{ value } );
        }

        // Logical, inclusive OR
        template<typename T>
        bool operator||(const T value) const
        {
            return ( (*this) || SInt24{ value } );
        }

        // Assignment, addition assignment
        template<typename T>
        SInt24& operator+=(const T value) 
        {
            (*this) = ((*this) + SInt24{ value });

            return ( *this );
        }

        // Assignment, subtraction assignment
        template<typename T>
        SInt24& operator-=(const T value)
        {
            (*this) = ((*this) - SInt24{ value });

            return ( *this );
        }

        // Assignment, multiplication assignment
        template<typename T>
        SInt24& operator*=(const T value)
        {
            (*this) = ((*this) * SInt24{ value });

            return ( *this );
        }

        // Assignment, division assignment
        template<typename T>
        SInt24& operator/=(const T value)
        {
            (*this) = ((*this) / SInt24{ value });

            return ( *this );
        }

        // Assignment, remainder assignment
        template<typename T>
        SInt24& operator%=(const T value)
        {
            (*this) = ((*this) % SInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise AND assignment
        template<typename T>
        SInt24& operator&=(const T value)
        {
            (*this) = ((*this) & SInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise OR assignment
        template<typename T>
        SInt24& operator|=(const T value) 
        {
            (*this) = ((*this) | SInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise XOR assignment
        template<typename T>
        SInt24& operator^=(const T value)
        {
            (*this) = ((*this) ^ SInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise left shift assignment
        template<typename T>
        SInt24& operator<<=(const T value)
        {
            (*this) = ((*this) << SInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise right shift assignment
        template<typename T>
        SInt24& operator>>=(const T value) 
        {
            (*this) = ((*this) >> SInt24{ value });

            return ( *this );
        }

        // Comparison, Equal to
        template<typename T>
        bool operator==(const T value) const
        {
            return ( (*this) == SInt24{ value } );
        }

        // Comparison, Not equal to
        template<typename T>
        bool operator!=(const T value) const
        {
            return ( (*this) != SInt24{ value } );
        }

        // Comparison, Less than
        template<typename T>
        bool operator<(const T value) const
        {
            return ( (*this) < SInt24{ value } );
        }

        // Comparison, Greater than
        template<typename T>
        bool operator>(const T value) const
        {
            return ( (*this) > SInt24{ value } );
        }

        // Comparison, Less than or equal to
        template<typename T>
        bool operator<=(const T value) const
        {
            return ( (*this) <= SInt24{ value } );
        }

        // Comparison, Greater than or equal to
        template<typename T>
        bool operator>=(const T value) const
        {
            return ( (*this) >= SInt24{ value } );
        }

    private:
        InternalType _value;
    };

    class UInt24 {
    public:
        static constexpr uint8_t SizeOf = 3;
        static constexpr uint32_t Max = 0xFFFFFF;
        static constexpr uint32_t Min = 0;

        using InternalType = uint32_t;

        static constexpr int32_t SignificantBitsBitMask = 0x00FFFFFF;

        UInt24()
            : _value(0)
        {}
        UInt24(const UInt24&) = default;
        UInt24(UInt24&&) = default;
        ~UInt24() = default;

        UInt24& operator=(const UInt24&) = default;
        UInt24& operator=(UInt24&&) = default;

        template <typename T = uint32_t>
        T AsUInt24() const
        {
            return ( static_cast<T>(_value & SignificantBitsBitMask) );
        }

        bool Overflowed() const
        {
            return ( _value == std::numeric_limits<InternalType>::max() );
        }

        // Arithmetic
        // ----------

        // Unary plus
        UInt24 operator+() const
        {
            return ( *this );
        }

        // Unary minus
        SInt24 operator-() const
        {
            return ( -SInt24(_value) );
        }

        // Addition
        UInt24 operator+(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false
                         || RHS.Overflowed() != false
                         || // Positive overflow
                            (_value > (UInt24::Max - RHS._value))
                       )
                     ?
                       UInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       UInt24{ _value + RHS._value }
                   );
        }

        // Subtraction
        UInt24 operator-(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false
                         || RHS.Overflowed() != false
                         || // Negative overflow
                            ((_value - UInt24::Min) < (RHS._value))
                       )
                     ?
                       UInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       UInt24{ _value - RHS._value }
                   );
        }

        // Multipication
        UInt24 operator*(const UInt24& RHS) const
        {
            return (   (
                            Overflowed() != false
                         || RHS.Overflowed() != false
                         || (     RHS._value > 0
                              &&  _value > (static_cast<InternalType>(UInt24::Max) / (RHS._value))
                            )
                       )
                     ?
                       UInt24{ std::numeric_limits<InternalType>::max() }
                     :
                       UInt24{ _value * RHS._value }
                   );
        }

        // Division
        UInt24 operator/(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value == 0
                       ) 
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value / RHS._value }
                   );
        }

        // Remainder
        UInt24 operator%(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value == 0
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value % RHS._value }
                   );
        }

        // BitWise NOT
        UInt24 operator~() const
        {
            return (   Overflowed() != false 
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ (~_value) & SignificantBitsBitMask }
                   );
        }

        // BitWise AND
        UInt24 operator&(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value & RHS._value }
                   );
        }

        // BitWise OR
        UInt24 operator|(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value | RHS._value }
                   );
        }

        // BitWise XOR
        UInt24 operator^(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value ^ RHS._value }
                   );
        }

        // BitWise left shift
        UInt24 operator<<(const UInt24& RHS) const
        {
            InternalType result{ _value << std::min(RHS._value, static_cast<InternalType>(23)) };

            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value >= 24
                         || _value < UInt24::Min
                         || result > UInt24::Max
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ result }
                   );
        }

        // BitWise right shift
        UInt24 operator>>(const UInt24& RHS) const
        {
            return (   (    Overflowed() != false 
                         || RHS.Overflowed() != false
                         || RHS._value >= 24
                         || _value < UInt24::Min
                       )
                     ? UInt24{ std::numeric_limits<InternalType>::max() }
                     : UInt24{ _value >> RHS._value }
                   );
        }

        // Logical
        // -------

        // Negation
        bool operator!() const
        {
            return ( !_value );
        }

        // AND
        bool operator&&(const UInt24& RHS) const
        {
            return ( _value && RHS._value );
        }

        // inclusive OR
        bool operator||(const UInt24& RHS) const
        {
            return ( _value || RHS._value );
        }

        // Assignment
        // ----------

        // single assignment
//        UInt24& operator=(const UInt24& RHS) = default;

        // addition assignment
        UInt24& operator+=(const UInt24& RHS)
        {
            (*this) = ((*this) + RHS);

            return ( *this );
        }

        // subtraction assignment
        UInt24& operator-=(const UInt24& RHS)
        {
            (*this) = ((*this) - RHS);

            return ( *this );
        }

        // multiplication assignment
        UInt24& operator*=(const UInt24& RHS)
        {
            (*this) = ((*this) * RHS);

            return ( *this );
        }

        // division assignment
        UInt24& operator/=(const UInt24& RHS)
        {
            (*this) = ((*this) / RHS);

            return ( *this );
        }

        // remainder assignment
        UInt24& operator%=(const UInt24& RHS)
        {
            (*this) = ((*this) % RHS);

            return ( *this );
        }

        // bitwise AND assignment
        UInt24& operator&=(const UInt24& RHS)
        {
            (*this) = ((*this) & RHS);

            return ( *this );
        }

        // bitwise OR assignment
        UInt24& operator|=(const UInt24& RHS)
        {
            (*this) = ((*this) | RHS);

            return ( *this );
        }

        // bitwise XOR assignment
        UInt24& operator^=(const UInt24& RHS)
        {
            (*this) = ((*this) ^ RHS);

            return ( *this );
        }

        // bitwise left shift assignment
        UInt24& operator<<=(const UInt24& RHS)
        {
            (*this) = ((*this) << RHS);

            return ( *this );
        }

        // bitwise right shift assignment
        UInt24& operator>>=(const UInt24& RHS)
        {
            (*this) = ((*this) >> RHS);

            return ( *this );
        }

        // increment and decrement
        // -----------------------

        // pre-increment
        UInt24& operator++()
        {
            (*this) += UInt24(1);

            return ( *this );
        }

        // pre-decrement
        UInt24& operator--()
        {
            (*this) -= UInt24(1);

            return ( *this );
        }

        // post-increment
        UInt24 operator++(int)
        {
            InternalType value{ _value };

            /* UInt24& */ ++(*this);
            
            return ( UInt24(value) );
        }

        // post-decrement
        UInt24 operator--(int)
        {
            InternalType value{ _value };

            /* UInt24& */ --(*this);

            return ( UInt24(value) );
        }

        // comparison
        // ----------

        // Equal to
        bool operator==(const UInt24& RHS) const
        {
            return ( ((*this) != RHS) != true );
        }

        // Not equal to
        bool operator!=(const UInt24& RHS) const
        {
            return (    ((*this) < RHS) != false
                     || ((*this) > RHS) != false
                   );
        }

        // Less than
        bool operator<(const UInt24& RHS) const
        {
             ASSERT(   Overflowed() != true
                   && RHS.Overflowed() != true
            );

            return ( _value  < RHS._value );
        }

        // Greater than
        bool operator>(const UInt24& RHS) const
        {
             ASSERT(   Overflowed() != true
                   && RHS.Overflowed() != true
            );

            return ( _value > RHS._value );
        }

        // Less than or equal to
        bool operator<=(const UInt24& RHS) const
        {
            return ( ((*this) < RHS) || ((*this) == RHS) );
        }

        // Greater than or equal to
        bool operator>=(const UInt24& RHS) const
        {
            return ( ((*this) > RHS) || ((*this) == RHS) );
        }

        // other
        // -----

        // comma
        UInt24& operator,(UInt24& RHS)
        {
            return RHS;
        }

        // Conversion constructor, integral
        template<typename T, typename std::enable_if<std::is_integral<T>::value != false, bool>::type = true>
        UInt24(const T value)
            : UInt24{}
        {
            bool overflow = !( (    sizeof(T) >= sizeof(InternalType)
                                 && (   (    std::is_unsigned<T>::value != false 
                                          && value <= static_cast<T>(UInt24::Max)
                                        )
                                     || (    std::is_signed<T>::value != false
                                          && value >= 0
                                          && value <= static_cast<T>(UInt24::Max)
                                        )
                                    )
                               )
                               ||
                               (    sizeof(T) < sizeof(InternalType)
                                 && (   (    std::is_unsigned<T>::value != false 
                                          && static_cast<InternalType>(value) <= static_cast<InternalType>(UInt24::Max)
                                        )
                                     || (    std::is_signed<T>::value != false
                                          && value >= 0
                                          && static_cast<InternalType>(value) <= static_cast<InternalType>(UInt24::Max)
                                        )
                                    )
                               )
                             );

            ASSERT(overflow != true);

            _value = ( overflow != false ? std::numeric_limits<InternalType>::max() : static_cast<InternalType>(value) );
        }

        // Conversion, assignment, integral
        template<typename T>
        typename std::enable_if<std::is_integral<T>::value != false, UInt24>::type
        operator=(const T value)
        {
            return (*this) = UInt24{ value };
        }

        // Conversion, integral
        template<typename T, typename std::enable_if<std::is_integral<T>::value != false, bool>::type = true>
        operator T() const
        {
            ASSERT(   Overflowed() != true
                   && ( (    sizeof(T) >= sizeof(InternalType)
                          && ( (    std::is_unsigned<T>::value != false 
                                 && static_cast<T>(_value) <= std::numeric_limits<T>::max()
                               )
                               ||
                               (    std::is_signed<T>::value != false
                                 && static_cast<T>(_value) >= std::numeric_limits<T>::lowest()
                                 && static_cast<T>(_value) <= std::numeric_limits<T>::max()
                               )
                             )
                        )
                        ||
                        (
                             sizeof(T) < sizeof(InternalType)
                          && ( (    std::is_unsigned<T>::value != false 
                                 && _value <= static_cast<InternalType>(std::numeric_limits<T>::max())
                               )
                               ||
                               (    std::is_signed<T>::value != false
                                 && _value >= static_cast<InternalType>(std::numeric_limits<T>::lowest())
                                 && _value <= static_cast<InternalType>(std::numeric_limits<T>::max())
                               )
                             )
                        )
                     )
            );

            return ( static_cast<T>(_value) );
        }

        explicit operator bool() const
        {
            return ( static_cast<bool>(_value) );
        }

        // Conversion, floating point
        template<typename T, typename std::enable_if<(std::is_floating_point<T>::value != false || std::is_same<T, ::Thunder::ExtraNumberDefinitions::float16_t>::value != false) && FE_ALL_EXCEPT != 0, bool>::type = true>
        operator T() const
        {
            ASSERT(Overflowed() != true);

            constexpr int floatingPointErrorMask = FE_ALL_EXCEPT; // FE_DIVBYZERO | FE_INEXACT | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW | additional implementation defined

            VARIABLE_IS_NOT_USED int exception = feclearexcept(floatingPointErrorMask);

            T result{ static_cast<T>(_value) };

            // FE_INEXACT may never be raised
            ASSERT(   exception == 0
                   && fetestexcept(floatingPointErrorMask) == 0
            );

            return ( result );
        }

#ifdef _0
        // Function call
        template<itypename... Args>
        R operator()(Args... args)()
        {}
#endif

        // Arithmetic, addition
        template<typename T>
        UInt24 operator +(const T value) const
        {
            return ( (*this) + UInt24{ value } );
        }

        // Arithmetic, subtraction
        template<typename T>
        UInt24 operator-(const T value) const
        {
            return ( (*this) - UInt24{ value } );
        }

        // Arithmetic, multiplication
        template<typename T>
        UInt24 operator*(const T value) const
        {
            return ( (*this) * UInt24{ value } );
        }

        // Arithmetic, divsion
        template<typename T>
        UInt24 operator/(const T value) const
        {
            return ( (*this) / UInt24{ value } );
        }

        // Arithmetic, remainder
         template<typename T>
        UInt24 operator%(const T value) const
        {
            return ( (*this) % UInt24{ value } );
        }

        // Arithmetic, bitwise AND
        template<typename T>
        UInt24 operator&(const T value) const
        {
            return ( (*this) & UInt24{ value } );
        }

        // Arithmetic, bitwise OR
        template<typename T>
        UInt24 operator|(const T value) const
        {
            return ( (*this) | UInt24{ value } );
        }

        // Arithmetic, bitwise XOR
        template<typename T>
        UInt24 operator^(const T value) const
        {
            return ( (*this) ^ UInt24{ value } );
        }

        // Arithmetic, bitwise left shift
        template<typename T>
        UInt24 operator<<(const T value) const
        {
            return ( (*this) << UInt24{ value } );
        }

        // Arithmetic, bitwise right shift
        template<typename T>
        UInt24 operator>>(const T value) const
        {
            return ( (*this) >> UInt24{ value } );
        }

        // Logical, AND
        template<typename T>
        bool operator&&(const T value) const
        {
            return ( (*this) && UInt24{ value } );
        }

        // Logical, inclusive OR
        template<typename T>
        bool operator||(const T value) const
        {
            return ( (*this) || UInt24{ value } );
        }

        // Assignment, addition assignment
        template<typename T>
        UInt24& operator+=(const T value) 
        {
            (*this) = ((*this) + UInt24{ value });

            return ( *this );
        }

        // Assignment, subtraction assignment
        template<typename T>
        UInt24& operator-=(const T value)
        {
            (*this) = ((*this) - UInt24{ value });

            return ( *this );
        }

        // Assignment, multiplication assignment
        template<typename T>
        UInt24& operator*=(const T value)
        {
            (*this) = ((*this) * UInt24{ value });

            return ( *this );
        }

        // Assignment, division assignment
        template<typename T>
        UInt24& operator/=(const T value)
        {
            (*this) = ((*this) / UInt24{ value });

            return ( *this );
        }

        // Assignment, remainder assignment
        template<typename T>
        UInt24& operator%=(const T value)
        {
            (*this) = ((*this) % UInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise AND assignment
        template<typename T>
        UInt24& operator&=(const T value)
        {
            (*this) = ((*this) & UInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise OR assignment
        template<typename T>
        UInt24& operator|=(const T value) 
        {
            (*this) = ((*this) | UInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise XOR assignment
        template<typename T>
        UInt24& operator^=(const T value)
        {
            (*this) = ((*this) ^ UInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise left shift assignment
        template<typename T>
        UInt24& operator<<=(const T value)
        {
            (*this) = ((*this) << UInt24{ value });

            return ( *this );
        }

        // Assignment, bitwise right shift assignment
        template<typename T>
        UInt24& operator>>=(const T value) 
        {
            (*this) = ((*this) >> UInt24{ value });

            return ( *this );
        }

        // Comparison, Equal to
        template<typename T>
        bool operator==(const T value) const
        {
            return ( (*this) == UInt24{ value } );
        }

        // Comparison, Not equal to
        template<typename T>
        bool operator!=(const T value) const
        {
            return ( (*this) != UInt24{ value } );
        }

        // Comparison, Less than
        template<typename T>
        bool operator<(const T value) const
        {
            return ( (*this) < UInt24{ value } );
        }

        // Comparison, Greater than
        template<typename T>
        bool operator>(const T value) const
        {
            return ( (*this) > UInt24{ value } );
        }

        // Comparison, Less than or equal to
        template<typename T>
        bool operator<=(const T value) const
        {
            return ( (*this) <= UInt24{ value } );
        }

        // Comparison, Greater than or equal to
        template<typename T>
        bool operator>=(const T value) const
        {
            return ( (*this) >= UInt24{ value } );
        }

    private:
        uint32_t _value;
    };

    // Out of class helpers for reordered operands of arithmetic operators
    // -------------------------------------------------------------------

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || // Negative overflow
                    (   value < 0
                     && static_cast<T>(RHS) < 0
                     && (value < (std::numeric_limits<T>::lowest() - static_cast<T>(RHS)))
                    )
                ||  // Positive overflow
                    (   value > 0
                     && static_cast<T>(RHS) > 0
                     && (value > (std::numeric_limits<T>::max() - static_cast<T>(RHS)))
                    )
                )
        );

        return ( value + static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 ||
                    (   value > 0
                     && (value > (std::numeric_limits<T>::max() - static_cast<T>(RHS)))
                    )
                )
        );

        return ( value + static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || // Negative overflow
                    (    value < 0
                      && static_cast<T>(RHS) > 0
                      && (value < (std::numeric_limits<T>::lowest() + static_cast<T>(RHS)))
                    )
                 || // Positive overflow
                    (    value > 0
                      && static_cast<T>(RHS) < 0
                      && (value > (std::numeric_limits<T>::max() + static_cast<T>(RHS)))
                    )
                )
        );

        return ( value - static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 ||
                    (    value < 0
                      && (value < (std::numeric_limits<T>::lowest() + static_cast<T>(RHS)))
                    )
                )
        );

        return ( value - static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || ( // Both positive or both negative
                         (    value > 0
                           && static_cast<T>(RHS) > 0
                           && static_cast<T>(value) > (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                         )
                      ||
                         (    static_cast<T>(value) < 0
                           && static_cast<T>(RHS) < 0
                           && static_cast<T>(value) < (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                         )
                    )
                 || ( // One positive and one negative
                         (
                              value > 0
                           && static_cast<T>(RHS) < 0
                           && static_cast<T>(RHS) < (std::numeric_limits<T>::lowest() / static_cast<T>(value))
                         )
                      || (
                              value < 0
                           && static_cast<T>(RHS) > 0
                           && static_cast<T>(value) < (std::numeric_limits<T>::lowest() / static_cast<T>(RHS))
                         )
                      )
                    )
        );

        return ( value * static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || (    value > 0
                      && RHS > 0
                      && static_cast<T>(value) > (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                    )
                 || (   value < 0
                     && RHS > 0
                     && static_cast<T>(value) < (std::numeric_limits<T>::lowest() / static_cast<T>(RHS))
                    )
                )
        );

        return ( value * static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator/(T value, const U& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value / static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator%(T value, const U& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value % static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator&(T value, const U& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value & static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator|(T value, const U& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value | static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator^(T value, const U& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value ^ static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                 || static_cast<T>(RHS) < 0
                 || value < 0
                )
        );

        return ( value << static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                 || value < 0
                )
        );

        return ( value << static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>(T value, const SInt24& RHS)
    {
        ASSERT(!(    RHS.Overflowed() != false 
                  || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                  || static_cast<T>(RHS) < 0
                     // Implementation defined, here defined as undefined
                  || value < 0
                )
        );

        return ( value >> static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>(T value, const UInt24& RHS)
    {
        ASSERT(!(    RHS.Overflowed() != false 
                  || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                     // Implementation defined, here defined as undefined
                  || value < 0
                )
        );

        return ( value >> static_cast<T>(RHS) );
    }

    // Out of class helpers for reordered operands of logical operators
    // ----------------------------------------------------------------

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator&&(T value, const U& RHS)
    {
        return ( value && static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator||(T value, const U& RHS)
    {
        return ( value || static_cast<T>(RHS) );
    }
 
    // Out of class helpers for reordered operands of assignment operators
    // -------------------------------------------------------------------

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator+=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value + RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator-=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value - RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator*=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value * RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator/=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value / RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator%=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value % RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator&=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value & RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator|=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value | RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator^=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value ^ RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator<<=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value << RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator>>=(T value, const U& RHS)
    {
        return ( value = static_cast<T>(value >> RHS) );
    }

    // Out of class helpers for reordered operands of comparison operators
    // -------------------------------------------------------------------

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator==(T value, const U& RHS)
    {
        return ( value == static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator!=(T value, const U& RHS)
    {
        return ( value != static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator<(T value, const U& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value < static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator>(T value, const U& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value > static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator<=(T value, const U& RHS)
    {
        return ( value <= static_cast<T>(RHS) );
    }

    template<typename T, typename U>
    typename std::enable_if<std::is_integral<T>::value && (std::is_same<U, SInt24>::value || std::is_same<U, UInt24>::value), T>::type
    operator>=(T value, const U& RHS)
    {
        return ( value >= static_cast<T>(RHS) );
    }

} // namespace Core
} // namespace Thunder 

namespace std { // seems to be allowed/mandatory to specialize inside the std namespace

template <>
class numeric_limits<Thunder::Core::SInt24> {

public:
    static constexpr bool is_specialized = true;

    static constexpr Thunder::Core::SInt24::InternalType min() noexcept { return Thunder::Core::SInt24::Min; }
    static constexpr Thunder::Core::SInt24::InternalType max() noexcept { return Thunder::Core::SInt24::Max; }
    static constexpr Thunder::Core::SInt24::InternalType lowest() noexcept { return min(); }

    static constexpr int digits = 23;
    static constexpr int digits10 = 6;
    static constexpr int max_digits10 = 0;

    static constexpr bool is_signed = true;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int radix = 2;

    static constexpr Thunder::Core::SInt24::InternalType epsilon() noexcept { return 0; }
    static constexpr Thunder::Core::SInt24::InternalType round_error() noexcept { return 0; }

    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;

    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;

    static constexpr Thunder::Core::SInt24::InternalType infinity() noexcept { return static_cast<Thunder::Core::SInt24::InternalType>(0); }
    static constexpr Thunder::Core::SInt24::InternalType quiet_NaN() noexcept { return static_cast<Thunder::Core::SInt24::InternalType>(0); }
    static constexpr Thunder::Core::SInt24::InternalType signaling_NaN() noexcept { return static_cast<Thunder::Core::SInt24::InternalType>(0); }
    static constexpr Thunder::Core::SInt24::InternalType denorm_min() noexcept { return static_cast<Thunder::Core::SInt24::InternalType>(0); }

    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = false;

    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr std::float_round_style round_style = std::round_toward_zero;
};

template <>
class numeric_limits<Thunder::Core::UInt24> {

public:
    static constexpr bool is_specialized = true;

    static constexpr Thunder::Core::UInt24::InternalType min() noexcept { return Thunder::Core::UInt24::Min; }
    static constexpr Thunder::Core::UInt24::InternalType max() noexcept { return Thunder::Core::UInt24::Max; }
    static constexpr Thunder::Core::UInt24::InternalType lowest() noexcept { return min(); }

    static constexpr int digits = 24;
    static constexpr int digits10 = 7;
    static constexpr int max_digits10 = 0;

    static constexpr bool is_signed = false;
    static constexpr bool is_integer = true;
    static constexpr bool is_exact = true;
    static constexpr int radix = 2;

    static constexpr Thunder::Core::UInt24::InternalType epsilon() noexcept { return 0; }
    static constexpr Thunder::Core::UInt24::InternalType round_error() noexcept { return 0; }

    static constexpr int min_exponent = 0;
    static constexpr int min_exponent10 = 0;
    static constexpr int max_exponent = 0;
    static constexpr int max_exponent10 = 0;

    static constexpr bool has_infinity = false;
    static constexpr bool has_quiet_NaN = false;
    static constexpr bool has_signaling_NaN = false;
    static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
    static constexpr bool has_denorm_loss = false;

    static constexpr Thunder::Core::UInt24::InternalType infinity() noexcept { return static_cast<Thunder::Core::UInt24::InternalType>(0); }
    static constexpr Thunder::Core::UInt24::InternalType quiet_NaN() noexcept { return static_cast<Thunder::Core::UInt24::InternalType>(0); }
    static constexpr Thunder::Core::UInt24::InternalType signaling_NaN() noexcept { return static_cast<Thunder::Core::UInt24::InternalType>(0); }
    static constexpr Thunder::Core::UInt24::InternalType denorm_min() noexcept { return static_cast<Thunder::Core::UInt24::InternalType>(0); }

    static constexpr bool is_iec559 = false;
    static constexpr bool is_bounded = true;
    static constexpr bool is_modulo = true;

    static constexpr bool traps = true;
    static constexpr bool tinyness_before = false;
    static constexpr std::float_round_style round_style = std::round_toward_zero;
};

} // namespace std

using uint24_t = Thunder::Core::UInt24;
using int24_t = Thunder::Core::SInt24;

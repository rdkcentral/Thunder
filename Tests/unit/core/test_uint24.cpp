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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include "core/core.h"

#define QUICK_TEST

namespace Thunder {
namespace Tests {
namespace Core {

    using SInt24InternalType = ::Thunder::Core::SInt24::InternalType;
    using UInt24InternalType = ::Thunder::Core::UInt24::InternalType;

    // Current types
    using UInt24InstantiationType = ::Thunder::Core::UInt24::InternalType;
    using SInt24InstantiationType = ::Thunder::Core::SInt24::InternalType;

    constexpr UInt24InternalType UInt24Minimum{ std::numeric_limits<::Thunder::Core::UInt24>::lowest() };
    constexpr UInt24InternalType UInt24Maximum{ std::numeric_limits<::Thunder::Core::UInt24>::max() };

    constexpr SInt24InternalType SInt24Maximum{ std::numeric_limits<::Thunder::Core::SInt24>::max() };

    constexpr UInt24InstantiationType UInt24InstantiationTypeMinimum{ std::numeric_limits<UInt24InstantiationType>::lowest() };
    constexpr UInt24InstantiationType UInt24InstantiationTypeMaximum{ std::numeric_limits<UInt24InstantiationType>::max() };

    constexpr size_t UInt24InternalTypeSize{ sizeof(UInt24InternalType) };

    // Preconditions
    static_assert(sizeof(UInt24InstantiationType) >= 4, "");

    static_assert(UInt24Minimum == 0, "");
    static_assert(UInt24Maximum > 0, "");

    static_assert(UInt24InstantiationTypeMinimum == 0, "");
    static_assert(UInt24InstantiationTypeMaximum > 0, "");

    static_assert(UInt24Minimum == UInt24InstantiationTypeMinimum, "");
    static_assert(UInt24Maximum < UInt24InstantiationTypeMaximum, "");

    static_assert(SInt24Maximum < UInt24InstantiationTypeMaximum, "");

    // Essentials
    // ----------

    // std::is_integral, std:;is_arithmetic
    TEST(UnsignedInt24, IsIntegralArithmetic)
    {
        EXPECT_FALSE(std::is_integral<::Thunder::Core::UInt24>::value);
        EXPECT_FALSE(std::is_arithmetic<::Thunder::Core::UInt24>::value);
    }

    // Conversion constructor, integral
    TEST(UnsignedInt24, ConversionConstructorIntegral)
    {
#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
            EXPECT_FALSE(::Thunder::Core::UInt24{ value }.Overflowed());
        }

#ifdef NDEBUG // ASSERTs in DEBUG
        UInt24InternalType value{ UInt24Maximum + 1 };

        EXPECT_TRUE(::Thunder::Core::UInt24{ value }.Overflowed());
#endif
    }

    // Conversion assignment, integral
    TEST(UnsignedInt24, ConversionAssignmentIntegral)
    {
#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
            ::Thunder::Core::UInt24 X = value;

            EXPECT_FALSE(X.Overflowed());
        }

#ifdef NDEBUG // ASSERTs in DEBUG
            UInt24InternalType value{ UInt24Maximum + 1 };

            ::Thunder::Core::UInt24 X = value;

            EXPECT_TRUE(X.Overflowed());
#endif
    }

    // Conversion, integral
    TEST(UnsignedInt24, ConversionIntegral)
    {
        using SignedSucceedingType = int64_t;
        using UnsignedSucceedingType = uint64_t;

        constexpr SignedSucceedingType SignedSucceedingTypeMinimum{ std::numeric_limits<SignedSucceedingType>::lowest() };
        constexpr SignedSucceedingType SignedSucceedingTypeMaximum{ std::numeric_limits<SignedSucceedingType>::max() };
        constexpr UnsignedSucceedingType UnsignedSucceedingTypeMinimum{ std::numeric_limits<UnsignedSucceedingType>::lowest() };
        constexpr UnsignedSucceedingType UnsignedSucceedingTypeMaximum{ std::numeric_limits<UnsignedSucceedingType>::max() };

        // Preconditions
        static_assert(    sizeof(SignedSucceedingType) >= UInt24InternalTypeSize
                      && SignedSucceedingTypeMinimum < 0
                      && SignedSucceedingTypeMaximum > 0
                      , ""
        );

        static_assert(   sizeof(UnsignedSucceedingType) >= UInt24InternalTypeSize
                      && UnsignedSucceedingTypeMinimum == 0
                      && UnsignedSucceedingTypeMaximum > 0
                      , ""
        );

#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
            ::Thunder::Core::UInt24 X{ value };

            SignedSucceedingType signedSucceedingValue = X;

            EXPECT_EQ(signedSucceedingValue, value);

            UnsignedSucceedingType unsignedSucceedingValue = X;

            EXPECT_EQ(unsignedSucceedingValue, value);
        }

#ifdef NDEBUG // ASSERTs in DEBUG
        {
            using SignedFailingType = int8_t;
            using UnsignedFailingType = uint8_t;

            constexpr SignedFailingType SignedFailingTypeMinimum{ std::numeric_limits<SignedFailingType>::lowest() };
            constexpr SignedFailingType SignedFailingTypeMaximum{ std::numeric_limits<SignedFailingType>::max() };
            constexpr UnsignedFailingType UnsignedFailingTypeMinimum{ std::numeric_limits<UnsignedFailingType>::lowest() };
            constexpr UnsignedFailingType UnsignedFailingTypeMaximum{ std::numeric_limits<UnsignedFailingType>::max() };

            // Preconditions
            static_assert(    sizeof(SignedFailingType) < UInt24InternalTypeSize
                          && SignedFailingTypeMinimum < 0
                          && SignedFailingTypeMaximum > 0
                          , ""
            );

            static_assert(sizeof(UnsignedFailingType) < UInt24InternalTypeSize
                          && UnsignedFailingTypeMinimum == 0
                          && UnsignedFailingTypeMaximum > 0
                          , ""
            );

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum,  SInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                UInt24InternalType x{ value };

                ::Thunder::Core::UInt24 X{ x };

                SignedFailingType signedFailingValue = X;

                if (x <= static_cast<UInt24InternalType>(SignedFailingTypeMaximum)) {
                    EXPECT_EQ(signedFailingValue, x);
                } else {
                    // Failure
                    EXPECT_NE(signedFailingValue, x);
                }

                UnsignedFailingType unsignedFailingValue = X;

                if (    value >= static_cast<UInt24InternalType>(UnsignedFailingTypeMinimum)
                     && value <= static_cast<UInt24InternalType>(UnsignedFailingTypeMaximum)
                   ) {
                    EXPECT_EQ(unsignedFailingValue, x);
                } else {
                    // Failure
                    EXPECT_NE(unsignedFailingValue, x);
                }
            }
        }
#endif
    }

    // Conversion floating point
    TEST(UsignedInt24, ConversionFloatingPoint)
    {
        // maximum accurate number of digits = mantissa  * log10(2)
        // float      : 24 * 0.30103 : at least 7 digits
        // double     : 53 * 0.30103 : at least 15 digits
        // float16_t  : 11 * 0.30103 : at least 3 digits

        // UInt24::Min and UInt24::Max, respectively, 0 and 16777215, uses 8 digits

#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
            ::Thunder::Core::UInt24 X{ value };

            float floatingPointValue = X;

            // Intentionally not EXPECT_FLOAT_EQ
            EXPECT_EQ(floatingPointValue, value);
        }

#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
            ::Thunder::Core::UInt24 X{ value };

            // Intentionally not EXPECT_DOUBLE_EQ
            double floatingPointValue = X;

            EXPECT_EQ(floatingPointValue, value);
        }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
        for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, SInt24Maximum } ) {
#else
        for (UInt24InternalType value{ UInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            UInt24InternalType x{ value };

            ::Thunder::Core::UInt24 X{ x };

            ::Thunder::ExtraNumberDefinitions::float16_t floatingPointValue = X;

            if (x <= 999) {
                EXPECT_EQ(floatingPointValue, x);
            }

            // Skip range of 4 digits

            if (   std::is_same<::Thunder::ExtraNumberDefinitions::float16_t, float>::value != true
                && x >= 99999
               ) {
                // Failure
                EXPECT_NE(floatingPointValue, x);
            }
        }
#endif
    }

    // (Other) operators
    // ---------------------

     TEST(UnsignedInt24, Logical)
    {
        {
            // Negation

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 1 }; value <= UInt24Maximum; value++) {
#endif
                ::Thunder::Core::UInt24 X{ value };

                EXPECT_TRUE(X);
                EXPECT_FALSE(!X);
            }

            UInt24InternalType y{ 0 };

            ::Thunder::Core::UInt24 Y{ y };

            EXPECT_FALSE(Y);
            EXPECT_TRUE(!Y);
        }

        {
            UInt24InternalType y{ 0 };

            ::Thunder::Core::UInt24 Y{ y };

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum + 1 }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ UInt24Minimum + 1 }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif

                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    // AND
                    EXPECT_TRUE(X && X);
                    EXPECT_TRUE(X && x);
                    EXPECT_FALSE(X && Y);
                    EXPECT_FALSE(X && y);
                    EXPECT_TRUE(X && Z);
                    EXPECT_TRUE(X && z);
                    EXPECT_TRUE(x && X);
                    EXPECT_FALSE(x && Y);
                    EXPECT_TRUE(x && Z);

                    EXPECT_FALSE(Y && X);
                    EXPECT_FALSE(Y && x);
                    EXPECT_FALSE(Y && Y);
                    EXPECT_FALSE(Y && y);
                    EXPECT_FALSE(Y && Z);
                    EXPECT_FALSE(Y && z);
                    EXPECT_FALSE(y && X);
                    EXPECT_FALSE(y && Y);
                    EXPECT_FALSE(y && Z);

                    EXPECT_TRUE(Z && X);
                    EXPECT_TRUE(Z && x);
                    EXPECT_FALSE(Z && Y);
                    EXPECT_FALSE(Z && y);
                    EXPECT_TRUE(Z && Z);
                    EXPECT_TRUE(Z && z);
                    EXPECT_TRUE(z && X);
                    EXPECT_FALSE(z && Y);
                    EXPECT_TRUE(z && Z);

                    // inclusive OR
                    EXPECT_TRUE(X || X);
                    EXPECT_TRUE(X || x);
                    EXPECT_TRUE(X || Y);
                    EXPECT_TRUE(X || y);
                    EXPECT_TRUE(X || Z);
                    EXPECT_TRUE(X || z);
                    EXPECT_TRUE(x || X);
                    EXPECT_TRUE(x || Y);
                    EXPECT_TRUE(x || Z);

                    EXPECT_TRUE(Y || X);
                    EXPECT_TRUE(Y || x);
                    EXPECT_FALSE(Y || Y);
                    EXPECT_FALSE(Y || y);
                    EXPECT_TRUE(Y || Z);
                    EXPECT_TRUE(Y || z);
                    EXPECT_TRUE(y || X);
                    EXPECT_FALSE(y || Y);
                    EXPECT_TRUE(y || Z);

                    EXPECT_TRUE(Z || X);
                    EXPECT_TRUE(Z || x);
                    EXPECT_TRUE(Z || Y);
                    EXPECT_TRUE(Z || y);
                    EXPECT_TRUE(Z || Z);
                    EXPECT_TRUE(Z || z);
                    EXPECT_TRUE(z || X);
                    EXPECT_TRUE(z || Y);
                    EXPECT_TRUE(z || Z);
                }
            }
        }
    }

    // Comparison
    TEST(UnsignedInt24, Comparison)
    {
        UInt24InternalType y{ 0 };

        ::Thunder::Core::UInt24 Y{ y };

#ifdef QUICK_TEST 
        for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
        for (UInt24InternalType outerLoopValue{ UInt24Minimum + 1 }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
            UInt24InternalType x{ outerLoopValue };

            ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST 
            for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType innerLoopValue{ UInt24Minimum + 1 }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                UInt24InternalType z{ innerLoopValue };

                ::Thunder::Core::UInt24 Z{ z };

                if (   x != y
                    && x != z
                    && y != z
                ) {
                    // Equal to
                    EXPECT_TRUE(X == X);
                    EXPECT_TRUE(X == x);
                    EXPECT_FALSE(X == Y);
                    EXPECT_FALSE(X == y);
                    EXPECT_FALSE(X == Z);
                    EXPECT_FALSE(X == z);
                    EXPECT_TRUE(x == X);
                    EXPECT_FALSE(x == Y);
                    EXPECT_FALSE(x == Z);

                    EXPECT_FALSE(Y == X);
                    EXPECT_FALSE(Y == x);
                    EXPECT_TRUE(Y == Y);
                    EXPECT_TRUE(Y == y);
                    EXPECT_FALSE(Y == Z);
                    EXPECT_FALSE(Y == z);
                    EXPECT_FALSE(y == X);
                    EXPECT_TRUE(y == Y);
                    EXPECT_FALSE(y == Z);

                    EXPECT_FALSE(Z == X);
                    EXPECT_FALSE(Z == x);
                    EXPECT_FALSE(Z == Y);
                    EXPECT_FALSE(Z == y);
                    EXPECT_TRUE(Z == Z);
                    EXPECT_TRUE(Z == z);
                    EXPECT_FALSE(z == X);
                    EXPECT_FALSE(z == Y);
                    EXPECT_TRUE(z == Z);
                }

                if (   x == y
                    && x == z
                    && y == z
                ) {
                    // Not equal to
                    EXPECT_FALSE(X != X);
                    EXPECT_FALSE(X != x);
                    EXPECT_TRUE(X != Y);
                    EXPECT_TRUE(X != y);
                    EXPECT_TRUE(X != Z);
                    EXPECT_TRUE(X != z);
                    EXPECT_FALSE(x != X);
                    EXPECT_TRUE(x != Y);
                    EXPECT_TRUE(x != Z);

                    EXPECT_TRUE(Y != X);
                    EXPECT_TRUE(Y != x);
                    EXPECT_FALSE(Y != Y);
                    EXPECT_FALSE(Y != y);
                    EXPECT_TRUE(Y != Z);
                    EXPECT_TRUE(Y != z);
                    EXPECT_TRUE(y != X);
                    EXPECT_FALSE(y != Y);
                    EXPECT_TRUE(Y != Z);

                    EXPECT_TRUE(Z != X);
                    EXPECT_TRUE(Z != x);
                    EXPECT_TRUE(Z != Y);
                    EXPECT_TRUE(Z != y);
                    EXPECT_FALSE(Z != Z);
                    EXPECT_FALSE(Z != z);
                    EXPECT_TRUE(z != X);
                    EXPECT_TRUE(z != Y);
                    EXPECT_FALSE(z != Z);
                }

                if (   x < y
                    && x < z
                    && y < z
                ) {
                    // Less than
                    EXPECT_FALSE(X < X);
                    EXPECT_FALSE(X < x);
                    EXPECT_TRUE(X < Y);
                    EXPECT_TRUE(X < y);
                    EXPECT_TRUE(X < Z);
                    EXPECT_TRUE(X < z);
                    EXPECT_FALSE(x < X);
                    EXPECT_TRUE(x < Y);
                    EXPECT_TRUE(x < Z);

                    EXPECT_FALSE(Y < X);
                    EXPECT_FALSE(Y < x);
                    EXPECT_FALSE(Y < Y);
                    EXPECT_FALSE(Y < y);
                    EXPECT_TRUE(Y < Z);
                    EXPECT_TRUE(Y < z);
                    EXPECT_FALSE(y < X);
                    EXPECT_FALSE(y < Y);
                    EXPECT_TRUE(Y < Z);

                    EXPECT_FALSE(Z < X);
                    EXPECT_FALSE(Z < x);
                    EXPECT_FALSE(Z < Y);
                    EXPECT_FALSE(Z < y);
                    EXPECT_FALSE(Z < Z);
                    EXPECT_FALSE(Z < z);
                    EXPECT_FALSE(z < X);
                    EXPECT_FALSE(z < Y);
                    EXPECT_FALSE(z < Z);
                }

                if (   x > y
                    && x > z
                    && y > z
                ) {
                    // Greater than
                    EXPECT_FALSE(X > X);
                    EXPECT_FALSE(X > x);
                    EXPECT_FALSE(X > Y);
                    EXPECT_FALSE(X > y);
                    EXPECT_FALSE(X > Z);
                    EXPECT_FALSE(X > z);
                    EXPECT_FALSE(x > X);
                    EXPECT_FALSE(x > Y);
                    EXPECT_FALSE(x > Z);

                    EXPECT_TRUE(Y > X);
                    EXPECT_TRUE(Y > x);
                    EXPECT_FALSE(Y > Y);
                    EXPECT_FALSE(Y > y);
                    EXPECT_FALSE(Y > Z);
                    EXPECT_FALSE(Y > z);
                    EXPECT_TRUE(y > X);
                    EXPECT_FALSE(y > Y);
                    EXPECT_FALSE(y > Z);

                    EXPECT_TRUE(Z > X);
                    EXPECT_TRUE(Z > x);
                    EXPECT_TRUE(Z > Y);
                    EXPECT_TRUE(Z > y);
                    EXPECT_FALSE(Z > Z);
                    EXPECT_FALSE(Z > z);
                    EXPECT_TRUE(z > X);
                    EXPECT_TRUE(z > Y);
                    EXPECT_FALSE(z > Z);
                }

                if (   x <= y
                    && x <= z
                    && y <= z
                ) {
                    // Less than or equal to
                    EXPECT_TRUE(X <= X);
                    EXPECT_TRUE(X <= x);
                    EXPECT_TRUE(X <= Y);
                    EXPECT_TRUE(X <= y);
                    EXPECT_TRUE(X <= Z);
                    EXPECT_TRUE(X <= z);
                    EXPECT_TRUE(x <= X);
                    EXPECT_TRUE(x <= Y);
                    EXPECT_TRUE(x <= Z);

                    EXPECT_FALSE(Y <= X);
                    EXPECT_FALSE(Y <= x);
                    EXPECT_TRUE(Y <= Y);
                    EXPECT_TRUE(Y <= y);
                    EXPECT_TRUE(Y <= Z);
                    EXPECT_TRUE(Y <= z);
                    EXPECT_FALSE(y <= X);
                    EXPECT_TRUE(y <= Y);
                    EXPECT_TRUE(Y <= Z);

                    EXPECT_FALSE(Z <= X);
                    EXPECT_FALSE(Z <= x);
                    EXPECT_FALSE(Z <= Y);
                    EXPECT_FALSE(Z <= y);
                    EXPECT_TRUE(Z <= Z);
                    EXPECT_TRUE(Z <= z);
                    EXPECT_FALSE(z <= X);
                    EXPECT_FALSE(z <= Y);
                    EXPECT_TRUE(z <= Z);
                }

                if (   x >= y
                    && x >= z
                    && y >= z
                ) {
                    // Greater than or equal to
                    EXPECT_TRUE(X >= X);
                    EXPECT_TRUE(X >= x);
                    EXPECT_FALSE(X >= Y);
                    EXPECT_FALSE(X >= y);
                    EXPECT_FALSE(X >= Z);
                    EXPECT_FALSE(X >= z);
                    EXPECT_TRUE(x >= X);
                    EXPECT_FALSE(x >= Y);
                    EXPECT_FALSE(x >= Z);

                    EXPECT_TRUE(Y >= X);
                    EXPECT_TRUE(Y >= x);
                    EXPECT_TRUE(Y >= Y);
                    EXPECT_TRUE(Y >= y);
                    EXPECT_FALSE(Y >= Z);
                    EXPECT_FALSE(Y >= z);
                    EXPECT_TRUE(y >= X);
                    EXPECT_TRUE(y >= Y);
                    EXPECT_FALSE(y >= Z);

                    EXPECT_TRUE(Z >= X);
                    EXPECT_TRUE(Z >= x);
                    EXPECT_TRUE(Z >= Y);
                    EXPECT_TRUE(Z >= y);
                    EXPECT_TRUE(Z >= Z);
                    EXPECT_TRUE(Z >= z);
                    EXPECT_TRUE(z >= X);
                    EXPECT_TRUE(z >= Y);
                    EXPECT_TRUE(z >= Z);
                }
            }
        }
    }

    // Arithmetic
    TEST(UnsignedInt24, Arithmetic)
    {
        {
            // Unary plus and unary minus

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum,  static_cast<UInt24InternalType>(SInt24Maximum) } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= static_cast<UInt24InternalType>(SInt24Maximum); value++) {
#endif
                ::Thunder::Core::UInt24 X{ value };

                // Unary plus, result is UInt24
                EXPECT_EQ((+X), value);

                // Unary minus, result is SInt24
                EXPECT_EQ((-X), static_cast<SInt24InternalType>(-value));
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ static_cast<UInt24InternalType>(SInt24Maximum) + 1,  UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ static_cast<UInt24InternalType>(SInt24Maximum) + 1 }; value <= UInt24Maximum; value++) {
#endif
                ::Thunder::Core::UInt24 X{ value };

                EXPECT_EQ((+X), value);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ static_cast<UInt24InternalType>(SInt24Maximum) + 1,  UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ static_cast<UInt24InternalType>(SInt24Maximum) + 1 }; value <= UInt24Maximum; value++) {
#endif
                ::Thunder::Core::UInt24 X{ value };

                EXPECT_TRUE((-X).Overflowed());
            }
#endif
        }

        {
            // Addition

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ((X + ::Thunder::Core::UInt24{ value }), (x + value));
                EXPECT_EQ((X + value),  (x + value));
                EXPECT_EQ((value + X ), (value + x));
            }

            UInt24InternalType value{ UInt24Minimum };

            EXPECT_EQ((Z + ::Thunder::Core::UInt24{ value }), (z + value));
            EXPECT_EQ((Z + value),  (z + value));
            EXPECT_EQ((value + Z ), (value + z));

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > (UInt24InstantiationTypeMaximum - outerLoopValue)) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue + ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 1 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Z + ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Z + value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > (UInt24InstantiationTypeMaximum - outerLoopValue)) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue + ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Subtraction

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

            UInt24InternalType value { UInt24Minimum,};

            EXPECT_EQ((X - ::Thunder::Core::UInt24{ value }), (x - value));
            EXPECT_EQ((X - value), (x - value));
            EXPECT_EQ((value - X), (value - x));

#ifdef QUICK_TEST
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ((Z - ::Thunder::Core::UInt24{ value }), (z - value));
                EXPECT_EQ((Z - value), (z - value));
                EXPECT_EQ((value - Z), (value - z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > outerLoopValue) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue - ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 1 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((X - ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((X - value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > outerLoopValue) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue - ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Multiplication

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ((X * ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x * value });
                EXPECT_EQ((X * ::Thunder::Core::UInt24{ value }), (x * value));
                EXPECT_EQ((X * value), (x * value));
                EXPECT_EQ((value * X), (value * x));
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 1 } ) {
#else
            for (UInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                EXPECT_EQ((Z * ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z * value });
                EXPECT_EQ((Z * ::Thunder::Core::UInt24{ value }), (z * value));
                EXPECT_EQ((Z * value), (z * value));
                EXPECT_EQ((value * Z), (value * z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    outerLoopValue > 0
                         && innerLoopValue > (UInt24InstantiationTypeMaximum / outerLoopValue)
                    ) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue * ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
              
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 2, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 2 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Z * ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Z * value).Overflowed());
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    outerLoopValue > 0
                         && innerLoopValue > (UInt24InstantiationTypeMaximum / outerLoopValue)
                    ) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue * ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif

        }

        {
            // Division

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    EXPECT_EQ((X / ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x / value });
                    EXPECT_EQ((X / ::Thunder::Core::UInt24{ value }), (x / value));
                    EXPECT_EQ((X / value), (x / value));

                    EXPECT_EQ((Z / ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z / value });
                    EXPECT_EQ((Z / ::Thunder::Core::UInt24{ value }), (z / value));
                    EXPECT_EQ((Z / value), (z / value));
                }

                EXPECT_EQ((value / Z), (value / z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue / ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ((value / X), (value / x));
            }

            // Equal value but overflow / possibly undefined behavior / exception 

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue / ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Remainder

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    EXPECT_EQ((X % ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x % value });
                    EXPECT_EQ((X % ::Thunder::Core::UInt24{ value }), (x % value));
                    EXPECT_EQ((X % value), (x % value));

                    EXPECT_EQ((Z % ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z % value });
                    EXPECT_EQ((Z % ::Thunder::Core::UInt24{ value }), (z % value));
                    EXPECT_EQ((Z % value), (z % value));
                }

                EXPECT_EQ((value % Z), (value % z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue % ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ((value % X), (value % x));
            }

            // Equal value but overflow % possibly undefined behavior % exception 

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue % ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
 
        }

        {
            // BitWise NOT

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_EQ(~::Thunder::Core::UInt24{ value }, ::Thunder::Core::UInt24{ (~value) & ::Thunder::Core::UInt24::SignificantBitsBitMask });
                EXPECT_EQ(~::Thunder::Core::UInt24{ value }, (~value) & ::Thunder::Core::UInt24::SignificantBitsBitMask);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise AND

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ UInt24Minimum }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    EXPECT_EQ((X & X), (x & x));
                    EXPECT_EQ((X & x), (x & x));
                    EXPECT_EQ((X & Z), (x & z));
                    EXPECT_EQ((X & z), (x & z));

                    EXPECT_EQ((x & X), (x & x));
                    EXPECT_EQ((z & X), (x & z));

                    EXPECT_EQ((Z & X), (z & x));
                    EXPECT_EQ((Z & x), (z & x));
                    EXPECT_EQ((Z & Z), (z & z));
                    EXPECT_EQ((Z & z), (z & z));

                    EXPECT_EQ((x & Z), (x & z));
                    EXPECT_EQ((z & Z), (z & z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue & ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue & outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise OR

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ UInt24Minimum }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    EXPECT_EQ((X | X), (x | x));
                    EXPECT_EQ((X | x), (x | x));
                    EXPECT_EQ((X | Z), (x | z));
                    EXPECT_EQ((X | z), (x | z));

                    EXPECT_EQ((x | X), (x | x));
                    EXPECT_EQ((z | X), (x | z));

                    EXPECT_EQ((Z | X), (z | x));
                    EXPECT_EQ((Z | x), (z | x));
                    EXPECT_EQ((Z | Z), (z | z));
                    EXPECT_EQ((Z | z), (z | z));

                    EXPECT_EQ((x | Z), (z | x));
                    EXPECT_EQ((z | Z), (z | z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue | ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue | outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise XOR

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    EXPECT_EQ((X ^ X), (x ^ x));
                    EXPECT_EQ((X ^ x), (x ^ x));
                    EXPECT_EQ((X ^ Z), (x ^ z));
                    EXPECT_EQ((X ^ z), (x ^ z));

                    EXPECT_EQ((x ^ X), (x ^ x));
                    EXPECT_EQ((z ^ X), (x ^ z));

                    EXPECT_EQ((Z ^ X), (z ^ x));
                    EXPECT_EQ((Z ^ x), (z ^ x));
                    EXPECT_EQ((Z ^ Z), (z ^ z));
                    EXPECT_EQ((Z ^ z), (z ^ z));

                    EXPECT_EQ((x ^ Z), (z ^ x));
                    EXPECT_EQ((z ^ Z), (z ^ z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue ^ ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue ^ outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise left shift

            UInt24InternalType y{ 0 }, z{ 1 };

            ::Thunder::Core::UInt24 Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 23 } ) {
#else
            for (UInt24InternalType value{ 0 }; value < 23; value++) {
#endif
                EXPECT_EQ((Y << ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ y << value });
                EXPECT_EQ((Y << ::Thunder::Core::UInt24{ value }), (y << value));
                EXPECT_EQ((Y << value), (y << value));

                EXPECT_EQ((Z << ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z << value });
                EXPECT_EQ((Z << ::Thunder::Core::UInt24{ value }), (z << value));
                EXPECT_EQ((Z << value), (z << value));
            } 

            constexpr UInt24InternalType maxShift{ static_cast<UInt24InternalType>(sizeof(UInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (UInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                EXPECT_EQ((value << ::Thunder::Core::UInt24{ shift }), (value << ::Thunder::Core::UInt24{ shift }));
            }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Y << ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Y << value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Z << ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Z << value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ maxShift, UInt24InstantiationTypeMaximum } ) {
#else
            for (UInt24InternalType shift{ maxShift }; shift <= UInt24InstantiationTypeMaximum; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                EXPECT_EQ((value << ::Thunder::Core::UInt24{ shift }), (value << ::Thunder::Core::UInt24{ shift }));
            }
#endif
        }

        {
            // BitWise right shift

            UInt24InternalType y{ 0 }, z{ 1 };

            ::Thunder::Core::UInt24 Y{ y }, Z{ z };

#ifdef NDEBUG // ASSERTs in DEBUG

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 23 } ) {
#else
            for (UInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                EXPECT_EQ((Y >> ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ y >> value });
                EXPECT_EQ((Y >> ::Thunder::Core::UInt24{ value }), (y >> value));
                EXPECT_EQ((Y >> value), (y >> value));
                EXPECT_EQ((value >> Y), (value >> y));

                EXPECT_EQ((Z >> ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z >> value });
                EXPECT_EQ((Z >> ::Thunder::Core::UInt24{ value }), (z >> value));
                EXPECT_EQ((Z >> value), (z >> value));
                EXPECT_EQ((value >> Z), (value >> z));
            } 

            constexpr UInt24InternalType maxShift{ static_cast<UInt24InternalType>(sizeof(UInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (UInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >> ::Thunder::Core::UInt24{ shift }), (value >> ::Thunder::Core::UInt24{ shift }));
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Y >> ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Y >> value).Overflowed());

                EXPECT_TRUE((Z >> ::Thunder::Core::UInt24{ value }).Overflowed());
                EXPECT_TRUE((Z >> value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ maxShift, UInt24InstantiationTypeMaximum } ) {
#else
            for (UInt24InternalType shift{ maxShift }; shift <= UInt24InstantiationTypeMaximum; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >> ::Thunder::Core::UInt24{ shift }), (value >> ::Thunder::Core::UInt24{ shift }));
            }
#endif
        }
    }

    // Assignment
    TEST(UnsignedInt24, Assignment)
    {
        {
            // Addition assignment

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_EQ((X += ::Thunder::Core::UInt24{ value }), (x + value));

                X = x;

                EXPECT_EQ((X += value),  (x + value));

                X = x;

                UInt24InternalType result{ value };

                EXPECT_EQ((result += X ), (value + x));
            }

            UInt24InternalType value{ UInt24Minimum };

            Z = z;

            EXPECT_EQ((Z += ::Thunder::Core::UInt24{ value }), (z + value));

            Z = z;

            EXPECT_EQ((Z += value),  (z + value));

            Z = z;

            UInt24InternalType result{ value };

            EXPECT_EQ((result += Z ), (value + z));

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > (UInt24InstantiationTypeMaximum - outerLoopValue)) {
                        // Overflow
                    } else {
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result += ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 1 }; value <= UInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z += ::Thunder::Core::UInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z += value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > (UInt24InstantiationTypeMaximum - outerLoopValue)) {
                        // Overflow
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result += ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Subtraction assignment

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

            UInt24InternalType value { UInt24Minimum,};

            X = x;

            EXPECT_EQ((X -= ::Thunder::Core::UInt24{ value }), (x - value));

            X = x;

            EXPECT_EQ((X -= value), (x - value));

            X = x;

            UInt24InternalType result{ value };

            EXPECT_EQ((result -= X), (value - x));

#ifdef QUICK_TEST
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z -= ::Thunder::Core::UInt24{ value }), (z - value));

                Z = z;

                EXPECT_EQ((Z -= value), (z - value));

                Z = z;

                UInt24InternalType result{ value };

                EXPECT_EQ((result -= Z), (value - z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > outerLoopValue) {
                        // Overflow
                    } else {
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result -= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 1 }; value <= UInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_TRUE((X -= ::Thunder::Core::UInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X -= value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (innerLoopValue > outerLoopValue) {
                        // Overflow
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result -= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Multiplication assignment

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_EQ((X *= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x * value });

                X = x;

                EXPECT_EQ((X *= ::Thunder::Core::UInt24{ value }), (x * value));

                X = x;

                EXPECT_EQ((X *= value), (x * value));

                X = x;

                UInt24InternalType result{ value };

                EXPECT_EQ((result *= X), (value * x));
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 1 } ) {
#else
            for (UInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z *= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z * value });

                Z = z;

                EXPECT_EQ((Z *= ::Thunder::Core::UInt24{ value }), (z * value));

                Z = z;

                EXPECT_EQ((Z *= value), (z * value));

                Z = z;

                UInt24InternalType result{ value };

                EXPECT_EQ((result *= Z), (value * z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    outerLoopValue > 0
                         && innerLoopValue > (UInt24InstantiationTypeMaximum / outerLoopValue)
                    ) {
                        // Overflow
                    } else {
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result *= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
              
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 2, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 2 }; value <= UInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z *= ::Thunder::Core::UInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z *= value).Overflowed());
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    outerLoopValue > 0
                         && innerLoopValue > (UInt24InstantiationTypeMaximum / outerLoopValue)
                    ) {
                        // Overflow
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result *= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif

        }

        {
            // Division assignment

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    X = x;

                    EXPECT_EQ((X /= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x / value });

                    X = x;

                    EXPECT_EQ((X /= ::Thunder::Core::UInt24{ value }), (x / value));

                    X = x;

                    EXPECT_EQ((X /= value), (x / value));

                    Z = z;

                    EXPECT_EQ((Z /= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z / value });

                    Z = z;

                    EXPECT_EQ((Z /= ::Thunder::Core::UInt24{ value }), (z / value));

                    Z = z;

                    EXPECT_EQ((Z /= value), (z / value));
                }


                Z = z;

                UInt24InternalType result{ value };

                EXPECT_EQ((result /= Z), (value / z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result /= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                X = x;

                UInt24InternalType result{ value };

                EXPECT_EQ((result /= X), (value / x));
            }

            // Equal value but overflow / possibly undefined behavior / exception 

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        UInt24InstantiationType result{ innerLoopValue };
                        
                        EXPECT_EQ(result /= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Remainder assignment

            UInt24InternalType x{ UInt24Minimum }, z{ UInt24Maximum };

            ::Thunder::Core::UInt24 X{ x }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    X = x;

                    EXPECT_EQ((X %= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ x % value });

                    X = x;

                    EXPECT_EQ((X %= ::Thunder::Core::UInt24{ value }), (x % value));

                    X = x;

                    EXPECT_EQ((X %= value), (x % value));

                    Z = z;

                    EXPECT_EQ((Z %= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z % value });

                    Z = z;

                    EXPECT_EQ((Z %= ::Thunder::Core::UInt24{ value }), (z % value));

                    Z = z;

                    EXPECT_EQ((Z %= value), (z % value));
                }

                Z = z;

                UInt24InternalType result{ value };

                EXPECT_EQ((result %= Z), (value % z));
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result %= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value <= UInt24Maximum; value++) {
#endif
                X = x;

                UInt24InternalType result{ value };

                EXPECT_EQ((result %= X), (value % x));
            }

            // Equal value but overflow % possibly undefined behavior % exception 

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        UInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result %= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // BitWise AND assignment

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ UInt24Minimum }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X &= X), (x & x));

                    X = x;

                    EXPECT_EQ((X &= x), (x & x));

                    X = x;

                    EXPECT_EQ((X &= Z), (x & z));

                    X = x;

                    EXPECT_EQ((X &= z), (x & z));

                    X = x;

                    UInt24InternalType result{ x };

                    EXPECT_EQ((result &= X), (x & x));

                    result = z;

                    EXPECT_EQ((result &= X), (x & z));

                    Z = z;

                    EXPECT_EQ((Z &= X), (z & x));

                    Z = z;

                    EXPECT_EQ((Z &= x), (z & x));

                    Z = z;

                    EXPECT_EQ((Z &= Z), (z & z));

                    Z = z;

                    EXPECT_EQ((Z &= z), (z & z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((result &= Z), (x & z));

                    result = z;

                    EXPECT_EQ((result &= Z), (z & z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    UInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result &= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue & outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise OR assignment
 
#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ UInt24Minimum }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X |= X), (x | x));

                    X = x;

                    EXPECT_EQ((X |= x), (x | x));

                    X = x;

                    EXPECT_EQ((X |= Z), (x | z));

                    X = x;

                    EXPECT_EQ((X |= z), (x | z));

                    X = x;

                    UInt24InternalType result{ x };

                    EXPECT_EQ((result |= X), (x | x));

                    result = z;

                    EXPECT_EQ((result |= X), (x | z));

                    Z = z;

                    EXPECT_EQ((Z |= X), (z | x));

                    Z = z;

                    EXPECT_EQ((Z |= x), (z | x));

                    Z = z;

                    EXPECT_EQ((Z |= Z), (z | z));

                    Z = z;

                    EXPECT_EQ((Z |= z), (z | z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((x |= Z), (z | x));

                    result = z;

                    EXPECT_EQ((z |= Z), (z | z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    UInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result |= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue | outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // Bitwise XOR assignment

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif

                UInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::UInt24 X{ x };

#ifdef QUICK_TEST
                for (UInt24InternalType innerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
                for (UInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= UInt24Maximum; innerLoopValue++) {
#endif
                    UInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::UInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X ^= X), (x ^ x));

                    X = x;

                    EXPECT_EQ((X ^= x), (x ^ x));

                    X = x;

                    EXPECT_EQ((X ^= Z), (x ^ z));

                    X = x;

                    EXPECT_EQ((X ^= z), (x ^ z));

                    X = x;

                    UInt24InternalType result{ x };

                    EXPECT_EQ((result ^= X), (x ^ x));

                    result = z;

                    EXPECT_EQ((result ^= X), (x ^ z));

                    Z = z;

                    EXPECT_EQ((Z ^= X), (z ^ x));

                    Z = z;

                    EXPECT_EQ((Z ^= x), (z ^ x));

                    Z = z;

                    EXPECT_EQ((Z ^= Z), (z ^ z));

                    Z = z;

                    EXPECT_EQ((Z ^= z), (z ^ z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((result ^= Z), (z ^ x));

                    result = z;

                    EXPECT_EQ((result ^= Z), (z ^ z));
                }
            }

#ifdef QUICK_TEST
            for (UInt24InternalType outerLoopValue : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum } ) {
#else
            for (UInt24InternalType outerLoopValue{ UInt24Minimum }; outerLoopValue <= UInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (UInt24InstantiationType innerLoopValue : std::vector<UInt24InstantiationType>{ UInt24InstantiationTypeMinimum, UInt24InstantiationTypeMaximum } ) {
#else
                for (UInt24InstantiationType innerLoopValue{ UInt24InstantiationTypeMinimum }; innerLoopValue <= UInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    UInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result ^= ::Thunder::Core::UInt24{ outerLoopValue }, innerLoopValue ^ outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // Bitwise left shift assignment

            UInt24InternalType y{ 0 }, z{ 1 };

            ::Thunder::Core::UInt24 Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 23 } ) {
#else
            for (UInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y <<= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ y << value });

                Y = y;

                EXPECT_EQ((Y <<= ::Thunder::Core::UInt24{ value }), (y << value));

                Y = y;

                EXPECT_EQ((Y <<= value), (y << value));

                Z = z;

                EXPECT_EQ((Z <<= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z << value });

                Z = z;

                EXPECT_EQ((Z <<= ::Thunder::Core::UInt24{ value }), (z << value));

                Z = z;

                EXPECT_EQ((Z <<= value), (z << value));
            } 

            constexpr UInt24InternalType maxShift{ static_cast<UInt24InternalType>(sizeof(UInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (UInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                UInt24InstantiationType result{ value };

                EXPECT_EQ((result <<= ::Thunder::Core::UInt24{ shift }), (value << ::Thunder::Core::UInt24{ shift }));
            }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_TRUE((Y <<= ::Thunder::Core::UInt24{ value }).Overflowed());

                Y = y;

                EXPECT_TRUE((Y <<= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z <<= ::Thunder::Core::UInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z <<= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ maxShift, UInt24InstantiationTypeMaximum } ) {
#else
            for (UInt24InternalType shift{ maxShift }; shift <= UInt24InstantiationTypeMaximum; shift++) {
#endif
                UInt24InstantiationType value{ 1 };
                
                UInt24InstantiationType result{ value };

                EXPECT_EQ((result <<= ::Thunder::Core::UInt24{ shift }), (value << ::Thunder::Core::UInt24{ shift }));
            }
#endif
        }

        {
            // BitWise right shift assignment

            UInt24InternalType y{ 0 }, z{ 1 };

            ::Thunder::Core::UInt24 Y{ y }, Z{ z };

#ifdef NDEBUG // ASSERTs in DEBUG

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 0, 23 } ) {
#else
            for (UInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y >>= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ y >> value });

                Y = y;

                EXPECT_EQ((Y >>= ::Thunder::Core::UInt24{ value }), (y >> value));

                Y = y;

                EXPECT_EQ((Y >>= value), (y >> value));

                Y = y;

                UInt24InternalType result{ value };

                EXPECT_EQ((result >>= Y), (value >> y));

                Z = z;

                EXPECT_EQ((Z >>= ::Thunder::Core::UInt24{ value }), ::Thunder::Core::UInt24{ z >> value });

                Z = z;

                EXPECT_EQ((Z >>= ::Thunder::Core::UInt24{ value }), (z >> value));


                EXPECT_EQ((Z >>= value), (z >> value));

                Z = z;

                result = value;

                EXPECT_EQ((result >>= Z), (value >> z));
            } 

            constexpr UInt24InternalType maxShift{ static_cast<UInt24InternalType>(sizeof(UInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (UInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                UInt24InstantiationType value{ 1 };
                
                UInt24InstantiationType result{ value };

                EXPECT_EQ((result >>= ::Thunder::Core::UInt24{ shift }), (value >> ::Thunder::Core::UInt24{ shift }));
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ 24, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ 24 }; value <= UInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_TRUE((Y >>= ::Thunder::Core::UInt24{ value }).Overflowed());

                Y = y;

                EXPECT_TRUE((Y >>= value).Overflowed());

                Z = z;

                EXPECT_TRUE((Z >>= ::Thunder::Core::UInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z >>= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (UInt24InternalType shift : std::vector<UInt24InternalType>{ maxShift, UInt24InstantiationTypeMaximum } ) {
#else
            for (UInt24InternalType shift{ maxShift }; shift <= UInt24InstantiationTypeMaximum; shift++) {
#endif
                UInt24InstantiationType value{ 1 };

                UInt24InstantiationType result{ value };

                EXPECT_EQ((result >>= ::Thunder::Core::UInt24{ shift }), (value >> ::Thunder::Core::UInt24{ shift }));
            }
#endif
        }
    }

    // Increment and decrement
    TEST(UnsignedInt24, IncrementDecrement)
    {
        {
            // Pre-increment

            ::Thunder::Core::UInt24 X { UInt24Minimum }, Y { X };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum, UInt24Maximum - 1 } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value < UInt24Maximum; ++value) {
#endif
                X = value;             
                Y = X;

                Y = ++X; 

                EXPECT_EQ(X - Y, 0);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = UInt24Maximum;
            Y = X;

            Y = ++X;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_TRUE(Y.Overflowed());
#endif
        }

        {
            // Pre-decrement

            ::Thunder::Core::UInt24 X { UInt24Maximum }, Y { X };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Maximum }; value > UInt24Minimum; --value) {
#endif
                X = value;             
                Y = X;

                Y = --X; 

                EXPECT_EQ(Y - X, 0);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = UInt24Minimum;
            Y = X;

            Y = --X;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_TRUE(Y.Overflowed());
#endif
        }

        {
            // Post-increment

            ::Thunder::Core::UInt24 X { UInt24Minimum }, Y { X };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum - 1 } ) {
#else
            for (UInt24InternalType value{ UInt24Minimum }; value < UInt24Maximum; ++value) {
#endif
                X = value;             
                Y = X;

                Y = X++; 

                EXPECT_EQ(X - Y, 1);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = UInt24Maximum;
            Y = X;

            Y = X++; 

            EXPECT_TRUE(X.Overflowed());
            EXPECT_FALSE(Y.Overflowed());
#endif
        }

        {
            // Post-decrement

            ::Thunder::Core::UInt24 X { UInt24Maximum }, Y { X };

#ifdef QUICK_TEST 
            for (UInt24InternalType value : std::vector<UInt24InternalType>{ UInt24Minimum + 1, UInt24Maximum } ) {
#else
            for (UInt24InternalType value{ UInt24Maximum }; value > UInt24Minimum; --value) {
#endif
                X = value;             
                Y = X;

                Y = X--; 

                EXPECT_EQ(Y - X, 1);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = UInt24Minimum;
            Y = X;

            Y = X--;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_FALSE(Y.Overflowed());
#endif
        }
    }

    // Other
    TEST(UnsignedInt24, Other)
    {
        // Comma

        ::Thunder::Core::UInt24 X { UInt24Minimum }, Y { 1 }, Z { UInt24Maximum };

        Z = (X, Y);

        EXPECT_EQ(Z, Y);
    }

} // Core
} // Tests
} // Thunder

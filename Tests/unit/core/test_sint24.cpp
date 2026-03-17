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

    // Current type
    using SInt24InstantiationType = ::Thunder::Core::SInt24::InternalType;

    constexpr SInt24InternalType SInt24Minimum{ std::numeric_limits<::Thunder::Core::SInt24>::lowest() };
    constexpr SInt24InternalType SInt24Maximum{ std::numeric_limits<::Thunder::Core::SInt24>::max() };

    constexpr SInt24InstantiationType SInt24InstantiationTypeMinimum{ std::numeric_limits<SInt24InstantiationType>::lowest() };
    constexpr SInt24InstantiationType SInt24InstantiationTypeMaximum{ std::numeric_limits<SInt24InstantiationType>::max() };

    constexpr size_t SInt24InternalTypeSize{ sizeof(SInt24InternalType) };

    // Preconditions
    static_assert(sizeof(SInt24InstantiationType) >= 4, "");

    static_assert(SInt24Minimum < 0, "");
    static_assert(SInt24Maximum > 0, "");

    static_assert(SInt24InstantiationTypeMinimum < 0, "");
    static_assert(SInt24InstantiationTypeMaximum > 0, "");

    static_assert(SInt24Minimum > SInt24InstantiationTypeMinimum, "");
    static_assert(SInt24Maximum < SInt24InstantiationTypeMaximum, "");

    // Essentials
    // ----------

    // std::is_integral, std:;is_arithmetic
    TEST(SignedInt24, IsIntegralArithmetic)
    {
        EXPECT_FALSE(std::is_integral<::Thunder::Core::SInt24>::value);
        EXPECT_FALSE(std::is_arithmetic<::Thunder::Core::SInt24>::value);
    }

    // Conversion constructor, integral
    TEST(SignedInt24, ConversionConstructorIntegral)
    {
#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            EXPECT_FALSE(::Thunder::Core::SInt24{ value }.Overflowed());
        }

#ifdef NDEBUG // ASSERTs in DEBUG
        SInt24InternalType value{ SInt24Minimum - 1 };

        EXPECT_TRUE(::Thunder::Core::SInt24{ value }.Overflowed());

        value = SInt24Maximum + 1;

        EXPECT_TRUE(::Thunder::Core::SInt24{ value }.Overflowed());
#endif
    }

    // Conversion assignment, integral
    TEST(SignedInt24, ConversionAssignmentIntegral)
    {
#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            ::Thunder::Core::SInt24 X = value;

            EXPECT_FALSE(X.Overflowed());
        }

#ifdef NDEBUG // ASSERTs in DEBUG
            SInt24InternalType value{ SInt24Minimum - 1 };

            ::Thunder::Core::SInt24 X = value;

            EXPECT_TRUE(X.Overflowed());

            value = SInt24Maximum + 1;

            X = value;

            EXPECT_TRUE(X.Overflowed());
#endif
    }

    // Conversion, integral
    TEST(SignedInt24, ConversionIntegral)
    {
        using SignedSucceedingType = int64_t;
        using UnsignedSucceedingType = uint64_t;

        constexpr SignedSucceedingType SignedSucceedingTypeMinimum{ std::numeric_limits<SignedSucceedingType>::lowest() };
        constexpr SignedSucceedingType SignedSucceedingTypeMaximum{ std::numeric_limits<SignedSucceedingType>::max() };
        constexpr UnsignedSucceedingType UnsignedSucceedingTypeMinimum{ std::numeric_limits<UnsignedSucceedingType>::lowest() };
        constexpr UnsignedSucceedingType UnsignedSucceedingTypeMaximum{ std::numeric_limits<UnsignedSucceedingType>::max() };

        // Preconditions
        static_assert(    sizeof(SignedSucceedingType) >= SInt24InternalTypeSize
                      && SignedSucceedingTypeMinimum < 0
                      && SignedSucceedingTypeMaximum > 0
                      , ""
        );

        static_assert(   sizeof(UnsignedSucceedingType) >= SInt24InternalTypeSize
                      && UnsignedSucceedingTypeMinimum == 0
                      && UnsignedSucceedingTypeMaximum > 0
                      , ""
        );

#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            ::Thunder::Core::SInt24 X{ value };

            SignedSucceedingType signedSucceedingValue = X;

            EXPECT_EQ(signedSucceedingValue, value);
        }

#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
            ::Thunder::Core::SInt24 X{ value };

            UnsignedSucceedingType unsignedSucceedingValue = X;

            EXPECT_EQ(unsignedSucceedingValue, value);
        }

#ifdef NDEBUG // ASSERTs in DEBUG
        {
            using UnsignedFailingType = UnsignedSucceedingType;

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value < 0; value++) {
#endif
                ::Thunder::Core::SInt24 X{ value };

                UnsignedFailingType unsignedFailingValue = X;

                EXPECT_EQ(unsignedFailingValue, static_cast<UnsignedFailingType>(value));
            }
        }

        {
            using SignedFailingType = int8_t;
            using UnsignedFailingType = uint8_t;

            constexpr SignedFailingType SignedFailingTypeMinimum{ std::numeric_limits<SignedFailingType>::lowest() };
            constexpr SignedFailingType SignedFailingTypeMaximum{ std::numeric_limits<SignedFailingType>::max() };
            constexpr UnsignedFailingType UnsignedFailingTypeMinimum{ std::numeric_limits<UnsignedFailingType>::lowest() };
            constexpr UnsignedFailingType UnsignedFailingTypeMaximum{ std::numeric_limits<UnsignedFailingType>::max() };

            // Preconditions
            static_assert(    sizeof(SignedFailingType) < SInt24InternalTypeSize
                          && SignedFailingTypeMinimum < 0
                          && SignedFailingTypeMaximum > 0
                          , ""
            );

            static_assert(sizeof(UnsignedFailingType) < SInt24InternalTypeSize
                          && UnsignedFailingTypeMinimum == 0
                          && UnsignedFailingTypeMaximum > 0
                          , ""
            );

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0,  SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                SInt24InternalType x{ value };

                ::Thunder::Core::SInt24 X{ x };

                SignedFailingType signedFailingValue = X;

                if (    x >= static_cast<SInt24InternalType>(SignedFailingTypeMinimum)
                     && x <= static_cast<SInt24InternalType>(SignedFailingTypeMaximum)
                   ) {
                    EXPECT_EQ(signedFailingValue, x);
                } else {
                    // Failure
                    EXPECT_NE(signedFailingValue, x);
                }

                UnsignedFailingType unsignedFailingValue = X;

                if (    value >= static_cast<SInt24InternalType>(UnsignedFailingTypeMinimum)
                     && value <= static_cast<SInt24InternalType>(UnsignedFailingTypeMaximum)
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
    TEST(SignedInt24, ConversionFloatingPoint)
    {
        // maximum accurate number of digits = mantissa  * log10(2)
        // float      : 24 * 0.30103 : at least 7 digits
        // double     : 53 * 0.30103 : at least 15 digits
        // float16_t  : 11 * 0.30103 : at least 3 digits

        // SInt24::Min and SInt24::Max, respectively, -8388608 and 8388607, uses 7 digits

#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            ::Thunder::Core::SInt24 X{ value };

            float floatingPointValue = X;

            // Intentionally not EXPECT_FLOAT_EQ
            EXPECT_EQ(floatingPointValue, value);
        }

#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            ::Thunder::Core::SInt24 X{ value };

            double floatingPointValue = X;

            // Intentionally not EXPECT_DOUBLE_EQ
            EXPECT_EQ(floatingPointValue, value);
        }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
        for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
        for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
            SInt24InternalType x{ value };

            ::Thunder::Core::SInt24 X{ x };

            ::Thunder::ExtraNumberDefinitions::float16_t floatingPointValue = X;

            if (    x >= -999
                 && x <= 999
               ) {
                EXPECT_EQ(floatingPointValue, x);
            }

            // Skip range of 4 digits

            if (   std::is_same<::Thunder::ExtraNumberDefinitions::float16_t, float>::value != true
                && x <= -99999
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

    // Logical
    TEST(SignedInt24, Logical)
    {
        {
            // Negation

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value < 0; value++) {
#endif
                ::Thunder::Core::SInt24 X{ value };

                EXPECT_TRUE(X);
                EXPECT_FALSE(!X);
            }

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

            EXPECT_FALSE(Y);
            EXPECT_TRUE(!Y);

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 1 }; value <= SInt24Maximum; value++) {
#endif
                ::Thunder::Core::SInt24 Z{ value };

                EXPECT_TRUE(Z);
                EXPECT_FALSE(!Z);
            }
        }

        {
            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif

                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

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
    TEST(SignedInt24, Comparison)
    {
        SInt24InternalType y{ 0 };

        ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST 
        for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
        for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif
            SInt24InternalType x{ outerLoopValue };

            ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST 
            for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                SInt24InternalType z{ innerLoopValue };

                ::Thunder::Core::SInt24 Z{ z };

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

    // Arithmetic
    TEST(SignedInt24, Arithmetic)
    {
        {
            // Unary plus and unary minus

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, 0,  SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum + 1 }; value <= SInt24Maximum; value++) {
#endif
                ::Thunder::Core::SInt24 X{ value };

                // Unary plus
                EXPECT_EQ((+X), value);

                // Unary minus
                EXPECT_EQ((-X), -value);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            ::Thunder::Core::SInt24 value{ SInt24Minimum };

            ::Thunder::Core::SInt24 X{ value };

            EXPECT_EQ((+X), value);
            EXPECT_TRUE((-X).Overflowed());
#endif
        }

        {
            // Addition

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ((X + ::Thunder::Core::SInt24{ value }), (x + value));
                EXPECT_EQ((X + value),  (x + value));
                EXPECT_EQ((value + X ), (value + x));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ((Y + ::Thunder::Core::SInt24{ value }), (y + value));
                EXPECT_EQ((Y + value), (y + value));
                EXPECT_EQ((value + Y ), (value + y));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= 0; value++) {
#endif
                EXPECT_EQ((Z + ::Thunder::Core::SInt24{ value }), (z + value));
                EXPECT_EQ((Z + value), (z + value));
                EXPECT_EQ((value + Z ), (value + z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue < 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum - outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue > 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum - outerLoopValue)
                        )
                    ) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue + ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -1; value++) {
#endif
                EXPECT_TRUE((X + ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X + value).Overflowed());
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 1 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Z + ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Z + value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue < 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum - outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue > 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum - outerLoopValue)
                        )
                    ) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue + ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    } else {
                    }
                }
            }
#endif
        }

        {
            // Subtraction

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= 0; value++) {
#endif
                EXPECT_EQ((X - ::Thunder::Core::SInt24{ value }), (x - value));
                EXPECT_EQ((X - value), (x - value));
                EXPECT_EQ((value - X), (value - x));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum + 1 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ((Y - ::Thunder::Core::SInt24{ value }), (y - value));
                EXPECT_EQ((Y - value), (y - value));
                EXPECT_EQ((value - Y), (value - y));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ((Z - ::Thunder::Core::SInt24{ value }), (z - value));
                EXPECT_EQ((Z - value), (z - value));
                EXPECT_EQ((value - Z), (value - z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue > 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum + outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue < 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum + outerLoopValue)
                        )
                    ) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue - ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 1 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((X - ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X - value).Overflowed());
            }

            SInt24InternalType value{ SInt24Minimum };

            EXPECT_TRUE((Y - ::Thunder::Core::SInt24{ value }).Overflowed());
            EXPECT_TRUE((Y - value).Overflowed());
 
#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -1; value++) {
#endif
                EXPECT_TRUE((Z - ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Z - value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue > 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum + outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue < 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum + outerLoopValue)
                        )
                    ) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue - ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Multiplication

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 1 } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                EXPECT_EQ((X * ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x * value });
                EXPECT_EQ((X * ::Thunder::Core::SInt24{ value }), (x * value));
                EXPECT_EQ((X * value), (x * value));
                EXPECT_EQ((value * X), (value * x));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ((Y * ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y * value });
                EXPECT_EQ((Y * ::Thunder::Core::SInt24{ value }), (y * value));
                EXPECT_EQ((Y * value), (y * value));
                EXPECT_EQ((value * Y), (value * y));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ -1, 0, 1 } ) {
#else
            for (SInt24InternalType value{ -1 }; value <= 1; value++) {
#endif
                EXPECT_EQ((Z * ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z * value });
                EXPECT_EQ((Z * ::Thunder::Core::SInt24{ value }), (z * value));
                EXPECT_EQ((Z * value), (z * value));
                EXPECT_EQ((value * Z), (value * z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    (    (    innerLoopValue > 0
                                   && outerLoopValue > 0
                                   && innerLoopValue > (SInt24InstantiationTypeMaximum / outerLoopValue)
                                 )
                              || (
                                      innerLoopValue < 0
                                   && outerLoopValue < 0
                                   && (   innerLoopValue < outerLoopValue
                                        ? outerLoopValue < (SInt24InstantiationTypeMaximum / innerLoopValue)
                                        : innerLoopValue < (SInt24InstantiationTypeMaximum / outerLoopValue)
                                      )
                                 )
                            )
                         || (    (    innerLoopValue > 0
                                   && outerLoopValue < 0
                                   && (SInt24InstantiationTypeMinimum / outerLoopValue) < innerLoopValue
                                 )
                              || (    innerLoopValue < 0
                                   && outerLoopValue > 0
                                   && (SInt24InstantiationTypeMinimum / innerLoopValue) < outerLoopValue
                                )
                            )
                    ) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue * ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                EXPECT_TRUE((X * ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X * ::Thunder::Core::SInt24{ value }).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 2, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 2 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((X * ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X * value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -2 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -2; value++) {
#endif
                EXPECT_TRUE((Z * ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Z * value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    (    (    innerLoopValue > 0
                                   && outerLoopValue > 0
                                   && innerLoopValue > (SInt24InstantiationTypeMaximum / outerLoopValue)
                                 )
                              || (
                                      innerLoopValue < 0
                                   && outerLoopValue < 0
                                   && (   innerLoopValue < outerLoopValue
                                        ? outerLoopValue < (SInt24InstantiationTypeMaximum / innerLoopValue)
                                        : innerLoopValue < (SInt24InstantiationTypeMaximum / outerLoopValue)
                                      )
                                 )
                            )
                         || (    (    innerLoopValue > 0
                                   && outerLoopValue < 0
                                   && (SInt24InstantiationTypeMinimum / outerLoopValue) < innerLoopValue
                                 )
                              || (    innerLoopValue < 0
                                   && outerLoopValue > 0
                                   && (SInt24InstantiationTypeMinimum / innerLoopValue) < outerLoopValue
                                )
                            )
                    ) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue * ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Division

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    EXPECT_EQ((X / ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x / value });
                    EXPECT_EQ((X / ::Thunder::Core::SInt24{ value }), (x / value));
                    EXPECT_EQ((X / value), (x / value));

                    EXPECT_EQ((Y / ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y / value });
                    EXPECT_EQ((Y / ::Thunder::Core::SInt24{ value }), (y / value));
                    EXPECT_EQ((Y / value), (y / value));

                    EXPECT_EQ((Z / ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z / value });
                    EXPECT_EQ((Z / ::Thunder::Core::SInt24{ value }), (z / value));
                    EXPECT_EQ((Z / value), (z / value));
                }

                EXPECT_EQ((value / X), (value / x));
                EXPECT_EQ((value / Z), (value / z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue / ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                    EXPECT_TRUE((::Thunder::Core::SInt24{ value } / Y).Overflowed());
            }

            // Equal value but overflow / possibly undefined behavior / exception 

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue / ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Remainder

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    EXPECT_EQ((X % ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x % value });
                    EXPECT_EQ((X % ::Thunder::Core::SInt24{ value }), (x % value));
                    EXPECT_EQ((X % value), (x % value));

                    EXPECT_EQ((Y % ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y % value });
                    EXPECT_EQ((Y % ::Thunder::Core::SInt24{ value }), (y % value));
                    EXPECT_EQ((Y % value), (y % value));

                    EXPECT_EQ((Z % ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z % value });
                    EXPECT_EQ((Z % ::Thunder::Core::SInt24{ value }), (z % value));
                    EXPECT_EQ((Z % value), (z % value));
                }

                EXPECT_EQ((value % X), (value % x));
                EXPECT_EQ((value % Z), (value % z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        EXPECT_EQ(innerLoopValue % ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                    EXPECT_TRUE((::Thunder::Core::SInt24{ value } % Y).Overflowed());
            }

            // Equal value but overflow / possibly undefined behavior / exception

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        EXPECT_EQ(innerLoopValue % ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // BitWise NOT

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_EQ(~::Thunder::Core::SInt24{ value }, ::Thunder::Core::SInt24{ ~value });
                EXPECT_EQ(~::Thunder::Core::SInt24{ value }, ~value);
            }
        }

        {
            // BitWise AND

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    EXPECT_EQ((X & X), (x & x));
                    EXPECT_EQ((X & x), (x & x));
                    EXPECT_EQ((X & Y), (x & y));
                    EXPECT_EQ((X & y), (x & y));
                    EXPECT_EQ((X & Z), (x & z));
                    EXPECT_EQ((X & z), (x & z));

                    EXPECT_EQ((x & X), (x & x));
                    EXPECT_EQ((y & X), (x & y));
                    EXPECT_EQ((z & X), (x & z));

                    EXPECT_EQ((Y & X), (y & x));
                    EXPECT_EQ((Y & x), (y & x));
                    EXPECT_EQ((Y & Y), (y & y));
                    EXPECT_EQ((Y & y), (y & y));
                    EXPECT_EQ((Y & Z), (y & z));
                    EXPECT_EQ((Y & z), (y & z));

                    EXPECT_EQ((x & Y), (y & x));
                    EXPECT_EQ((y & Y), (y & y));
                    EXPECT_EQ((z & Y), (y & z));

                    EXPECT_EQ((Z & X), (z & x));
                    EXPECT_EQ((Z & x), (z & x));
                    EXPECT_EQ((Z & Y), (z & y));
                    EXPECT_EQ((Z & y), (z & y));
                    EXPECT_EQ((Z & Z), (z & z));
                    EXPECT_EQ((Z & z), (z & z));

                    EXPECT_EQ((x & Z), (z & x));
                    EXPECT_EQ((y & Z), (z & y));
                    EXPECT_EQ((z & Z), (z & z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue & ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue & outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise OR

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    EXPECT_EQ((X | X), (x | x));
                    EXPECT_EQ((X | x), (x | x));

                    EXPECT_EQ((X | Y), (x | y));
                    EXPECT_EQ((X | y), (x | y));

                    EXPECT_EQ((X | Z), (x | z));
                    EXPECT_EQ((X | z), (x | z));
                    EXPECT_EQ((x | X), (x | x));
                    EXPECT_EQ((y | X), (x | y));
                    EXPECT_EQ((z | X), (x | z));

                    EXPECT_EQ((Y | X), (y | x));
                    EXPECT_EQ((Y | x), (y | x));

                    EXPECT_EQ((Y | Y), (y | y));
                    EXPECT_EQ((Y | y), (y | y));

                    EXPECT_EQ((Y | Z), (y | z));
                    EXPECT_EQ((Y | z), (y | z));

                    EXPECT_EQ((x | Y), (y | x));
                    EXPECT_EQ((y | Y), (y | y));
                    EXPECT_EQ((z | Y), (y | z));

                    EXPECT_EQ((Z | X), (z | x));
                    EXPECT_EQ((Z | x), (z | x));

                    EXPECT_EQ((Z | Y), (z | y));
                    EXPECT_EQ((Z | y), (z | y));

                    EXPECT_EQ((Z | Z), (z | z));
                    EXPECT_EQ((Z | z), (z | z));

                    EXPECT_EQ((x | Z), (z | x));
                    EXPECT_EQ((y | Z), (z | y));
                    EXPECT_EQ((z | Z), (z | z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue | ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue | outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise XOR

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    EXPECT_EQ((X ^ X), (x ^ x));
                    EXPECT_EQ((X ^ x), (x ^ x));

                    EXPECT_EQ((X ^ Y), (x ^ y));
                    EXPECT_EQ((X ^ y), (x ^ y));

                    EXPECT_EQ((X ^ Z), (x ^ z));
                    EXPECT_EQ((X ^ z), (x ^ z));
                    EXPECT_EQ((x ^ X), (x ^ x));
                    EXPECT_EQ((y ^ X), (x ^ y));
                    EXPECT_EQ((z ^ X), (x ^ z));

                    EXPECT_EQ((Y ^ X), (y ^ x));
                    EXPECT_EQ((Y ^ x), (y ^ x));

                    EXPECT_EQ((Y ^ Y), (y ^ y));
                    EXPECT_EQ((Y ^ y), (y ^ y));

                    EXPECT_EQ((Y ^ Z), (y ^ z));
                    EXPECT_EQ((Y ^ z), (y ^ z));

                    EXPECT_EQ((x ^ Y), (y ^ x));
                    EXPECT_EQ((y ^ Y), (y ^ y));
                    EXPECT_EQ((z ^ Y), (y ^ z));

                    EXPECT_EQ((Z ^ X), (z ^ x));
                    EXPECT_EQ((Z ^ x), (z ^ x));

                    EXPECT_EQ((Z ^ Y), (z ^ y));
                    EXPECT_EQ((Z ^ y), (z ^ y));

                    EXPECT_EQ((Z ^ Z), (z ^ z));
                    EXPECT_EQ((Z ^ z), (z ^ z));

                    EXPECT_EQ((x ^ Z), (z ^ x));
                    EXPECT_EQ((y ^ Z), (z ^ y));
                    EXPECT_EQ((z ^ Z), (z ^ z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    EXPECT_EQ(innerLoopValue ^ ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue ^ outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise left shift

            SInt24InternalType x{ -1 }, y{ 0 }, z{ 1 };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 23 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                EXPECT_EQ((Y << ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y << value });
                EXPECT_EQ((Y << ::Thunder::Core::SInt24{ value }), (y << value));
                EXPECT_EQ((Y << value), (y << value));
            } 

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 22 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 23; value++) {
#endif
                EXPECT_EQ((Z << ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z << value });
                EXPECT_EQ((Z << ::Thunder::Core::SInt24{ value }), (z << value));
                EXPECT_EQ((Z << value), (z << value));
            } 

            constexpr SInt24InternalType maxShift{ static_cast<SInt24InternalType>(sizeof(SInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (SInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value << ::Thunder::Core::SInt24{ shift }), (value << ::Thunder::Core::SInt24{ shift }));
            }

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((X << ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X << value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 24, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 24 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Y << ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Y << value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 23, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 23 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Z << ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Z << value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ maxShift, SInt24InstantiationTypeMaximum } ) {
#else
            for (SInt24InternalType shift{ maxShift }; shift <= SInt24InstantiationTypeMaximum; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value << ::Thunder::Core::SInt24{ shift }), (value << ::Thunder::Core::SInt24{ shift }));
            }
#endif
        }

        {
            // BitWise right shift

            SInt24InternalType x{ -1 }, y{ 0 }, z{ 1 };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef NDEBUG // ASSERTs in DEBUG

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 23 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                EXPECT_EQ((Y >> ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y >> value });
                EXPECT_EQ((Y >> ::Thunder::Core::SInt24{ value }), (y >> value));
                EXPECT_EQ((Y >> value), (y >> value));
                EXPECT_EQ((value >> Y), (value >> y));

                EXPECT_EQ((Z >> ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z >> value });
                EXPECT_EQ((Z >> ::Thunder::Core::SInt24{ value }), (z >> value));
                EXPECT_EQ((Z >> value), (z >> value));
                EXPECT_EQ((value >> Z), (value >> z));
            } 

            constexpr SInt24InternalType maxShift{ static_cast<SInt24InternalType>(sizeof(SInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (SInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >> ::Thunder::Core::SInt24{ shift }), (value >> ::Thunder::Core::SInt24{ shift }));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((X >> ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((X >> value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 24, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 24 }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((Y >> ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Y >> value).Overflowed());

                EXPECT_TRUE((Z >> ::Thunder::Core::SInt24{ value }).Overflowed());
                EXPECT_TRUE((Z >> value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ maxShift, SInt24InstantiationTypeMaximum } ) {
#else
            for (SInt24InternalType shift{ maxShift }; shift <= SInt24InstantiationTypeMaximum; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >> ::Thunder::Core::SInt24{ shift }), (value >> ::Thunder::Core::SInt24{ shift }));
            }
#endif
        }
    }

    // Assignment
    TEST(SignedInt24, Assignment)
    {
        {
            // Addition assignment

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_EQ((X += ::Thunder::Core::SInt24{ value }), (x + value));

                X = x;

                EXPECT_EQ((X += value),  (x + value));

                X = x;

                SInt24InternalType result{ value };

                EXPECT_EQ((result += X ), (value + x));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y += ::Thunder::Core::SInt24{ value }), (y + value));

                Y = y;

                EXPECT_EQ((Y += value), (y + value));

                Y = y;

                SInt24InternalType result{ value };

                EXPECT_EQ((result += Y ), (value + y));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= 0; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z += ::Thunder::Core::SInt24{ value }), (z + value));

                Z = z;

                EXPECT_EQ((Z += value), (z + value));

                Z = z;

                EXPECT_EQ((value += Z ), (value + z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue < 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum - outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue > 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum - outerLoopValue)
                        )
                    ) {
                        // Overflow
                    } else {
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result += ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -1; value++) {
#endif
                X = x;

                EXPECT_TRUE((X += ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X += value).Overflowed());
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 1 }; value <= SInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z += ::Thunder::Core::SInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z += value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue < 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum - outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue > 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum - outerLoopValue)
                        )
                    ) {
                        // Overflow
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result += ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue + outerLoopValue);
                    } else {
                    }
                }
            }
#endif
        }

        {
            // Subtraction assignment

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= 0; value++) {
#endif
                X = x;
                
                EXPECT_EQ((X -= ::Thunder::Core::SInt24{ value }), (x - value));

                X = x;

                EXPECT_EQ((X -= value), (x - value));

                X = x;

                SInt24InternalType result{ value };

                EXPECT_EQ((result -= X), (value - x));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum + 1 }; value <= SInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y -= ::Thunder::Core::SInt24{ value }), (y - value));

                Y = y;

                EXPECT_EQ((Y -= value), (y - value));

                Y = y;

                SInt24InternalType result{ value };

                EXPECT_EQ((result -= Y), (value - y));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z -= ::Thunder::Core::SInt24{ value }), (z - value));

                Z = z;

                EXPECT_EQ((Z -= value), (z - value));

                Z = z;

                SInt24InternalType result{ value };

                EXPECT_EQ((result -= Z), (value - z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue > 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum + outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue < 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum + outerLoopValue)
                        )
                    ) {
                        // Overflow
                    } else {
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result -= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 1 }; value <= SInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_TRUE((X -= ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X -= value).Overflowed());
            }

            SInt24InternalType value{ SInt24Minimum };

            Y = y;

            EXPECT_TRUE((Y -= ::Thunder::Core::SInt24{ value }).Overflowed());

            Y = y;

            EXPECT_TRUE((Y -= value).Overflowed());
 
#ifdef QUICK_TEST
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -1; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z -= ::Thunder::Core::SInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z -= value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if ((    innerLoopValue < 0
                          && outerLoopValue > 0
                          && innerLoopValue < (SInt24InstantiationTypeMinimum + outerLoopValue)
                        )
                        ||
                        (
                             innerLoopValue > 0
                          && outerLoopValue < 0
                          && innerLoopValue > (SInt24InstantiationTypeMaximum + outerLoopValue)
                        )
                    ) {
                        // Overflow
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result -= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue - outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Multiplication assignment

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 1 } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                X = x;

                EXPECT_EQ((X *= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x * value });
                
                X = x;

                EXPECT_EQ((X *= ::Thunder::Core::SInt24{ value }), (x * value));

                X = x;

                EXPECT_EQ((X *= value), (x * value));

                X = x;

                SInt24InternalType result{ value };

                EXPECT_EQ((result *= X), (value * x));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y *= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y * value });

                Y = y;

                EXPECT_EQ((Y *= ::Thunder::Core::SInt24{ value }), (y * value));

                Y = y;

                EXPECT_EQ((Y *= value), (y * value));

                Y = y;

                SInt24InternalType result{ value };

                EXPECT_EQ((result *= Y), (value * y));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ -1, 0, 1 } ) {
#else
            for (SInt24InternalType value{ -1 }; value <= 1; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z *= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z * value });

                Z = z;

                EXPECT_EQ((Z *= ::Thunder::Core::SInt24{ value }), (z * value));

                Z = z;

                EXPECT_EQ((Z *= value), (z * value));

                Z = z;

                SInt24InternalType result{ value };

                EXPECT_EQ((result *= Z), (value * z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    (    (    innerLoopValue > 0
                                   && outerLoopValue > 0
                                   && innerLoopValue > (SInt24InstantiationTypeMaximum / outerLoopValue)
                                 )
                              || (
                                      innerLoopValue < 0
                                   && outerLoopValue < 0
                                   && (   innerLoopValue < outerLoopValue
                                        ? outerLoopValue < (SInt24InstantiationTypeMaximum / innerLoopValue)
                                        : innerLoopValue < (SInt24InstantiationTypeMaximum / outerLoopValue)
                                      )
                                 )
                            )
                         || (    (    innerLoopValue > 0
                                   && outerLoopValue < 0
                                   && (SInt24InstantiationTypeMinimum / outerLoopValue) < innerLoopValue
                                 )
                              || (    innerLoopValue < 0
                                   && outerLoopValue > 0
                                   && (SInt24InstantiationTypeMinimum / innerLoopValue) < outerLoopValue
                                )
                            )
                    ) {
                        // Overflow
                    } else {
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result *= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= 1; value++) {
#endif
                X = x;

                EXPECT_TRUE((X *= ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X *= ::Thunder::Core::SInt24{ value }).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 2, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 2 }; value <= SInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_TRUE((X *= ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X *= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, -2 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= -2; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z *= ::Thunder::Core::SInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z *= value).Overflowed());
            }

            // Equal value but overflow

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (    (    (    innerLoopValue > 0
                                   && outerLoopValue > 0
                                   && innerLoopValue > (SInt24InstantiationTypeMaximum / outerLoopValue)
                                 )
                              || (
                                      innerLoopValue < 0
                                   && outerLoopValue < 0
                                   && (   innerLoopValue < outerLoopValue
                                        ? outerLoopValue < (SInt24InstantiationTypeMaximum / innerLoopValue)
                                        : innerLoopValue < (SInt24InstantiationTypeMaximum / outerLoopValue)
                                      )
                                 )
                            )
                         || (    (    innerLoopValue > 0
                                   && outerLoopValue < 0
                                   && (SInt24InstantiationTypeMinimum / outerLoopValue) < innerLoopValue
                                 )
                              || (    innerLoopValue < 0
                                   && outerLoopValue > 0
                                   && (SInt24InstantiationTypeMinimum / innerLoopValue) < outerLoopValue
                                )
                            )
                    ) {
                        // Overflow
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result *= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue * outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Division assignment

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    X = x;

                    EXPECT_EQ((X /= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x / value });

                    X = x;

                    EXPECT_EQ((X /= ::Thunder::Core::SInt24{ value }), (x / value));

                    X = x;

                    EXPECT_EQ((X /= value), (x / value));

                    Y = y;

                    EXPECT_EQ((Y /= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y / value });

                    Y = y;

                    EXPECT_EQ((Y /= ::Thunder::Core::SInt24{ value }), (y / value));

                    Y = y;

                    EXPECT_EQ((Y /= value), (y / value));

                    Z = z;

                    EXPECT_EQ((Z /= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z / value });

                    Z = z;

                    EXPECT_EQ((Z /= ::Thunder::Core::SInt24{ value }), (z / value));

                    Z = z;

                    EXPECT_EQ((Z /= value), (z / value));
                }

                X = x;

                SInt24InternalType result{ value };

                EXPECT_EQ((result /= X), (value / x));

                Z = z;

                result = value;

                EXPECT_EQ((result / Z), (value / z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result /= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((::Thunder::Core::SInt24{ value } /= Y).Overflowed());
            }

            // Equal value but overflow / possibly undefined behavior / exception 

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result /= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue / outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // Remainder assignment

            SInt24InternalType x{ SInt24Minimum }, y{ 0 }, z{ SInt24Maximum };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                if ( value != 0) {
                    X = x;

                    EXPECT_EQ((X %= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ x % value });

                    X = x;

                    EXPECT_EQ((X %= ::Thunder::Core::SInt24{ value }), (x % value));

                    X = x;

                    EXPECT_EQ((X %= value), (x % value));

                    Y = y;

                    EXPECT_EQ((Y %= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y % value });

                    Y = y;

                    EXPECT_EQ((Y %= ::Thunder::Core::SInt24{ value }), (y % value));

                    Y = y;

                    EXPECT_EQ((Y %= value), (y % value));

                    Z = z;

                    EXPECT_EQ((Z %= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z % value });

                    Z = z;

                    EXPECT_EQ((Z %= ::Thunder::Core::SInt24{ value }), (z % value));

                    Z = z;

                    EXPECT_EQ((Z %= value), (z % value));
                }

                X = x;

                SInt24InternalType result{ value };

                EXPECT_EQ((result %= X), (value % x));

                Z = z;

                result = value;

                EXPECT_EQ((result %= Z), (value % z));
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                    } else {
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result %= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    }
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value <= SInt24Maximum; value++) {
#endif
                EXPECT_TRUE((::Thunder::Core::SInt24{ value } %= Y).Overflowed());
            }

            // Equal value but overflow / possibly undefined behavior / exception

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    if (outerLoopValue == 0) {
                        // Overflow
                        SInt24InstantiationType result{ innerLoopValue };

                        EXPECT_EQ(result %= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue % outerLoopValue);
                    } else {
                    }
                } 
            } 
#endif
        }

        {
            // BitWise AND assignment

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X &= X), (x & x));

                    X = x;

                    EXPECT_EQ((X &= x), (x & x));

                    X = x;
                    Y = y;

                    EXPECT_EQ((X &= Y), (x & y));

                    X = x;

                    EXPECT_EQ((X &= y), (x & y));

                    X = x;
                    Z = z;

                    EXPECT_EQ((X &= Z), (x & z));

                    X = x;

                    EXPECT_EQ((X &= z), (x & z));

                    X = x;

                    SInt24InternalType result{ x };

                    EXPECT_EQ((result &= X), (x & x));

                    result = y;

                    EXPECT_EQ((result &= X), (x & y));

                    result = z;

                    EXPECT_EQ((result &= X), (x & z));

                    X = x;
                    Y = y;

                    EXPECT_EQ((Y &= X), (y & x));

                    Y = y;

                    EXPECT_EQ((Y &= x), (y & x));

                    Y = y;

                    EXPECT_EQ((Y &= Y), (y & y));

                    Y = y;

                    EXPECT_EQ((Y &= y), (y & y));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Y &= Z), (y & z));

                    Y = y;

                    EXPECT_EQ((Y &= z), (y & z));

                    Y = y;

                    result = x;

                    EXPECT_EQ((result &= Y), (y & x));

                    result = y;

                    EXPECT_EQ((result &= Y), (y & y));

                    result = z;

                    EXPECT_EQ((result &= Y), (y & z));

                    X = x;
                    Z = z;

                    EXPECT_EQ((Z &= X), (z & x));

                    Z = z;

                    EXPECT_EQ((Z &= x), (z & x));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Z &= Y), (z & y));

                    Z = z;

                    EXPECT_EQ((Z &= y), (z & y));

                    Z = z;

                    EXPECT_EQ((Z &= Z), (z & z));

                    Z = z;

                    EXPECT_EQ((Z &= z), (z & z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((result &= Z), (z & x));

                    result = y;
                    
                    EXPECT_EQ((result &= Z), (z & y));

                    result = z;

                    EXPECT_EQ((result &= Z), (z & z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    SInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result &= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue & outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // BitWise OR assignment
 
            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X |= X), (x | x));

                    X = x;

                    EXPECT_EQ((X |= x), (x | x));

                    X = x;
                    Y = y;

                    EXPECT_EQ((X |= Y), (x | y));

                    X = x;

                    EXPECT_EQ((X |= y), (x | y));

                    X = x;
                    Z = z;

                    EXPECT_EQ((X |= Z), (x | z));

                    X = x;

                    EXPECT_EQ((X |= z), (x | z));

                    X = x;

                    SInt24InternalType result{ x };

                    EXPECT_EQ((result |= X), (x | x));

                    result = y;

                    EXPECT_EQ((result |= X), (x | y));

                    result = z;

                    EXPECT_EQ((result |= X), (x | z));

                    X = x;
                    Y = y;

                    EXPECT_EQ((Y |= X), (y | x));

                    Y = y;

                    EXPECT_EQ((Y |= x), (y | x));

                    Y = y;

                    EXPECT_EQ((Y |= Y), (y | y));

                    Y = y;

                    EXPECT_EQ((Y |= y), (y | y));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Y |= Z), (y | z));

                    Y = y;

                    EXPECT_EQ((Y |= z), (y | z));

                    Y = y;

                    result = x;

                    EXPECT_EQ((result |= Y), (y | x));

                    result = y;

                    EXPECT_EQ((result |= Y), (y | y));

                    result = z;

                    EXPECT_EQ((result |= Y), (y | z));

                    X = x;
                    Z = z;

                    EXPECT_EQ((Z |= X), (z | x));

                    Z = z;

                    EXPECT_EQ((Z |= x), (z | x));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Z |= Y), (z | y));

                    Z = z;

                    EXPECT_EQ((Z |= y), (z | y));

                    Z = z;

                    EXPECT_EQ((Z |= Z), (z | z));

                    Z = z;

                    EXPECT_EQ((Z |= z), (z | z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((result |= Z), (z | x));

                    result = y;
                    
                    EXPECT_EQ((result |= Z), (z | y));

                    result = z;

                    EXPECT_EQ((result |= Z), (z | z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    SInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result |= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue | outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // Bitwise XOR assignment

            SInt24InternalType y{ 0 };

            ::Thunder::Core::SInt24 Y{ y };

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, -1 } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= -1; outerLoopValue++) {
#endif

                SInt24InternalType x{ outerLoopValue };

                ::Thunder::Core::SInt24 X{ x };

#ifdef QUICK_TEST
                for (SInt24InternalType innerLoopValue : std::vector<SInt24InternalType>{ 1, SInt24Maximum } ) {
#else
                for (SInt24InternalType innerLoopValue{ 1 }; innerLoopValue <= SInt24Maximum; innerLoopValue++) {
#endif
                    SInt24InternalType z{ innerLoopValue };

                    ::Thunder::Core::SInt24 Z{ z };

                    X = x;

                    EXPECT_EQ((X ^= X), (x ^ x));

                    X = x;

                    EXPECT_EQ((X ^= x), (x ^ x));

                    X = x;
                    Y = y;

                    EXPECT_EQ((X ^= Y), (x ^ y));

                    X = x;

                    EXPECT_EQ((X ^= y), (x ^ y));

                    X = x;
                    Z = z;

                    EXPECT_EQ((X ^= Z), (x ^ z));

                    X = x;

                    EXPECT_EQ((X ^= z), (x ^ z));

                    X = x;

                    SInt24InternalType result{ x };

                    EXPECT_EQ((result ^= X), (x ^ x));

                    result = y;

                    EXPECT_EQ((result ^= X), (x ^ y));

                    result = z;

                    EXPECT_EQ((result ^= X), (x ^ z));

                    X = x;
                    Y = y;

                    EXPECT_EQ((Y ^= X), (y ^ x));

                    Y = y;

                    EXPECT_EQ((Y ^= x), (y ^ x));

                    Y = y;

                    EXPECT_EQ((Y ^= Y), (y ^ y));

                    Y = y;

                    EXPECT_EQ((Y ^= y), (y ^ y));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Y ^= Z), (y ^ z));

                    Y = y;

                    EXPECT_EQ((Y ^= z), (y ^ z));

                    Y = y;

                    result = x;

                    EXPECT_EQ((result ^= Y), (y ^ x));

                    result = y;

                    EXPECT_EQ((result ^= Y), (y ^ y));

                    result = z;

                    EXPECT_EQ((result ^= Y), (y ^ z));

                    X = x;
                    Z = z;

                    EXPECT_EQ((Z ^= X), (z ^ x));

                    Z = z;

                    EXPECT_EQ((Z ^= x), (z ^ x));

                    Y = y;
                    Z = z;

                    EXPECT_EQ((Z ^= Y), (z ^ y));

                    Z = z;

                    EXPECT_EQ((Z ^= y), (z ^ y));

                    Z = z;

                    EXPECT_EQ((Z ^= Z), (z ^ z));

                    Z = z;

                    EXPECT_EQ((Z ^= z), (z ^ z));

                    Z = z;

                    result = x;

                    EXPECT_EQ((result ^= Z), (z ^ x));

                    result = y;
                    
                    EXPECT_EQ((result ^= Z), (z ^ y));

                    result = z;

                    EXPECT_EQ((result ^= Z), (z ^ z));
                }
            }

#ifdef QUICK_TEST
            for (SInt24InternalType outerLoopValue : std::vector<SInt24InternalType>{ SInt24Minimum, 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType outerLoopValue{ SInt24Minimum }; outerLoopValue <= SInt24Maximum; outerLoopValue++) {
#endif
#ifdef QUICK_TEST
                for (SInt24InstantiationType innerLoopValue : std::vector<SInt24InstantiationType>{ SInt24InstantiationTypeMinimum, 0, SInt24InstantiationTypeMaximum } ) {
#else
                for (SInt24InstantiationType innerLoopValue{ SInt24InstantiationTypeMinimum }; innerLoopValue <= SInt24InstantiationTypeMaximum; innerLoopValue++) {
#endif
                    SInt24InstantiationType result{ innerLoopValue };

                    EXPECT_EQ(result ^= ::Thunder::Core::SInt24{ outerLoopValue }, innerLoopValue ^ outerLoopValue);
                } 
            } 

#ifdef NDEBUG // ASSERTs in DEBUG
#endif
        }

        {
            // Bitwise left shift assignment

            SInt24InternalType x{ -1 }, y{ 0 }, z{ 1 };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 23 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y <<= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y << value });

                Y = y;

                EXPECT_EQ((Y <<=::Thunder::Core::SInt24{ value }), (y << value));

                Y = y;

                EXPECT_EQ((Y <<= value), (y << value));
            } 

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 22 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 23; value++) {
#endif
                Z = z;

                EXPECT_EQ((Z <<= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z << value });

                Z = z;

                EXPECT_EQ((Z <<= ::Thunder::Core::SInt24{ value }), (z << value));

                Z = z;

                EXPECT_EQ((Z <<= value), (z << value));
            } 

            constexpr SInt24InternalType maxShift{ static_cast<SInt24InternalType>(sizeof(SInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (SInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value <<= ::Thunder::Core::SInt24{ shift }), (value << ::Thunder::Core::SInt24{ shift }));
            }

#ifdef NDEBUG // ASSERTs in DEBUG

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_TRUE((X <<= ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X <<= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 24, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 24 }; value <= SInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_TRUE((Y <<= ::Thunder::Core::SInt24{ value }).Overflowed());

                Y = y;

                EXPECT_TRUE((Y <<= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 23, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 23 }; value <= SInt24Maximum; value++) {
#endif
                Z = z;

                EXPECT_TRUE((Z <<= ::Thunder::Core::SInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z <<= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ maxShift, SInt24InstantiationTypeMaximum } ) {
#else
            for (SInt24InternalType shift{ maxShift }; shift <= SInt24InstantiationTypeMaximum; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value <<= ::Thunder::Core::SInt24{ shift }), (value << ::Thunder::Core::SInt24{ shift }));
            }
#endif
        }

        {
            // Bitwise right shift assignment

            SInt24InternalType x{ -1 }, y{ 0 }, z{ 1 };

            ::Thunder::Core::SInt24 X{ x }, Y{ y }, Z{ z };

#ifdef NDEBUG // ASSERTs in DEBUG
#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, 23 } ) {
#else
            for (SInt24InternalType value{ 0 }; value < 24; value++) {
#endif
                Y = y;

                EXPECT_EQ((Y >>= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ y >> value });

                Y = y;

                EXPECT_EQ((Y >>= ::Thunder::Core::SInt24{ value }), (y >> value));

                Y = y;

                EXPECT_EQ((Y >>= value), (y >> value));

                SInt24InternalType result{ value };

                EXPECT_EQ((value >>= Y), (value >> y));

                Z = z;

                EXPECT_EQ((Z >>= ::Thunder::Core::SInt24{ value }), ::Thunder::Core::SInt24{ z >> value });

                Z = z;

                EXPECT_EQ((Z >>= ::Thunder::Core::SInt24{ value }), (z >> value));

                Z = z;

                EXPECT_EQ((Z >>= value), (z >> value));

                Z = z;

                result = value;

                EXPECT_EQ((result >>= Z), (value >> z));
            } 

            constexpr SInt24InternalType maxShift{ static_cast<SInt24InternalType>(sizeof(SInt24InstantiationType) * 8)};

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ 0, maxShift - 1 } ) {
#else
            for (SInt24InternalType shift{ 0 }; shift < maxShift; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >>= ::Thunder::Core::SInt24{ shift }), (value >> ::Thunder::Core::SInt24{ shift }));
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 0, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 0 }; value <= SInt24Maximum; value++) {
#endif
                X = x;

                EXPECT_TRUE((X >>= ::Thunder::Core::SInt24{ value }).Overflowed());

                X = x;

                EXPECT_TRUE((X >>= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ 24, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ 24 }; value <= SInt24Maximum; value++) {
#endif
                Y = y;

                EXPECT_TRUE((Y >>= ::Thunder::Core::SInt24{ value }).Overflowed());

                Y = y;

                EXPECT_TRUE((Y >>= value).Overflowed());

                Z = z;

                EXPECT_TRUE((Z >>= ::Thunder::Core::SInt24{ value }).Overflowed());

                Z = z;

                EXPECT_TRUE((Z >>= value).Overflowed());
            }

#ifdef QUICK_TEST 
            for (SInt24InternalType shift : std::vector<SInt24InternalType>{ maxShift, SInt24InstantiationTypeMaximum } ) {
#else
            for (SInt24InternalType shift{ maxShift }; shift <= SInt24InstantiationTypeMaximum; shift++) {
#endif
                SInt24InstantiationType value{ 1 };

                EXPECT_EQ((value >>= ::Thunder::Core::SInt24{ shift }), (value >> ::Thunder::Core::SInt24{ shift }));
            }
#endif
        }
    }

    // Increment and decrement
    TEST(SignedInt24, IncrementDecrement)
    {
        {
            // Pre-increment

            ::Thunder::Core::SInt24 X { SInt24Minimum }, Y { X };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum, SInt24Maximum - 1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value < SInt24Maximum; ++value) {
#endif
                X = value;             
                Y = X;

                Y = ++X; 

                EXPECT_EQ(X - Y, 0);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = SInt24Maximum;
            Y = X;

            Y = ++X;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_TRUE(Y.Overflowed());
#endif
        }

        {
            // Pre-decrement

            ::Thunder::Core::SInt24 X { SInt24Maximum }, Y { X };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Maximum }; value > SInt24Minimum; --value) {
#endif
                X = value;             
                Y = X;

                Y = --X; 

                EXPECT_EQ(Y - X, 0);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = SInt24Minimum;
            Y = X;

            Y = --X;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_TRUE(Y.Overflowed());
#endif
        }

        {
            // Post-increment

            ::Thunder::Core::SInt24 X { SInt24Minimum }, Y { X };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, SInt24Maximum - 1 } ) {
#else
            for (SInt24InternalType value{ SInt24Minimum }; value < SInt24Maximum; ++value) {
#endif
                X = value;             
                Y = X;

                Y = X++; 

                EXPECT_EQ(X - Y, 1);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = SInt24Maximum;
            Y = X;

            Y = X++; 

            EXPECT_TRUE(X.Overflowed());
            EXPECT_FALSE(Y.Overflowed());
#endif
        }

        {
            // Post-decrement

            ::Thunder::Core::SInt24 X { SInt24Maximum }, Y { X };

#ifdef QUICK_TEST 
            for (SInt24InternalType value : std::vector<SInt24InternalType>{ SInt24Minimum + 1, SInt24Maximum } ) {
#else
            for (SInt24InternalType value{ SInt24Maximum }; value > SInt24Minimum; --value) {
#endif
                X = value;             
                Y = X;

                Y = X--; 

                EXPECT_EQ(Y - X, 1);
            }

#ifdef NDEBUG // ASSERTs in DEBUG
            X = SInt24Minimum;
            Y = X;

            Y = X--;

            EXPECT_TRUE(X.Overflowed());
            EXPECT_FALSE(Y.Overflowed());
#endif
        }
    }

    // Other
    TEST(SignedInt24, Other)
    {
        // Comma

        ::Thunder::Core::SInt24 X { -1 }, Y { 0 }, Z { 1 };

        Z = (X, Y);

        EXPECT_EQ(Z, Y);
    }

} // Core
} // Tests
} // Thunder

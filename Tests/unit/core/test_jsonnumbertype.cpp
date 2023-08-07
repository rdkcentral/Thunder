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

#include <functional>
#include <sstream>

#include <gtest/gtest.h>

#include "../IPTestAdministrator.h"
#include <core/core.h>
#include "JSON.h"

namespace WPEFramework {
namespace Tests {

    template <typename T>
    bool TestJSONFormat(const std::string& json, bool FromTo, bool AllowChange)
    {
        auto compare = [](const std::string& reference, const std::string& data) -> bool {
            bool result = reference.size() == data.size();

            if (   std::is_same<T, Core::JSON::HexUInt8>::value
                || std::is_same<T, Core::JSON::HexSInt8>::value
                || std::is_same<T, Core::JSON::HexUInt16>::value
                || std::is_same<T, Core::JSON::HexSInt16>::value
                || std::is_same<T, Core::JSON::HexUInt32>::value
                || std::is_same<T, Core::JSON::HexSInt32>::value
                || std::is_same<T, Core::JSON::HexUInt64>::value
                || std::is_same<T, Core::JSON::HexSInt64>::value
               ) {
                for (std::string::size_type index = 0, end = data.size(); index < end; index++) {
                    result = result && std::toupper(reference[index]) == std::toupper(data[index]);
                }
            }

            return result;
        };

        T object;

        Core::OptionalType<Core::JSON::Error> status;

        std::string stream;

//#define _INTERMEDIATE
#ifndef _INTERMEDIATE

        bool result =    object.FromString(json, status)
                      && !(status.IsSet())
                      && (   !FromTo
                          || (   object.ToString(stream)
                              && (    AllowChange
                                   || compare(json, stream)
                                 )
                             )
                         )
                      ;
#else
        bool result = object.FromString(json, status);
             result =    result
                      && !(status.IsSet())
                      ;

            if (FromTo) {
                result =    result
                         && object.ToString(stream)
                         ;

                if (!AllowChange) {
                    result =    result
                             && compare(json, stream)
                             ;
                }
            } else {
            }
#endif

        return result;
    }

    template<>
    bool TestJSONFormat<Core::JSON::String>(const std::string& json, bool FromTo, bool AllowChange)
    {
        Core::JSON::String object;

        uint8_t count = 0;

        // Affects ToString
        bool quoted = false;

        do {
            quoted = !quoted;

            Core::JSON::String object(quoted);

            Core::OptionalType<Core::JSON::Error> status;

            std::string stream;

//#define _INTERMEDIATE
#ifndef _INTERMEDIATE
            count +=    object.FromString(json, status)
                     && !(status.IsSet())
                     &&    !FromTo
                        || (     // Checking communitative property
                               object.ToString(stream)
                            && quoted || AllowChange ? json != std::string(stream) : json == std::string(stream)
                            && object.FromString(stream, status)
                            && !(status.IsSet())
                            && AllowChange ? json != std::string(object.Value().c_str()) : json == std::string(object.Value().c_str())
                           )
                     ;
#else

            bool result = object.FromString(json, status);
                 result =    !(status.IsSet())
                          && result
                          ;

                 if (FromTo) {
                     result =    object.ToString(stream)
                              && result
                              ;

                    if (quoted || AllowChange) {
                        result =    json != std::string(stream)
                                  && result
                                 ;
                    } else {
                        result =    json == std::string(stream)
                                  && result
                                  ;
                    }

                    result =   object.FromString(stream, status)
                             && !(status.IsSet())
                             && result
                             ;

                    result =    AllowChange ? json != std::string(object.Value().c_str()) : json == std::string(object.Value().c_str())
                             && result
                             ;
                 }

            count += result;
#endif
        } while (quoted);

        return count == 2;
    }

    template<typename S, typename T>
    bool TestJSONEqual(const T data, const std::string& reference)
    {
        std::string stream;

        return    std::is_same<decltype(std::declval<S>().Value()), T>::value
               && data == S(data).Default()
               && data == S(data).Value()
               && data == (S() = data).Value()
               && S(data).ToString(stream)
               && reference == std::string(stream)
               ;
    }

    // Implementation specific types may have additional constraint

    template <typename T>
    bool TestDecUIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("0", FromTo, AllowChange);
                count += TestJSONFormat<T>("1", FromTo, AllowChange); // Digit any out of 1-9

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += TestJSONFormat<T>("255", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += TestJSONFormat<T>("65535", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += TestJSONFormat<T>("4294967295", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += TestJSONFormat<T>("18446744073709551615", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty character sequence, no distinction with 0
                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("1\0", FromTo, AllowChange);

                // Quoted character sequence
                count += TestJSONFormat<T>("\"0\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"1\"", FromTo, AllowChange);

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("00", FromTo, AllowChange); // Second digit and out of 0-9
                count += !TestJSONFormat<T>("+1", FromTo, AllowChange); // Digit any out of 0-9
                count += !TestJSONFormat<T>("-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-1", FromTo, AllowChange); // Digit any out of 1-9

                // C(++) floating point convention instead of scientific notation
                count += !TestJSONFormat<T>("0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1.0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0e+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0e-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0E+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0E-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("+0e+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0e-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0e+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0e-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E0.0", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);

                // Empty quoted character sequence
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                // Out-of-range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += !TestJSONFormat<T>("256", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += !TestJSONFormat<T>("65536", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += !TestJSONFormat<T>("4294967296", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += !TestJSONFormat<T>("18446744073709551616", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"1\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1a", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 24
                           : count == 78
               ;
    }

    template <typename T>
    bool TestDecSIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("-0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-1", FromTo, AllowChange); // Digit any out of 1-9

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += TestJSONFormat<T>("-128", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("127", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += TestJSONFormat<T>("-32768", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("32767", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += TestJSONFormat<T>("-2147483648", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("2147483647", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += TestJSONFormat<T>("-9223372036854775808", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("9223372036854775807", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with 0
                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("-1\0", FromTo, AllowChange);

                count += TestJSONFormat<T>("\"-0\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-1\"", FromTo, AllowChange);

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("00", FromTo, AllowChange); // Second digit andy out of 0-9
                count += !TestJSONFormat<T>("-00", FromTo, AllowChange); // Second digit andy out of 0-9

                count += !TestJSONFormat<T>("+1", FromTo, AllowChange); // Digit any out of 0-9

                count += !TestJSONFormat<T>("--1", FromTo, AllowChange); // Digit andy out of 0-9

                // C(++) floating point convention instead of scientific notation
                count += !TestJSONFormat<T>("-0e+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0e-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("+0e+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0e-0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E+0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0E0.0", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                // Out-of-range
                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += !TestJSONFormat<T>("-129", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("128", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += !TestJSONFormat<T>("-32769", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("32768", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += !TestJSONFormat<T>("-2147483649", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("2147483648", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += !TestJSONFormat<T>("-9223372036854775809", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("9223372036854775808", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"1\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"-0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"-1\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1a", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-1a", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("-1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 26
                           : count == 76
               ;
    }

    template <typename T>
    bool TestHexUIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("0x0", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("0xA", FromTo, AllowChange);
                count += TestJSONFormat<T>("0xa", FromTo, AllowChange); // Lower case
                count += TestJSONFormat<T>("0xaF", FromTo, AllowChange); // Mix of Lower and upper case

                count += TestJSONFormat<T>("0X0", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("0XA", FromTo, AllowChange);
                count += TestJSONFormat<T>("0Xa", FromTo, AllowChange); // Lower case
                count += TestJSONFormat<T>("0XaF", FromTo, AllowChange); // Mix of Lower and upper case

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with 0
                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("\"0x0\"", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("\"0xA\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"0xa\"", FromTo, AllowChange); // Lower case
                count += TestJSONFormat<T>("\"0xaF\"", FromTo, AllowChange); // Mix of Lower and upper case

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += TestJSONFormat<T>("0xFF", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += TestJSONFormat<T>("0xFFFF", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += TestJSONFormat<T>("0xFFFFFFFF", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += TestJSONFormat<T>("0xFFFFFFFFFFFFFFFF", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("1x0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("ax0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("Ax0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("1X0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("aX0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("AX0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0x0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F
                count += !TestJSONFormat<T>("+0x0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F

                count += !TestJSONFormat<T>("-0X0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F
                count += !TestJSONFormat<T>("+0X0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F

                count += !TestJSONFormat<T>("x", FromTo, AllowChange); // No x or X
                count += !TestJSONFormat<T>("X", FromTo, AllowChange); 
                count += !TestJSONFormat<T>("x0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("X0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0x", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0X", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0xG", FromTo, AllowChange); // Second digit any out of G-Z
                count += !TestJSONFormat<T>("0XG", FromTo, AllowChange); // Second digit any out of G-Z

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                // Out-of-range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += !TestJSONFormat<T>("0x100", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += !TestJSONFormat<T>("0x10000", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += !TestJSONFormat<T>("0x100000000", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += !TestJSONFormat<T>("0x10000000000000000", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"0x0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0x\"1\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-\"0x0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0x\"0\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1a", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-1a", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("0x1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 38
                           : count == 74
               ;
    }

    template <typename T>
    bool TestHexSIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("-0x0", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("-0xA", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0xa", FromTo, AllowChange); // Lower case
                count += TestJSONFormat<T>("-0x80", FromTo, AllowChange);

                count += TestJSONFormat<T>("-0X0", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("-0XA", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0Xa", FromTo, AllowChange); // Lower case

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with 0
                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("\"-0x0\"", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("\"-0xA\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-0xa\"", FromTo, AllowChange); // Lower case

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += TestJSONFormat<T>("-0x80", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0x7F", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += TestJSONFormat<T>("-0x8000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0x7FFF", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += TestJSONFormat<T>("-0x80000000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0x7FFFFFFF", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += TestJSONFormat<T>("-0x8000000000000000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0x7FFFFFFFFFFFFFFF", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("1x0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("ax0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("Ax0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("1X0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("aX0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("AX0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("+0x0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F
                count += !TestJSONFormat<T>("+0X0", FromTo, AllowChange); // Second digit any out of 0-9, a-f, A-F

                count += !TestJSONFormat<T>("x", FromTo, AllowChange); // No x or X
                count += !TestJSONFormat<T>("X", FromTo, AllowChange); 
                count += !TestJSONFormat<T>("x0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("X0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0x", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0X", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0xG", FromTo, AllowChange); // Second digit any out of G-Z
                count += !TestJSONFormat<T>("0XG", FromTo, AllowChange); // Second digit any out of G-Z

                count += !TestJSONFormat<T>("-1x0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("-ax0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-Ax0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-1X0", FromTo, AllowChange); // First digit any out of 1-9, a-f, A-F
                count += !TestJSONFormat<T>("-aX0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-AX0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0x", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0X", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0xG", FromTo, AllowChange); // Second digit any out of G-Z
                count += !TestJSONFormat<T>("-0XG", FromTo, AllowChange); // Second digit any out of G-Z

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                // Out-of-range
                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += !TestJSONFormat<T>("-0x81", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("0x80", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += !TestJSONFormat<T>("-0x8001", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("0x8000", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += !TestJSONFormat<T>("-0x80000001", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("0x80000000", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += !TestJSONFormat<T>("-0x8000000000000001", FromTo, AllowChange);
                                           count += !TestJSONFormat<T>("0x8000000000000000", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"0x0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0x\"1\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-\"0x0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0x\"0\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1a", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-a1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-1a", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("-0x1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 36
                           : count == 92
               ;
    }

    template <typename T>
    bool TestOctUIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("00", FromTo, AllowChange); // JSON value {'0','0'}
                count += TestJSONFormat<T>("01", FromTo, AllowChange); // Second digit any out of 1-7

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);
                // Empty string, no distinction with 0

                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("\"00\"", FromTo, AllowChange); // JSON value {'0','0'}
                count += TestJSONFormat<T>("\"00\"", FromTo, AllowChange); // JSON value {'0','0'}

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += TestJSONFormat<T>("0377", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += TestJSONFormat<T>("017777", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += TestJSONFormat<T>("037777777777", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += TestJSONFormat<T>("01777777777777777777777", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========
                count += !TestJSONFormat<T>("0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("08", FromTo, AllowChange); // Second digit any out of 8-9
                count += !TestJSONFormat<T>("000", FromTo, AllowChange); // JSON value {'0','0','0'}

                count += !TestJSONFormat<T>("+00", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+01", FromTo, AllowChange); // Second digit any out of 1-7
                count += !TestJSONFormat<T>("-00", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-01", FromTo, AllowChange); // Second digit any out of 1-7
                count += !TestJSONFormat<T>("-08", FromTo, AllowChange); // Second digit any out of 8-9

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                // Out-of-range
                if (   std::is_same<decltype(std::declval<T>().Value()), uint8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), uint64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(uint8_t)  : // uint8_t
                                            count += !TestJSONFormat<T>("0400", FromTo, AllowChange);
                                            break;
                    case sizeof(uint16_t) : // uint16_t
                                            count += !TestJSONFormat<T>("0200000", FromTo, AllowChange);
                                            break;
                    case sizeof(uint32_t) : // uint32_t
                                            count += !TestJSONFormat<T>("040000000000", FromTo, AllowChange);
                                            break;
                    case sizeof(uint64_t) : // uint64_t
                                            count += !TestJSONFormat<T>("02000000000000000000000", FromTo, AllowChange);
                                            break;
                    default               : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"00\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"1\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("100", FromTo, AllowChange);
                count += !TestJSONFormat<T>("101", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("01\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 22
                           : count == 46
               ;
    }

    template <typename T>
    bool TestOctSIntFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("-00", FromTo, AllowChange);
                count += TestJSONFormat<T>("-01", FromTo, AllowChange); // Second digit any out of 1-7

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with 0
                count += (TestJSONFormat<T>("", FromTo, AllowChange) || !AllowChange);
                count += (TestJSONFormat<T>("\0", FromTo, AllowChange) || !AllowChange);

                count += TestJSONFormat<T>("\"-00\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-01\"", FromTo, AllowChange);

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += TestJSONFormat<T>("-0200", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0177", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += TestJSONFormat<T>("-0100000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("077777", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += TestJSONFormat<T>("-020000000000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("017777777777", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += TestJSONFormat<T>("-01000000000000000000000", FromTo, AllowChange);
                                           count += TestJSONFormat<T>("0777777777777777777777", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-01\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\"\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\u0009\u000A\u000D\u0020\"", FromTo, AllowChange || !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("-0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-08", FromTo, AllowChange); // Second digit any out of 8-9

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                if (   std::is_same<decltype(std::declval<T>().Value()), int8_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int16_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int32_t>::value
                    || std::is_same<decltype(std::declval<T>().Value()), int64_t>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(int8_t)  : // int8_t
                                           count += !TestJSONFormat<T>("-0401", FromTo, AllowChange);
                                           break;
                    case sizeof(int16_t) : // int16_t
                                           count += !TestJSONFormat<T>("-0200001", FromTo, AllowChange);
                                           break;
                    case sizeof(int32_t) : // int32_t
                                           count += !TestJSONFormat<T>("-040000000001", FromTo, AllowChange);
                                           break;
                    case sizeof(int64_t) : // int64_t
                                           count += !TestJSONFormat<T>("-02000000000000000000001", FromTo, AllowChange);
                                           break;
                    default              : ASSERT(false);
                    }
                }

                // Prefix / postfix
                count += !TestJSONFormat<T>("\"00\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"1\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-\"0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0\"0\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-100", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-101", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("-01\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00200-1\u0009\u000A\u000D\u00200", FromTo, AllowChange || !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\"0\u0009\u000A\u000D\u0020", FromTo, AllowChange || !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 24
                           : count == 38
               ;
    }

    template <typename T>
    bool TestInstanceIdFromString(bool malformed, uint8_t& count)
    {
        return TestHexUIntFromString<T>(malformed, count);
    }

    template <typename T>
    bool TestPointerFromString(bool malformed, uint8_t& count)
    {
        return TestHexUIntFromString<T>(malformed, count);
    }

    template <typename T, typename S>
    bool TestDecUIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(uint8_t)  : // uint8_t
                                    count += TestJSONEqual<T, uint8_t>(0u, "0");
                                    count += TestJSONEqual<T, uint8_t>(255u, "255");
                                    break;
            case sizeof(uint16_t) : // uint16_t
                                    count += TestJSONEqual<T, uint16_t>(0u, "0");
                                    count += TestJSONEqual<T, uint16_t>(65535u, "65535");
                                    break;
            case sizeof(uint32_t) : // uint32_t
                                    count += TestJSONEqual<T, uint32_t>(0u, "0");
                                    count += TestJSONEqual<T, uint32_t>(4294967295u, "4294967295");
                                    break;
            case sizeof(uint64_t) : // uint64_t
                                    count += TestJSONEqual<T, uint64_t>(0u, "0");
                                    count += TestJSONEqual<T, uint64_t>(18446744073709551615u, "18446744073709551615");
                                    break;
            default               : ASSERT(false);
            }
        }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestDecSIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(int8_t)  : // int8_t
                                   count += TestJSONEqual<T, int8_t>(-127 - 1, "-128");
                                   count += TestJSONEqual<T, int8_t>(127, "127");
                                   break;
            case sizeof(int16_t) : // int16_t
                                   count += TestJSONEqual<T, int16_t>(-32767 - 1, "-32768");
                                   count += TestJSONEqual<T, int16_t>(32767, "32767");
                                   break;
            case sizeof(int32_t) : // int32_t
                                   count += TestJSONEqual<T, int32_t>(-2147483647 - 1, "-2147483648");
                                   count += TestJSONEqual<T, int32_t>(2147483647, "2147483647");
                                   break;
            case sizeof(int64_t) : // int64_t
                                   // Suppress a compiler warning
                                   count += TestJSONEqual<T, int64_t>(-9223372036854775807 - 1, "-9223372036854775808");
                                   count += TestJSONEqual<T, int64_t>(9223372036854775807, "9223372036854775807");
                                   break;
            default              : ASSERT(false);
            }
        }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestHexUIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(uint8_t)  : // uint8_t
                                    count += TestJSONEqual<T, uint8_t>(0x0u, "0X0");
                                    count += TestJSONEqual<T, uint8_t>(0xFFu, "0XFF");
                                    break;
            case sizeof(uint16_t) : // uint16_t
                                    count += TestJSONEqual<T, uint16_t>(0x0u, "0X0");
                                    count += TestJSONEqual<T, uint16_t>(0xFFFFu, "0XFFFF");
                                    break;
            case sizeof(uint32_t) : // uint32_t
                                    count += TestJSONEqual<T, uint32_t>(0x0u, "0X0");
                                    count += TestJSONEqual<T, uint32_t>(0xFFFFFFFFu, "0XFFFFFFFF");
                                    break;
            case sizeof(uint64_t) : // uint64_t
                                    count += TestJSONEqual<T, uint64_t>(0x0u, "0X0");
                                    count += TestJSONEqual<T, uint64_t>(0xFFFFFFFFFFFFFFFFu, "0XFFFFFFFFFFFFFFFF");
                                    break;
            default               : ASSERT(false);
                    }
            }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestHexSIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(int8_t)  :  // int8_t
                                    count += TestJSONEqual<T, int8_t>(-0x7F - 0x1, "-0X80");
                                    count += TestJSONEqual<T, int8_t>(0x7F, "0X7F");
                                    break;
            case sizeof(int16_t) :  // int16_t
                                    count += TestJSONEqual<T, int16_t>(-0x7FFF - 0x1, "-0X8000");
                                    count += TestJSONEqual<T, int16_t>(0x7FFF, "0X7FFF");
                                    break;
            case sizeof(int32_t) :  // int32_t
                                    count += TestJSONEqual<T, int32_t>(-0x7FFFFFFF - 0x1, "-0X80000000");
                                    count += TestJSONEqual<T, int32_t>(0x7FFFFFFF, "0X7FFFFFFF");
                                    break;
            case sizeof(int64_t) :  // int64_t
                                    count += TestJSONEqual<T, int64_t>(-0x7FFFFFFFFFFFFFFF - 0x1, "-0X8000000000000000");
                                    count += TestJSONEqual<T, int64_t>(0x7FFFFFFFFFFFFFFF, "0X7FFFFFFFFFFFFFFF");
                                    break;
            default               : ASSERT(false);
                    }
            }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestOctUIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(uint8_t)  : // uint8_t
                                    count += TestJSONEqual<T, uint8_t>(00u, "00");
                                    count += TestJSONEqual<T, uint8_t>(0377u, "0377");
                                    break;
            case sizeof(uint16_t) : // uint16_t
                                    count += TestJSONEqual<T, uint16_t>(00u, "00");
                                    count += TestJSONEqual<T, uint16_t>(017777u, "017777");
                                    break;
            case sizeof(uint32_t) : // uint32_t
                                    count += TestJSONEqual<T, uint32_t>(00u, "00");
                                    count += TestJSONEqual<T, uint32_t>(037777777777u, "037777777777");
                                    break;
            case sizeof(uint64_t) : // uint64_t
                                    count += TestJSONEqual<T, uint64_t>(00u, "00");
                                    count += TestJSONEqual<T, uint64_t>(01777777777777777777777u, "01777777777777777777777");
                                    break;
            default               : ASSERT(false);
                    }
            }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestOctSIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value);

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(int8_t)  :  // int8_t
                                    count += TestJSONEqual<T, int8_t>(-0177 - 01, "-0200");
                                    count += TestJSONEqual<T, int8_t>(0177, "0177");
                                    break;
            case sizeof(int16_t) :  // int16_t
                                    count += TestJSONEqual<T, int16_t>(-077777 - 01, "-0100000");
                                    count += TestJSONEqual<T, int16_t>(077777, "077777");
                                    break;
            case sizeof(int32_t) :  // int32_t
                                    count += TestJSONEqual<T, int32_t>(-017777777777 - 01, "-020000000000");
                                    count += TestJSONEqual<T, int32_t>(017777777777, "017777777777");
                                    break;
            case sizeof(int64_t) :  // int64_t
                                    count += TestJSONEqual<T, int64_t>(-0777777777777777777777 - 01, "-01000000000000000000000");
                                    count += TestJSONEqual<T, int64_t>(0777777777777777777777, "0777777777777777777777");
                                    break;
            default               : ASSERT(false);
                    }
            }

        return count == 2;
    }

    template <typename T, typename S>
    bool TestInstanceIdFromValue()
    {
        return TestHexUIntFromValue<T, S>();
    }

    template <typename T, typename S>
    bool TestPointerFromValue()
    {
        return TestHexUIntFromValue<T, S>();
    }

    TEST(JSONParser, DecUInt8)
    {
        using json_type = Core::JSON::DecUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 78);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecSInt8)
    {
        using json_type = Core::JSON::DecSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 76);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecUInt16)
    {
        using json_type = Core::JSON::DecUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 78);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecSInt16)
    {
        using json_type = Core::JSON::DecSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 76);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecUInt32)
    {
        using json_type = Core::JSON::DecUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 78);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecSInt32)
    {
        using json_type = Core::JSON::DecSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 76);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecUInt64)
    {
        using json_type = Core::JSON::DecUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 78);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, DecSInt64)
    {
        using json_type = Core::JSON::DecSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 76);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, HexUInt8)
    {
        using json_type = Core::JSON::HexUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }

    TEST(JSONParser, HexSInt8)
    {
        using json_type = Core::JSON::HexSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 92);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 30);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 30);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 30);
    }

    TEST(JSONParser, HexUInt16)
    {
        using json_type = Core::JSON::HexUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }

    TEST(JSONParser, HexSInt16)
    {
        using json_type = Core::JSON::HexSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 92);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);
    }

    TEST(JSONParser, HexUInt32)
    {
        using json_type = Core::JSON::HexUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }

    TEST(JSONParser, HexSInt32)
    {
        using json_type = Core::JSON::HexSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 92);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);
    }

    TEST(JSONParser, HexUInt64)
    {
        using json_type = Core::JSON::HexUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }

    TEST(JSONParser, HexSInt64)
    {
        using json_type = Core::JSON::HexSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 92);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 36);
    }

    TEST(JSONParser, OctUInt8)
    {
        using json_type = Core::JSON::OctUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctSInt8)
    {
        using json_type = Core::JSON::OctSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctUInt16)
    {
        using json_type = Core::JSON::OctUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctSInt16)
    {
        using json_type = Core::JSON::OctSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctUInt32)
    {
        using json_type = Core::JSON::OctUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctSInt32)
    {
        using json_type = Core::JSON::OctSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctUInt64)
    {
        using json_type = Core::JSON::OctUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, OctSInt64)
    {
        using json_type = Core::JSON::OctSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, InstanceId)
    {
        using json_type = Core::JSON::InstanceId;
        using actual_type = Core::instance_id;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestInstanceIdFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }

    TEST(JSONParser, Pointer)
    {
        using json_type = Core::JSON::Pointer;
        using actual_type = Core::instance_id;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value);

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestPointerFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestPointerFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 74);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
    }
}
}

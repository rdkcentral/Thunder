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

//#define _INTERMEDIATE

namespace WPEFramework {
namespace Tests {

    template<typename S, typename T, typename std::enable_if<!std::is_floating_point<T>::value, T*>::type = nullptr>
    bool TestJSONEqual(const T data, const std::string& reference)
    {
        std::string stream;

#ifndef _INTERMEDIATE
        bool result =    std::is_same<decltype(std::declval<S>().Value()), T>::value
                      && data == S(data).Default()
                      && data == S(data).Value()
                      && data == (S() = data).Value()
                      && S(data).ToString(stream)
                      && reference == std::string(stream)
                     ;
#else
        bool result = std::is_same<decltype(std::declval<S>().Value()), T>::value;

             result =    data == S(data).Default()
                      && result
                      ;

             result =    data == S(data).Value()
                      && result
                      ;

             result =    data == (S() = data).Value()
                      && result
                      ;

             result =    S(data).ToString(stream)
                      && result
                      ;

             result =     reference == std::string(stream)
                      && result
                      ;
#endif
        return result;
    }

    template<typename S, typename T, typename std::enable_if<std::is_floating_point<T>::value, T*>::type = nullptr>
    bool TestJSONEqual(const T data, VARIABLE_IS_NOT_USED const std::string& reference)
    {
        // Units in the last place (measure of accuracy)
        constexpr const int ulp = 2;

        return std::is_same<decltype(std::declval<S>().Value()), T>::value
               && (    std::fabs(S(data).Default() - data) <= std::numeric_limits<T>::epsilon() * std::fabs(S(data).Default() + data) * ulp
                    || std::fabs(S(data).Default() - data) < std::numeric_limits<T>::min()
                  )
               && (    std::fabs(S(data).Value() - data) <= std::numeric_limits<T>::epsilon() * std::fabs(S(data).Value() + data) * ulp
                    || std::fabs(S(data).Value() - data) < std::numeric_limits<T>::min()
                  )
               && (    std::fabs((S() = data).Value() - data) <= std::numeric_limits<T>::epsilon() * std::fabs((S() = data).Value() + data) * ulp
                    || std::fabs((S() = data).Value() - data) < std::numeric_limits<T>::min()
                  )
               ;
    }

// FIXME: alternative IS_STATIC_MEMBER_AVAIABLE requires a return which is unknow for arbitrary JSON value type

    template <typename T>
    class HasValue
    {
        private :

            using yes = struct { char y[1]; };
            using no = struct { char n[2]; };

            template <typename U>
            static yes test(decltype(&U::Value)) {yes y; return y;}

            template <typename U>
            static no test(...) {no n; return n;}

        public :

            static const bool value = sizeof(test<T>(0)) == sizeof(yes);
    };

    template<typename T, typename std::enable_if<HasValue<T>::value, T*>::type = nullptr>
    bool TestJSONFormat(const std::string& json, bool FromTo, bool AllowChange)
    {
        auto compare = [](const std::string& reference, const std::string& data) -> bool {
            // Extra \'0' at the end are counted but do not add to the 'true' lenght of the character sequence
            bool result = std::string(reference.c_str()).size() == std::string(data.c_str()).size();

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

            if (   std::is_same<T, Core::JSON::Float>::value
                || std::is_same<T, Core::JSON::Double>::value
               ) {
                T object;

                Core::OptionalType<Core::JSON::Error> status;

                result = object.FromString(data, status)
                && !(status.IsSet())
                && TestJSONEqual<T, decltype(std::declval<T>().Value())>(object.Value(), reference)
                ;
            }

            return result;
        };

        T object;

        Core::OptionalType<Core::JSON::Error> status;

        std::string stream;

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

    template<typename T, typename std::enable_if<!HasValue<T>::value, T*>::type = nullptr>
    bool TestJSONFormat(const std::string& json, bool FromTo, bool AllowChange)
    {
        auto compare = [](const std::string& reference, const std::string& data) -> bool {
            // Extra \'0' at the end are counted  but do not add to the 'true' lenght of the character sequence
            bool result = std::string(reference.c_str()).size() == std::string(data.c_str()).size();

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

            if (   std::is_same<T, Core::JSON::Float>::value
                || std::is_same<T, Core::JSON::Double>::value
               ) {
                T object;

                Core::OptionalType<Core::JSON::Error> status;

                result = object.FromString(data, status)
                && !(status.IsSet())
                ;
            }

            return result;
        };

        T object;

        Core::OptionalType<Core::JSON::Error> status;

        std::string stream;

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

#ifndef _INTERMEDIATE
            count +=    object.FromString(json, status)
                     && !(status.IsSet())
                     && (     !FromTo
                         || (   // Checking communitative property
                                object.ToString(stream)
                             && AllowChange ? std::string(json.c_str()) != std::string(stream.c_str()) : std::string(json.c_str()) == std::string(stream.c_str())
                             && object.FromString(stream, status)
                             && !(status.IsSet())
                             && std::string(stream.c_str()) == std::string(object.Value().c_str())
                           )
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

                    if (AllowChange) {
                        result =    std::string(json.c_str()) != std::string(stream,c_str())
                                  && result
                                 ;
                    } else {
                        result =    std::string(json.c_str()) == std::string(stream.c_str())
                                  && result
                                  ;
                    }

                    result =   object.FromString(stream, status)
                             && !(status.IsSet())
                             && result
                             ;

                    result =    std::string(stream.c_str()) == std::string(object.Value().c_str())
                             && result
                             ;
                 }

            count += result;
#endif
        } while (quoted);

        return count == 2;
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("1\0", FromTo, AllowChange);

                // Quoted character sequence
                count += TestJSONFormat<T>("\"0\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"1\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00201\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u00201\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 26
                           : count == 84
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("-1\0", FromTo, AllowChange);

                count += TestJSONFormat<T>("\"-0\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-1\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("-1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 28
                           : count == 82
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"0x0\"", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("\"0xA\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"0xa\"", FromTo, AllowChange); // Lower case
                count += TestJSONFormat<T>("\"0xaF\"", FromTo, AllowChange); // Mix of Lower and upper case

                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

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
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("0x1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00200x1\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u00200x1\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 40
                           : count == 80
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"-0x0\"", FromTo, AllowChange); // Second digit any out of 0-9, A-F
                count += TestJSONFormat<T>("\"-0xA\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-0xa\"", FromTo, AllowChange); // Lower case

                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

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

                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("-0x1\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020-0x1\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-0x1\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 38
                           : count == 98
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"00\"", FromTo, AllowChange); // JSON value {'0','0'}
                count += TestJSONFormat<T>("\"00\"", FromTo, AllowChange); // JSON value {'0','0'}

                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

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
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("01\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u002001\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u002001\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 24
                           : count == 52
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
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"-00\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-01\"", FromTo, AllowChange);

                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

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
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-01\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
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

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

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
                count += !TestJSONFormat<T>("-01\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u00200-1\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-01\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 26
                           : count == 44
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

    template <typename T>
    bool TestFPFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("0.0", FromTo, AllowChange);
                count += TestJSONFormat<T>("0.00", FromTo, AllowChange); // The length of the fraction sequence it typically unknown
                count += TestJSONFormat<T>("1.0", FromTo, AllowChange); // First digit any out of 1-9

                count += TestJSONFormat<T>("0.1", FromTo, AllowChange); // Second digit any out of 0-9
                count += TestJSONFormat<T>("1.1", FromTo, AllowChange); // Second digit any out of 0-9

                // C(++) floating point convention instead of scientific notation
                count += TestJSONFormat<T>("0e+0", FromTo, AllowChange);
                count += TestJSONFormat<T>("0e-0", FromTo, AllowChange);
                count += TestJSONFormat<T>("0E+0", FromTo, AllowChange);
                count += TestJSONFormat<T>("0E-0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0e+0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0e-0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0E+0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-0E-0", FromTo, AllowChange);

                count += TestJSONFormat<T>("0e+00", FromTo, AllowChange);

                count += TestJSONFormat<T>("2e-1", FromTo, AllowChange); // First digit out of 2-9
                count += TestJSONFormat<T>("2E-1", FromTo, AllowChange); // First digit out of 2-9

                count += TestJSONFormat<T>("0.0e-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("0.0E-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("0.0e-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("0.0E-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9

                count += TestJSONFormat<T>("0.0e+1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digits any out of 1-9;
                count += TestJSONFormat<T>("0.0E+1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digits any out of 1-9;

                count += TestJSONFormat<T>("-0.0", FromTo, AllowChange);
                count += TestJSONFormat<T>("-1.0", FromTo, AllowChange); // First digit any out of 1-9

                count += TestJSONFormat<T>("-0.1", FromTo, AllowChange); // Second digit any out of 0-9
                count += TestJSONFormat<T>("-1.1", FromTo, AllowChange); // Second digit any out of 0-9

                count += TestJSONFormat<T>("-2e-1", FromTo, AllowChange); // First digit out of 2-9
                count += TestJSONFormat<T>("-2E-1", FromTo, AllowChange); // First digit out of 2-9

                count += TestJSONFormat<T>("-0.0e-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("-0.0E-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("-0.0e-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9
                count += TestJSONFormat<T>("-0.0E-1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digit out of 1-9

                count += TestJSONFormat<T>("-0.0e+1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digits any out of 1-9;
                count += TestJSONFormat<T>("-0.0E+1", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digits any out of 1-9;

                count += TestJSONFormat<T>("1.1E-2", FromTo, AllowChange); // Fractional digits any out of 0-9, exponent digits any out of 1-9;

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with 0
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"-0.0\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"-0.1\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

                // Range
                if (   std::is_same<decltype(std::declval<T>().Value()), float>::value
                    || std::is_same<decltype(std::declval<T>().Value()), double>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(float)  : // float
                                          count += TestJSONFormat<T>("-3.402823466e+38", FromTo, AllowChange);
                                          count += TestJSONFormat<T>("3.402823466e+38", FromTo, AllowChange);
                                          break;
                    case sizeof(double) : // double
                                          count += TestJSONFormat<T>("-1.7976931348623158e+308", FromTo, AllowChange);
                                          count += TestJSONFormat<T>("1.7976931348623158e+308", FromTo, AllowChange);
                                          break;
                    default             : ASSERT(false);
                    }
                }

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1.0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1.0\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1.0\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
            } else {
                // Malformed
                // =========

                // Missing fractional part
                count += !TestJSONFormat<T>("0", FromTo, AllowChange); // Digits any out of 0-9

                count += !TestJSONFormat<T>("00.0", FromTo, AllowChange); // Digits any out of 0-9

                count += !TestJSONFormat<T>("--0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("+1.0", FromTo, AllowChange); // Digits any out of 0-9
                count += !TestJSONFormat<T>("0.", FromTo, AllowChange); // Digit any out of 0-9

                count += !TestJSONFormat<T>("0.e", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0.0e+", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0e-", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0e++", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0e--", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0.0e+", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0e-", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0e++", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0e--", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0e0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0E0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0.0e0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0E0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0e0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0E0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0.0e+0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0E+0.0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("0.0e-0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0.0E-0.0", FromTo, AllowChange);

                count += !TestJSONFormat<T>("-0.0e-0.0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("-0.0E-0.0", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);

                // Non-numbers
                count += !TestJSONFormat<T>("Infinity", FromTo, AllowChange);
                count += !TestJSONFormat<T>("NaN", FromTo, AllowChange);

                count += !TestJSONFormat<T>("\"-0.0\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"-0.1\"2", FromTo, AllowChange);

                count += !TestJSONFormat<T>("\"-00\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"-01\"", FromTo, AllowChange);

                // Incomplete null
                count += !TestJSONFormat<T>("nul", FromTo, AllowChange);
                count += !TestJSONFormat<T>("nul0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("null0", FromTo, AllowChange);

                // Out-of-range
                // The values might not be the smallest (absolute) out-of-range value
                if (   std::is_same<decltype(std::declval<T>().Value()), float>::value
                    || std::is_same<decltype(std::declval<T>().Value()), double>::value
                   ) {
                    switch (sizeof(std::declval<T>().Value())) {
                    case sizeof(float)  : // float
                                          count += !TestJSONFormat<T>("-3.403e+38", FromTo, AllowChange);
                                          count += !TestJSONFormat<T>("3.403e+38", FromTo, AllowChange);
                                          break;
                    case sizeof(double) : // double
                                          count += !TestJSONFormat<T>("-1.798e+308", FromTo, AllowChange);
                                          count += !TestJSONFormat<T>("1.798e+308", FromTo, AllowChange);
                                          break;
                    default             : ASSERT(false);
                    }
                }
                // Insignificant white space
                count += !TestJSONFormat<T>("-1.\u0009\u000A\u000D\u00200\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020-1.\u0009\u000A\u000D\u00200", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020-1.\"0\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 90
                           : count == 94
                           ;
    }

    template <typename T>
    bool TestBoolFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                count += TestJSONFormat<T>("true", FromTo, AllowChange);
                count += TestJSONFormat<T>("false", FromTo, AllowChange);
                // 16-bit as UTF-8
                count += TestJSONFormat<T>(u8"\u0074\u0072\u0075\u0065", FromTo, AllowChange);
                count += TestJSONFormat<T>(u8"\u0066\u0061\u006C\u0073\u0065", FromTo, AllowChange);
                // 32-bit as UTF-8
                count += TestJSONFormat<T>(u8"\U00000074\U00000072\U00000075\U00000065", FromTo, AllowChange);
                count += TestJSONFormat<T>(u8"\U00000066\U00000061\U0000006C\U00000073\U00000065", FromTo, AllowChange);

                // Implementation constraint
                count += TestJSONFormat<T>("0", FromTo, !AllowChange); // false
                count += TestJSONFormat<T>("1", FromTo, !AllowChange); // true

                // Implementation constraint: all IElement objects can be nullified
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Empty string, no distinction with
                count += TestJSONFormat<T>("", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\0", FromTo, !AllowChange);

                count += TestJSONFormat<T>("\"true\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"false\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"1\"", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"0\"", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);

                // Insignificant white space
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020true\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020true\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020true\u0009\u000A\u000D\u0020\"", FromTo, !AllowChange);
            } else {
                // Malformed
                // =========

                count += !TestJSONFormat<T>("1true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0false", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("truet", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("falsef", FromTo, AllowChange);

                count += !TestJSONFormat<T>("TRUE", FromTo, AllowChange);
                count += !TestJSONFormat<T>("FALSE", FromTo, AllowChange);

                count += !TestJSONFormat<T>(u8"\u0054\u0052\u0055\u0045", FromTo, AllowChange);
                count += !TestJSONFormat<T>(u8"\U00000054\U00000052\U00000055\U00000045", FromTo, AllowChange);
                count += !TestJSONFormat<T>(u8"\u0046\u0041\u004C\u0053\u0045", FromTo, AllowChange);
                count += !TestJSONFormat<T>(u8"\U00000046\U00000041\U0000004C\U00000053\U00000045", FromTo, AllowChange);

                count += !TestJSONFormat<T>("00", FromTo, AllowChange);
                count += !TestJSONFormat<T>("01", FromTo, AllowChange);
                count += !TestJSONFormat<T>("10", FromTo, AllowChange);
                count += !TestJSONFormat<T>("11", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\"", FromTo, AllowChange);

                count += !TestJSONFormat<T>("\"true\"true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"false\"false", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"true\"1", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"false\"0", FromTo, AllowChange);
                count += !TestJSONFormat<T>("1\"true\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("0\"false\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true\"true\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false\"false\"", FromTo, AllowChange);

                // Insignificant white space
                count += !TestJSONFormat<T>("tru\u0009\u000A\u000D\u0020e\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\u0009\u000A\u000D\u0020tru\u0009\u000A\u000D\u0020e", FromTo, !AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020true\"e\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
            }
        } while (FromTo);

        return !malformed ? count == 38
                          : count == 60
               ;
    }

    template <typename T>
    bool TestStringFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                // Opaque strings
//                count += TestJSONFormat<T>("", FromTo, AllowChange); // Empty, by definition considered opaque but Deserialize is never triggered to categorize it as such
                count += TestJSONFormat<T>(" ", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc 123 ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>(" abc 123 ABC ", FromTo, AllowChange);

                // JSON value strings
                count += TestJSONFormat<T>("\"\"", FromTo, AllowChange); // Empty
                count += TestJSONFormat<T>("\" \"", FromTo, AllowChange); // Regular space
                count += TestJSONFormat<T>("\"abc123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"abc 123 ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\" abc 123 ABC \"", FromTo, AllowChange);

                // Opaque strings for reverse solidus
                count += TestJSONFormat<T>("abc\\123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u005C", FromTo, AllowChange);

                // JSON value string reverse solidus
                count += TestJSONFormat<T>("\"\\\\\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u005C\"", FromTo, AllowChange);

                // Opaque strings for quotation mark do not exist, except if the character is preceded by a non-quotation mark
                count += TestJSONFormat<T>("abc\"123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\"123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u0022", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\u0022", FromTo, AllowChange); // Requires preceding character not to indicate start of JSON value string

                // JSON value string for quotation mark
                count += TestJSONFormat<T>("\"abc\\\"123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\"\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0022\"", FromTo, AllowChange);

                // Opaque strings for solidus
                count += TestJSONFormat<T>("abc/123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\/123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("/", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\/", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u002F", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u002F", FromTo, AllowChange);

                // JSON value strings for solidus
                count += TestJSONFormat<T>("\"abc\\/123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\/\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\/\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u002F\"", FromTo, AllowChange);

                // Opaque strrings for backspace
                count += TestJSONFormat<T>("abc\b123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\b123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\b", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\b", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u0008", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u0008", FromTo, AllowChange);

                // JSON value strings for backspace
                count += TestJSONFormat<T>("\"abc\\\b123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\b\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0008\"", FromTo, AllowChange);

                // Opaque strings for form feed
                count += TestJSONFormat<T>("abc\f123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\f123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\f", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\f", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u000C", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u000C", FromTo, AllowChange);

                // JSON value strings for form feed
                count += TestJSONFormat<T>("\"abc\\\f123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\f\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000C\"", FromTo, AllowChange);

                // Opaque strings for line feed
                count += TestJSONFormat<T>("abc\n123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\n123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\n", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\n", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u000A", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u000A", FromTo, AllowChange);

                // JSON value strings for line feed
                count += TestJSONFormat<T>("\"abc\\\n123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\n\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000A\"", FromTo, AllowChange);

                // Opaque strings for carriage return
                count += TestJSONFormat<T>("abc\r123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\r123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\r", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\r", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u000D", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u000D", FromTo, AllowChange);

                // JSON value strings for carriage return
                count += TestJSONFormat<T>("\"abc\\\r123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\r\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000D\"", FromTo, AllowChange);

                // Opaque strings for character tabulation
                count += TestJSONFormat<T>("abc\b123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("abc\\\b123ABC", FromTo, AllowChange);
                count += TestJSONFormat<T>("\b", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\\b", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u0009", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u0009", FromTo, AllowChange);

                // JSON value strings for character tabulation
                count += TestJSONFormat<T>("\"abc\\\b123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\b\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0009\"", FromTo, AllowChange);

                // Opaque strings for escape boundaries
                count += TestJSONFormat<T>("\\u0000", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u001F", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u0020", FromTo, AllowChange);
//                count += TestJSONFormat<T>("\u0000", FromTo, AllowChange);// Empty, by definition considered opaque but Deserialize is never triggered to categorize it as such
                count += TestJSONFormat<T>("\u001F", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u0020", FromTo, AllowChange);

                // JSON value strings for escape boundaries
                count += TestJSONFormat<T>("\"\\u0000\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u001F\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0020\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\u0020\"", FromTo, AllowChange);

                // Opaque strings for incomplete unicode
                count += TestJSONFormat<T>("\\u", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u0", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u00", FromTo, AllowChange);
                count += TestJSONFormat<T>("\\u002", FromTo, AllowChange);

                // Opaque string for 'nullifying'
                count += TestJSONFormat<T>("null", FromTo, AllowChange); // Not nullifying!
                count += TestJSONFormat<T>("null0", FromTo, AllowChange);

                // JSON value string 'nullifying'
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"nul0\"", FromTo, AllowChange);

                // Opaque strings for tokens
                count += TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += TestJSONFormat<T>("true", FromTo, AllowChange);
                count += TestJSONFormat<T>("false", FromTo, AllowChange);

                // JSON value string for tokens
                count += TestJSONFormat<T>("\"{}\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"[]\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"true\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"false\"", FromTo, AllowChange);

                // Opaque strings for Insignificant white space
                count += TestJSONFormat<T>("  ", FromTo, AllowChange);
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020abc123ABC\u0009\u000A\u000D\u0020", FromTo, AllowChange);

                // JSON value strings for Insignificant white space
                count += TestJSONFormat<T>(" \"\" ", FromTo, !AllowChange); // Empty with insignificant white spaces, stripped away
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020\"abc123ABC\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // Idem
            } else {
                // Malformed
                // =========

                // JSON value string reverse solidus
                count += !TestJSONFormat<T>("\"abc\\123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\\\"", FromTo, AllowChange);

                // Opaque strings for quotation mark
                count += !TestJSONFormat<T>("\u0022", FromTo, AllowChange);

                // JSON value string for quotation mark
                count += !TestJSONFormat<T>("\"abc\"123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u0022\"", FromTo, AllowChange);

                // JSON value strings for solidus
                count += !TestJSONFormat<T>("\"abc/123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u002F\"", FromTo, AllowChange);

                // JSON value strings for backspace
                count += !TestJSONFormat<T>("\"abc\b123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\b\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u0008\"", FromTo, AllowChange);

                // JSON value strings for form feed
                count += !TestJSONFormat<T>("\"abc\f123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\f\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u000C\"", FromTo, AllowChange);

                // JSON value strings for line feed
                count += !TestJSONFormat<T>("\"abc\n123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\n\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u000A\"", FromTo, AllowChange);

                // JSON value strings for carriage return
                count += !TestJSONFormat<T>("\"abc\r123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\r\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u000D\"", FromTo, AllowChange);

                // JSON value strings for character tabulation
                count += !TestJSONFormat<T>("\"abc\b123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\b\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\"", FromTo, AllowChange);

                // JSON value strings for escape boundaries
                count += !TestJSONFormat<T>("\"\u0000\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u001F\"", FromTo, AllowChange);

                // JSON value string for incomplete unicode
                count += !TestJSONFormat<T>("\"\\u\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\\u0\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\\u00\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\\u002\"", FromTo, AllowChange);

                // JSON value string 'nullifying'
                count += !TestJSONFormat<T>("\"null0\"", FromTo, AllowChange);

                // JSON value strings for Insignificant white space
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020abc123ABC\"\u0009\u000A\u000D\u0020", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u0009\u000A\u000D\u0020abc123ABC\u0009\u000A\u000D\u0020\"", FromTo, AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 212
                           : count == 62
               ;
    }

    template <typename S>
    bool TestArrayFromString(bool malformed, uint8_t& count)
    {
        // Core::JSON::ArrayType is constricted to a single type and as a consequence not a genuine JSON value array

        constexpr bool AllowChange = false;

        using T = Core::JSON::ArrayType<S>;
        using W = Core::JSON::ArrayType<T>;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================
                // 'Empty' and nested JSON value arrays are allowed

                count += TestJSONFormat<T>("", FromTo, !AllowChange); // Empty by definition, returns ['content of empty type']
                count += TestJSONFormat<T>("[]", FromTo, !AllowChange); // returns ['content of empty type']

                // Insignificant white space before token is allowed and may be stipped
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020[]", FromTo, !AllowChange); // ['content of empty type']
                count += TestJSONFormat<T>("[]\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // ['content of empty type]
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020[]\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // ['content of empty type']

                count += TestJSONFormat<W>("[[]]", FromTo, !AllowChange); // [['content of empty type']]
                count += TestJSONFormat<W>("[[],[]]", FromTo, !AllowChange); // [['content of empty type']]
                count += TestJSONFormat<W>("[[],\u0009\u000A\u000D\u0020[]]", FromTo, !AllowChange); // [['content of empty type']]
                count += TestJSONFormat<W>("[[]\u0009\u000A\u000D\u0020,[]]", FromTo, !AllowChange); // [['content of empty type']]
                count += TestJSONFormat<W>("[[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[]]", FromTo, !AllowChange); // [['content of empty type']]

                // Nullify ArrayType
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Nullify contained types, except for string, that treats it opaque
                count += TestJSONFormat<T>("[null]", FromTo, AllowChange);
                count += TestJSONFormat<T>("[null,null]", FromTo, AllowChange);
                count += TestJSONFormat<T>("[null,\u0009\u000A\u000D\u0020null]", FromTo, !AllowChange); // [null,null]
                count += TestJSONFormat<T>("[null\u0009\u000A\u000D\u0020,null]", FromTo, !AllowChange); // [null,null]
                count += TestJSONFormat<T>("[null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020null]", FromTo, !AllowChange); // [null,null]

                count += TestJSONFormat<W>("[[null]]", FromTo, AllowChange);
                count += TestJSONFormat<W>("[[null],[null]]", FromTo, AllowChange);
                count += TestJSONFormat<W>("[[null],\u0009\u000A\u000D\u0020[null]]", FromTo, !AllowChange); // [[null],[null]]
                count += TestJSONFormat<W>("[[null]\u0009\u000A\u000D\u0020,[null]]", FromTo, !AllowChange); // [[null],[null]]
                count += TestJSONFormat<W>("[[null]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[null]]", FromTo, !AllowChange); // [[null],[null]]

                if (std::is_same<S, Core::JSON::Container>::value) {
                    count += TestJSONFormat<T>("[{}]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[{},{}]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[{},\u0009\u000A\u000D\u0020{}]", FromTo, !AllowChange); // [{},{}]
                    count += TestJSONFormat<T>("[{}\u0009\u000A\u000D\u0020,{}]", FromTo, !AllowChange); // [{},{}]
                    count += TestJSONFormat<T>("[{}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020{}]", FromTo, !AllowChange); // [{},{}]

                    count += TestJSONFormat<W>("[[{}]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[{}],[{}]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[{[}],\u0009\u000A\u000D\u0020[{}]]", FromTo, !AllowChange); // [[{}],[{}]]
                    count += TestJSONFormat<W>("[[{}]\u0009\u000A\u000D\u0020,[{}]]", FromTo, !AllowChange); // [[{}],[{}]]
                    count += TestJSONFormat<W>("[[{}]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[{}]]", FromTo, !AllowChange); // [[{}],[{}]]

                    // Scope tests
                    count += TestJSONFormat<T>("[{{},{}},{{},{}}]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[{{},{}},{{},{}}]\"", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"{{},{}}\",\"{{},{}}\"]\"", FromTo, AllowChange);

                    count += 15;
                }

                if (   std::is_same<S, Core::JSON::DecUInt8>::value
                    || std::is_same<S, Core::JSON::DecSInt8>::value
                    || std::is_same<S, Core::JSON::DecUInt16>::value
                    || std::is_same<S, Core::JSON::DecSInt16>::value
                    || std::is_same<S, Core::JSON::DecUInt32>::value
                    || std::is_same<S, Core::JSON::DecSInt32>::value
                    || std::is_same<S, Core::JSON::DecUInt64>::value
                    || std::is_same<S, Core::JSON::DecSInt64>::value
                ) {
                    count += TestJSONFormat<T>("[0]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[0,0]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[0,\u0009\u000A\u000D\u00200]", FromTo, !AllowChange); // [0,0]
                    count += TestJSONFormat<T>("[0\u0009\u000A\u000D\u0020,0]", FromTo, !AllowChange); // [0,0]
                    count += TestJSONFormat<T>("[0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u00200]", FromTo, !AllowChange); // [0,0]

                    count += TestJSONFormat<W>("[[0]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[0],[0]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[0],\u0009\u000A\u000D\u0020[0]]", FromTo, !AllowChange); // [[0],[0]]
                    count += TestJSONFormat<W>("[[0]\u0009\u000A\u000D\u0020,[0]]", FromTo, !AllowChange); // [[0],[0]]
                    count += TestJSONFormat<W>("[[0]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[0]]", FromTo, !AllowChange); // [[0],[0]]

                    // Scope tests
                    count += TestJSONFormat<T>("[\"0\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"0\"]\"", FromTo, AllowChange);

                    count += 16;
                }

                if (   std::is_same<S, Core::JSON::HexUInt8>::value
                    || std::is_same<S, Core::JSON::HexSInt8>::value
                    || std::is_same<S, Core::JSON::HexUInt16>::value
                    || std::is_same<S, Core::JSON::HexSInt16>::value
                    || std::is_same<S, Core::JSON::HexUInt32>::value
                    || std::is_same<S, Core::JSON::HexSInt32>::value
                    || std::is_same<S, Core::JSON::HexUInt64>::value
                    || std::is_same<S, Core::JSON::HexSInt64>::value
                    || std::is_same<S, Core::JSON::InstanceId>::value
                    || std::is_same<S, Core::JSON::Pointer>::value
                ) {
                    count += TestJSONFormat<T>("[0x0]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[0x0,0x0]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[0x0,\u0009\u000A\u000D\u00200x0]", FromTo, !AllowChange); // [0x0,0x0]
                    count += TestJSONFormat<T>("[0x0\u0009\u000A\u000D\u0020,0x0]", FromTo, !AllowChange); // [0x0,0x0]
                    count += TestJSONFormat<T>("[0x0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u00200x0]", FromTo, !AllowChange); // [0x0,0x0]

                    count += TestJSONFormat<W>("[[0x0]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[0x0],[0x0]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[0x0],\u0009\u000A\u000D\u0020[0x0]]", FromTo, !AllowChange); // [[0x0,0x0]]
                    count += TestJSONFormat<W>("[[0x0]\u0009\u000A\u000D\u0020,[0x0]]", FromTo, !AllowChange); // [[0x0,0x0]]
                    count += TestJSONFormat<W>("[[0x0]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[0x0]]", FromTo, !AllowChange); // [[0x0,0x0]]

                    // Scope tests
                    count += TestJSONFormat<T>("[\"0x0\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"0x0\"]\"", FromTo, AllowChange);

                    count += 16;
                }

                if (   std::is_same<S, Core::JSON::OctUInt8>::value
                    || std::is_same<S, Core::JSON::OctSInt8>::value
                    || std::is_same<S, Core::JSON::OctUInt16>::value
                    || std::is_same<S, Core::JSON::OctSInt16>::value
                    || std::is_same<S, Core::JSON::OctUInt32>::value
                    || std::is_same<S, Core::JSON::OctSInt32>::value
                    || std::is_same<S, Core::JSON::OctUInt64>::value
                    || std::is_same<S, Core::JSON::OctSInt64>::value
                ) {
                    count += TestJSONFormat<T>("[00]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[00,00]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[00,\u0009\u000A\u000D\u002000]", FromTo, !AllowChange); // [00,00]
                    count += TestJSONFormat<T>("[00\u0009\u000A\u000D\u0020,00]", FromTo, !AllowChange); // [00,00]
                    count += TestJSONFormat<T>("[00\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u002000]", FromTo, !AllowChange); // [00,00]

                    count += TestJSONFormat<W>("[[00]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[00],[00]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[00],\u0009\u000A\u000D\u0020[00]]", FromTo, !AllowChange); // [[00,00]]
                    count += TestJSONFormat<W>("[[00]\u0009\u000A\u000D\u0020,[00]]", FromTo, !AllowChange); // [[00,00]]
                    count += TestJSONFormat<W>("[[00]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[00]]", FromTo, !AllowChange); // [[00,00]]

                    // Scope tests
                    count += TestJSONFormat<T>("[\"00\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"00\"]\"", FromTo, AllowChange);

                    count += 16;
                }

                if (   std::is_same<S, Core::JSON::Float>::value
                    || std::is_same<S, Core::JSON::Double>::value
                ) {
                    // All re-created character sequences for arrays are not compared on float value basis but on character per character basis.
                    // This alost always fails.
                    count += TestJSONFormat<T>("[0.0]", FromTo, !AllowChange);
                    count += TestJSONFormat<T>("[0.0,0.0]", FromTo, !AllowChange);
                    count += TestJSONFormat<T>("[0.0,\u0009\u000A\u000D\u00200.0]", FromTo, !AllowChange); // [[0.0,0.0]
                    count += TestJSONFormat<T>("[0.0\u0009\u000A\u000D\u0020,0.0]", FromTo, !AllowChange); // [[0.0,0.0]
                    count += TestJSONFormat<T>("[0.0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u00200.0]", FromTo, !AllowChange); // [[0.0,0.0]

                    count += TestJSONFormat<W>("[[0.0]]", FromTo, !AllowChange);
                    count += TestJSONFormat<W>("[[0.0],[0.0]]", FromTo, !AllowChange);
                    count += TestJSONFormat<W>("[[0.0],\u0009\u000A\u000D\u0020[0.0]]", FromTo, !AllowChange); // [[0.0],[0.0]]
                    count += TestJSONFormat<W>("[[0.0]\u0009\u000A\u000D\u0020,[0.0]]", FromTo, !AllowChange); // [[0.0],[0.0]]
                    count += TestJSONFormat<W>("[[0.0]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[0.0]]", FromTo, !AllowChange); // [[0.0],[0.0]]

                    // Scope tests
                    count += TestJSONFormat<T>("[\"0.0\"]", FromTo, !AllowChange);
                    count += TestJSONFormat<T>("\"[\"0.0\"]\"", FromTo, !AllowChange);

                    count += 16;
                }

                if (std::is_same<S, Core::JSON::String>::value) {
                    count += TestJSONFormat<T>("[\"\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[\"\",\"\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[\"\",\u0009\u000A\u000D\u0020\"\"]", FromTo, !AllowChange); // [\"\",\"\"]
                    count += TestJSONFormat<T>("[\"\"\u0009\u000A\u000D\u0020,\"\"]", FromTo, !AllowChange); // [\"\",\"\"]
                    count += TestJSONFormat<T>("[\"\"\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\"]", FromTo, !AllowChange); // [\"\",\"\"]

                    count += TestJSONFormat<W>("[[\"\"]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[\"\"],[\"\"]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[\"\"],\u0009\u000A\u000D\u0020[\"\"]]", FromTo, !AllowChange); // [[\"\"],[\"\"]]
                    count += TestJSONFormat<W>("[[\"\"]\u0009\u000A\u000D\u0020,[\"\"]]", FromTo, !AllowChange); // [[\"\"],[\"\"]]
                    count += TestJSONFormat<W>("[[\"\"]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[\"\"]]", FromTo, !AllowChange); // [[\"\"],[\"\"]]

                    // Nullifying string element
                    count += TestJSONFormat<T>("[\"null\"]", FromTo, AllowChange);

                    // Scope tests
                    count += TestJSONFormat<T>("[\"[\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[\"{}\"]", FromTo, AllowChange);

                    count += 15;
                }

                if (std::is_same<S, Core::JSON::Boolean>::value) {
                    count += TestJSONFormat<T>("[true]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[false]", FromTo, AllowChange);

                    count += TestJSONFormat<T>("[true,true]", FromTo, AllowChange); 
                    count += TestJSONFormat<T>("[true,\u0009\u000A\u000D\u0020true]", FromTo, !AllowChange); // [true,true]
                    count += TestJSONFormat<T>("[true\u0009\u000A\u000D\u0020,true]", FromTo, !AllowChange); // [true,true]
                    count += TestJSONFormat<T>("[true\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020true]", FromTo, !AllowChange); // [true,true]
                    count += TestJSONFormat<T>("[false,false]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[false,\u0009\u000A\u000D\u0020false]", FromTo, !AllowChange); // [false,false]
                    count += TestJSONFormat<T>("[false\u0009\u000A\u000D\u0020,false]", FromTo, !AllowChange); // [false,false]
                    count += TestJSONFormat<T>("[false\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020false]", FromTo, !AllowChange); // [false,false]

                    count += TestJSONFormat<T>("[true,false]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[false,true]", FromTo, AllowChange);

                    count += TestJSONFormat<W>("[[true]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[false]]", FromTo, AllowChange);

                    count += TestJSONFormat<W>("[[true],[true]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[true],\u0009\u000A\u000D\u0020[true]]", FromTo, !AllowChange); // [[true],[true]]
                    count += TestJSONFormat<W>("[[true]\u0009\u000A\u000D\u0020,[true]]", FromTo, !AllowChange); // [[true],[true]]
                    count += TestJSONFormat<W>("[[true]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[true]]", FromTo, !AllowChange); // [[true],[true]]
                    count += TestJSONFormat<W>("[[false],[false]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[false],\u0009\u000A\u000D\u0020[false]]", FromTo, !AllowChange); // [[false],[false]]
                    count += TestJSONFormat<W>("[[false]\u0009\u000A\u000D\u0020,[false]]", FromTo, !AllowChange); // [[false],[false]]
                    count += TestJSONFormat<W>("[[false]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[false]]", FromTo, !AllowChange); // [[false],[false]]

                    count += TestJSONFormat<W>("[[true],[false]]", FromTo, AllowChange);
                    count += TestJSONFormat<W>("[[false],[true]]", FromTo, AllowChange);

                    // Scope tests
                    count += TestJSONFormat<T>("[\"true\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"true\"]\"", FromTo, AllowChange);
                    count += TestJSONFormat<T>("[\"false\"]", FromTo, AllowChange);
                    count += TestJSONFormat<T>("\"[\"false\"]\"", FromTo, AllowChange);
                }
            }  else {
                // Malformed
                // =========

                // Take [] as JSON value
                count += !TestJSONFormat<T>("[,]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[],", FromTo, AllowChange);
                count += !TestJSONFormat<T>(",[]", FromTo, AllowChange);
                count += !TestJSONFormat<W>("[,[]]", FromTo, AllowChange);
                count += !TestJSONFormat<W>("[[],]", FromTo, AllowChange);
                count += !TestJSONFormat<W>("[[][]]", FromTo, AllowChange);
                count += !TestJSONFormat<W>("[[].[]]", FromTo, AllowChange); // Any character not being ,

                // Mismatch opening and closing
                count += !TestJSONFormat<T>("[", FromTo, AllowChange);
                count += !TestJSONFormat<T>("]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("(]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[)", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("()", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 98
                           : count == 34
               ;
    }

    template <typename S>
    bool TestContainerFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        using T = Core::JSON::Container;
        using W = Core::JSON::Container;

        count = 0;

        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================
                // 'Empty' and nested JSON value containers are allowed
                count += TestJSONFormat<T>("", FromTo, !AllowChange); // {}
                count += TestJSONFormat<T>("{}", FromTo, AllowChange); // {}

                // Insignificant white space before token is allowed and may be stipped
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020{}", FromTo, !AllowChange); // {}
                count += TestJSONFormat<T>("{}\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // {}
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020{}\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // {}

                count += TestJSONFormat<W>("{\"\":{}}", FromTo, AllowChange); // {\"\":{}}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<W>("{\"\":{},\"\":{}}", FromTo, !AllowChange); // {\"\":{}}
                count += TestJSONFormat<W>("{\"\":{},\u0009\u000A\u000D\u0020\"\":{}}", FromTo, !AllowChange); // {\"\":{}}
                count += TestJSONFormat<W>("{\"\":{}\u0009\u000A\u000D\u0020,\"\":{}}", FromTo, !AllowChange); // {\"\":{}}
                count += TestJSONFormat<W>("{\"\":{}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{}}", FromTo, !AllowChange); // {\"\":{}}

                count += TestJSONFormat<W>("{\"A\":{},\"B\":{}}", FromTo, AllowChange); // {\"A\":{},\"B\":{}}
                count += TestJSONFormat<W>("{\"A\":{},\u0009\u000A\u000D\u0020\"B\":{}}", FromTo, !AllowChange); // {\"A\":{},\"B\":{}}
                count += TestJSONFormat<W>("{\"A\":{}\u0009\u000A\u000D\u0020,\"B\":{}}", FromTo, !AllowChange); // {\"A\":{},\"B\":{}}
                count += TestJSONFormat<W>("{\"A\":{}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{}}", FromTo, !AllowChange); // {\"A\":{},\"B\":{}}

                // Nullify Container
                count += TestJSONFormat<T>("null", FromTo, AllowChange);

                // Nullify contained types, except for string, that treats it opaque
                count += TestJSONFormat<T>("{\"\":null}", FromTo, AllowChange); // {\"\":null}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<T>("{\"\":null,\"\":null}", FromTo, !AllowChange); // {\"\":null}
                count += TestJSONFormat<T>("{\"\":null,\u0009\u000A\u000D\u0020\"\":null}", FromTo, !AllowChange); // {\"\":null}
                count += TestJSONFormat<T>("{\"\":null\u0009\u000A\u000D\u0020,\"\":null}", FromTo, !AllowChange); // {\"\":null}
                count += TestJSONFormat<T>("{\"\":null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":null}", FromTo, !AllowChange); // {\"\":null}

                count += TestJSONFormat<T>("{\"A\":null,\"B\":null}", FromTo, !AllowChange); // {\"A\":null,\"B\":null}
                count += TestJSONFormat<T>("{\"A\":null,\u0009\u000A\u000D\u0020\"B\":null}", FromTo, !AllowChange); // {\"A\":null,\"B\":null}
                count += TestJSONFormat<T>("{\"A\":null\u0009\u000A\u000D\u0020,\"B\":null}", FromTo, !AllowChange); // {\"A\":null,\"B\":null}
                count += TestJSONFormat<T>("{\"A\":null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":null}", FromTo, !AllowChange); // {\"A\":null,\"B\":null}

                count += TestJSONFormat<W>("{\"\":{\"\":null}}", FromTo, AllowChange); // {\"\":{\"\":null}}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<W>("{\"\":{\"\":null},\"\":{\"\":null}}", FromTo, !AllowChange); // {\"\":{\"\":null}}
                count += TestJSONFormat<W>("{\"\":{\"\":null},\u0009\u000A\u000D\u0020\"\":{\"\":null}}", FromTo, !AllowChange); // {\"\":{\"\":null}}
                count += TestJSONFormat<W>("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\"\":{\"\":null}}", FromTo, !AllowChange); // {\"\":{\"\":null}}
                count += TestJSONFormat<W>("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":null}}", FromTo, !AllowChange); // {\"\":{\"\":null}}

                count += TestJSONFormat<W>("{\"A\":{\"\":null},\"B\":{\"\":null}}", FromTo, !AllowChange); // {\"A\":{\"\":null},\"B\":{\"\":{}}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null},\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", FromTo, !AllowChange); // {\"A\":{\"\":null},\"B\":{\"\":null}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\"B\":{\"\":null}}", FromTo, !AllowChange); // {\"A\":{\"\":null},\"B\":{\"\":null}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", FromTo, !AllowChange); // {\"A\":{\"\":null},\"\":{\"\":null}}

                // An empty array element is defined as String since its type is unknown and, thus, a type cannot be deduced from a character sequence.

                count += TestJSONFormat<T>("{\"\":[]}", FromTo, !AllowChange); // {\"\":[\"\"]}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<T>("{\"\":[],\"\":[]}", FromTo, !AllowChange); // {\"\":[\"\"]}
                count += TestJSONFormat<T>("{\"\":[],\u0009\u000A\u000D\u0020\"\":[]}", FromTo, !AllowChange); // {\"\":[\"\"]}
                count += TestJSONFormat<T>("{\"\":[]\u0009\u000A\u000D\u0020,\"\":[]}", FromTo, !AllowChange); // {\"\":[\"\"]}
                count += TestJSONFormat<T>("{\"\":[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":[]}", FromTo, !AllowChange); // {\"\":[\"\"]}

                count += TestJSONFormat<T>("{\"\":[[],[]]}", FromTo, AllowChange); // {\"\":[[],[]]}, detected as an ArrayType of String with each String being the opaque sequence of characters '[' and ']'

                count += TestJSONFormat<W>("{\"\":{\"\":[]}}", FromTo, !AllowChange); // {\"\":{\"\":[\"\"]}}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<W>("{\"\":{\"\":[]},\"\":{\"\":[]}}", FromTo, !AllowChange); // {\"\":{\"\":[\"\"]}}
                count += TestJSONFormat<W>("{\"\":{\"\":[]},\u0009\u000A\u000D\u0020\"\":{\"\":[]}}", FromTo, !AllowChange); // {\"\":{\"\":[\"\"]}}
                count += TestJSONFormat<W>("{\"\":{\"\":[]}\u0009\u000A\u000D\u0020,\"\":{\"\":[]}}", FromTo, !AllowChange); // {\"\":{\"\":[\"\"]}}
                count += TestJSONFormat<W>("{\"\":{\"\":[]}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":[]}}", FromTo, !AllowChange); // {\"\":{\"\":[\"\"]}}

                if (   std::is_same<S, Core::JSON::DecUInt8>::value
                    || std::is_same<S, Core::JSON::DecSInt8>::value
                    || std::is_same<S, Core::JSON::DecUInt16>::value
                    || std::is_same<S, Core::JSON::DecSInt16>::value
                    || std::is_same<S, Core::JSON::DecUInt32>::value
                    || std::is_same<S, Core::JSON::DecSInt32>::value
                    || std::is_same<S, Core::JSON::DecUInt64>::value
                    || std::is_same<S, Core::JSON::DecSInt64>::value
                ) {
                    count += TestJSONFormat<T>("{\"\":0}", FromTo, AllowChange); // {\"\":0}

                    count += TestJSONFormat<W>("{\"\":{\"\":0}}", FromTo, AllowChange); // {\"\":{\"\":0}}

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[0]}", FromTo, AllowChange); // {\"\":[0]}
                    count += TestJSONFormat<T>("{\"\":[[0]]}", FromTo, AllowChange); // {\"\":[[0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0]]]}", FromTo, AllowChange); // {\"\":[[[0]]]}

                    count += TestJSONFormat<T>("{\"\":[0,0]}", FromTo, AllowChange); // {\"\":[0,0]}
                    count += TestJSONFormat<T>("{\"\":[[0,0]]}", FromTo, AllowChange); // {\"\":[[0,0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0,0]]]}", FromTo, AllowChange); // {\"\":[[[0,0]]}

                    count += TestJSONFormat<T>("{\"\":[\"0\"]}", FromTo, AllowChange); // {\"\":[\"0\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0\"]}\"

                    count += 11;
                }

                if (   std::is_same<S, Core::JSON::HexUInt8>::value
                    || std::is_same<S, Core::JSON::HexSInt8>::value
                    || std::is_same<S, Core::JSON::HexUInt16>::value
                    || std::is_same<S, Core::JSON::HexSInt16>::value
                    || std::is_same<S, Core::JSON::HexUInt32>::value
                    || std::is_same<S, Core::JSON::HexSInt32>::value
                    || std::is_same<S, Core::JSON::HexUInt64>::value
                    || std::is_same<S, Core::JSON::HexSInt64>::value
                    || std::is_same<S, Core::JSON::InstanceId>::value
                    || std::is_same<S, Core::JSON::Pointer>::value
                ) {
                    count += TestJSONFormat<T>("{\"\":0x0}", FromTo, AllowChange); // {\"\":0x0}
                    count += TestJSONFormat<W>("{\"\":{\"\":0x0}}", FromTo, AllowChange); // {\"\":{\"\":0x0}}

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[0x0]}", FromTo, AllowChange); // {\"\":[0x0]}
                    count += TestJSONFormat<T>("{\"\":[[0x0]]}", FromTo, AllowChange); // {\"\":[[0x0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0x0]]]}", FromTo, AllowChange); // {\"\":[[[0x0]]]}

                    count += TestJSONFormat<T>("{\"\":[0x0,0x0]}", FromTo, AllowChange); // {\"\":[0x0,0x0]}
                    count += TestJSONFormat<T>("{\"\":[[0x0,0x0]]}", FromTo, AllowChange); // {\"\":[[0x0,0x0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0x0,0x0]]]}", FromTo, AllowChange); // {\"\":[[[0x0,0x0]]}

                    count += TestJSONFormat<T>("{\"\":[\"0x0\"]}", FromTo, AllowChange); // {\"\":[\"0X0\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"0x0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0X0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0x0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0X0\"]}\"

                    count += 11;
                }

                if (   std::is_same<S, Core::JSON::OctUInt8>::value
                    || std::is_same<S, Core::JSON::OctSInt8>::value
                    || std::is_same<S, Core::JSON::OctUInt16>::value
                    || std::is_same<S, Core::JSON::OctSInt16>::value
                    || std::is_same<S, Core::JSON::OctUInt32>::value
                    || std::is_same<S, Core::JSON::OctSInt32>::value
                    || std::is_same<S, Core::JSON::OctUInt64>::value
                    || std::is_same<S, Core::JSON::OctSInt64>::value
                ) {
                    count += TestJSONFormat<T>("{\"\":00}", FromTo, AllowChange); // {\"\":00}

                    count += TestJSONFormat<W>("{\"\":{\"\":00}}", FromTo, AllowChange); // {\"\":{\"\":00}}

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[00]}", FromTo, AllowChange); // {\"\":[00]}
                    count += TestJSONFormat<T>("{\"\":[[00]]}", FromTo, AllowChange); // {\"\":[[00]]}
                    count += TestJSONFormat<T>("{\"\":[[[00]]]}", FromTo, AllowChange); // {\"\":[[[00]]]}

                    count += TestJSONFormat<T>("{\"\":[00,00]}", FromTo, AllowChange); // {\"\":[00,00]}
                    count += TestJSONFormat<T>("{\"\":[[00,00]]}", FromTo, AllowChange); // {\"\":[[00,00]]}
                    count += TestJSONFormat<T>("{\"\":[[[00,00]]]}", FromTo, AllowChange); // {\"\":[[[00,00]]}

                    count += TestJSONFormat<T>("{\"\":[\"00\"]}", FromTo, AllowChange); // {\"\":[\"00\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"00\"]\"}", FromTo, AllowChange); // {\"\":\"[\"00\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"00\"]}\"", FromTo, AllowChange); // \"{\"\":[\"00\"]}\"

                    count += 11;
                }

                if (   std::is_same<S, Core::JSON::Float>::value
                    || std::is_same<S, Core::JSON::Double>::value
                ) {
                    // All re-created character sequences for arrays are not compared on float value basis but on character per character basis.
                    // This may fail.
                    count += TestJSONFormat<T>("{\"\":0.0}", FromTo, !AllowChange); // {\"\":0.0}

                    count += TestJSONFormat<W>("{\"\":{\"\":0.0}}", FromTo, !AllowChange); // {\"\":{\"\":0.0}}

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[0.0]}", FromTo, AllowChange); // {\"\":[0.0]}
                    count += TestJSONFormat<T>("{\"\":[[0.0]]}", FromTo, AllowChange); // {\"\":[[0.0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0.0]]]}", FromTo, AllowChange); // {\"\":[[[0.0]]]}

                    count += TestJSONFormat<T>("{\"\":[0.0,0.0]}", FromTo, AllowChange); // {\"\":[0.0,0.0]}
                    count += TestJSONFormat<T>("{\"\":[[0.0,0.0]]}", FromTo, AllowChange); // {\"\":[[0.0,0.0]]}
                    count += TestJSONFormat<T>("{\"\":[[[0.0,0.0]]]}", FromTo, AllowChange); // {\"\":[[[0.0,0.0]]}

                    // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                    count += TestJSONFormat<T>("{\"\":[0.0,0.0],\"\":[0.0,0.0]}", FromTo, !AllowChange); // {\"\":[0.0,0.0]}

                    count += TestJSONFormat<T>("{\"\":[\"0.0\"]}", FromTo, AllowChange); // {\"\":[\"0.0\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"0.0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0.0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0.0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0.0\"]}\"

                    count += 10;
                }

                if (std::is_same<S, Core::JSON::String>::value) {
                    count += TestJSONFormat<T>("{\"\":\"\"}", FromTo, AllowChange); // {\"\":\"\"}

                    count += TestJSONFormat<W>("{\"\":{\"\":\"\"}}", FromTo, AllowChange); // {\"\":{\"\":\"\"}}

                    // Nullifying container ?
                    count += TestJSONFormat<T>("{\"\":\"null\"}", FromTo, AllowChange); // null

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[\"\"]}", FromTo, AllowChange); // {\"\":[\"\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"\"]\"}", FromTo, AllowChange); // {\"\":\"[\"\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"\"]}\"", FromTo, AllowChange); // \"{\"\":[\"\"]}\"

                    count += 16;
                }

                if (std::is_same<S, Core::JSON::Boolean>::value) {
                    count += TestJSONFormat<T>("{\"\":true}", FromTo, AllowChange); // {\"\":true}

                    count += TestJSONFormat<T>("{\"\":false}", FromTo, AllowChange); // {\"\":false}

                    count += TestJSONFormat<W>("{\"\":{\"\":true}}", FromTo, AllowChange); // {\"\":{\"\":true}}
                    count += TestJSONFormat<W>("{\"\":{\"\":false}}", FromTo, AllowChange); // {\"\":{\"\":false}}

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[true]}", FromTo, AllowChange); // {\"\":[true]}
                    count += TestJSONFormat<T>("{\"\":[[true]]}", FromTo, AllowChange); // {\"\":[[true]]}
                    count += TestJSONFormat<T>("{\"\":[[[true]]]}", FromTo, AllowChange); // {\"\":[[[true]]]}

                    count += TestJSONFormat<T>("{\"\":[true,true]}", FromTo, AllowChange); // {\"\":[true,true]}
                    count += TestJSONFormat<T>("{\"\":[[true,true]]}", FromTo, AllowChange); // {\"\":[[true,true]]}
                    count += TestJSONFormat<T>("{\"\":[[[true,true]]]}", FromTo, AllowChange); // {\"\":[[[true,true]]]}

                    count += TestJSONFormat<T>("{\"\":[\"true\"]}", FromTo, AllowChange); // {\"\":[\"true\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"true\"]\"}", FromTo, AllowChange); // {\"\":\"[\"true\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"true\"]}\"", FromTo, AllowChange); // \"{\"\":[true]}\"

                    count += TestJSONFormat<T>("{\"\":[false]}", FromTo, AllowChange); // {\"\":[false]}
                    count += TestJSONFormat<T>("{\"\":[[false]]}", FromTo, AllowChange); // {\"\":[[false]]}
                    count += TestJSONFormat<T>("{\"\":[[[false]]]}", FromTo, AllowChange); // \"{\"\":[[[false]]]}

                    count += TestJSONFormat<T>("{\"\":[false,false]}", FromTo, AllowChange); // {\"\":[false,false]}
                    count += TestJSONFormat<T>("{\"\":[[false,false]]}", FromTo, AllowChange); // {\"\":[[false,false]]}
                    count += TestJSONFormat<T>("{\"\":[[[false,false]]]}", FromTo, AllowChange); // {\"\":[[[false,false]]]}

                    count += TestJSONFormat<T>("{\"\":[\"false\"]}", FromTo, AllowChange); // {\"\":[\"false\"]}
                    count += TestJSONFormat<T>("{\"\":\"[\"false\"]\"}", FromTo, AllowChange); // {\"\":\"[\"false\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"false\"]}\"", FromTo, AllowChange); // \"{\"\":[\"false\"]}\"}
                }
            }  else {
                // Malformed
                // =========

                // Take {\"\":} as JSON value
                count += !TestJSONFormat<T>("{\"\":,}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{\"\":},", FromTo, AllowChange);
                count += !TestJSONFormat<T>(",{\"\":}", FromTo, AllowChange);
                count += !TestJSONFormat<W>("{\"\":,\"\":{\"\":\"\"}}", FromTo, AllowChange);
                count += !TestJSONFormat<W>("{\"\":{\"\":\"\"},}", FromTo, AllowChange);
                count += !TestJSONFormat<W>("{\"\":{\"\":\"\"}{\"\":\"\"}}", FromTo, AllowChange);
                count += !TestJSONFormat<W>("{\"\":{\"\":\"\"}.\"\":{\"\":\"\"}}", FromTo, AllowChange); // Any character not being ,

                count += !TestJSONFormat<T>("{\":\"\"}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{\"\":\"}", FromTo, AllowChange);

                count += !TestJSONFormat<T>("{\"\":}}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{\"\":{}", FromTo, AllowChange);

                count += !TestJSONFormat<T>("{\"\":[}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{\"\":]}", FromTo, AllowChange);

                // Mismatch opening and closing
                count += !TestJSONFormat<T>("[", FromTo, AllowChange);
                count += !TestJSONFormat<T>("]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("(]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[)", FromTo, AllowChange);
                count += !TestJSONFormat<T>("{]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("[}", FromTo, AllowChange);
                count += !TestJSONFormat<T>("()", FromTo, AllowChange);

                // Values that cannot be used
                count += !TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += !TestJSONFormat<T>("true", FromTo, AllowChange);
                count += !TestJSONFormat<T>("false", FromTo, AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 132
                           : count == 46
               ;
    }

    template <typename T, typename S>
    bool TestDecUIntFromValue()
    {
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value, "Type S should be an unsigned integral");

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
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value, "Type S should be a signed integral");

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
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value, "Type S should be an unsigned integral");

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
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value, "Type S should be a signed integral");

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
        static_assert(std::is_integral<S>::value && std::is_unsigned<S>::value, "Type S should be an unsigned integral");

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
        static_assert(std::is_integral<S>::value && std::is_signed<S>::value, "Type S should be a signed integral");

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

    template <typename T, typename S>
    bool TestFPFromValue()
    {
        static_assert(std::is_floating_point<S>::value, "Type S should be floating point");

        uint8_t count = 0;

        if (std::is_same<decltype(std::declval<T>().Value()), S>::value) {
            switch (sizeof(S)) {
            case sizeof(float)  : // float
                                  count += TestJSONEqual<T, float>(-FLT_MIN, "-1.175494351e-38");
                                  count += TestJSONEqual<T, float>(FLT_MIN, "1.175494351e-38");
                                  count += TestJSONEqual<T, float>(-FLT_MAX, "-3.402823466e+38");
                                  count += TestJSONEqual<T, float>(FLT_MAX, "3.402823466e+38");
                                  break;
            case sizeof(double) : // double
                                  count += TestJSONEqual<T, double>(-DBL_MIN, "-2.2250738585072014e-308");
                                  count += TestJSONEqual<T, double>(DBL_MIN, "2.2250738585072014e-308");
                                  count += TestJSONEqual<T, double>(-DBL_MAX, "-1.7976931348623158e+308");
                                  count += TestJSONEqual<T, double>(DBL_MAX, "1.7976931348623158e+308");
                                  break;
            default             : ASSERT(false);
            }
        }

        return count == 4;
    }

    template <typename T, typename S>
    bool TestBoolFromValue()
    {
        static_assert(   std::is_integral<S>::value
                      && std::is_unsigned<S>::value
                      && std::is_same<decltype(std::declval<T>().Value()), S>::value
                      , "Type mismatch and/or type S should be unsigned integral (boolean)"
                     );

        uint8_t count = 0;

        count += TestJSONEqual<T, S>(true, "true");
        count += TestJSONEqual<T, S>(false, "false");

        return count == 2;
    }

    template <typename T, typename S>
    bool TestStringFromValue()
    {
        static_assert(std::is_same<S, std::string>::value, "Type mismatch");

        uint8_t count = 0;

        T object;

        count += object.FromString("abc123ABC");
        count += S(object.Value().c_str()) == S("abc123ABC");
        count += object.FromString("\u0009\u000A\u000D\u0020\"abc123ABC\"\u0009\u000A\u000D\u0020");
        count += S(object.Value().c_str()) == S("\"abc123ABC\"");

        return count == 4;
    }

    template <typename T, typename S>
    bool TestContainerFromValue()
    {
        static_assert(std::is_same<T, Core::JSON::Container>::value, "Type mismatch");

        auto TestEquality = [](const std::string& in, const std::string& out) -> bool
        {
            T object;

            Core::OptionalType<Core::JSON::Error> status;

            std::string stream;

            bool result = object.FromString(in)
                          && object.ToString(stream)
                          && !(status.IsSet())
                          ;

            if (   std::is_same<S, Core::JSON::Float>::value
                || std::is_same<S, Core::JSON::Double>::value
               ) {
            } else {
                result = result && stream == out;
            }

            return result;
        };

        uint8_t count = 0;

        count += TestEquality("", "{}");
        count += TestEquality("\u0009\u000A\u000D\u0020{}", "{}");
        count += TestEquality("{}\u0009\u000A\u000D\u0020", "{}");
        count += TestEquality("\u0009\u000A\u000D\u0020{}\u0009\u000A\u000D\u0020", "{}");

        count += TestEquality("{\"\":{},\"\":{}}", "{\"\":{}}");
        count += TestEquality("{\"\":{},\u0009\u000A\u000D\u0020\"\":{}}", "{\"\":{}}");
        count += TestEquality("{\"\":{}\u0009\u000A\u000D\u0020,\"\":{}}", "{\"\":{}}");
        count += TestEquality("{\"\":{}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{}}", "{\"\":{}}");

        count += TestEquality("{\"A\":{},\u0009\u000A\u000D\u0020\"B\":{}}", "{\"A\":{},\"B\":{}}");
        count += TestEquality("{\"A\":{}\u0009\u000A\u000D\u0020,\"B\":{}}", "{\"A\":{},\"B\":{}}");
        count += TestEquality("{\"A\":{}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{}}", "{\"A\":{},\"B\":{}}");

        count += TestEquality("{\"\":{\"\":null},\"\":{\"\":null}}", "{\"\":{\"\":null}}");
        count += TestEquality("{\"\":{\"\":null},\u0009\u000A\u000D\u0020\"\":{\"\":null}}", "{\"\":{\"\":null}}");
        count += TestEquality("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\"\":{\"\":null}}", "{\"\":{\"\":null}}");
        count += TestEquality("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":null}}", "{\"\":{\"\":null}}");

        count += TestEquality("{\"A\":{\"\":null},\"B\":{\"\":null}}", "{\"A\":{\"\":null},\"B\":{\"\":null}}");

        count += TestEquality("{\"A\":{\"\":null},\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", "{\"A\":{\"\":null},\"B\":{\"\":null}}");

        count += TestEquality("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\"B\":{\"\":null}}", "{\"A\":{\"\":null},\"B\":{\"\":null}}");

        count += TestEquality("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", "{\"A\":{\"\":null},\"B\":{\"\":null}}");

        // An empty array element is defined as String since its type is unknown and, thus, a type cannot be deduced from a character sequence.

        count += TestEquality("{\"\":[],\"\":[]}", "{\"\":[\"\"]}");
        count += TestEquality("{\"\":[],\u0009\u000A\u000D\u0020\"\":[]}", "{\"\":[\"\"]}");
        count += TestEquality("{\"\":[]\u0009\u000A\u000D\u0020,\"\":[]}", "{\"\":[\"\"]}");
        count += TestEquality("{\"\":[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":[]}", "{\"\":[\"\"]}");

        count += TestEquality("{\"A\":[],\"B\":[]}", "{\"A\":[\"\"],\"B\":[\"\"]}");
        count += TestEquality("{\"A\":[],\u0009\u000A\u000D\u0020\"B\":[]}", "{\"A\":[\"\"],\"B\":[\"\"]}");
        count += TestEquality("{\"A\":[]\u0009\u000A\u000D\u0020,\"B\":[]}", "{\"A\":[\"\"],\"B\":[\"\"]}");
        count += TestEquality("{\"A\":[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":[]}", "{\"A\":[\"\"],\"B\":[\"\"]}");

        if (   std::is_same<S, Core::JSON::DecUInt8>::value
            || std::is_same<S, Core::JSON::DecSInt8>::value
            || std::is_same<S, Core::JSON::DecUInt16>::value
            || std::is_same<S, Core::JSON::DecSInt16>::value
            || std::is_same<S, Core::JSON::DecUInt32>::value
            || std::is_same<S, Core::JSON::DecSInt32>::value
            || std::is_same<S, Core::JSON::DecUInt64>::value
            || std::is_same<S, Core::JSON::DecSInt64>::value
        ) {
            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":0,\"\":0}", "{\"\":0}");
            count += TestEquality("{\"\":0,\u0009\u000A\u000D\u0020\"\":0}", "{\"\":0}");
            count += TestEquality("{\"\":0\u0009\u000A\u000D\u0020,\"\":0}", "{\"\":0}");
            count += TestEquality("{\"\":0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":0}", "{\"\":0}");

            count += TestEquality("{\"A\":0,\"B\":0}", "{\"A\":0,\"B\":0}");
            count += TestEquality("{\"A\":0,\u0009\u000A\u000D\u0020\"B\":0}", "{\"A\":0,\"B\":0}");
            count += TestEquality("{\"A\":0\u0009\u000A\u000D\u0020,\"B\":0}", "{\"A\":0,\"B\":0}");
            count += TestEquality("{\"A\":0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":0}", "{\"A\":0,\"B\":0}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":0},\"\":{\"\":0}}", "{\"\":{\"\":0}}");
            count += TestEquality("{\"\":{\"\":0},\u0009\u000A\u000D\u0020\"\":{\"\":0}}", "{\"\":{\"\":0}}");
            count += TestEquality("{\"\":{\"\":0}\u0009\u000A\u000D\u0020,\"\":{\"\":0}}", "{\"\":{\"\":0}}");
            count += TestEquality("{\"\":{\"\":0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":0}}", "{\"\":{\"\":0}}");

            count += TestEquality("{\"A\":{\"\":0},\"B\":{\"\":0}}", "{\"A\":{\"\":0},\"B\":{\"\":0}}");
            count += TestEquality("{\"A\":{\"\":0},\u0009\u000A\u000D\u0020\"B\":{\"\":0}}", "{\"A\":{\"\":0},\"B\":{\"\":0}}");
            count += TestEquality("{\"A\":{\"\":0}\u0009\u000A\u000D\u0020,\"B\":{\"\":0}}", "{\"A\":{\"\":0},\"B\":{\"\":0}}");
            count += TestEquality("{\"A\":{\"\":0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":0}}", "{\"A\":{\"\":0},\"B\":{\"\":0}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[0,0],\"\":[0,0]}","{\"\":[0,0]}");

            count += TestEquality("{\"A\":[0,0],\"B\":[0,0]}","{\"A\":[0,0],\"B\":[0,0]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"0\"],\"\":[\"0\"]}", "{\"\":[\"0\"]}");
            count += TestEquality("{\"\":\"[\"0\"]\",\"\":\"[\"0\"]\"}", "{\"\":\"[\"0\"]\"}");
            count += TestEquality("\"{\"\":[\"0\"],\"\":[\"0\"]}\"", "\"{\"\":[\"0\"]}\"");

            count += TestEquality("{\"A\":[\"0\"],\"B\":[\"0\"]}", "{\"A\":[\"0\"],\"B\":[\"0\"]}");
            count += TestEquality("{\"A\":\"[\"0\"]\",\"B\":\"[\"0\"]\"}", "{\"A\":\"[\"0\"]\",\"B\":\"[\"0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0\"],\"B\":[\"0\"]}\"", "\"{\"A\":[\"0\"],\"B\":[\"0\"]}\"");

            count += 32;
        }

        if (   std::is_same<S, Core::JSON::HexUInt8>::value
            || std::is_same<S, Core::JSON::HexSInt8>::value
            || std::is_same<S, Core::JSON::HexUInt16>::value
            || std::is_same<S, Core::JSON::HexSInt16>::value
            || std::is_same<S, Core::JSON::HexUInt32>::value
            || std::is_same<S, Core::JSON::HexSInt32>::value
            || std::is_same<S, Core::JSON::HexUInt64>::value
            || std::is_same<S, Core::JSON::HexSInt64>::value
            || std::is_same<S, Core::JSON::InstanceId>::value
            || std::is_same<S, Core::JSON::Pointer>::value
        ) {
            // X in hexadecimal is 'always' uppercase in ToString. Important for 'exact' string comparison

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":0x0,\"\":0x0}", "{\"\":0X0}");
            count += TestEquality("{\"\":0x0,\u0009\u000A\u000D\u0020\"\":0x0}", "{\"\":0X0}");
            count += TestEquality("{\"\":0x0\u0009\u000A\u000D\u0020,\"\":0x0}", "{\"\":0X0}");
            count += TestEquality("{\"\":0x0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":0x0}", "{\"\":0X0}");

            count += TestEquality("{\"A\":0x0,\"B\":0x0}", "{\"A\":0X0,\"B\":0X0}");
            count += TestEquality("{\"A\":0x0,\u0009\u000A\u000D\u0020\"B\":0x0}", "{\"A\":0X0,\"B\":0X0}");
            count += TestEquality("{\"A\":0x0\u0009\u000A\u000D\u0020,\"B\":0x0}", "{\"A\":0X0,\"B\":0X0}");
            count += TestEquality("{\"A\":0x0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":0x0}", "{\"A\":0X0,\"B\":0X0}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":0x0},\"\":{\"\":0x0}}", "{\"\":{\"\":0X0}}");
            count += TestEquality("{\"\":{\"\":0x0},\u0009\u000A\u000D\u0020\"\":{\"\":0x0}}", "{\"\":{\"\":0X0}}");
            count += TestEquality("{\"\":{\"\":0x0}\u0009\u000A\u000D\u0020,\"\":{\"\":0x0}}", "{\"\":{\"\":0X0}}");
            count += TestEquality("{\"\":{\"\":0x0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":0x0}}", "{\"\":{\"\":0X0}}");

            count += TestEquality("{\"A\":{\"\":0x0},\"B\":{\"\":0x0}}", "{\"A\":{\"\":0X0},\"B\":{\"\":0X0}}");
            count += TestEquality("{\"A\":{\"\":0x0},\u0009\u000A\u000D\u0020\"B\":{\"\":0x0}}", "{\"A\":{\"\":0X0},\"B\":{\"\":0X0}}");
            count += TestEquality("{\"A\":{\"\":0x0}\u0009\u000A\u000D\u0020,\"B\":{\"\":0x0}}", "{\"A\":{\"\":0X0},\"B\":{\"\":0X0}}");
            count += TestEquality("{\"A\":{\"\":0x0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":0x0}}", "{\"A\":{\"\":0X0},\"B\":{\"\":0X0}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[0x0,0x0],\"\":[0x0,0x0]}","{\"\":[0x0,0x0]}"); // Detected as ArrayType<String>

            count += TestEquality("{\"A\":[0x0,0x0],\"B\":[0x0,0x0]}","{\"A\":[0x0,0x0],\"B\":[0x0,0x0]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"0x0\"],\"\":[\"0x0\"]}", "{\"\":[\"0x0\"]}");
            count += TestEquality("{\"\":\"[\"0x0\"]\",\"\":\"[\"0x0\"]\"}", "{\"\":\"[\"0x0\"]\"}");
            count += TestEquality("\"{\"\":[\"0x0\"],\"\":[\"0x0\"]}\"", "\"{\"\":[\"0x0\"]}\"");

            count += TestEquality("{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}", "{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}");
            count += TestEquality("{\"A\":\"[\"0x0\"]\",\"B\":\"[\"0x0\"]\"}", "{\"A\":\"[\"0x0\"]\",\"B\":\"[\"0x0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}\"", "\"{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}\"");

            count += 32;
        }

        if (   std::is_same<S, Core::JSON::OctUInt8>::value
            || std::is_same<S, Core::JSON::OctSInt8>::value
            || std::is_same<S, Core::JSON::OctUInt16>::value
            || std::is_same<S, Core::JSON::OctSInt16>::value
            || std::is_same<S, Core::JSON::OctUInt32>::value
            || std::is_same<S, Core::JSON::OctSInt32>::value
            || std::is_same<S, Core::JSON::OctUInt64>::value
            || std::is_same<S, Core::JSON::OctSInt64>::value
        ) {
            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":00,\"\":00}", "{\"\":00}");
            count += TestEquality("{\"\":00,\u0009\u000A\u000D\u0020\"\":00}", "{\"\":00}");
            count += TestEquality("{\"\":00\u0009\u000A\u000D\u0020,\"\":00}", "{\"\":00}");
            count += TestEquality("{\"\":00\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":00}", "{\"\":00}");

            count += TestEquality("{\"A\":00,\"B\":00}", "{\"A\":00,\"B\":00}");
            count += TestEquality("{\"A\":00,\u0009\u000A\u000D\u0020\"B\":00}", "{\"A\":00,\"B\":00}");
            count += TestEquality("{\"A\":00\u0009\u000A\u000D\u0020,\"B\":00}", "{\"A\":00,\"B\":00}");
            count += TestEquality("{\"A\":00\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":00}", "{\"A\":00,\"B\":00}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":00},\"\":{\"\":00}}", "{\"\":{\"\":00}}");
            count += TestEquality("{\"\":{\"\":00},\u0009\u000A\u000D\u0020\"\":{\"\":00}}", "{\"\":{\"\":00}}");
            count += TestEquality("{\"\":{\"\":00}\u0009\u000A\u000D\u0020,\"\":{\"\":00}}", "{\"\":{\"\":00}}");
            count += TestEquality("{\"\":{\"\":00}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":00}}", "{\"\":{\"\":00}}");

            count += TestEquality("{\"A\":{\"\":00},\"B\":{\"\":00}}", "{\"A\":{\"\":00},\"B\":{\"\":00}}");
            count += TestEquality("{\"A\":{\"\":00},\u0009\u000A\u000D\u0020\"B\":{\"\":00}}", "{\"A\":{\"\":00},\"B\":{\"\":00}}");
            count += TestEquality("{\"A\":{\"\":00}\u0009\u000A\u000D\u0020,\"B\":{\"\":00}}", "{\"A\":{\"\":00},\"B\":{\"\":00}}");
            count += TestEquality("{\"A\":{\"\":00}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":00}}", "{\"A\":{\"\":00},\"B\":{\"\":00}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[00,00],\"\":[00,00]}","{\"\":[00,00]}");

            count += TestEquality("{\"A\":[00,00],\"B\":[00,00]}","{\"A\":[00,00],\"B\":[00,00]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"00\"],\"\":[\"00\"]}", "{\"\":[\"00\"]}");
            count += TestEquality("{\"\":\"[\"00\"]\",\"\":\"[\"00\"]\"}", "{\"\":\"[\"00\"]\"}");
            count += TestEquality("\"{\"\":[\"00\"],\"\":[\"00\"]}\"", "\"{\"\":[\"00\"]}\"");

            count += TestEquality("{\"A\":[\"00\"],\"B\":[\"00\"]}", "{\"A\":[\"00\"],\"B\":[\"00\"]}");
            count += TestEquality("{\"A\":\"[\"00\"]\",\"B\":\"[\"00\"]\"}", "{\"A\":\"[\"00\"]\",\"B\":\"[\"00\"]\"}");
            count += TestEquality("\"{\"A\":[\"00\"],\"B\":[\"00\"]}\"", "\"{\"A\":[\"00\"],\"B\":[\"00\"]}\"");

            count += 32;
        }

        if (   std::is_same<S, Core::JSON::Float>::value
            || std::is_same<S, Core::JSON::Double>::value
           ) {
            // Floating point results are NOT compared on character sequence equality

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":0.0,\"\":0.0}", "{\"\":0.0}");
            count += TestEquality("{\"\":0.0,\u0009\u000A\u000D\u0020\"\":0.0}", "{\"\":0.0}");
            count += TestEquality("{\"\":0.0\u0009\u000A\u000D\u0020,\"\":0.0}", "{\"\":0.0}");
            count += TestEquality("{\"\":0.0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":0.0}", "{\"\":0.0}");

            count += TestEquality("{\"A\":0.0,\"B\":0.0}", "{\"A\":0.0,\"B\":0.0}");
            count += TestEquality("{\"A\":0.0,\u0009\u000A\u000D\u0020\"B\":0.0}", "{\"A\":0.0,\"B\":0.0}");
            count += TestEquality("{\"A\":0.0\u0009\u000A\u000D\u0020,\"B\":0.0}", "{\"A\":0.0,\"B\":0.0}");
            count += TestEquality("{\"A\":0.0\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":0.0}", "{\"A\":0.0,\"B\":0.0}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":0.0},\"\":{\"\":0.0}}", "{\"\":{\"\":0.0}}");
            count += TestEquality("{\"\":{\"\":0.0},\u0009\u000A\u000D\u0020\"\":{\"\":0.0}}", "{\"\":{\"\":0.0}}");
            count += TestEquality("{\"\":{\"\":0.0}\u0009\u000A\u000D\u0020,\"\":{\"\":0.0}}", "{\"\":{\"\":0.0}}");
            count += TestEquality("{\"\":{\"\":0.0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":0.0}}", "{\"\":{\"\":0.0}}");

            count += TestEquality("{\"A\":{\"\":0.0},\"B\":{\"\":0.0}}", "{\"A\":{\"\":0.0},\"B\":{\"\":0.0}}");
            count += TestEquality("{\"A\":{\"\":0.0},\u0009\u000A\u000D\u0020\"B\":{\"\":0.0}}", "{\"A\":{\"\":0.0},\"B\":{\"\":0.0}}");
            count += TestEquality("{\"A\":{\"\":0.0}\u0009\u000A\u000D\u0020,\"B\":{\"\":0.0}}", "{\"A\":{\"\":0.0},\"B\":{\"\":0.0}}");
            count += TestEquality("{\"A\":{\"\":0.0}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":0.0}}", "{\"A\":{\"\":0.0},\"B\":{\"\":0.0}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[0.0,0.0],\"\":[0.0,0.0]}","\"\":[0.0,0.0]}");

            count += TestEquality("{\"A\":[0.0,0.0],\"B\":[0.0,0.0]}","\"A\":[0.0,0.0],\"B\":[0.0,0.0]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"0.0\"],\"\":[\"0.0\"]}", "{\"\":[\"0.0\"]}");
            count += TestEquality("{\"\":\"[\"0.0\"]\",\"\":\"[\"0.0\"]\"}", "{\"\":\"[\"0.0\"]\"}");
            count += TestEquality("\"{\"\":[\"0.0\"],\"\":[\"0.0\"]}\"", "\"{\"\":[\"0.0\"]}\"");

            count += TestEquality("{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}", "{\"A\":[\"00\"],\"B\":[\"0.0\"]}");
            count += TestEquality("{\"A\":\"[\"0.0\"]\",\"B\":\"[\"0.0\"]\"}", "{\"A\":\"[\"0.0\"]\",\"B\":\"[\"0.0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}\"", "\"{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}\"");

            count += 32;
        }


        if (std::is_same<S, Core::JSON::String>::value) {
            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":\"\",\"\":\"\"}", "{\"\":\"\"}");
            count += TestEquality("{\"\":\"\",\u0009\u000A\u000D\u0020\"\":\"\"}", "{\"\":\"\"}");
            count += TestEquality("{\"\":\"\"\u0009\u000A\u000D\u0020,\"\":\"\"}", "{\"\":\"\"}");
            count += TestEquality("{\"\":\"\"\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":\"\"}", "{\"\":\"\"}");

            count += TestEquality("{\"A\":\"\",\"B\":\"\"}", "{\"A\":\"\",\"B\":\"\"}");
            count += TestEquality("{\"A\":\"\",\u0009\u000A\u000D\u0020\"B\":\"\"}", "{\"A\":\"\",\"B\":\"\"}");
            count += TestEquality("{\"A\":\"\"\u0009\u000A\u000D\u0020,\"B\":\"\"}", "{\"A\":\"\",\"B\":\"\"}");
            count += TestEquality("{\"A\":\"\"\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":\"\"}", "{\"A\":\"\",\"B\":\"\"}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":\"\"},\"\":{\"\":\"\"}}", "{\"\":{\"\":\"\"}}");
            count += TestEquality("{\"\":{\"\":\"\"},\u0009\u000A\u000D\u0020\"\":{\"\":\"\"}}", "{\"\":{\"\":\"\"}}");
            count += TestEquality("{\"\":{\"\":\"\"}\u0009\u000A\u000D\u0020,\"\":{\"\":\"\"}}", "{\"\":{\"\":\"\"}}");
            count += TestEquality("{\"\":{\"\":\"\"}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":\"\"}}", "{\"\":{\"\":\"\"}}");

            count += TestEquality("{\"A\":{\"\":\"\"},\"B\":{\"\":\"\"}}", "{\"A\":{\"\":\"\"},\"B\":{\"\":\"\"}}");
            count += TestEquality("{\"A\":{\"\":\"\"},\u0009\u000A\u000D\u0020\"B\":{\"\":\"\"}}", "{\"A\":{\"\":\"\"},\"B\":{\"\":\"\"}}");
            count += TestEquality("{\"A\":{\"\":\"\"}\u0009\u000A\u000D\u0020,\"B\":{\"\":\"\"}}", "{\"A\":{\"\":\"\"},\"B\":{\"\":\"\"}}");
            count += TestEquality("{\"A\":{\"\":\"\"}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":\"\"}}", "{\"A\":{\"\":\"\"},\"B\":{\"\":\"\"}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"\"],\"\":[\"\"]}", "{\"\":[\"\"]}");
            count += TestEquality("{\"\":\"[\"\"]\",\"\":\"[\"\"]\"}", "{\"\":\"[\"\"]\"}");
            count += TestEquality("\"{\"\":[\"\"],\"\":[\"\"]}\"", "\"{\"\":[\"\"]}\"");

            count += TestEquality("{\"A\":[\"\"],\"B\":[\"\"]}", "{\"A\":[\"\"],\"B\":[\"\"]}");
            count += TestEquality("{\"A\":\"[\"\"]\",\"B\":\"[\"\"]\"}","{\"A\":\"[\"\"]\",\"B\":\"[\"\"]\"}");
            count += TestEquality("\"{\"A\":[\"\"],\"B\":[\"\"]}\"", "\"{\"A\":[\"\"],\"B\":[\"\"]}\"");

            count += 34;
        }

        if (std::is_same<S, Core::JSON::Boolean>::value) {
            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":true,\"\":true}", "{\"\":true}");
            count += TestEquality("{\"\":true,\u0009\u000A\u000D\u0020\"\":true}", "{\"\":true}");
            count += TestEquality("{\"\":true\u0009\u000A\u000D\u0020,\"\":true}", "{\"\":true}");
            count += TestEquality("{\"\":true\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":true}", "{\"\":true}");

            count += TestEquality("{\"\":false}", "{\"\":false}");
            count += TestEquality("{\"\":false,\u0009\u000A\u000D\u0020\"\":false}", "{\"\":false}");
            count += TestEquality("{\"\":false\u0009\u000A\u000D\u0020,\"\":false}", "{\"\":false}");
            count += TestEquality("{\"\":false\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":false}", "{\"\":false}");

            count += TestEquality("{\"A\":true,\"B\":true}", "{\"A\":true,\"B\":true}");
            count += TestEquality("{\"A\":true,\u0009\u000A\u000D\u0020\"B\":true}", "{\"A\":true,\"B\":true}");
            count += TestEquality("{\"A\":true\u0009\u000A\u000D\u0020,\"B\":true}", "{\"A\":true,\"B\":true}");
            count += TestEquality("{\"A\":true\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":true}", "{\"A\":true,\"B\":true}");

            count += TestEquality("{\"A\":false,\"B\":false}", "{\"A\":false,\"B\":false}");
            count += TestEquality("{\"A\":false,\u0009\u000A\u000D\u0020\"B\":false}", "{\"A\":false,\"B\":false}");
            count += TestEquality("{\"A\":false\u0009\u000A\u000D\u0020,\"B\":false}", "{\"A\":false,\"B\":false}");
            count += TestEquality("{\"A\":false\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":false}", "{\"A\":false,\"B\":false}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":true,\"\":false}", "{\"\":false}");
            count += TestEquality("{\"\":false,\"\":true}", "{\"\":true}");

            count += TestEquality("{\"A\":true,\"B\":false}", "{\"A\":true,\"B\":false}");
            count += TestEquality("{\"A\":false,\"B\":true}", "{\"A\":false,\"B\":true}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":true},\"\":{\"\":true}}", "{\"\":{\"\":true}}");
            count += TestEquality("{\"\":{\"\":true},\u0009\u000A\u000D\u0020\"\":{\"\":true}}", "{\"\":{\"\":true}}");
            count += TestEquality("{\"\":{\"\":true}\u0009\u000A\u000D\u0020,\"\":{\"\":true}}", "{\"\":{\"\":true}}");
            count += TestEquality("{\"\":{\"\":true}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":true}}", "{\"\":{\"\":true}}");
            count += TestEquality("{\"\":{\"\":false},\"\":{\"\":false}}", "{\"\":{\"\":false}}");
            count += TestEquality("{\"\":{\"\":false},\u0009\u000A\u000D\u0020\"\":{\"\":false}}", "{\"\":{\"\":false}}");
            count += TestEquality("{\"\":{\"\":false}\u0009\u000A\u000D\u0020,\"\":{\"\":false}}", "{\"\":{\"\":false}}");
            count += TestEquality("{\"\":{\"\":false}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":false}}", "{\"\":{\"\":false}}");

            count += TestEquality("{\"A\":{\"\":true},\"B\":{\"\":true}}", "{\"A\":{\"\":true},\"B\":{\"\":true}}");
            count += TestEquality("{\"A\":{\"\":true},\u0009\u000A\u000D\u0020\"B\":{\"\":true}}", "{\"A\":{\"\":true},\"B\":{\"\":true}}");
            count += TestEquality("{\"A\":{\"\":true}\u0009\u000A\u000D\u0020,\"B\":{\"\":true}}", "{\"A\":{\"\":true},\"B\":{\"\":true}}");
            count += TestEquality("{\"A\":{\"\":true}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":true}}","{\"A\":{\"\":true},\"B\":{\"\":true}}");
            count += TestEquality("{\"A\":{\"\":false},\"B\":{\"\":false}}", "{\"A\":{\"\":false},\"B\":{\"\":false}}");
            count += TestEquality("{\"A\":{\"\":false},\u0009\u000A\u000D\u0020\"B\":{\"\":false}}", "{\"A\":{\"\":false},\"B\":{\"\":false}}");
            count += TestEquality("{\"A\":{\"\":false}\u0009\u000A\u000D\u0020,\"B\":{\"\":false}}", "{\"A\":{\"\":false},\"B\":{\"\":false}}");
            count += TestEquality("{\"A\":{\"\":false}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":false}}", "{\"A\":{\"\":false},\"B\":{\"\":false}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":{\"\":true},\"\":{\"\":false}}", "{\"\":{\"\":false}}");
            count += TestEquality("{\"\":{\"\":false},\"\":{\"\":true}}", "{\"\":{\"\":true}}");

            count += TestEquality("{\"A\":{\"\":true},\"B\":{\"\":false}}", "{\"A\":{\"\":true},\"B\":{\"\":false}}");
            count += TestEquality("{\"A\":{\"\":false},\"B\":{\"\":true}}", "{\"A\":{\"\":false},\"B\":{\"\":true}}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[true,true],\"\":[true,true]}", "{\"\":[true,true]}");

            count += TestEquality("{\"A\":[true,true],\"B\":[true,true]}", "{\"A\":[true,true],\"B\":[true,true]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"true\"],\"\":[\"true\"]}", "{\"\":[\"true\"]}");
            count += TestEquality("{\"\":\"[\"true\"]\",\"\":\"[\"true\"]\"}", "{\"\":\"[\"true\"]\"}");
            count += TestEquality("\"{\"\":[\"true\"],\"\":[\"true\"]}\"", "\"{\"\":[\"true\"]}\"");

            count += TestEquality("{\"A\":[\"true\"],\"B\":[\"true\"]}", "{\"A\":[\"true\"],\"B\":[\"true\"]}");
            count += TestEquality("{\"A\":\"[\"true\"]\",\"B\":\"[\"true\"]\"}", "{\"A\":\"[\"true\"]\",\"B\":\"[\"true\"]\"}");
            count += TestEquality("\"{\"A\":[\"true\"],\"B\":[\"true\"]}\"", "\"{\"A\":[\"true\"],\"B\":[\"true\"]}\"");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[false,false],\"\":[false,false]}", "{\"\":[false,false]}");

            count += TestEquality("{\"A\":[false,false],\"B\":[false,false]}", "{\"A\":[false,false],\"B\":[false,false]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"false\"],\"\":[\"false\"]}", "{\"\":[\"false\"]}");
            count += TestEquality("{\"\":\"[\"false\"]\",\"\":\"[\"false\"]\"}", "{\"\":\"[\"false\"]\"}");
            count += TestEquality("\"{\"\":[\"false\"],\"\":[\"false\"]}\"", "\"{\"\":[\"false\"]}\"");

            count += TestEquality("{\"A\":[\"false\"],\"B\":[\"false\"]}", "{\"A\":[\"false\"],\"B\":[\"false\"]}");
            count += TestEquality("{\"A\":\"[\"false\"]\",\"B\":\"[\"false\"]\"}", "{\"A\":\"[\"false\"]\",\"B\":\"[\"false\"]\"}");
            count += TestEquality("\"{\"A\":[\"false\"],\"B\":[\"false\"]}\"", "\"{\"A\":[\"false\"],\"B\":[\"false\"]}\"");
        }

        return count == 83;
    }

    TEST(JSONParser, DecUInt8)
    {
        using json_type = Core::JSON::DecUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 84);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecSInt8)
    {
        using json_type = Core::JSON::DecSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 28);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 82);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecUInt16)
    {
        using json_type = Core::JSON::DecUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 84);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecSInt16)
    {
        using json_type = Core::JSON::DecSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 28);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 82);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecUInt32)
    {
        using json_type = Core::JSON::DecUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 84);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecSInt32)
    {
        using json_type = Core::JSON::DecSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 28);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 82);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecUInt64)
    {
        using json_type = Core::JSON::DecUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestDecUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 84);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, DecSInt64)
    {
        using json_type = Core::JSON::DecSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestDecSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 28);

        EXPECT_TRUE(TestDecSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 82);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexUInt8)
    {
        using json_type = Core::JSON::HexUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexSInt8)
    {
        using json_type = Core::JSON::HexSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 98);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 32);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 32);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 32);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexUInt16)
    {
        using json_type = Core::JSON::HexUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexSInt16)
    {
        using json_type = Core::JSON::HexSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 98);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexUInt32)
    {
        using json_type = Core::JSON::HexUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexSInt32)
    {
        using json_type = Core::JSON::HexSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);
        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 98);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexUInt64)
    {
        using json_type = Core::JSON::HexUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, HexSInt64)
    {
        using json_type = Core::JSON::HexSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestHexSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestHexSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 98);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctUInt8)
    {
        using json_type = Core::JSON::OctUInt8;
        using actual_type = uint8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 52);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctSInt8)
    {
        using json_type = Core::JSON::OctSInt8;
        using actual_type = int8_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 44);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctUInt16)
    {
        using json_type = Core::JSON::OctUInt16;
        using actual_type = uint16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 52);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctSInt16)
    {
        using json_type = Core::JSON::OctSInt16;
        using actual_type = int16_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);
        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 44);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctUInt32)
    {
        using json_type = Core::JSON::OctUInt32;
        using actual_type = uint32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 52);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctSInt32)
    {
        using json_type = Core::JSON::OctSInt32;
        using actual_type = int32_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 44);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctUInt64)
    {
        using json_type = Core::JSON::OctUInt64;
        using actual_type = uint64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctUIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 24);

        EXPECT_TRUE(TestOctUIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 52);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, OctSInt64)
    {
        using json_type = Core::JSON::OctSInt64;
        using actual_type = int64_t;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestOctSIntFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestOctSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 26);

        EXPECT_TRUE(TestOctSIntFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 44);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 22);

        EXPECT_FALSE(TestDecSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestHexSIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, InstanceId)
    {
        using json_type = Core::JSON::InstanceId;
        using actual_type = Core::instance_id;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestInstanceIdFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, Pointer)
    {
        using json_type = Core::JSON::Pointer;
        using actual_type = Core::instance_id;

        static_assert(std::is_same<actual_type, decltype(std::declval<json_type>().Value())>::value, "Type mismatch");

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestPointerFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_TRUE(TestPointerFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 80);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);

        EXPECT_TRUE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 40);

        EXPECT_FALSE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 8);
    }

    TEST(JSONParser, Float)
    {
        using json_type = Core::JSON::Float;
        using actual_type = float;

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestFPFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 90);

        EXPECT_TRUE(TestFPFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 94);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, Double)
    {
        using json_type = Core::JSON::Double;
        using actual_type = double;

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestFPFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestFPFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 90);

        EXPECT_TRUE(TestFPFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 94);

        EXPECT_FALSE(TestDecUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestHexUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestOctUIntFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestInstanceIdFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);

        EXPECT_FALSE(TestPointerFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 6);
    }

    TEST(JSONParser, Boolean)
    {
        using json_type = Core::JSON::Boolean;
        using actual_type = bool;

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestBoolFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestBoolFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 38);

        EXPECT_TRUE(TestBoolFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 60);
    }

    TEST(JSONParser, String)
    {
        using json_type = Core::JSON::String;
        using actual_type = std::string;

        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestStringFromValue<json_type, actual_type>()));

        EXPECT_TRUE(TestStringFromString<json_type>(malformed, count));
        EXPECT_EQ(count, 212);

        EXPECT_TRUE(TestStringFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 62);
    }

    TEST(JSONParser, ArrayType)
    {
        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecUInt64>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::DecSInt64>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexUInt64>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::HexSInt64>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt8>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt8>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt16>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt16>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt32>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt32>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctUInt64>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt64>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::OctSInt64>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::InstanceId>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::InstanceId>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Pointer>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Pointer>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Float>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Float>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Double>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Double>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Boolean>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Boolean>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE(TestArrayFromString<Core::JSON::String>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::String>(!malformed, count));
        EXPECT_EQ(count, 34);
    }

    TEST(JSONParser, Container)
    {
        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecUInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecUInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecUInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecUInt64>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecSInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecSInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecSInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::DecSInt64>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecUInt64>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::DecSInt64>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexUInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexUInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexUInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexUInt64>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexSInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexSInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexSInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::HexSInt64>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexUInt64>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::HexSInt64>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctUInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctUInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctUInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctUInt64>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctSInt8>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctSInt16>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctSInt32>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::OctSInt64>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt8>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt8>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt16>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt16>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt32>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt32>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctUInt64>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt64>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::OctSInt64>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::InstanceId>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::InstanceId>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::InstanceId>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::Pointer>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::Pointer>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::Pointer>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::Float>()));
        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::Double>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::Float>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::Float>(!malformed, count));
        EXPECT_EQ(count, 46);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::Double>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::Double>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::Boolean>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::Boolean>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::Boolean>(!malformed, count));
        EXPECT_EQ(count, 46);

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::String>()));

        EXPECT_TRUE(TestContainerFromString<Core::JSON::String>(malformed, count));
        EXPECT_EQ(count, 132);
        EXPECT_TRUE(TestContainerFromString<Core::JSON::String>(!malformed, count));
        EXPECT_EQ(count, 46);
    }
}
}

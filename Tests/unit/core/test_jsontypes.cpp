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
#include <core/JSON.h>

#define _INTERMEDIATE
namespace WPEFramework {
    namespace Core {
        namespace JSON {

            class DebugContainer : public Container {
            public :

            DebugContainer() : Container(), _data() {};
            ~DebugContainer()
            {
                std::for_each(_data.begin(), _data.end(), [](std::pair<const std::string, IElement*> & item){ delete item.second; });
            }

            virtual IElement* Find(const std::string& label) override
            {
                IElement* element = Container::Find(label);

                if (element == nullptr) {
                    auto item = _data.find(label);

                    if (item != _data.end()) {
                        element = item->second;
                    }
                }

                return element;
            }

            virtual uint16_t Deserialize(IElement* element, VARIABLE_IS_NOT_USED const std::string& label, const char stream[], const uint16_t maxLength, uint32_t& offset, Core::OptionalType<Error>& error) override
            {
                uint16_t loaded = 0;

                // Successive calls on the identical element are possible
                auto item = Find(label);
                if (item != nullptr) {
                    loaded += item->Deserialize(stream, maxLength, offset, error);
                } else {

                    if (element != nullptr) {
                        element->Clear();
                        loaded += element->Deserialize(stream, maxLength, offset, error);
                    } else {
                        std::string message{"No matching key-value pair for Container."};

                        TypeEstimatedSupportedJSONTypes type;

                        element = TypeEstimate(stream, type);

                        message.append(" Value type estimated as '");

                        switch (type){
                        case CONTAINER      : message.append("Container"); break;
                        case ARRAYTYPE      : message.append("ArrayType"); break;
                        case DECIMALU64     : message.append("DecUInt64"); break;
                        case DECIMALS64     : message.append("DecSInt64"); break;
                        case HEXADECIMALU64 : message.append("HexUInt64"); break;
                        case HEXADECIMALS64 : message.append("HexSInt64"); break;
                        case OCTALU64       : message.append("OctUInt64"); break;
                        case OCTALS64       : message.append("OctSInt64"); break;
                        case DOUBLE         : message.append("Double"); break;
                        case BOOLEAN        : message.append("Boolean"); break;
                        case STRING         : message.append("String"); break;
                        case UNDETERMINED   : FALLTHROUGH;
                        default             : type = UNDETERMINED;
                                              message.append("Undetermined");
                        }

                        message.append("'.");

                        if (element != nullptr && type != UNDETERMINED) {
                            TRACE_L1("%s", message.c_str());

                            switch (type) {
                            case CONTAINER : // Replace by DebugContainer
                                             delete element;
                                             element = new DebugContainer();
                                             break;
                            case ARRAYTYPE : // Currently, ArrayType does not detect underlying type, use opaque string
                                             delete element;
                                             element = new String(false);
                                             break;
                            default        :;
                            }

// FIXME: can fail
                            auto it = _data.insert({label, element});

                            Add(it.first->first.c_str(), element);

                            loaded += element->Deserialize(stream, maxLength, offset, error);
                        } else {
                            error = Error{message.c_str()};
// FIXME: correct?
                            offset = ~0;
                        }
                    }

                }

                return loaded;
            }

            private :

                enum TypeEstimatedSupportedJSONTypes {
                    UNDETERMINED,
                    CONTAINER,
                    ARRAYTYPE,
                    DECIMALU64,
                    DECIMALS64,
                    HEXADECIMALU64,
                    HEXADECIMALS64,
                    OCTALU64,
                    OCTALS64,
                    DOUBLE,
                    BOOLEAN,
                    STRING,
                };

            IElement* TypeEstimate(const std::string& value) const
            {
                TypeEstimatedSupportedJSONTypes type = UNDETERMINED;
                return TypeEstimate(value, type);
            }

            IElement* TypeEstimate(const std::string& value, enum TypeEstimatedSupportedJSONTypes& type) const
            {
                IElement* element = nullptr;

                // Estimate type
                std::string::size_type nonSpaceFirstPos = value.find_first_not_of("\u0009\u000A\u000D\u0020");
                std::string::size_type nonSpaceLastPos = value.find_last_not_of("\u0009\u000A\u000D\u0020");

                if (   nonSpaceFirstPos == std::string::npos
                    || nonSpaceLastPos == std::string::npos
                ) {
                    // Empty?
                    return element;
                }

                // All types can be quoted

                if (value[nonSpaceFirstPos] == '"'
                ) {
                    // Type might be quoted or just String
                    ++nonSpaceFirstPos;
                }

                switch (value[nonSpaceFirstPos]) {
                    case '{' :  // Assume it is a Container
                    case '}' :  // Assume a Container is intended but wrongly formatted
                                element = new Container();
                                type = CONTAINER;
                                break;
                    case '[' :  // Assume it is an ArrayType
                    case ']' :  // Assume an ArrayType is intended but wrongly formatted
                                // ArrayType is of fixed type TYPE and as consequence does not support (compile time) type deducton recursion
                                {
                                    uint16_t depth = 0, typeStartPos = nonSpaceFirstPos, typeEndPos = nonSpaceFirstPos;

                                    for (auto index = nonSpaceFirstPos, end = value.length(); index < end; index++) {
                                        const char character = value[index];

                                        if (character == ']') {
                                            typeEndPos = index;
                                            break;
                                        }

                                        if (character == '[') {
                                            typeStartPos = index;
                                            ++depth;
                                        }
                                    }

                                    if ((typeStartPos + 1) < typeEndPos) {
                                        // Determine type of contained element
// FIXME: Inefficient
                                        element = TypeEstimate(value.substr(typeStartPos + 1, typeEndPos - typeStartPos - 1), type);
                                        delete element;

// FIXME: Currently fallback to underlying String instead of actual type
                                        switch(depth) {
                                        case 1  : element = new ArrayType<String>;
                                                  break;
                                        case 2  : element = new NestedArrayType<String, 1>::type;
                                                  break;
                                        case 3  : FALLTHROUGH;
                                                  // Maximum depth without warning
                                        default : element = new NestedArrayType<String, 2>::type;
                                        }

                                        type = ARRAYTYPE;
                                    } else {
                                        // No contained element. Hence unknown contained type, thus (opaque) String is the most suitable option
                                        element = new ArrayType<String>;
                                        type = ARRAYTYPE;
                                    }
                                }
                                break;
                    case 't' :  {
                                    const std::string T(IElement::TrueTag);
                                    if (   T.length() <= (nonSpaceLastPos - nonSpaceFirstPos + 1)
                                        && T == std::string(&value[nonSpaceFirstPos], nonSpaceLastPos - nonSpaceFirstPos + 1)
                                    ) {
                                        // Assume it is a Boolean
                                        element = new Boolean;
                                        type = BOOLEAN;
                                        break;
                                    }
                                }
                                FALLTHROUGH;
                    case 'f' :  {
                                    const std::string F(IElement::FalseTag);
                                    if (   F.length() <= (nonSpaceLastPos - nonSpaceFirstPos + 1)
                                        && F == std::string(&value[nonSpaceFirstPos], nonSpaceLastPos - nonSpaceFirstPos + 1)
                                    ) {
                                        // Assume it is a Boolean
                                        element = new Boolean;
                                        type = BOOLEAN;
                                        break;
                                    }
                                }
                                FALLTHROUGH;
                    case 'n' :  {
                                    const std::string N(IElement::NullTag);
                                    if (   N.length() <= (nonSpaceLastPos - nonSpaceFirstPos + 1)
                                        && N == std::string(&value[nonSpaceFirstPos], nonSpaceLastPos - nonSpaceFirstPos + 1)
                                    ) {
                                        // Assume it is a nullified element of unknown type thus (opaque) String is the most suitable option
                                        element = new String();
                                        type = STRING;
                                        break;
                                    }
                                }
                                FALLTHROUGH;
                    default  :  bool negative = value[nonSpaceFirstPos] == '-';

                                if (negative) {
                                    ++nonSpaceFirstPos;
                                }

                                if (std::isdigit(value[nonSpaceFirstPos])) {
                                    // Assume it is a FloatType or NumberType
                                    if (value.find_first_of(".eE") != std::string::npos) {
                                        // Assume it is a FloatType
                                        element = static_cast<IElement*>(new Double());
                                        type = DOUBLE;
                                    } else if (value.find_first_of("x") != std::string::npos) {
                                        // Assume it is a Hex(S/U)Int(8/16/32/64)
                                        element = negative ? static_cast<IElement*>(new HexSInt64()) : static_cast<IElement*>(new HexUInt64());
                                        type = negative ? HEXADECIMALS64 : HEXADECIMALU64;
                                    } else if (    (nonSpaceLastPos ==  nonSpaceFirstPos && value[nonSpaceFirstPos] == '0')
                                                || (nonSpaceLastPos > nonSpaceFirstPos && value[nonSpaceFirstPos] == '0' && value[nonSpaceFirstPos + 1] != '0')
                                              ) {
                                        // Assume it is a Dec(S/U)Int(8/16/32/64)
                                        element = negative ? static_cast<IElement*>(new DecSInt64()) : static_cast<IElement*>(new DecUInt64());
                                        type = negative ? DECIMALS64 : DECIMALU64;
                                    } else {
                                        // Assume it is a Oct(S/U)Int(8/16/32/64)
                                        element = negative ? static_cast<IElement*>(new OctSInt64()) : static_cast<IElement*>(new OctUInt64());
                                        type = negative ? OCTALS64 : OCTALU64;
                                    }

                                    break;
                                }

                                // What remains should be considered as an opaque string
                                element = static_cast<IElement*>(new String());
                                type = STRING;
                }

                ASSERT(element != nullptr);

                return element;
            }

                mutable std::unordered_map<std::string, IElement*> _data;
            };

        }
    }
}

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

            bool result = object.FromString(json, status);
                 result =    !(status.IsSet())
                          && result
                          ;

                 if (FromTo) {
                     result =    object.ToString(stream)
                              && result
                              ;

                    if (AllowChange) {
                        result =    std::string(json.c_str()) != std::string(stream.c_str())
                                  && result
                                 ;
                    } else {
                        // Inhibit QuotedSerializeBit
                        object.SetQuoted(false);

                        if (!object.IsQuoted() && quoted) {
                            // QuotedSerializeBit 'adds' quotes via ToStream() / Serialize()
                            // Stream will be quoted regardless eg, a quoted string that might not be a properly formatted JSON string
                            result =   ("\"" + std::string(json.c_str()) + "\"") == std::string(stream.c_str())
                                    && result
                                    ;
                        } else {
                            // Quotes are added to stream based on json input
                            result =    std::string(json.c_str()) == std::string(stream.c_str())
                                    && result
                                    ;

                            // Deserialize and Value can only output identical values if
                            // - QuotedSerializeBit is not set
                            // - No special characters are present beacuse Value contains comppressed data
#ifdef _0
                            result = (   object.IsNull()
                                      || object.IsSet()
                                     ) 
                                     && std::string(stream.c_str()) == std::string(object.Value().c_str())
                                     && result
                                     ;
#endif
                        }

                        // Restore the QuotedSerializeBit
                        object.SetQuoted(quoted);
                    }

                    object.Clear();
                 }

            count += result;
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
                count += TestJSONFormat<T>("{ }", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc 123 ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{ abc 123 ABC }", FromTo, AllowChange);

                // JSON value strings
                count += TestJSONFormat<T>("\"\"", FromTo, AllowChange); // Empty
                count += TestJSONFormat<T>("\" \"", FromTo, AllowChange); // Regular space
                count += TestJSONFormat<T>("\"abc123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"abc 123 ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\" abc 123 ABC \"", FromTo, AllowChange);

                // Opaque strings for reverse solidus
                count += TestJSONFormat<T>("{abc\\123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u005C}", FromTo, AllowChange);

                // JSON value string reverse solidus
                count += TestJSONFormat<T>("\"\\\\\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u005C\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"abc\\123ABC\"", FromTo, AllowChange);

                // Opaque strings for quotation mark do not exist, except if the character is preceded by a non-quotation mark
                count += TestJSONFormat<T>("{abc\"123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\"123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\"}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u0022}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\u0022}", FromTo, AllowChange); // Requires preceding character not to indicate start of JSON value string

                // JSON value string for quotation mark
                count += TestJSONFormat<T>("\"abc\\\"123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\"\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0022\"", FromTo, AllowChange);

                // Opaque strings for solidus
                count += TestJSONFormat<T>("{abc/123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\/123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{/}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\/}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u002F}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u002F}", FromTo, AllowChange);

                // JSON value strings for solidus, non-mandatory, popular two character escape
                count += TestJSONFormat<T>("\"abc\\/123ABC\"", FromTo, !AllowChange); // Ambiguous ToString, either / or \\/ are possible in FromString, implementation assumes former
                count += TestJSONFormat<T>("\"\\/\"", FromTo, !AllowChange);
                count += TestJSONFormat<T>("\"\\u002F\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"abc/123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\u002F\"", FromTo, AllowChange);

                // Opaque strrings for backspace
                count += TestJSONFormat<T>("{abc\b123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\b123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\b}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\b}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u0008}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u0008}", FromTo, AllowChange);

                // JSON value strings for backspace
                count += TestJSONFormat<T>("\"abc\\\b123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\b\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0008\"", FromTo, AllowChange);

                // Opaque strings for form feed
                count += TestJSONFormat<T>("{abc\f123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\f123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\f}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\f}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u000C}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u000C}", FromTo, AllowChange);

                // JSON value strings for form feed
                count += TestJSONFormat<T>("\"abc\\\f123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\f\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000C\"", FromTo, AllowChange);

                // Opaque strings for line feed
                count += TestJSONFormat<T>("{abc\n123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\n123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\n}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\n}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u000A}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u000A}", FromTo, AllowChange);

                // JSON value strings for line feed
                count += TestJSONFormat<T>("\"abc\\\n123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\n\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000A\"", FromTo, AllowChange);

                // Opaque strings for carriage return
                count += TestJSONFormat<T>("{abc\r123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\r123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\r}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\r}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u000D}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u000D}", FromTo, AllowChange);

                // JSON value strings for carriage return
                count += TestJSONFormat<T>("\"abc\\\r123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\r\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u000D\"", FromTo, AllowChange);

                // Opaque strings for character tabulation
                count += TestJSONFormat<T>("{abc\b123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{abc\\\b123ABC}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\b}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\\b}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u0009}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u0009}", FromTo, AllowChange);

                // JSON value strings for character tabulation
                count += TestJSONFormat<T>("\"abc\\\b123ABC\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\\b\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0009\"", FromTo, AllowChange);

                // Opaque strings for escape boundaries
                count += TestJSONFormat<T>("{\\u0000}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u001F}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u0020}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u001F}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\u0020}", FromTo, AllowChange);

                // JSON value strings for escape boundaries
                count += TestJSONFormat<T>("\"\\u0000\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u001F\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\u0020\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\u0020\"", FromTo, AllowChange);

                // Opaque strings for incomplete unicode
                count += TestJSONFormat<T>("{\\u}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u0}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u00}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{\\u002}", FromTo, AllowChange);

                // Opaque string for 'nullifying'
                count += TestJSONFormat<T>("{null}", FromTo, AllowChange); // Not nullifying!
                count += TestJSONFormat<T>("{null0}", FromTo, AllowChange);

                // JSON value string 'nullifying'
                count += TestJSONFormat<T>("\"null\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"nul0\"", FromTo, AllowChange);

                // Opaque strings for tokens
                count += TestJSONFormat<T>("{}", FromTo, AllowChange);
                count += TestJSONFormat<T>("[]", FromTo, AllowChange);
                count += TestJSONFormat<T>("{true}", FromTo, AllowChange);
                count += TestJSONFormat<T>("{false}", FromTo, AllowChange);

                // JSON value string for tokens
                count += TestJSONFormat<T>("\"{}\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"[]\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"true\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"false\"", FromTo, AllowChange);

                // Opaque strings for Insignificant white space
                count += TestJSONFormat<T>("  {}  ", FromTo, !AllowChange); // {}
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020{abc123ABC}\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // {abs123ABC}

                // JSON value strings for Insignificant white space
                count += TestJSONFormat<T>(" \"\" ", FromTo, !AllowChange); // Empty with insignificant white spaces, stripped away
                count += TestJSONFormat<T>("\u0009\u000A\u000D\u0020\"abc123ABC\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange); // Idem

                // Surrogate pairs
                // Examples taken from https://en.wikipedia.org/wiki/UTF-16#Code_points_from_U+010000_to_U+10FFFF

                count += TestJSONFormat<T>("\"\\u10437\"", FromTo, !AllowChange); // non BMP unicode 10437 but FromString returns surrogate pair \\uD801 \\uDC37
                count += TestJSONFormat<T>("\"\\uD801\\uDC37\"", FromTo, AllowChange); // Internally stored as \\u10437
                count += TestJSONFormat<T>("\"\\uD801D\"", FromTo, !AllowChange); // non BMP unicode D801D, but FromString returns surrogate pair \\uDB20 \\uDC1D 

                count += TestJSONFormat<T>("\"\\u24B62\"", FromTo, !AllowChange); // unicode 24B62 but FromString returns surrogate pair \\uD852 \\uDF62
                count += TestJSONFormat<T>("\"\\uD852\\uDF62\"", FromTo, AllowChange); // Internally stored as \\u24B62
                count += TestJSONFormat<T>("\"\\uD8526\"", FromTo, !AllowChange); // non BMP unicode D8526, but FromString returns surrogate pair \\uDB21 \\uDD26
                count += TestJSONFormat<T>("\"\\u10FFFF\"", FromTo, !AllowChange); // unicode 10FFFF FromString returns surrogate pair \\uDBFF \\uDFFF
                count += TestJSONFormat<T>("\"\\uDBFF\\uDFFF\"", FromTo, AllowChange); // Internally stored as \\u10FFFF

                // Invalid low surrogate, no internal conversion, transparently processed as two 4-hexadecimal character sequences
                count += TestJSONFormat<T>("\"\\uD801\\uDBFF\"", FromTo, AllowChange);
                count += TestJSONFormat<T>("\"\\uD852\\uDBFF\"", FromTo, AllowChange);
            } else {
                // Malformed
                // =========
                // JSON value string reverse solidus
                count += !TestJSONFormat<T>("\"\\\"", FromTo, AllowChange);

                // Opaque strings for quotation mark
                count += !TestJSONFormat<T>("\u0022", FromTo, AllowChange);

                // JSON value string for quotation mark
                count += !TestJSONFormat<T>("\"abc\"123ABC\"", FromTo, AllowChange);
                count += !TestJSONFormat<T>("\"\u0022\"", FromTo, AllowChange);

                // JSON value strings for solidus

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
                count += !TestJSONFormat<T>("{\u0000}", FromTo, AllowChange); // 'Premature' string terminaton
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

                // Surrogate pairs
                // Incomplete low surrogate
                count += !TestJSONFormat<T>("\"\\uD801\\uD\"", FromTo, AllowChange);
            }
        } while (FromTo);

        return  !malformed ? count == 236
                           : count == 60
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

                // Nullify contained types
                count += TestJSONFormat<T>("[null]", FromTo, AllowChange || std::is_same<S, Core::JSON::String>::value); // [null] or ["null"]
                count += TestJSONFormat<T>("[null,null]", FromTo, AllowChange || std::is_same<S, Core::JSON::String>::value); // [null,null] or ["null","null"]
                count += TestJSONFormat<T>("[null,\u0009\u000A\u000D\u0020null]", FromTo, !AllowChange); // [null,null] or ["null","null"]
                count += TestJSONFormat<T>("[null\u0009\u000A\u000D\u0020,null]", FromTo, !AllowChange); // [null,null] or ["null","null"]
                count += TestJSONFormat<T>("[null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020null]", FromTo, !AllowChange); // [null,null] or ["null","null"]

                count += TestJSONFormat<W>("[[null]]", FromTo, AllowChange || std::is_same<S, Core::JSON::String>::value); // [[null,[null]] or [["null"],["null"]]
                count += TestJSONFormat<W>("[[null],[null]]", FromTo, AllowChange || std::is_same<S, Core::JSON::String>::value);// [[null,[null]] or [["null"],["null"]]
                count += TestJSONFormat<W>("[[null],\u0009\u000A\u000D\u0020[null]]", FromTo, !AllowChange); // [[null],[null]] or [["null"],["null"]]
                count += TestJSONFormat<W>("[[null]\u0009\u000A\u000D\u0020,[null]]", FromTo, !AllowChange); // [[null],[null]] or [["null"],["null"]]
                count += TestJSONFormat<W>("[[null]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020[null]]", FromTo, !AllowChange); // [[null],[null]] or [["null"],["null"]]

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

        using T = Core::JSON::DebugContainer;
        using W = Core::JSON::DebugContainer;

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

                // Nullify contained types
                count += TestJSONFormat<T>("{\"\":null}", FromTo, !AllowChange); // null is detected as String, {\"\":\"null\"}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<T>("{\"\":null,\"\":null}", FromTo, !AllowChange); // null is detected as String, {\"\":\"null\"}
                count += TestJSONFormat<T>("{\"\":null,\u0009\u000A\u000D\u0020\"\":null}", FromTo, !AllowChange); // null is detected as String, {\"\":\"null\"}
                count += TestJSONFormat<T>("{\"\":null\u0009\u000A\u000D\u0020,\"\":null}", FromTo, !AllowChange); // null is detected as String, {\"\":\"null\"}
                count += TestJSONFormat<T>("{\"\":null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":null}", FromTo, !AllowChange); // null is detected as String, {\"\":\"null\"}

                count += TestJSONFormat<T>("{\"A\":null,\"B\":null}", FromTo, !AllowChange); // null is detected as String, {\"A\":\"null\",\"B\":\"null\"}
                count += TestJSONFormat<T>("{\"A\":null,\u0009\u000A\u000D\u0020\"B\":null}", FromTo, !AllowChange); // null is detected as String, {\"A\":\"null\",\"B\":\"null\"}
                count += TestJSONFormat<T>("{\"A\":null\u0009\u000A\u000D\u0020,\"B\":null}", FromTo, !AllowChange); // null is detected as String, {\"A\":\"null\",\"B\":\"null\"}
                count += TestJSONFormat<T>("{\"A\":null\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":null}", FromTo, !AllowChange); // null is detected as String, {\"A\":\"null\",\"B\":\"null\"}

                count += TestJSONFormat<W>("{\"\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"\":{\"\":null}}

                // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
                count += TestJSONFormat<W>("{\"\":{\"\":null},\"\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"\":{\"\":\"null\"}}
                count += TestJSONFormat<W>("{\"\":{\"\":null},\u0009\u000A\u000D\u0020\"\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"\":{\"\":\"null\"}}
                count += TestJSONFormat<W>("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\"\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"\":{\"\":\"null\"}}
                count += TestJSONFormat<W>("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"\":{\"\":\"null\"}}

                count += TestJSONFormat<W>("{\"A\":{\"\":null},\"B\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"A\":{\"\":\"null\"},\"B\":{\"\":{\"null\"}}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null},\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\"B\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}
                count += TestJSONFormat<W>("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", FromTo, !AllowChange); // null is detected as String, {\"A\":{\"\":\"null\"},\"\":{\"\":\"null\"}}

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
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0\"]}\"

                    count += 12;
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
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"0x0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0X0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0x0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0X0\"]}\"

                    count += 12;
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
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"00\"]\"}", FromTo, AllowChange); // {\"\":\"[\"00\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"00\"]}\"", FromTo, AllowChange); // \"{\"\":[\"00\"]}\"

                    count += 12;
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
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"0.0\"]\"}", FromTo, AllowChange); // {\"\":\"[\"0.0\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"0.0\"]}\"", FromTo, AllowChange); // \"{\"\":[\"0.0\"]}\"

                    count += 11;
                }

                if (std::is_same<S, Core::JSON::String>::value) {
                    count += TestJSONFormat<T>("{\"\":\"\"}", FromTo, AllowChange); // {\"\":\"\"}

                    count += TestJSONFormat<W>("{\"\":{\"\":\"\"}}", FromTo, AllowChange); // {\"\":{\"\":\"\"}}

                    // Nullifying container ?
                    count += TestJSONFormat<T>("{\"\":\"null\"}", FromTo, AllowChange); // null

                    // Scope tests
                    count += TestJSONFormat<T>("{\"\":[\"\"]}", FromTo, AllowChange); // {\"\":[\"\"]}
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"\"]\"}", FromTo, AllowChange); // {\"\":\"[\"\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"\"]}\"", FromTo, AllowChange); // \"{\"\":[\"\"]}\"

                    count += 17;
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
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"true\"]\"}", FromTo, AllowChange); // {\"\":\"[\"true\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"true\"]}\"", FromTo, AllowChange); // \"{\"\":[true]}\"

                    count += TestJSONFormat<T>("{\"\":[false]}", FromTo, AllowChange); // {\"\":[false]}
                    count += TestJSONFormat<T>("{\"\":[[false]]}", FromTo, AllowChange); // {\"\":[[false]]}
                    count += TestJSONFormat<T>("{\"\":[[[false]]]}", FromTo, AllowChange); // \"{\"\":[[[false]]]}

                    count += TestJSONFormat<T>("{\"\":[false,false]}", FromTo, AllowChange); // {\"\":[false,false]}
                    count += TestJSONFormat<T>("{\"\":[[false,false]]}", FromTo, AllowChange); // {\"\":[[false,false]]}
                    count += TestJSONFormat<T>("{\"\":[[[false,false]]]}", FromTo, AllowChange); // {\"\":[[[false,false]]]}

                    count += TestJSONFormat<T>("{\"\":[\"false\"]}", FromTo, AllowChange); // {\"\":[\"false\"]}
                    // Currently, ArrayType is detected as String which does not allow unescaped quotes
//                    count += TestJSONFormat<T>("{\"\":\"[\"false\"]\"}", FromTo, AllowChange); // {\"\":\"[\"false\"]\"}
                    count += TestJSONFormat<T>("\"{\"\":[\"false\"]}\"", FromTo, AllowChange); // \"{\"\":[\"false\"]}\"}

                    count += 2;
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
}

    // ENUM_CONVERSION* does not check on uniqueness!

    // Underlying type is integral
    enum class TypicalEnum {A, B, C};

    // EnumType uses String internally, hence, the values should JSON or opaque formatted
    ENUM_CONVERSION_BEGIN(TypicalEnum)
    // {T, const TCHAR*, uint32_t},
    // _TXT() expands to _T() and size
    { TypicalEnum::A, _TXT("TE_A") }, // Fails
    { TypicalEnum::B, _TXT("{TE_B}") },
    // Always add ',' to last entry
    { TypicalEnum::C, _TXT("\"TE_C\"") },
    ENUM_CONVERSION_END(TypicalEnum)

    //using IntegralEnum = uint32_t; // Fails in overload resultion
    using IntegralEnum = uint64_t;

    ENUM_CONVERSION_BEGIN(IntegralEnum)
    { IntegralEnum(1), _TXT("{IE_1}") },
    { IntegralEnum(2), _TXT("\"IE_2\"") },
    ENUM_CONVERSION_END(IntegralEnum)
    // using uint32_t; // Fails

//FIXME: using ArrayType<> and usng Container may fail

    using DecUInt64WithoutSetEnum = Core::JSON::DecUInt64;
    using DecSInt64WithSetEnum = Core::JSON::DecSInt64;

    // Take a intializing value different than the default 0!

    ENUM_CONVERSION_BEGIN(DecUInt64WithoutSetEnum)
    { DecUInt64WithoutSetEnum(1), _TXT("{DUI64_1}") },
    { DecUInt64WithoutSetEnum(2), _TXT("\"DUI64_2\"") },
    ENUM_CONVERSION_END(DecUInt64WithoutSetEnum)

    ENUM_CONVERSION_BEGIN(DecSInt64WithSetEnum)
    { DecSInt64WithSetEnum(1) = 2, _TXT("{DSI64_1}") },
    { DecSInt64WithSetEnum(3) = 4, _TXT("\"DSI64_3\"") },
    ENUM_CONVERSION_END(DecSInt64WithSetEnum)

    using HexUInt64WithoutSetEnum = Core::JSON::HexUInt64;
    using HexSInt64WithSetEnum = Core::JSON::HexSInt64;

    // Take a intializing value different than the default 0x0!

    ENUM_CONVERSION_BEGIN(HexUInt64WithoutSetEnum)
    { HexUInt64WithoutSetEnum(0x1), _TXT("{HUI64_0x1}") },
    { HexUInt64WithoutSetEnum(0x2), _TXT("\"HUI64_0x2\"") },
    ENUM_CONVERSION_END(HexUInt64WithoutSetEnum)

    ENUM_CONVERSION_BEGIN(HexSInt64WithSetEnum)
    { HexSInt64WithSetEnum(0x1) = 0x2, _TXT("{HSI64_0x1}") },
    { HexSInt64WithSetEnum(0x3) = 0x4, _TXT("\"HSI64_0x3\"") },
    ENUM_CONVERSION_END(HexSInt64WithSetEnum)

    using OctUInt64WithoutSetEnum = Core::JSON::OctUInt64;
    using OctSInt64WithSetEnum = Core::JSON::OctSInt64;

    // Take a intializing value different than the default 00!

    ENUM_CONVERSION_BEGIN(OctUInt64WithoutSetEnum)
    { OctUInt64WithoutSetEnum(01), _TXT("{OUI64_01}") },
    { OctUInt64WithoutSetEnum(02), _TXT("\"OUI64_02\"") },
    ENUM_CONVERSION_END(OctUInt64WithoutSetEnum)

    ENUM_CONVERSION_BEGIN(OctSInt64WithSetEnum)
    { OctSInt64WithSetEnum(01) = 02, _TXT("{OSI64_01}") },
    { OctSInt64WithSetEnum(03) = 04, _TXT("\"OSI64_03\"") },
    ENUM_CONVERSION_END(OctSInt64WithSetEnum)

    using DoubleWithoutSetEnum = Core::JSON::Float;

    ENUM_CONVERSION_BEGIN(DoubleWithoutSetEnum)
    { DoubleWithoutSetEnum(1.0), _TXT("{D_1.0}") },
    { DoubleWithoutSetEnum(2.0), _TXT("\"D_2.0\"") },
    ENUM_CONVERSION_END(DoubleWithoutSetEnum)

    // Use another FloatType to avoid (global) redefinition
    using DoubleWithSetEnum = Core::JSON::Double;

    ENUM_CONVERSION_BEGIN(DoubleWithSetEnum)
    { DoubleWithSetEnum(1.0) = 2.0, _TXT("{D_1.0}") },
    { DoubleWithSetEnum(3.0) = 4.0, _TXT("\"D_3.0\"") },
    ENUM_CONVERSION_END(DoubleWithSetEnum)

    using BoolEnum = Core::JSON::Boolean;

//#define _UNSET

    ENUM_CONVERSION_BEGIN(BoolEnum)
#ifdef _UNSET
    // Without 'set'
    { BoolEnum(true), _TXT("{B_F}") },
    { BoolEnum(false), _TXT("\"B_T\"") },
#else
    // With 'set'
    { BoolEnum(true) = false, _TXT("{BW_F}") },
    { BoolEnum(false) = true, _TXT("\"BW_T\"") },
#endif
    ENUM_CONVERSION_END(BoolEnum)

namespace Tests {

    template <typename T>
    bool TestEnumTypeFromString(bool malformed, uint8_t& count)
    {
        constexpr bool AllowChange = false;

        count = 0;
        bool FromTo = false;

        do {
            FromTo = !FromTo;

            if (!malformed) {
                // Correctly formatted
                // ===================

                // Enum uses String internally, hence, the values are JSON or opaque formatted
                count += TestJSONFormat<Core::JSON::EnumType<TypicalEnum>>("{TE_B}", FromTo, AllowChange);
                count += TestJSONFormat<Core::JSON::EnumType<TypicalEnum>>("\"TE_C\"", FromTo, AllowChange);

                count += TestJSONFormat<Core::JSON::EnumType<IntegralEnum>>("{IE_1}", FromTo, AllowChange);
                count += TestJSONFormat<Core::JSON::EnumType<IntegralEnum>>("\"IE_2\"", FromTo, AllowChange);

                if (   std::is_same<T, Core::JSON::DecUInt8>::value
                    || std::is_same<T, Core::JSON::DecSInt8>::value
                    || std::is_same<T, Core::JSON::DecUInt16>::value
                    || std::is_same<T, Core::JSON::DecSInt16>::value
                    || std::is_same<T, Core::JSON::DecUInt32>::value
                    || std::is_same<T, Core::JSON::DecSInt32>::value
                    || std::is_same<T, Core::JSON::DecUInt64>::value
                    || std::is_same<T, Core::JSON::DecSInt64>::value
                ) {
                    count += TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("{DSI64_1}", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("\"DSI64_3\"", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020{DSI64_1}\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020\"DSI64_3\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                }

                if (   std::is_same<T, Core::JSON::HexUInt8>::value
                    || std::is_same<T, Core::JSON::HexSInt8>::value
                    || std::is_same<T, Core::JSON::HexUInt16>::value
                    || std::is_same<T, Core::JSON::HexSInt16>::value
                    || std::is_same<T, Core::JSON::HexUInt32>::value
                    || std::is_same<T, Core::JSON::HexSInt32>::value
                    || std::is_same<T, Core::JSON::HexUInt64>::value
                    || std::is_same<T, Core::JSON::HexSInt64>::value
                ) {
                    count += TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("{HSI64_0x1}", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("\"HSI64_0x3\"", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020{HSI64_0x1}\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020\"HSI64_0x3\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                }

                if (   std::is_same<T, Core::JSON::OctUInt8>::value
                    || std::is_same<T, Core::JSON::OctSInt8>::value
                    || std::is_same<T, Core::JSON::OctUInt16>::value
                    || std::is_same<T, Core::JSON::OctSInt16>::value
                    || std::is_same<T, Core::JSON::OctUInt32>::value
                    || std::is_same<T, Core::JSON::OctSInt32>::value
                    || std::is_same<T, Core::JSON::OctUInt64>::value
                    || std::is_same<T, Core::JSON::OctSInt64>::value
                ) {
                    count += TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("{OSI64_01}", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("\"OSI64_03\"", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020{OSI64_01}\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("\u0009\u000A\u000D\u0020\"OSI64_03\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                }

                if (   std::is_same<T, Core::JSON::Float>::value
                    || std::is_same<T, Core::JSON::Double>::value
                ) {
                    count += TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("{D_1.0}", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("\"D_3.0\"", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("\u0009\u000A\u000D\u0020{D_1.0}\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("\u0009\u000A\u000D\u0020\"D_3.0\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                }

                if (std::is_same<T, Core::JSON::Boolean>::value ) {
#ifdef _UNSET
                    count += 4;
#else
                    count += TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("{BW_F}", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("\"BW_T\"", FromTo, AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("\u0009\u000A\u000D\u0020{BW_F}\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
                    count += TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("\u0009\u000A\u000D\u0020\"BW_T\"\u0009\u000A\u000D\u0020", FromTo, !AllowChange);
#endif
                }
            } else {
                // Malformed
                // =========
                // Internally EnumType uses String, hence that behavior influences match

                count += !TestJSONFormat<Core::JSON::EnumType<TypicalEnum>>("TE_A", FromTo, AllowChange);

                count += !TestJSONFormat<Core::JSON::EnumType<TypicalEnum>>("\"TE\"", FromTo, AllowChange);
                count += !TestJSONFormat<Core::JSON::EnumType<TypicalEnum>>("{TE_C}", FromTo, AllowChange);

                count += !TestJSONFormat<Core::JSON::EnumType<IntegralEnum>>("\"IE\"", FromTo, AllowChange);
                count += !TestJSONFormat<Core::JSON::EnumType<IntegralEnum>>("\"IE_1\"", FromTo, AllowChange);

                if (   std::is_same<T, Core::JSON::DecUInt8>::value
                    || std::is_same<T, Core::JSON::DecSInt8>::value
                    || std::is_same<T, Core::JSON::DecUInt16>::value
                    || std::is_same<T, Core::JSON::DecSInt16>::value
                    || std::is_same<T, Core::JSON::DecUInt32>::value
                    || std::is_same<T, Core::JSON::DecSInt32>::value
                    || std::is_same<T, Core::JSON::DecUInt64>::value
                    || std::is_same<T, Core::JSON::DecSInt64>::value
                ) {
                    count += !TestJSONFormat<Core::JSON::EnumType<DecUInt64WithoutSetEnum>>("{DUI64_1}", true, AllowChange); // unset, uses default

                    count += !TestJSONFormat<Core::JSON::EnumType<DecUInt64WithoutSetEnum>>("\"{DUI64_1}\"", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<DecUInt64WithoutSetEnum>>("\"DUI64_1\"", FromTo, AllowChange);

                    count += !TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("{DSI64}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<DecSInt64WithSetEnum>>("\"DSI64_1\"", FromTo, AllowChange);
                }

                if (   std::is_same<T, Core::JSON::HexUInt8>::value
                    || std::is_same<T, Core::JSON::HexSInt8>::value
                    || std::is_same<T, Core::JSON::HexUInt16>::value
                    || std::is_same<T, Core::JSON::HexSInt16>::value
                    || std::is_same<T, Core::JSON::HexUInt32>::value
                    || std::is_same<T, Core::JSON::HexSInt32>::value
                    || std::is_same<T, Core::JSON::HexUInt64>::value
                    || std::is_same<T, Core::JSON::HexSInt64>::value
                ) {
                    count += !TestJSONFormat<Core::JSON::EnumType<HexUInt64WithoutSetEnum>>("{HUI64_0x1}", true, AllowChange); // unset, uses default

                    count += !TestJSONFormat<Core::JSON::EnumType<HexUInt64WithoutSetEnum>>("\"{HUI64_0x1}\"", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<HexUInt64WithoutSetEnum>>("\"HUI64_0x1\"", FromTo, AllowChange);

                    count += !TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("{HSI64}", FromTo, AllowChange); // Does not exist
                    count += !TestJSONFormat<Core::JSON::EnumType<HexSInt64WithSetEnum>>("\"HSI64_0x1\"", FromTo, AllowChange); // No exact match
                }

                if (   std::is_same<T, Core::JSON::OctUInt8>::value
                    || std::is_same<T, Core::JSON::OctSInt8>::value
                    || std::is_same<T, Core::JSON::OctUInt16>::value
                    || std::is_same<T, Core::JSON::OctSInt16>::value
                    || std::is_same<T, Core::JSON::OctUInt32>::value
                    || std::is_same<T, Core::JSON::OctSInt32>::value
                    || std::is_same<T, Core::JSON::OctUInt64>::value
                    || std::is_same<T, Core::JSON::OctSInt64>::value
                ) {
                    count += !TestJSONFormat<Core::JSON::EnumType<OctUInt64WithoutSetEnum>>("{OUI64_01}", true, AllowChange); // unset, uses default

                    count += !TestJSONFormat<Core::JSON::EnumType<OctUInt64WithoutSetEnum>>("\"{OUI64_01}\"", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<OctUInt64WithoutSetEnum>>("\"OUI64_01\"", FromTo, AllowChange);

                    count += !TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("{OSI64}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<OctSInt64WithSetEnum>>("\"OSI64_01\"", FromTo, AllowChange);
                }

                if (   std::is_same<T, Core::JSON::Float>::value
                    || std::is_same<T, Core::JSON::Double>::value
                ) {
                    count += !TestJSONFormat<Core::JSON::EnumType<DoubleWithoutSetEnum>>("{D_1.0}", true, AllowChange); // unset, uses default

                    count += !TestJSONFormat<Core::JSON::EnumType<DoubleWithoutSetEnum>>("\"{D_1.0}\"", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<DoubleWithoutSetEnum>>("\"D_1.0\"", FromTo, AllowChange);

                    count += !TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("{D_1}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<DoubleWithSetEnum>>("\"D_1.0\"", FromTo, AllowChange);
                }

                if (std::is_same<T, Core::JSON::Boolean>::value ) {
#ifdef _UNSET
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("{B_}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("{B_T}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("\"B_F\"", FromTo, AllowChange);

                    count += 2;
#else
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("{BW_}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("{BW_T}", FromTo, AllowChange);
                    count += !TestJSONFormat<Core::JSON::EnumType<BoolEnum>>("\"BW_F\"", FromTo, AllowChange);

                    count += 2;
#endif
                }
            }
        } while (FromTo);

        return !malformed ? count == 16
                          : count == 20
               ;
    }

#ifdef _UNSET
#undef _UNSET
#endif

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

        // Inhibit QuotedSerializeBit
        object.SetQuoted(false);

        count += object.FromString("{abc123ABC}"); // Opaque string
        count += S(object.Value().c_str()) == S("{abc123ABC}");
        count += object.FromString("\u0009\u000A\u000D\u0020\"abc123ABC\"\u0009\u000A\u000D\u0020"); // JSON String
        count += S(object.Value().c_str()) == S("\"abc123ABC\"");
        count += object.FromString("{\"abc123ABC\"}"); // Opaque string
        count += S(object.Value().c_str()) == S("{\"abc123ABC\"}");
        count += object.FromString("\"{abc123ABC}\""); // JSON String
        count += S(object.Value().c_str()) == S("\"{abc123ABC}\"");

        Core::OptionalType<Core::JSON::Error> error;
        Core::File opaqueFile("lorem_ipsum_opaque_string.dat");
        count +=    opaqueFile.Open(true)
                 && object.Core::JSON::IElement::FromFile(opaqueFile, error)
                 && !object.IsQuoted()
                 ;
        opaqueFile.Close();

        Core::File stringFile("lorem_ipsum_json_string.dat");
        count +=    stringFile.Open(true)
                 && object.Core::JSON::IElement::FromFile(stringFile, error)
                 && object.IsQuoted()
                 ;
        stringFile.Close();

        return count == 10;
    }

    template <typename T, typename S>
    bool TestArrayFromValue()
    {
        static_assert(std::is_same<T, Core::JSON::ArrayType<S>>::value, "Type mismatch");

        auto TestEquality = [](const std::string& in, const std::string& out) -> bool
        {
            Core::JSON::ArrayType<S> object;

            Core::OptionalType<Core::JSON::Error> status;

            std::string stream;

            bool result = object.FromString(in);

            // FromString clears the object, so, (re)do settings after

            object.SetExtractOnSingle(true);

            result =    result
                     && object.IsExtractOnSingleSet()
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

        if (   std::is_same<S, Core::JSON::DecUInt8>::value
            || std::is_same<S, Core::JSON::DecSInt8>::value
            || std::is_same<S, Core::JSON::DecUInt16>::value
            || std::is_same<S, Core::JSON::DecSInt16>::value
            || std::is_same<S, Core::JSON::DecUInt32>::value
            || std::is_same<S, Core::JSON::DecSInt32>::value
            || std::is_same<S, Core::JSON::DecUInt64>::value
            || std::is_same<S, Core::JSON::DecSInt64>::value
           ) {
            count += TestEquality("[1]", "1");
            count += !TestEquality("[1]", "[1]");
            count += !TestEquality("[1,2]", "1");
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
            count += TestEquality("[0x1]", "0X1");
            count += !TestEquality("[0x1]", "[0X1]");
            count += !TestEquality("[0x1,0x2]", "0X1");
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
            count += TestEquality("[01]", "01");
            count += !TestEquality("[01]", "[01]");
            count += !TestEquality("[01,02]", "01");
        }

        if (   std::is_same<S, Core::JSON::Float>::value
            || std::is_same<S, Core::JSON::Double>::value
           ) {
            count += TestEquality("[0.1]", "0.1");
            count += !TestEquality("[0.1]", "[0.1]");
            count += !TestEquality("[0.1,0.2]", "0.1");
        }

        if (std::is_same<S, Core::JSON::String>::value) {
            count += TestEquality("[\"ABC\"]", "\"ABC\"");
            count += !TestEquality("[\"ABC\"]", "[\"ABC\"]");
            count += !TestEquality("[\"ABC\",\"DEF\"]", "\"ABC\"");
        }

        if (std::is_same<S, Core::JSON::Boolean>::value) {
            count += TestEquality("[true]", "true");
            count += !TestEquality("[true]", "[true]");
            count += !TestEquality("[true,false]", "true");
        }

        if (std::is_same<S, Core::JSON::Container>::value) {
            count += TestEquality("[{}]", "{}");
            count += !TestEquality("[{}]", "[{}]");
            count += !TestEquality("[{},{}]", "{}");
        }

        return count == 3;
    }

    template <typename T, typename S>
    bool TestContainerFromValue()
    {
        static_assert(std::is_same<T, Core::JSON::Container>::value, "Type mismatch");

        auto TestEquality = [](const std::string& in, const std::string& out) -> bool
        {
            Core::JSON::DebugContainer object;

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

        count += TestEquality("{\"\":{\"\":null},\"\":{\"\":null}}", "{\"\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting
        count += TestEquality("{\"\":{\"\":null},\u0009\u000A\u000D\u0020\"\":{\"\":null}}", "{\"\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting
        count += TestEquality("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\"\":{\"\":null}}", "{\"\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting
        count += TestEquality("{\"\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":{\"\":null}}", "{\"\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting

        count += TestEquality("{\"A\":{\"\":null},\"B\":{\"\":null}}", "{\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting

        count += TestEquality("{\"A\":{\"\":null},\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", "{\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting

        count += TestEquality("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\"B\":{\"\":null}}", "{\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting

        count += TestEquality("{\"A\":{\"\":null}\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":{\"\":null}}", "{\"A\":{\"\":\"null\"},\"B\":{\"\":\"null\"}}"); // Fallback to String defaulted to quoting

        // An empty array element is defined as String since its type is unknown and, thus, a type cannot be deduced from a character sequence.

        // Currenlty, DebugContainer redefines any ArrayType as (opaque) string

        count += TestEquality("{\"\":[],\"\":[]}", "{\"\":[]}");
        count += TestEquality("{\"\":[],\u0009\u000A\u000D\u0020\"\":[]}", "{\"\":[]}");
        count += TestEquality("{\"\":[]\u0009\u000A\u000D\u0020,\"\":[]}", "{\"\":[]}");
        count += TestEquality("{\"\":[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"\":[]}", "{\"\":[]}");

        count += TestEquality("{\"A\":[],\"B\":[]}", "{\"A\":[],\"B\":[]}");
        count += TestEquality("{\"A\":[],\u0009\u000A\u000D\u0020\"B\":[]}", "{\"A\":[],\"B\":[]}");
        count += TestEquality("{\"A\":[]\u0009\u000A\u000D\u0020,\"B\":[]}", "{\"A\":[],\"B\":[]}");
        count += TestEquality("{\"A\":[]\u0009\u000A\u000D\u0020,\u0009\u000A\u000D\u0020\"B\":[]}", "{\"A\":[],\"B\":[]}");

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
            count += TestEquality("{\"\":[0,0],\"\":[0,0]}","{\"\":[0,0]}"); // TypeEstimate only detects ArrayType<String>

            count += TestEquality("{\"A\":[0,0],\"B\":[0,0]}","{\"A\":[0,0],\"B\":[0,0]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"0\"],\"\":[\"0\"]}", "{\"\":[\"0\"]}");

            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"0\"]\",\"\":\"[\"0\"]\"}", "{\"\":\"[\"0\"]\"}");
            count += TestEquality("\"{\"\":[\"0\"],\"\":[\"0\"]}\"", "\"{\"\":[\"0\"]}\"");

            count += TestEquality("{\"A\":[\"0\"],\"B\":[\"0\"]}", "{\"A\":[\"0\"],\"B\":[\"0\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"0\"]\",\"B\":\"[\"0\"]\"}", "{\"A\":\"[\"0\"]\",\"B\":\"[\"0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0\"],\"B\":[\"0\"]}\"", "\"{\"A\":[\"0\"],\"B\":[\"0\"]}\"");

            count += 34;
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
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"0x0\"]\",\"\":\"[\"0x0\"]\"}", "{\"\":\"[\"0x0\"]\"}");
            count += TestEquality("\"{\"\":[\"0x0\"],\"\":[\"0x0\"]}\"", "\"{\"\":[\"0x0\"]}\"");

            count += TestEquality("{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}", "{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"0x0\"]\",\"B\":\"[\"0x0\"]\"}", "{\"A\":\"[\"0x0\"]\",\"B\":\"[\"0x0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}\"", "\"{\"A\":[\"0x0\"],\"B\":[\"0x0\"]}\"");

            count += 34;
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
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"00\"]\",\"\":\"[\"00\"]\"}", "{\"\":\"[\"00\"]\"}");
            count += TestEquality("\"{\"\":[\"00\"],\"\":[\"00\"]}\"", "\"{\"\":[\"00\"]}\"");

            count += TestEquality("{\"A\":[\"00\"],\"B\":[\"00\"]}", "{\"A\":[\"00\"],\"B\":[\"00\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"00\"]\",\"B\":\"[\"00\"]\"}", "{\"A\":\"[\"00\"]\",\"B\":\"[\"00\"]\"}");
            count += TestEquality("\"{\"A\":[\"00\"],\"B\":[\"00\"]}\"", "\"{\"A\":[\"00\"],\"B\":[\"00\"]}\"");

            count += 34;
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
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"0.0\"]\",\"\":\"[\"0.0\"]\"}", "{\"\":\"[\"0.0\"]\"}");
            count += TestEquality("\"{\"\":[\"0.0\"],\"\":[\"0.0\"]}\"", "\"{\"\":[\"0.0\"]}\"");

            count += TestEquality("{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}", "{\"A\":[\"00\"],\"B\":[\"0.0\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"0.0\"]\",\"B\":\"[\"0.0\"]\"}", "{\"A\":\"[\"0.0\"]\",\"B\":\"[\"0.0\"]\"}");
            count += TestEquality("\"{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}\"", "\"{\"A\":[\"0.0\"],\"B\":[\"0.0\"]}\"");

            count += 34;
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
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"\"]\",\"\":\"[\"\"]\"}", "{\"\":\"[\"\"]\"}");
            count += TestEquality("\"{\"\":[\"\"],\"\":[\"\"]}\"", "\"{\"\":[\"\"]}\"");

            count += TestEquality("{\"A\":[\"\"],\"B\":[\"\"]}", "{\"A\":[\"\"],\"B\":[\"\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"\"]\",\"B\":\"[\"\"]\"}","{\"A\":\"[\"\"]\",\"B\":\"[\"\"]\"}");
            count += TestEquality("\"{\"A\":[\"\"],\"B\":[\"\"]}\"", "\"{\"A\":[\"\"],\"B\":[\"\"]}\"");

            count += 36;
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
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"true\"]\",\"\":\"[\"true\"]\"}", "{\"\":\"[\"true\"]\"}");
            count += TestEquality("\"{\"\":[\"true\"],\"\":[\"true\"]}\"", "\"{\"\":[\"true\"]}\"");

            count += TestEquality("{\"A\":[\"true\"],\"B\":[\"true\"]}", "{\"A\":[\"true\"],\"B\":[\"true\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"true\"]\",\"B\":\"[\"true\"]\"}", "{\"A\":\"[\"true\"]\",\"B\":\"[\"true\"]\"}");
            count += TestEquality("\"{\"A\":[\"true\"],\"B\":[\"true\"]}\"", "\"{\"A\":[\"true\"],\"B\":[\"true\"]}\"");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[false,false],\"\":[false,false]}", "{\"\":[false,false]}");

            count += TestEquality("{\"A\":[false,false],\"B\":[false,false]}", "{\"A\":[false,false],\"B\":[false,false]}");

            // Implementation detail: Identical keys are not distinghuised and result in the last key-value pair being recorded.
            count += TestEquality("{\"\":[\"false\"],\"\":[\"false\"]}", "{\"\":[\"false\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"\":\"[\"false\"]\",\"\":\"[\"false\"]\"}", "{\"\":\"[\"false\"]\"}");
            count += TestEquality("\"{\"\":[\"false\"],\"\":[\"false\"]}\"", "\"{\"\":[\"false\"]}\"");

            count += TestEquality("{\"A\":[\"false\"],\"B\":[\"false\"]}", "{\"A\":[\"false\"],\"B\":[\"false\"]}");
            // Currently, ArrayType is detected as String which does not allow unescaped quotes
//            count += TestEquality("{\"A\":\"[\"false\"]\",\"B\":\"[\"false\"]\"}", "{\"A\":\"[\"false\"]\",\"B\":\"[\"false\"]\"}");
            count += TestEquality("\"{\"A\":[\"false\"],\"B\":[\"false\"]}\"", "\"{\"A\":[\"false\"],\"B\":[\"false\"]}\"");

            count += 4;
        }

        if (std::is_same<S, Core::JSON::Container>::value) {
            T object;

            Core::JSON::OctSInt16 octval(01);
            Core::JSON::DecUInt16 decval(2);
            Core::JSON::ArrayType<Core::JSON::DecUInt16> arrval;

            /*Core::JSON::DecUInt16&*/ arrval.Add(Core::JSON::DecUInt16(3));
            /*Core::JSON::DecUInt16&*/ arrval.Add(Core::JSON::DecUInt16(4));
            /*Core::JSON::DecUInt16&*/ arrval.Add(Core::JSON::DecUInt16(5));

            // Interface that uses C-style strings!
            object.Add(_T("octval"), &octval);
            object.Add(_T("decval"), &decval);
            object.Add(_T("arrval"), &arrval);

            std::string result;
            count +=    object.ToString(result)
                        // Be aware one uses a const char* on the interface whereas the other return its JSON String equivalent
                     && result == "{octval:01,decval:2,arrval:[3,4,5]}"
                     ;

            count += 55;
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
        EXPECT_EQ(count, 236);

        EXPECT_TRUE(TestStringFromString<json_type>(!malformed, count));
        EXPECT_EQ(count, 60);
    }

    TEST(JSONParser, ArrayType)
    {
        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecUInt8>, Core::JSON::DecUInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecUInt16>, Core::JSON::DecUInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecUInt32>, Core::JSON::DecUInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecUInt64>, Core::JSON::DecUInt64>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecSInt8>, Core::JSON::DecSInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecSInt16>, Core::JSON::DecSInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecSInt32>, Core::JSON::DecSInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::DecSInt64>, Core::JSON::DecSInt64>()));

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

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexUInt8>, Core::JSON::HexUInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexUInt16>, Core::JSON::HexUInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexUInt32>, Core::JSON::HexUInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexUInt64>, Core::JSON::HexUInt64>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexSInt8>, Core::JSON::HexSInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexSInt16>, Core::JSON::HexSInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexSInt32>, Core::JSON::HexSInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::HexSInt64>, Core::JSON::HexSInt64>()));

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

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctUInt8>, Core::JSON::OctUInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctUInt16>, Core::JSON::OctUInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctUInt32>, Core::JSON::OctUInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctUInt64>, Core::JSON::OctUInt64>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctSInt8>, Core::JSON::OctSInt8>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctSInt16>, Core::JSON::OctSInt16>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctSInt32>, Core::JSON::OctSInt32>()));
        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::OctSInt64>, Core::JSON::OctSInt64>()));

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

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::InstanceId>, Core::JSON::InstanceId>()));

        EXPECT_TRUE(TestArrayFromString<Core::JSON::InstanceId>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::InstanceId>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::Pointer>, Core::JSON::Pointer>()));

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Pointer>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Pointer>(!malformed, count));
        EXPECT_EQ(count, 34);

//        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::Float>, Core::JSON::Float>()));
//        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::Double>, Core::JSON::Double>()));

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Float>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Float>(!malformed, count));
        EXPECT_EQ(count, 34);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Double>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Double>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::Boolean>, Core::JSON::Boolean>()));

        EXPECT_TRUE(TestArrayFromString<Core::JSON::Boolean>(malformed, count));
        EXPECT_EQ(count, 98);
        EXPECT_TRUE(TestArrayFromString<Core::JSON::Boolean>(!malformed, count));
        EXPECT_EQ(count, 34);

        EXPECT_TRUE((TestArrayFromValue<Core::JSON::ArrayType<Core::JSON::String>, Core::JSON::String>()));

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

        EXPECT_TRUE((TestContainerFromValue<Core::JSON::Container, Core::JSON::Container>()));
    }

    TEST(JSONParser, EnumType)
    {
        constexpr const bool malformed = false;
        uint8_t count = 0;

        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::DecUInt8>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::DecUInt8>(!malformed, count));
        EXPECT_EQ(count, 20);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::HexUInt8>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::HexUInt8>(!malformed, count));
        EXPECT_EQ(count, 20);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::OctUInt8>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::OctUInt8>(!malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Float>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Float>(!malformed, count));
        EXPECT_EQ(count, 20);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Double>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Double>(!malformed, count));
        EXPECT_EQ(count, 20);

        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Boolean>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::Boolean>(!malformed, count));
        EXPECT_EQ(count, 20);

        // String not viable for EnumType
#ifdef _0
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::String>(malformed, count));
        EXPECT_EQ(count, 16);
        EXPECT_TRUE(TestEnumTypeFromString<Core::JSON::String>(!malformed, count));
        EXPECT_EQ(count, 20);
#endif
    }
}
}

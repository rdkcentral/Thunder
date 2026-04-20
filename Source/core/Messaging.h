/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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
#include "MessageStore.h"

#include <cstdio>
#include <type_traits>

namespace Thunder {

namespace Core {

    namespace Messaging {

        template <const Core::Messaging::Metadata::type TYPE>
        class BaseCategoryType {
        public:
            ~BaseCategoryType() = default;
            BaseCategoryType& operator=(const BaseCategoryType&) = delete;

            template<typename... Args>
            BaseCategoryType(const string& formatter, Args... args)
                : _text()
            {
                Core::Format(_text, formatter.c_str(), args...);
            }

            BaseCategoryType(const string& text)
                : _text(text)
            {
            }

            BaseCategoryType()
                : _text()
            {
            }

        public:
            using BaseCategory = BaseCategoryType<TYPE>;

            static constexpr Core::Messaging::Metadata::type Type = TYPE;

            const char* Data() const {
                return (_text.c_str());
            }

            uint16_t Length() const {
                return (static_cast<uint16_t>(_text.length()));
            }

        protected:
            void Set(const string& text) {
                _text = text;
            }

        private:
            string _text;
        };

        class EXTERNAL TextMessage : public Core::Messaging::IEvent {
        public:
            TextMessage() = default;
            TextMessage(const string& text)
                : _text(text)
            {
            }
            TextMessage(const uint16_t length, const TCHAR buffer[])
                : _text(buffer, length)
            {
            }

            TextMessage(const TextMessage&) = delete;
            TextMessage& operator=(const TextMessage&) = delete;

            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) override;

            const string& Data() const override {
                return (_text);
            }

        private:
            string _text;
        };

        /**
         * @brief Event class for telemetry messages that can carry typed values.
         *
         *        Supports the following ValueType values:
         *        - TEXT:    a formatted string (default, backward-compatible)
         *        - INT8:    an 8-bit signed integer
         *        - UINT8:   an 8-bit unsigned integer
         *        - INT16:   a 16-bit signed integer
         *        - UINT16:  a 16-bit unsigned integer
         *        - INT32:   a 32-bit signed integer
         *        - UINT32:  a 32-bit unsigned integer
         *        - INT64:   a 64-bit signed integer
         *        - UINT64:  a 64-bit unsigned integer
         *        - FLOAT32: a single-precision floating-point value
         *        - FLOAT64: a double-precision floating-point value
         *
         *        The Data() method always returns a string representation for
         *        publishers that only understand text (Console, Syslog, etc.).
         *        Consumers that support typed telemetry can inspect Type() to
         *        forward or process the native value without precision loss.
         *
         *        Serialization format:
         *          [1 byte: ValueType]  then:
         *            TEXT:      [null-terminated string]
         *            INT8:      [1 byte]
         *            UINT8:     [1 byte]
         *            INT16:     [2 bytes]
         *            UINT16:    [2 bytes]
         *            INT32:     [4 bytes]
         *            UINT32:    [4 bytes]
         *            INT64:     [8 bytes]
         *            UINT64:    [8 bytes]
         *            FLOAT32:   [4 bytes, IEEE 754]
         *            FLOAT64:   [8 bytes, IEEE 754]
         */
        class EXTERNAL TelemetryMessage : public Core::Messaging::IEvent {
        public:
            enum class ValueType : uint8_t {
                TEXT    = 0,
                INT8    = 1,
                UINT8   = 2,
                INT16   = 3,
                UINT16  = 4,
                INT32   = 5,
                UINT32  = 6,
                INT64   = 7,
                UINT64  = 8,
                FLOAT32 = 9,
                FLOAT64 = 10
            };

            TelemetryMessage()
                : _type(ValueType::TEXT)
                , _text()
                , _numericValue{}
            {
            }

            explicit TelemetryMessage(const string& text)
                : _type(ValueType::TEXT)
                , _text(text)
                , _numericValue{}
            {
            }

            // Signed integral types
            template<typename T, std::enable_if_t<std::is_integral_v<std::decay_t<T>> && std::is_signed_v<std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, bool>, int> = 0>
            explicit TelemetryMessage(const T value)
                : _type(SignedTag<sizeof(T)>())
                , _text()
                , _numericValue{}
            {
                _numericValue._signed = static_cast<int64_t>(value);
                Stringify(_numericValue._signed);
            }

            // Unsigned integral types
            template<typename T, std::enable_if_t<std::is_integral_v<std::decay_t<T>> && std::is_unsigned_v<std::decay_t<T>> && !std::is_same_v<std::decay_t<T>, bool>, int> = 0>
            explicit TelemetryMessage(const T value)
                : _type(UnsignedTag<sizeof(T)>())
                , _text()
                , _numericValue{}
            {
                _numericValue._unsigned = static_cast<uint64_t>(value);
                Stringify(_numericValue._unsigned);
            }

            explicit TelemetryMessage(const float value);
            explicit TelemetryMessage(const double value);

            TelemetryMessage(const TelemetryMessage&) = delete;
            TelemetryMessage& operator=(const TelemetryMessage&) = delete;

            uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override;
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t bufferSize) override;

            const string& Data() const override {
                return (_text);
            }

            ValueType Type() const {
                return (_type);
            }

            bool IsSigned() const {
                return (_type == ValueType::INT8  || _type == ValueType::INT16 ||
                        _type == ValueType::INT32 || _type == ValueType::INT64);
            }

            bool IsUnsigned() const {
                return (_type == ValueType::UINT8  || _type == ValueType::UINT16 ||
                        _type == ValueType::UINT32 || _type == ValueType::UINT64);
            }

            bool IsFloatingPoint() const {
                return (_type == ValueType::FLOAT32 || _type == ValueType::FLOAT64);
            }

            int64_t SignedValue() const {
                return (_numericValue._signed);
            }

            uint64_t UnsignedValue() const {
                return (_numericValue._unsigned);
            }

            float Float32Value() const {
                return (_numericValue._float);
            }

            double Float64Value() const {
                return (_numericValue._double);
            }

            const void* RawValue() const
            {
                const void* result = nullptr;

                if (_type == ValueType::TEXT) {
                    result = _text.c_str();
                } else {
                    result = &_numericValue;
                }

                return (result);
            }

        private:
            template<size_t N>
            static constexpr ValueType SignedTag()
            {
                static_assert(N == 1 || N == 2 || N == 4 || N == 8, "Unsupported signed integer size");

                return (N == 1) ? ValueType::INT8
                     : (N == 2) ? ValueType::INT16
                     : (N == 4) ? ValueType::INT32
                     :            ValueType::INT64;
            }

            template<size_t N>
            static constexpr ValueType UnsignedTag()
            {
                static_assert(N == 1 || N == 2 || N == 4 || N == 8, "Unsupported unsigned integer size");

                return (N == 1) ? ValueType::UINT8
                     : (N == 2) ? ValueType::UINT16
                     : (N == 4) ? ValueType::UINT32
                     :            ValueType::UINT64;
            }

            void Stringify(int64_t value);
            void Stringify(uint64_t value);

            ValueType _type;
            string _text;
            union NumericValue {
                int64_t  _signed;
                uint64_t _unsigned;
                float    _float;
                double   _double;
            } _numericValue;
        };

    }
}
}

#define DEFINE_MESSAGING_CATEGORY(BASECATEGORY, CATEGORY)   \
    class EXTERNAL CATEGORY : public BASECATEGORY {         \
    private:                                                \
        using BaseClass = BASECATEGORY;                     \
    public:                                                 \
        using BaseClass::BaseClass;                         \
        CATEGORY() = default;                               \
        ~CATEGORY() = default;                              \
        CATEGORY(const CATEGORY&) = delete;                 \
        CATEGORY& operator=(const CATEGORY&) = delete;      \
    };

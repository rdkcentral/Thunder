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

namespace Thunder {
namespace Core {

    class SInt24 {
    public:
        static constexpr uint8_t SizeOf = 3;
        static constexpr uint32_t Max = 0x7FFFFF;
        static constexpr int32_t Min = 0xFF800000;

        using InternalType = int32_t;

        SInt24()
            : _value(0)
        {
        }
        SInt24(const int32_t value)
            : _value(value)
        {
            bool overflow = ((static_cast<uint32_t>(value) >> 23) != 0) && ((static_cast<uint32_t>(value) >> 23) != 0x1FF);
            ASSERT(overflow == false);
            if (overflow == true) {
                _value = std::numeric_limits<int32_t>::max();
            }
        }
        // if it is an uint32_t it considered a 24 bit only filled value
        SInt24(const uint32_t value)
            : _value(value | (value & 0x800000 ? 0xFF000000 : 0))
        {
            bool overflow = ((static_cast<uint32_t>(value) >> 23) != 0) && ((static_cast<uint32_t>(value) >> 23) != 0x1);
            ASSERT(overflow == false);
            if (overflow == true) {
                _value = std::numeric_limits<int32_t>::max();
            }
        }
        SInt24(const SInt24&) = default;
        SInt24(SInt24&&) = default;
        ~SInt24() = default;

    public:
        SInt24& operator=(const SInt24& value) = default;
        SInt24& operator=(SInt24&& value) = default;
        SInt24& operator=(const int32_t value)
        {
            _value = value;
            bool overflow = ((static_cast<uint32_t>(value) >> 23) != 0) && ((static_cast<uint32_t>(value) >> 23) != 0x1FF);
            ASSERT(overflow == false);
            if (overflow == true) {
                _value = std::numeric_limits<int32_t>::max();
            }
            return (*this);
        }
        // if it is an uint32_t it considered a 24 bit only filled value
        SInt24& operator=(const uint32_t value)
        {
            _value = (value | (value & 0x800000 ? 0xFF000000 : 0));
            bool overflow = ((static_cast<uint32_t>(value) >> 23) != 0) && ((static_cast<uint32_t>(value) >> 23) != 0x1);
            ASSERT(overflow == false);
            if (overflow == true) {
                _value = std::numeric_limits<int32_t>::max();
            }
            return (*this);
        }
        operator int32_t() const
        {
            return (_value);
        }

        int32_t AsSInt24() const
        {
            return (_value & 0xFFFFFF);
        }

        bool Overflowed() const {
            return (_value == std::numeric_limits<int32_t>::max());
        }

    private:
        int32_t _value;
    };

    class UInt24 {
    public:
        static constexpr uint8_t SizeOf = 3;
        static constexpr uint32_t Max = 0xFFFFFF;
        static constexpr uint32_t Min = 0;

        using InternalType = uint32_t;

        UInt24()
            : _value(0)
        {
        }
        UInt24(const UInt24& value) = default;
        UInt24(UInt24&& value) = default;
        UInt24(const uint32_t value)
            : _value(value)
        {
            bool overflow = ((value >> 24) != 0);
            ASSERT(overflow == false);
            if (overflow == true) {
                _value = std::numeric_limits<int32_t>::max();
            }
        }
        ~UInt24() = default;

    public:
        UInt24& operator=(const UInt24& copy) = default;
        UInt24& operator=(UInt24&& move) = default;
        UInt24& operator=(const uint32_t value)
        {
            bool overflow = ((value >> 24) != 0);
            ASSERT(overflow == false);
            if (overflow == false) {
                _value = value;
            } else {
                _value = std::numeric_limits<int32_t>::max();
            }
            return (*this);
        }
        operator uint32_t() const
        {
            return (_value);
        }

        uint32_t AsUInt24() const // just to be consistent with SInt24
        {
            return (_value & 0xFFFFFF); // in debug assert should already have fired on assignment
        }

        bool Overflowed() const
        {
            return (_value == std::numeric_limits<int32_t>::max());
        }

    private:
        uint32_t _value;
    };

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


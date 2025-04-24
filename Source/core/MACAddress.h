 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Metrological
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
#include "Serialization.h"

namespace Thunder {

    namespace Core {
        class MACAddress {
        private:
            static constexpr TCHAR delimiter = ':';
            enum stage {
                HEX,
                HEX_DELIMITED,
                BASE64,
                FAILED
            };

        public:
            inline MACAddress() {
                ::memset(_address, 0xFF, sizeof(_address));
            }
            inline MACAddress(const TCHAR input[]) {
                uint8_t begin = 0;

                // Skip the whitespace in front...
                while ((input[begin] != '\0') && (::isspace(input[begin]))) {
                    begin++;
                }

                if (Convert(Analyze(&input[begin]), &input[begin]) == false) {
                    ::memset(_address, 0xFF, sizeof(_address));
                }
            }
            inline MACAddress(const uint8_t address[6]) {
                ::memcpy(_address, address, sizeof(_address));
            }
            inline MACAddress(MACAddress&& move) noexcept {
                ::memcpy(_address, move._address, sizeof(_address));
            }
            inline MACAddress(const MACAddress& copy) {
                ::memcpy(_address, copy._address, sizeof(_address));
            }
            inline ~MACAddress() = default;

            inline MACAddress& operator=(MACAddress&& rhs) noexcept {
                ::memcpy(_address, rhs._address, sizeof(_address));
                return (*this);
            }
            inline MACAddress& operator=(const MACAddress& rhs) {
                ::memcpy(_address, rhs._address, sizeof(_address));
                return (*this);
            }

            inline bool operator== (const MACAddress& rhs) const {
                return (::memcmp(rhs._address, _address, sizeof(_address)) == 0);
            }
            inline bool operator!= (const MACAddress& rhs) const {
                return (!operator==(rhs));
            }
            inline operator uint8_t* () {
                return (_address);
            }
            inline operator const uint8_t* () const {
                return (_address);
            }
            inline uint8_t& operator[] (const uint8_t index) {
                ASSERT(index < sizeof(_address));
                return (_address[index]);
            }
            inline const uint8_t& operator[] (const uint8_t index) const{
                ASSERT(index < sizeof(_address));
                return (_address[index]);
            }

        public:
            inline bool IsValid() const {
                uint8_t index = 0;
                while ((index < sizeof(_address)) && (_address[index] == 0xFF)) {
                    index++;
                }
                return (index != sizeof(_address));
            }
            inline string ToString() const {
                string output;

                for (uint8_t i = 0; i < sizeof(_address); i++) {
                    // Reason for the low-level approch is performance.
                    // In stead of using string operations, we know that each byte exists of 2 nibbles,
                    // lets just translate these nibbles to Hexadecimal numbers and add them to the output.
                    // This saves a setup of several string manipulation operations.
                    uint8_t highNibble = ((_address[i] & 0xF0) >> 4);
                    uint8_t lowNibble = (_address[i] & 0x0F);
                    if (i != 0) {
                        output += ':';
                    }
                    output += static_cast<char>(highNibble + (highNibble >= 10 ? ('a' - 10) : '0'));
                    output += static_cast<char>(lowNibble + (lowNibble >= 10 ? ('a' - 10) : '0'));
                }
                return(output);
            }
            inline uint8_t Length() const {
                return (sizeof(_address));
            }
            inline stage Analyze(const TCHAR* input) {
                stage state = stage::HEX;
                uint8_t index = 0;

                while ((input[index] != '\0') && (state != stage::FAILED)) {
                    TCHAR entry = input[index];
                    if (!::isxdigit(entry)) {
                        if (entry == delimiter) {
                            state = (state == stage::BASE64 ? stage::FAILED : stage::HEX_DELIMITED);
                        }
                        else if (((entry > 'f') && (entry <= 'z')) ||
                            ((entry > 'F') && (entry <= 'Z')) ||
                            (entry == '+') ||
                            (entry == '/') ||
                            (entry == '=')) {
                            // No need to check a..f, A..F or 0..9 as these are already caught by the isxdigit
                            state = (state == stage::HEX_DELIMITED ? stage::FAILED : stage::BASE64);
                        }
                    }
                    if (((state == stage::BASE64) && (index > 8)) ||
                        ((state == stage::HEX) && (index > 12)) ||
                        ((state == stage::HEX_DELIMITED) && (index > 17))) {
                        state = stage::FAILED;
                    }
                    else {
                        index++;
                    }
                }
                return (state);
            }
            inline bool Convert(const stage state, const TCHAR* input) {
                bool converted = false;
                // See if it is base64, length is than shorter than ((6 * 8) / 6) == 8 bytes bytes
                switch (state) {
                case stage::BASE64: {
                    uint32_t bufferSize = sizeof(_address);
                    Core::FromString(input, _address, bufferSize, nullptr);
                    converted = (bufferSize == sizeof(_address));
                    break;
                }
                case stage::HEX: {
                    converted = (Core::FromHexString(input, _address, sizeof(_address)) == sizeof(_address));
                    break;
                }
                case stage::HEX_DELIMITED: {
                    converted = (Core::FromHexString(input, _address, sizeof(_address), delimiter) == sizeof(_address));
                    break;
                }
                default:
                case stage::FAILED: {
                    break;
                }
                }
                return (converted);
            }

        private:
            uint8_t _address[6];
        };
    }
}

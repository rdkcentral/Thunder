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

#include "Module.h"

namespace WPEFramework {

namespace Bluetooth {

    class EXTERNAL UUID {
    private:
        static const uint8_t BASE[];

    public:
        UUID() {
            _uuid[0] = 0;
        }
        UUID(const uint16_t uuid)
        {
            _uuid[0] = 2;
            ::memcpy(&(_uuid[1]), BASE, sizeof(_uuid) - 5);
            _uuid[13] = (uuid & 0xFF);
            _uuid[14] = (uuid >> 8) & 0xFF;
            _uuid[15] = 0;
            _uuid[16] = 0;
        }
        explicit UUID(const uint8_t uuid[16])
        {
            ::memcpy(&(_uuid[1]), uuid, 16);

            // See if this contains the Base, cause than it can be a short...
            if ((::memcmp(BASE, uuid, 12) == 0) && (uuid[14] == 0) && (uuid[15] == 0)) {
                _uuid[0] = 2;
            }
            else {
                _uuid[0] = 16;
            }
        }
        explicit UUID(const string& uuidStr)
        {
            FromString(uuidStr);
        }
        UUID(const UUID& copy)
        {
            ::memcpy(_uuid, copy._uuid, sizeof(_uuid));
        }
        ~UUID()
        {
        }

        UUID& operator=(const UUID& rhs)
        {
            ::memcpy(_uuid, rhs._uuid, sizeof(_uuid));
            return (*this);
        }

    public:
        bool IsValid() const {
            return (_uuid[0] != 0);
        }
        uint16_t Short() const
        {
            ASSERT(_uuid[0] == 2);
            return ((_uuid[14] << 8) | _uuid[13]);
        }
        bool operator==(const UUID& rhs) const
        {
            return ((rhs._uuid[0] == _uuid[0]) &&
                    ((_uuid[0] == 2) ? ((rhs._uuid[13] == _uuid[13]) && (rhs._uuid[14] == _uuid[14])) :
                                       (::memcmp(_uuid, rhs._uuid, _uuid[0] + 1) == 0)));
        }
        bool operator!=(const UUID& rhs) const
        {
            return !(operator==(rhs));
        }
        bool operator==(const uint16_t shortUuid) const
        {
            return ((HasShort() == true) && (Short() == shortUuid));
        }
        bool operator!=(const uint16_t shortUuid) const
        {
            return !(operator==(shortUuid));
        }
        bool operator<(const UUID& rhs) const
        {
            for (uint8_t i = 16; i > 0; i--) {
                if (_uuid[i] != rhs._uuid[i]) {
                    return(_uuid[i] < rhs._uuid[i]);
                }
            }
            return (false);
        }
        bool HasShort() const
        {
            return (_uuid[0] == 2);
        }
        uint8_t Length() const
        {
            return (_uuid[0]);
        }
        const uint8_t* Data() const
        {
             return (_uuid[0] == 2 ? &(_uuid[13]) :  &(_uuid[1]));
        }
        string ToString(const bool full = false) const
        {
            static const TCHAR hexArray[] = "0123456789abcdef";

            uint8_t index = 0;
            string result;

            if ((HasShort() == false) || (full == true)) {
                result.resize(36);
                for (uint8_t byte = 12 + 4; byte > 12; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 10 + 2; byte > 10; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 8 + 2; byte > 8; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 6 + 2; byte > 6; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
                result[index++] = '-';
                for (uint8_t byte = 0 + 6; byte > 0; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
            }
            else {
                result.resize(4);

                for (uint8_t byte = 12 + 2; byte > 12; byte--) {
                    result[index++] = hexArray[_uuid[byte] >> 4];
                    result[index++] = hexArray[_uuid[byte] & 0xF];
                }
            }
            return (result);
        }
        bool FromString(const string& uuidStr)
        {
            if ((uuidStr.length() == 4) || (uuidStr.length() == ((16 * 2) + 4))) {
                uint8_t buf[16];
                const uint16_t size = uuidStr.length();
                uint8_t* p = (buf + sizeof(buf));
                int16_t idx = 0;

                if (size == 4) {
                    memcpy(buf, BASE, sizeof(buf));
                    p -= 2;
                }

                while (idx < size) {
                    if ((idx == 8) || (idx == 13) || (idx == 18) || (idx == 23)) {
                        if (uuidStr[idx] != '-') {
                            break;
                        } else {
                            idx++;
                        }
                    } else {
                        (*--p) = ((Core::FromHexDigits(uuidStr[idx]) << 4) | Core::FromHexDigits(uuidStr[idx + 1]));
                        idx += 2;
                    }
                }

                if (idx == size) {
                    (*this) = UUID(buf);
                    return true;
                }
            }

            return false;
        }

    private:
        uint8_t _uuid[17];
    };

} // namespace Bluetooth

}


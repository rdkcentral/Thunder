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
#include "URL.h"

namespace Thunder {
namespace Web {

    class EXTERNAL JSONWebToken {
    private:
        JSONWebToken() = delete;
        JSONWebToken(JSONWebToken&&) = delete;
        JSONWebToken(const JSONWebToken&) = delete;
        JSONWebToken& operator=(JSONWebToken&&) = delete;
        JSONWebToken& operator=(const JSONWebToken&) = delete;

    public:
        enum mode : uint8_t {
            SHA256
        };

        JSONWebToken(const mode type, const uint8_t length, const uint8_t key[]);
        ~JSONWebToken();

    public:
        uint16_t Encode(string& token, const uint16_t length, const uint8_t payload[]) const;
        uint16_t Decode(const string& token, const uint16_t maxLength, uint8_t payload[]) const;
        uint16_t PayloadLength(const string& token) const;

    private:
        bool ValidSignature(const mode type, const string& token) const;

    private:
        mode _mode;
        string _header; 
        string _key;
    };

} } // namespace Thunder::Web

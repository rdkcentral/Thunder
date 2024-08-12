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

#include "JSONWebToken.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Web::JSONWebToken::mode)

    { Web::JSONWebToken::mode::SHA256, _TXT("HS256") },

ENUM_CONVERSION_END(Web::JSONWebToken::mode)

namespace Web
{
    class EXTERNAL JSONWebData : public Core::JSON::Container {
    private:
        JSONWebData(const JSONWebData&);
        JSONWebData& operator=(const JSONWebData&);

    public:
        JSONWebData()
            : Core::JSON::Container()
            , Type()
            , Algorithm(JSONWebToken::mode::SHA256)
        {
            Add(_T("alg"), &Algorithm);
            Add(_T("typ"), &Type);
        }
        JSONWebData(const TCHAR data[], const uint16_t length)
            : Core::JSON::Container()
            , Type()
            , Algorithm(JSONWebToken::mode::SHA256)

        {
            Add(_T("alg"), &Algorithm);
            Add(_T("typ"), &Type);

            FromString(string(data, length));
        }
        ~JSONWebData()
        {
        }

    public:
        Core::JSON::String Type;
        Core::JSON::EnumType<JSONWebToken::mode> Algorithm;
    };

    JSONWebToken::JSONWebToken(const mode type, const uint8_t length, const uint8_t key[])
        : _mode(type)
        , _key(string(reinterpret_cast<const char*>(key), length))
    {
        Core::EnumerateType<mode> modeData(type);
        string sourceBuffer(_T("{\"alg\":\"") + string(modeData.Data()) + _T("\",\"typ\":\"JWT\"}"));
        uint16_t sourceLength = static_cast<uint16_t>(sourceBuffer.length());
        uint16_t destinationLength = (((sourceLength * 8) / 6) + 4) * sizeof(TCHAR);
        TCHAR* destinationBuffer = reinterpret_cast<TCHAR*>(ALLOCA(destinationLength * sizeof(TCHAR)));
        uint16_t convertedLength = Core::URL::Base64Encode(
            reinterpret_cast<const uint8_t*>(sourceBuffer.c_str()),
            sourceLength * sizeof(TCHAR),
            destinationBuffer,
            destinationLength / sizeof(TCHAR));
        ASSERT(convertedLength < destinationLength);
        _header = string(destinationBuffer, convertedLength);
    }
    JSONWebToken::~JSONWebToken()
    {
    }
    uint16_t JSONWebToken::Encode(string & token, const uint16_t length, const uint8_t payload[]) const
    {
        uint16_t destinationLength = (((length * 8) / 6) + 4) * sizeof(TCHAR);

        TCHAR* destinationBuffer = reinterpret_cast<TCHAR*>(ALLOCA(destinationLength * sizeof(TCHAR)));
        uint16_t convertedLength = Core::URL::Base64Encode(
            payload,
            length,
            destinationBuffer,
            destinationLength / sizeof(TCHAR));

        token = (_header + '.' + string(destinationBuffer, convertedLength));

        if (_mode == JSONWebToken::SHA256) {
            TCHAR signature[((Crypto::SHA256HMAC::Length * 8) / 6) + 4];
            Crypto::SHA256HMAC hash(_key);
            hash.Input(reinterpret_cast<const uint8_t*>(token.c_str()), static_cast<uint16_t>(token.length()));
            const uint8_t* inputSignature = hash.Result(); // 32 length
           
            convertedLength = Core::URL::Base64Encode(inputSignature, hash.Length, signature, sizeof(signature), false);
            token += '.' + string(signature, convertedLength);
        }

        ASSERT(token.length() < 0xFFFF);

        return (static_cast<uint16_t>(token.length()));
    }
    uint16_t JSONWebToken::Decode(const string& token, const uint16_t maxLength, uint8_t payload[]) const
    {
        uint16_t length = 0;

        // Check what method to use
        size_t pos = token.find_first_of('.');

        if (pos == string::npos) {
            length = ~0;
        } else {

            // Extract the header
            string header(token.substr(0, pos));
            TCHAR* output = reinterpret_cast<TCHAR*>(ALLOCA(header.length() * sizeof(TCHAR)));

            length = Core::URL::Base64Decode(
                header.c_str(),
                static_cast<uint16_t>(header.length()),
                reinterpret_cast<uint8_t*>(output),
                static_cast<uint16_t>(header.length() * sizeof(TCHAR)),
                nullptr);

            JSONWebData info(output, length);

            length = ~0;

            if ((info.Type.Value() == _T("JWT")) && (info.Algorithm.IsSet() == true) && (ValidSignature(info.Algorithm.Value(), token) == true)) {

                // Check if the Hash is correct..
                size_t sig_pos = token.find_last_of('.');

                if ( (sig_pos != string::npos) && (sig_pos > pos) ) {

                    length = static_cast<uint16_t>(sig_pos - pos);
                    // Oke, this is a valid frame, let extract the payload..
                    length = Core::URL::Base64Decode(token.substr(pos + 1).c_str(), length - 1, payload, maxLength, nullptr);
                }
            }
        }

        return (length);
    }

    bool JSONWebToken::ValidSignature(const mode type, const string& token) const 
    {
        bool result = false;

        // Check if the Hash is correct..
        size_t pos = token.find_last_of('.');

        if (pos != string::npos) {

            if (type == JSONWebToken::mode::SHA256) {
                // Now calculate what we think it should be..
                Crypto::SHA256HMAC hash(_key);

                // Extract the signature and convert it to a binary string.
                uint8_t signature[Crypto::SHA256HMAC::Length];
                if ((token.length() - pos - 1) == ((((8 * sizeof(signature)) + 5)/6))) {
                    if (Core::URL::Base64Decode(token.substr(pos + 1).c_str(), static_cast<uint16_t>(token.length() - pos - 1), signature, sizeof(signature), nullptr) == sizeof(signature)) {

                        hash.Input(reinterpret_cast<const uint8_t*>(token.substr(0, pos).c_str()), static_cast<uint16_t>(pos * sizeof(TCHAR)));
                        result = (::memcmp(hash.Result(), signature, sizeof(signature)) == 0);
                    }
                }
            }
        }

        return (result);
    }

    uint16_t JSONWebToken::PayloadLength(const string& token) const
    {
        uint16_t result = ~0;
        // Check if the Hash is correct..
        size_t first = token.find_first_of('.');

        if (first != string::npos) {
            size_t last = token.find_last_of('.');
            result = static_cast<uint16_t>((((last - first) * 6) + 6) / 8);
        }
        return (result);
    }
} } // namespace Thunder::Web

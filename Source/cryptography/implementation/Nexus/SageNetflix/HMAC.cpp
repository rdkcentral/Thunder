/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "../../../Module.h"

#include "../Administrator.h"
#include "TEE.h"


namespace Implementation {

namespace Platform {

namespace Netflix {

    class HMAC : public Platform::IHashImplementation {
    public:
        HMAC(const HMAC&) = delete;
        HMAC& operator=(const HMAC) = delete;
        HMAC() = delete;

        HMAC(const hash_type type, const uint32_t secretId)
            : _secretId(secretId)
            , _hashType(HashType(type))
            , _buffer()
        {
        }

        ~HMAC() override = default;

    public:
        uint32_t Ingest(const uint32_t length, const uint8_t data[]) override
        {
            uint32_t ingested = 0;

            if (length > 0) {
                _buffer.append(reinterpret_cast<const char*>(data), length);
                ingested = length;
            }

            return (ingested);
        }

        uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) override
        {
            uint32_t outLength = 0;
            uint8_t result = 0;

            switch (_hashType) {
                case TEE::hashtype::SHA256:
                    outLength = 32;
                    break;
                default:
                    ASSERT(false && "Invalid hash type");
            }

            if ((outLength > 0) && (maxLength >= static_cast<uint8_t>(outLength))) {

                uint32_t rc = TEE::Instance().HMAC(_secretId, _hashType,
                                                   reinterpret_cast<const uint8_t*>(_buffer.data()), _buffer.length(),
                                                   data, outLength);
                if (rc != 0) {
                    TRACE_L1(_T("NetflixTA: HMAC() failed [0x%08x]"), rc);
                } else {
                    result = static_cast<uint8_t>(outLength);
                }

                ASSERT(outLength <= 64);
            }

            return (result);
        }

        static TEE::hashtype HashType(const hash_type type)
        {
            TEE::hashtype teeType = TEE::hashtype::UNDEFINED;

            switch (type) {
            case hash_type::HASH_TYPE_SHA256:
                teeType = TEE::hashtype::SHA256;
                break;
            default:
                break;
            }

            return (teeType);
        }

    private:
        uint32_t _secretId;
        TEE::hashtype _hashType;
        std::string _buffer;
    }; // class HMAC

    class HMACFactory : public HMACFactoryType<TEE::ID, HMAC> {
        Platform::IHashImplementation* Create(const hash_type type, const uint32_t secret_id) override
        {
            Platform::IHashImplementation* impl = nullptr;

            if (HMAC::HashType(type) != TEE::hashtype::UNDEFINED) {
                impl = new HMAC(type, secret_id);
                ASSERT(impl != nullptr);
            } else {
                TRACE_L1(_T("NetflixTA: Hash type not supported for HMAC [%i]"), type);
            }

            return (impl);
        }
    }; // class HMACFactory

} // namespace Netflix


static PlatformRegistrationType<IHMACFactory, Netflix::HMACFactory> registration;

} // namespace Platform

} // namespace Implementation

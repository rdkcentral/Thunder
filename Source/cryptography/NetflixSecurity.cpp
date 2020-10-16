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

#include "Module.h"
#include <INetflixSecurity.h>

#include "implementation/netflix_security_implementation.h"


namespace WPEFramework {

namespace Implementation {

    class NetflixSecurity : public Cryptography::INetflixSecurity  {
    public:
        NetflixSecurity(const NetflixSecurity&) = delete;
        NetflixSecurity& operator=(const NetflixSecurity&) = delete;
        NetflixSecurity() = default;
        ~NetflixSecurity() = default;

    public:
        std::string ESN() const override
        {
            std::string esn;

            uint8_t length = netflix_security_esn(0, nullptr);
            if (length != 0) {
                uint8_t* buffer = reinterpret_cast<uint8_t*>(ALLOCA(length));
                ASSERT(buffer != nullptr);

                if (netflix_security_esn(length, buffer) != 0) {
                    esn = std::string(reinterpret_cast<char*>(buffer), length);
                }
            }

            return (esn);
        }

        uint32_t EncryptionKey() const override
        {
            return (netflix_security_encryption_key());
        }

        uint32_t HMACKey() const override
        {
            return (netflix_security_hmac_key());
        }

        uint32_t WrappingKey() const override
        {
            return (netflix_security_wrapping_key());
        }

        uint32_t DeriveKeys(const uint32_t privateDhKeyId, const uint32_t peerPublicDhKeyId, const uint32_t derivationKeyId,
                            uint32_t& encryptionKeyId, uint32_t& hmacKeyId, uint32_t& wrappingKeyId) override
        {
            return (netflix_security_derive_keys(privateDhKeyId, peerPublicDhKeyId, derivationKeyId,
                                                 &encryptionKeyId, &hmacKeyId, &wrappingKeyId));
        }

    public:
        BEGIN_INTERFACE_MAP(NetflixSecurity)
        INTERFACE_ENTRY(Cryptography::INetflixSecurity)
        END_INTERFACE_MAP
    }; // class NetflixSecurity

} // namespace Implementation

/* static */ Cryptography::INetflixSecurity* Cryptography::INetflixSecurity::Instance()
{
    return (Core::Service<Implementation::NetflixSecurity>::Create<Cryptography::INetflixSecurity>());
}

}

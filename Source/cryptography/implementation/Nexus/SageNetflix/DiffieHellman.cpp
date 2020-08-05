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

    class DiffieHellman : public Platform::IDiffieHellmanImplementation {
    private:
        DiffieHellman(const DiffieHellman&) = delete;
        DiffieHellman& operator=(const DiffieHellman) = delete;
        DiffieHellman() = default;
        ~DiffieHellman() = default;

    public:
        static DiffieHellman& Instance()
        {
            static DiffieHellman instance;
            return (instance);
        }

        uint32_t Generate(const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                          uint32_t& private_key_id, uint32_t& public_key_id) override
        {
            uint32_t result = -1;

            ASSERT(generator > 1);
            ASSERT(modulusSize != 0);
            ASSERT(modulus != nullptr);

            public_key_id = 0;

            uint32_t rc = TEE::Instance().DHGenerateKeys(&generator, sizeof(uint8_t), modulus, modulusSize, private_key_id);
            if (rc != 0) {
                TRACE_L1(_T("NetflixTA: DHGenerateKeys() failed [0x%08x]"), rc);
            } else if (private_key_id != 0) {
                uint32_t publicKeySize = TEE::MAX_CLEAR_KEY_SIZE;
                uint8_t* publicKey = reinterpret_cast<uint8_t*>(ALLOCA(publicKeySize));
                ASSERT(publicKey != nullptr);

                // DH key pair is produced as one key in the TEE, so put the public key part in a separate blob.
                rc = TEE::Instance().ExportClearKey(private_key_id, TEE::keyformat::RAW, publicKey, publicKeySize);
                if (rc != 0) {
                    TRACE_L1(_T("NetflixTA: ExportClearKey() failed [0x%08x]"), rc);
                } else {
                    TEE::keytype type;

                    rc = TEE::Instance().ImportClearKey(publicKey, publicKeySize, TEE::keyformat::RAW, 0, 0xC, public_key_id, type);
                    if (rc != 0) {
                        TRACE_L1(_T("NetflixTA: ImportClearKey() failed [0x%08x]"), rc);
                    } else if (private_key_id != 0) {
                        result = 0;
                    }
                }
            }

            return (result);
        }

        uint32_t Derive(const uint32_t /* privateKey */, const uint32_t /* peerPublicKeyId */, uint32_t& /* secretId */) override
        {
            TRACE_L1(_T("NetflixTA: Derive() not implemented"));

            return (-1);
        }
    }; // class DiffieHellman

    class DHFactory : public DHFactoryType<TEE::ID, DiffieHellman> {
        Platform::IDiffieHellmanImplementation* Create() override
        {
            return (&DiffieHellman::Instance());
        }
    }; // class DHFactory

} // namespace Netflix


static PlatformRegistrationType<IDHFactory, Netflix::DHFactory> registration;

} // namespace Platform

} // namespace Implementation

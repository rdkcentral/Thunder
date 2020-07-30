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

    class AESCBCCipher : public Platform::ICipherImplementation {
    public:
        AESCBCCipher(const AESCBCCipher&) = delete;
        AESCBCCipher& operator=(const AESCBCCipher) = delete;
        AESCBCCipher() = delete;

        AESCBCCipher(const uint32_t keyId)
            : _keyId(keyId)
        {
        }

        ~AESCBCCipher() override = default;

    public:

        uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                        const uint32_t inputLength, const uint8_t input[],
                        const uint32_t maxOutputLength, uint8_t output[]) const override
        {
            return (Operation(TEE::cryptop::ENCRYPT, ivLength, iv, inputLength, input, maxOutputLength, output));
        }

        uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                        const uint32_t inputLength, const uint8_t input[],
                        const uint32_t maxOutputLength, uint8_t output[]) const override
        {
            return (Operation(TEE::cryptop::DECRYPT, ivLength, iv, inputLength, input, maxOutputLength, output));
        }

    private:
        uint32_t Operation(TEE::cryptop operation,
                           const uint8_t ivLength, const uint8_t iv[],
                           const uint32_t inputLength, const uint8_t input[],
                           const uint32_t maxOutputLength, uint8_t output[]) const
        {
            uint32_t result = -1;

            /* To accommodate PKCS5 padding; however a bigger buffer also appears to be needed during decryption. */
            if (((operation == TEE::cryptop::DECRYPT) && (maxOutputLength >= inputLength))
                || ((operation == TEE::cryptop::ENCRYPT) && (maxOutputLength >= (inputLength + 32 - (inputLength % 16))))) {
                uint32_t outputLength = maxOutputLength;

                uint32_t rc = TEE::Instance().AESCBC(_keyId, operation, iv, ivLength,
                                                     input, inputLength, output, outputLength);
                if (rc != 0) {
                    TRACE_L1(_T("NetflixTA: AESCBC() failed [0x%08x]"), rc);
                } else {
                    result = outputLength;
                }
            } else {
                TRACE_L1(_T("NetflixTA: Too small output buffer size [%i]"), maxOutputLength);
            }

            return (result);
        }

    private:
        uint32_t _keyId;
    }; // class AESCBCCipher

    class AESFactory : public AESFactoryType<TEE::ID, AESCBCCipher> {
        Platform::ICipherImplementation* Create(const aes_mode mode, const uint32_t secret_id) override
        {
            Platform::ICipherImplementation* impl = nullptr;

            if (mode == aes_mode::AES_MODE_CBC) {
                impl =  new AESCBCCipher(secret_id);
                ASSERT(impl != nullptr);
            } else {
                TRACE_L1(_T("NetflixTA: AES block cipher mode not supported [%i]"), mode);
            }

            return (impl);
        }
    }; // class AESFactory

} // namespace Netflix


static PlatformRegistrationType<IAESFactory, Netflix::AESFactory> registration;

} // namespace Platform

} // namespace Implementation


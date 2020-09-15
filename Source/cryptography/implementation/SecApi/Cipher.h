/*f not stated otherwise in this file or this component's LICENSE file the
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


#include <cipher_implementation.h>
#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

 /*Secapi headers */
#include <sec_security.h>
#include <sec_security_utils.h>

#include "Vault.h"
#include "../../Module.h"


struct CipherImplementation {
    virtual uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual ~CipherImplementation() { }
};


namespace Implementation {


    class Cipher : public CipherImplementation {

    public:

        Cipher(const Cipher&) = delete;
        Cipher& operator=(const Cipher) = delete;
        Cipher(const Implementation::Vault* vault, const Sec_CipherAlgorithm algorithm, const uint32_t keyId, const uint8_t keyLength, const uint8_t ivLength);
        ~Cipher() override;

    public:

        uint32_t Operation(bool encrypt, const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const;

        const Sec_CipherAlgorithm AESCipher(const aes_mode mode);
        uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const override;

        uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const override;

    private:

        const Implementation::Vault* _vault;
        uint32_t _keyId;
        uint8_t _keyLength;
        uint8_t _ivLength;
        const Sec_CipherAlgorithm _algorithm;

    };

    class CipherNetflix : public CipherImplementation {

    public:

        CipherNetflix(const Implementation::VaultNetflix* vaultNetflix, const uint32_t keyId);
        ~CipherNetflix()override;

    private:

        const Implementation::VaultNetflix* _vaultNetflix;
        Sec_NetflixHandle* _netflixHandle;
        uint32_t _keyId;

    public:

        const Sec_CipherAlgorithm AESCipher(const aes_mode mode);
        uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const override;

        uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const override;

        uint32_t Operation(bool encrypt, const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
            const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const;
    };

    //implementation
}

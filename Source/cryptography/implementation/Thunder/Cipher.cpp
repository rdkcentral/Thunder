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

#include "../../Module.h"

#include <ICryptographic.h>
#include <cipher_implementation.h>

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

#include "Vault.h"


struct CryptImplementation {
    virtual uint32_t Operation(const uint8_t ivLength, const uint8_t iv[],
                               const uint32_t inputLength, const uint8_t input[],
                               const uint32_t maxOutputLength, uint8_t output[]) = 0;

    virtual ~CryptImplementation() { }
};


namespace Implementation {

namespace Operation {

    // Wrappers to get around different method names for encryption/decryption.

    struct Encrypt {
        typedef WPEFramework::Crypto::AESEncryption Implementation;
        static uint32_t Operation(Implementation& impl, const uint32_t length, const uint8_t input[], uint8_t output[]) {
            return (impl.Encrypt(length, input, output));
        }
    };

    struct Decrypt {
        typedef WPEFramework::Crypto::AESDecryption Implementation;
        static uint32_t Operation(Implementation& impl, const uint32_t length, const uint8_t input[], uint8_t output[]) {
            return (impl.Decrypt(length, input, output));
        }
    };

} // namespace Operation


template<typename OPERATION>
class AESCryptor : public CryptImplementation {
    const uint8_t IV_LENGTH = 16;

public:
    AESCryptor(const AESCryptor<OPERATION>&) = delete;
    AESCryptor& operator=(const AESCryptor<OPERATION>) = delete;
    AESCryptor() = delete;

    AESCryptor(WPEFramework::Crypto::aesType blockMode, const uint32_t keyId)
        : _cryptor(blockMode)
        , _keyId(keyId)
    {
    }

    ~AESCryptor() override = default;

public:
    uint32_t Operation(const uint8_t ivLength, const uint8_t iv[],
                       const uint32_t inputLength, const uint8_t input[],
                       const uint32_t maxOutputLength, uint8_t output[]) override
    {
        uint32_t result = 0;

        ASSERT(iv != nullptr);
        ASSERT(input != nullptr);
        ASSERT(inputLength != 0);

        uint16_t keySize = Implementation::Vault::Instance().Size(_keyId, true);
        if (keySize == 0) {
            TRACE_L1(_T("Failed to retrieve key id 0x%08x"), _keyId);
        } else if (ivLength != IV_LENGTH) {
            TRACE_L1(_T("Invalid IV length: %i"), ivLength);
        } else if (maxOutputLength < inputLength) {
            TRACE_L1(_T("Output buffer too small, need  %i bytes"), inputLength);
            result = (-inputLength);
        } else {
            _cryptor.InitialVector(iv);

            uint8_t* key = reinterpret_cast<uint8_t*>(ALLOCA(keySize));
            ASSERT(key != nullptr);

            keySize = Implementation::Vault::Instance().Export(_keyId, keySize, key, true);
            ASSERT(keySize != 0);

            if (keySize != 0) {
                _cryptor.Key(keySize, key);
                ::memset(key, 0xFF, keySize); // shred :)

                result = OPERATION::Operation(_cryptor, inputLength, input, output);
                if (result != 0) {
                    TRACE_L1(_T("Operation() failed: %i"), result);
                } else {
                    TRACE_L2(_T("Succesfuly AES en/de-crypted %i bytes to %i bytes"), inputLength, inputLength);
                    result = inputLength;
                }
            }
        }

        return (result);
    }

private:
    typename OPERATION::Implementation _cryptor;
    uint32_t _keyId;
};

template<typename OPERATION>
CryptImplementation* CreateCryptor(const cipher_algorithm algorithm, const cipher_mode mode, const uint32_t keyId)
{
    CryptImplementation* crypt = nullptr;

    auto AESBlockMode = [](const cipher_mode mode, WPEFramework::Crypto::aesType aesType) -> bool {
        bool converted = false;
        switch (mode) {
        case cipher_mode::CIPHER_MODE_ECB:
            aesType = WPEFramework::Crypto::aesType::AES_ECB;
            converted = true;
            break;
        case cipher_mode::CIPHER_MODE_CBC:
            aesType = WPEFramework::Crypto::aesType::AES_CBC;
            converted = true;
            break;
        case cipher_mode::CIPHER_MODE_OFB:
            aesType = WPEFramework::Crypto::aesType::AES_CBC;
            converted = true;
            break;
        case cipher_mode::CIPHER_MODE_CFB8:
            aesType = WPEFramework::Crypto::aesType::AES_CFB8;
            converted = true;
            break;
        case cipher_mode::CIPHER_MODE_CFB128:
            aesType = WPEFramework::Crypto::aesType::AES_CFB128;
            converted = true;
            break;
        default:
            TRACE_L1(_T("Cipher block mode %i not supported"), mode);
            break;
        }
        return (converted);
    };

    switch (algorithm) {
    case cipher_algorithm::CIPHER_ALGORITHM_AES: {
        WPEFramework::Crypto::aesType aesType = WPEFramework::Crypto::aesType::AES_ECB;
        if (AESBlockMode(mode, aesType) == true) {
            crypt = new AESCryptor<OPERATION>(aesType, keyId);
        }
        break;
    }
    default:
        TRACE_L1(_T("Cipher algorithm %i not supported"), algorithm);
        break;
    }

    return (crypt);
}

} // namespace Implementation


extern "C" {

uint32_t cipher_generate(const cipher_algorithm algorithm, const uint16_t length_bits, uint32_t* private_key_id, uint32_t* public_key_id)
{
    TRACE_L1(_T("cipher_generate() not implemented"));

    return (-1);
}

uint32_t cipher_dh_derive(const uint32_t private_key_id, const uint32_t peer_public_key_id, uint32_t* secret_id)
{
    TRACE_L1(_T("cipher_dh_derive() not implemented"));

    return (-1);
}

uint32_t cipher_dh_authenticated_derive(const uint32_t private_key_id, const uint32_t peer_public_key_id, const uint32_t derivation_key_id,
                                        uint32_t* encryption_key_id, uint32_t* hmac_key_id, uint32_t* wrapping_key_id)
{
    TRACE_L1(_T("cipher_dh_authenticated_derive() not implemented"));

    return (-1);
}

struct CryptImplementation* cipher_create_encryptor(const cipher_algorithm algorithm, const cipher_mode mode, const uint32_t key_id)
{
    return (Implementation::CreateCryptor<Implementation::Operation::Encrypt>(algorithm, mode, key_id));
}

struct CryptImplementation* cipher_create_decryptor(const cipher_algorithm algorithm, const cipher_mode mode, const uint32_t key_id)
{
    return (Implementation::CreateCryptor<Implementation::Operation::Decrypt>(algorithm, mode, key_id));
}

void cipher_destroy(struct CryptImplementation* crypt)
{
    ASSERT(crypt != nullptr);

    delete crypt;
}

uint32_t cipher_operation(struct CryptImplementation* crypt, const uint8_t iv_length, const uint8_t iv[],
                          const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[])
{
    ASSERT(crypt != nullptr);

    return (crypt->Operation(iv_length, iv, input_length, input, max_output_length, output));
}

} // extern "C"

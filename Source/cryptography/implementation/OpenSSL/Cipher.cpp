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

#include <cipher_implementation.h>

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

#include <limits.h>

#include "Vault.h"


struct CipherImplementation {
    virtual uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                             const uint32_t inputLength, const uint8_t input[],
                             const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                             const uint32_t inputLength, const uint8_t input[],
                             const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual ~CipherImplementation() { }
};


namespace Implementation {

class Cipher : public CipherImplementation {
public:
    Cipher(const Cipher&) = delete;
    Cipher& operator=(const Cipher) = delete;
    Cipher() = delete;

    Cipher(const Implementation::Vault* vault, const EVP_CIPHER *cipher, const uint32_t keyId, const uint8_t keyLength, const uint8_t ivLength)
        : _context(nullptr)
        , _vault(vault)
        , _cipher(cipher)
        , _keyId(keyId)
        , _keyLength(keyLength)
        , _ivLength(ivLength)
    {
        ASSERT(vault != nullptr);
        ASSERT(cipher != nullptr);
        ASSERT(keyId != 0);
        ASSERT(keyLength != 0);
        ASSERT(ivLength != 0);

        _context = EVP_CIPHER_CTX_new();
        ASSERT(_context != nullptr);
    }

    ~Cipher() override
    {
        if (_context != nullptr) {
            EVP_CIPHER_CTX_free(_context);
        }
    }

    uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                     const uint32_t inputLength, const uint8_t input[],
                     const uint32_t maxOutputLength, uint8_t output[]) const override
    {
        return (Operation(true, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

    uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                     const uint32_t inputLength, const uint8_t input[],
                     const uint32_t maxOutputLength, uint8_t output[]) const override
    {
        return (Operation(false, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

private:
    uint32_t Operation(bool encrypt,
                       const uint8_t ivLength, const uint8_t iv[],
                       const uint32_t inputLength, const uint8_t input[],
                       const uint32_t maxOutputLength, uint8_t output[]) const
    {
        uint32_t result = 0;

        ASSERT(iv != nullptr);
        ASSERT(ivLength != 0);
        ASSERT(input != nullptr);
        ASSERT(inputLength != 0);

        if (ivLength != _ivLength) {
            TRACE_L1(_T("Invalid IV length! [%i]"), ivLength);
        } else if (maxOutputLength < inputLength) {
            // Note: Pitfall, AES CBC/ECB will use padding
            TRACE_L1(_T("Too small output buffer, expected: %i bytes"), inputLength);
            result = (-inputLength);
        } else {
            uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(_keyLength));
            ASSERT(keyBuf != nullptr);

            uint16_t length = _vault->Export(_keyId, _keyLength, keyBuf, true);
            ASSERT(length != 0);

            if (length != _keyLength) {
                TRACE_L1(_T("Failed to retrieve a valid encryption key from id 0x%08x"), _keyId);
            } else {
                int len = 0;
                int initResult = EVP_CipherInit_ex(_context, _cipher, nullptr, keyBuf, iv, encrypt);
                ::memset(keyBuf, 0xFF, length);

                if (initResult == 0) {
                    TRACE_L1(_T("EVP_CipherInit_ex() failed"));
                } else {
                    if (EVP_CipherUpdate(_context, output, &len, input, inputLength) == 0) {
                        TRACE_L1(_T("EVP_CipherUpdate() failed"));
                    } else {
                        result = len;
                        // Note: EVP_CipherFinal_ex() can still write to the output buffer!
                        if (EVP_CipherFinal_ex(_context, (output + len), &len) == 0) {
                            TRACE_L1(_T("EVP_CipherFinal_ex() failed"));
                            result = 0;
                        } else {
                            result += len;
                            TRACE_L2(_T("Completed %scryption, input size: %i, output size: %i"),
                                        (encrypt? "en" : "de"), inputLength, result);
                        }
                    }
                }
            }
        }

        return (result);
    }

private:
    EVP_CIPHER_CTX* _context;
    const Implementation::Vault* _vault;
    const EVP_CIPHER* _cipher;
    uint32_t _keyId;
    uint8_t _keyLength;
    uint8_t _ivLength;
};

const EVP_CIPHER* AESCipher(const uint8_t keySize, const aes_mode mode)
{
    const EVP_CIPHER* cipher = nullptr;

    typedef const EVP_CIPHER* (*cipherfn)(void);

    static const cipherfn cipherTable[][7] = {
        { EVP_aes_128_ecb, EVP_aes_128_cbc, EVP_aes_128_ofb, EVP_aes_128_cfb1, EVP_aes_128_cfb8, EVP_aes_128_cfb128, EVP_aes_128_ctr },
        { EVP_aes_192_ecb, EVP_aes_192_cbc, EVP_aes_192_ofb, EVP_aes_192_cfb1, EVP_aes_192_cfb8, EVP_aes_192_cfb128, EVP_aes_192_ctr },
        { EVP_aes_256_ecb, EVP_aes_256_cbc, EVP_aes_256_ofb, EVP_aes_256_cfb1, EVP_aes_256_cfb8, EVP_aes_256_cfb128, EVP_aes_256_ctr }
    };

    uint8_t idx = -1;
    switch (mode) {
    case aes_mode::AES_MODE_ECB:
        idx = 0;
        break;
    case aes_mode::AES_MODE_CBC:
        idx = 1;
        break;
    case aes_mode::AES_MODE_OFB:
        idx = 2;
        break;
    case aes_mode::AES_MODE_CFB1:
        idx = 3;
        break;
    case aes_mode::AES_MODE_CFB8:
        idx = 4;
        break;
    case aes_mode::AES_MODE_CFB128:
        idx = 5;
        break;
    case aes_mode::AES_MODE_CTR:
        idx = 6;
        break;
    default:
        TRACE_L1(_T("Unsupported AES cipher block mode %i"), mode);
    }

    if (idx != UCHAR_MAX) {
        if (keySize == 16) {
            cipher = cipherTable[0][idx]();
        } else if (keySize == 24) {
            cipher = cipherTable[1][idx]();
        } else if (keySize == 32) {
            cipher = cipherTable[2][idx]();
        } else {
            TRACE_L1(_T("Unsupported AES key size: %i bits"), (keySize * 8));
        }
    }

    return (cipher);
}

} // namespace Implementation


extern "C" {

struct CipherImplementation* cipher_create_aes(const struct VaultImplementation* vault , const aes_mode mode, const uint32_t key_id)
{
    ASSERT(vault != nullptr);

    CipherImplementation* cipher = nullptr;
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);

    uint16_t keyLength = vaultImpl->Size(key_id, true);
    if (keyLength == 0) {
        TRACE_L1(_T("Key 0x%08x does not exist"), key_id);
    } else {
        const EVP_CIPHER* evpcipher = Implementation::AESCipher(keyLength, mode);
        ASSERT(evpcipher != nullptr);
        if (evpcipher != nullptr) {
            cipher = new Implementation::Cipher(vaultImpl, evpcipher, key_id, keyLength, 16);
        }
    }

    return (cipher);
}

void cipher_destroy(struct CipherImplementation* cipher)
{
    ASSERT(cipher != nullptr);
    delete cipher;
}

uint32_t cipher_encrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[])
{
    ASSERT(cipher != nullptr);
    return (cipher->Encrypt(iv_length, iv, input_length, input, max_output_length, output));
}

uint32_t cipher_decrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[])
{
    ASSERT(cipher != nullptr);
    return (cipher->Decrypt(iv_length, iv, input_length, input, max_output_length, output));
}

} // extern "C"

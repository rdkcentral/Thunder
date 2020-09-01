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
#include "Cipher.h"
#include "../../Module.h"


namespace Implementation {

    /* COTR */
    Cipher::Cipher(const Implementation::Vault* vault, const Sec_CipherAlgorithm algorithm, const uint32_t keyId, const uint8_t keyLength, const uint8_t ivLength)
        : _vault(vault)
        , _keyId(keyId)
        , _keyLength(keyLength)
        , _ivLength(ivLength)
        , _algorithm(algorithm)
    {

        ASSERT(vault != nullptr);
        ASSERT(keyId != 0);
        ASSERT(keyLength != 0);
        ASSERT(ivLength != 0)

    }

    /* DOTR */
    Cipher::~Cipher()
    {

    }

    /*********************************************************************
     * @function Function Encrypt
     *
     * @brief   brief Used to encrypt a blob of data 
     *
     * @param[in] ivLength - Length of the iv value
     * @param[in] iv - intitialization vector
     * @param[in] inputLength - Length of input blob
     * @param[in] input -input blob of data
     * @param[in] maxOutputLength - max possible length of output buffer
     * @param[out] output - Encrypted output buffer
     *
     * @return Length of the bytesWritten as encrypted blob 
     *
     *********************************************************************/
    uint32_t Cipher::Encrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {
        return (Operation(true, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

    /*********************************************************************
     * @function Function Decrypt
     *
     * @brief   brief Used to decrypt an encrypted blob of data 
     *
     * @param[in] ivLength - Length of the iv value
     * @param[in] iv - intitialization vector
     * @param[in] inputLength - Length of input blob
     * @param[in] input -input blob of data
     * @param[in] maxOutputLength - max possible length of output buffer
     * @param[out] output - Decrypted output buffer
     *
     * @return Length of the bytesWritten as decrypted blob/data
     *
     *********************************************************************/
    uint32_t Cipher::Decrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {
        return (Operation(false, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

    /*********************************************************************
     * @function Function Operation
     *
     * @brief   brief Used to encrypt/decrypt data 
     *
     * @param[in] encrypt - mode :true for enc and false for decrypt
     * @param[in] ivLength - Length of the iv value
     * @param[in] iv - intitialization vector
     * @param[in] inputLength - Length of input blob
     * @param[in] input -input blob of data
     * @param[in] maxOutputLength - max possible length of output buffer
     * @param[out] output - Decrypted output buffer
     *
     * @return Length of the bytesWritten as encrypted/decrypted blob/data
     *
     *********************************************************************/
    uint32_t Cipher::Operation(bool encrypt, const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {
        SEC_SIZE OutputLength = 0;
        ASSERT(iv != nullptr);
        ASSERT(ivLength != 0);
        ASSERT(input != nullptr);
        ASSERT(inputLength != 0);

        if (ivLength != _ivLength) {
            TRACE_L1(_T("SEC: Invalid IV length! [%i]"), ivLength);
        }
        else if (maxOutputLength < inputLength) {
            // Note: Pitfall, AES CBC/ECB will use padding
            TRACE_L1(_T("Too small output buffer, expected: %i bytes"), inputLength);
            OutputLength = (-inputLength);
        }
        else {
            Sec_CipherHandle* cipher_handle = NULL;
            Sec_Result sec_res = SEC_RESULT_FAILURE;
            Sec_KeyHandle* sec_key;
            SEC_OBJECTID id_sec;
            Sec_CipherMode mode_val = SEC_CIPHERMODE_DECRYPT;
            IdStore* ids;

            uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(sizeof(ids)));
            ASSERT(keyBuf != nullptr);

            uint16_t length = _vault->Export(_keyId, _keyLength, keyBuf, true);
            ASSERT(length != 0);
            if (length != _keyLength) {
                TRACE_L1(_T("SEC: Failed to retrieve a valid encryption key from id 0x%08x"), _keyId);
            }
            else {
                // Perform cipher operation 
                if (_vault->getSecProcHandle() != nullptr) {
                    std::memcpy(&ids, keyBuf, sizeof(ids));
                    ASSERT(ids->idAes != 0);
                    id_sec = ids->idAes;

                    TRACE_L2(_T("SEC : the object id sec from export is %llu \n"), id_sec);
                    Sec_Result sec_result = SecKey_GetInstance(_vault->getSecProcHandle(), id_sec, &sec_key);
                    if (sec_key != nullptr && sec_result == SEC_RESULT_SUCCESS) {
                        if (encrypt) {
                            mode_val = SEC_CIPHERMODE_ENCRYPT;
                        }
                        SEC_BYTE* iv_data = const_cast<SEC_BYTE*>(iv);
                        sec_res = SecCipher_GetInstance(_vault->getSecProcHandle(), _algorithm, mode_val, sec_key, iv_data, &cipher_handle);
                        if (sec_res != SEC_RESULT_SUCCESS || cipher_handle == NULL) {
                            TRACE_L1(_T("SEC:cipher handle not created retVal = %d and cipher handle =%p \n"),sec_res,cipher_handle);
                            SecKey_Release(sec_key);
                            return 0;
                        }
                        //Processes data with the speciﬁed cipher and mode
                        SEC_BYTE* input_data = const_cast<SEC_BYTE*>(input);
                        sec_res = SecCipher_Process(cipher_handle, input_data, inputLength, SEC_TRUE, output, maxOutputLength, &OutputLength);
                        if (sec_res != SEC_RESULT_SUCCESS) {
                            TRACE_L1(_T("SEC SecCipher_Process failed retVal = %d \n"),sec_res);
                        }
                        else {
                            TRACE_L2(_T("SEC: Completed %scryption, input size: %i, output size: %i"), (encrypt ? "en" : "de"), inputLength, OutputLength);
                        }
                        //release cipher handle
                        SecCipher_Release(cipher_handle);
                        SecKey_Release(sec_key);
                    }
                    else {
                        TRACE_L1(_T("SEC: Key instance failed ,retVal = %d \n"),sec_result);
                    }
                }
                else { 
                    TRACE_L1(_T("SEC: Unable to have a valid secproc handle from vault \n"));
                }
            }
        }

        return (OutputLength);

    }

    /*********************************************************************
     * @function AESCipher
     *
     * @brief    choose the required cipher alogorithm
     *
     * @param[in] mode - mode of the aes type 
     *
     * @return Sec_CipherAlgorithm based on the mode
     *
     *********************************************************************/
    const Sec_CipherAlgorithm AESCipher(const aes_mode mode)
    {
        Sec_CipherAlgorithm algorithm = SEC_CIPHERALGORITHM_NUM;
        switch (mode) {
        case aes_mode::AES_MODE_ECB:
            //algorithm = SEC_CIPHERALGORITHM_AES_ECB_NO_PADDING;
            algorithm = SEC_CIPHERALGORITHM_AES_ECB_PKCS7_PADDING;
            break;
        case aes_mode::AES_MODE_CBC:
            // algorithm = SEC_CIPHERALGORITHM_AES_CBC_NO_PADDING;
            algorithm = SEC_CIPHERALGORITHM_AES_CBC_PKCS7_PADDING;
            break;
        case aes_mode::AES_MODE_CTR:
            algorithm = SEC_CIPHERALGORITHM_AES_CTR;
            break;
        case aes_mode::AES_MODE_OFB:
            TRACE_L1(_T("Unsupported AES cipher block mode"));
            break;
        case aes_mode::AES_MODE_CFB1:
            TRACE_L1(_T("Unsupported AES cipher block mode"));
            break;
        case aes_mode::AES_MODE_CFB8:
            TRACE_L1(_T("Unsupported AES cipher block mode"));
            break;
        case aes_mode::AES_MODE_CFB128:
            TRACE_L1(_T("Unsupported AES cipher block mode"));
            break;
        default:
            TRACE_L1(_T("not %i  implemented"), mode);
        }
        return (algorithm);
    }
} // namespace Implementation


extern "C" {

    struct CipherImplementation* cipher_create_aes(const struct VaultImplementation* vault, const aes_mode mode, const uint32_t key_id)
    {
        const struct CipherImplementation* cipher;
        CipherImplementation* implementation = nullptr;
        ASSERT(vault != nullptr);

        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_DEFAULT) {
            const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
            uint16_t keyLength = vaultImpl->Size(key_id, true);
            if (0 == keyLength) {
                TRACE_L1(_T("SEC :Key 0x%08x does not exist"), key_id);
            }
            else {
                /*Get the algorithm and mode from AESCipher() and create an instance with Implementation::Cipher*/
                const Sec_CipherAlgorithm cryptoAlg = Implementation::AESCipher(mode);
                implementation = new Implementation::Cipher(vaultImpl, cryptoAlg, key_id, keyLength, 16);
            }
        }
        else if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX)
        {
            const Implementation::VaultNetflix* vaultImplNetflix = reinterpret_cast<const Implementation::VaultNetflix*>(vault);
            uint16_t key_Length = vaultImplNetflix->Size(key_id, true);
            if (0 == key_Length) {
                TRACE_L1(_T("SEC :Key 0x%08x does not exist"), key_id);
            }
            else {
                implementation = new Implementation::CipherNetflix(vaultImplNetflix, key_id);
            }
        }
        return (implementation);
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


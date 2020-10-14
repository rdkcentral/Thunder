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

#define AES_128_KEY_SIZE         (16)
#define AES_128_BLOCK_SIZE       (16)
#define AES_CBC_IV_SIZE          (16)
#define HMAC_SHA256_OUTPUT_SIZE  (32)


namespace Implementation {
    /* COTR */
    CipherNetflix::CipherNetflix(const Implementation::VaultNetflix* vaultnetflix, const uint32_t keyId)
        : _vaultNetflix(vaultnetflix)
        , _keyId(keyId)
    {
        ASSERT(vaultnetflix != nullptr);
        ASSERT(keyId != 0);
    }

    CipherNetflix::~CipherNetflix()
    {

    }

    /*********************************************************************
     * @function Function Encrypt
     *
     * @brief   brief Used to encrypt a blob of data using sec netflix calls
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
    uint32_t CipherNetflix::Encrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {
        return (Operation(true, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

    /*********************************************************************
     * @function Function Decrypt
     *
     * @brief   brief Used to decrypt an encrypted blob of data using sec netflix calls
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
    uint32_t CipherNetflix::Decrypt(const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {
        return (Operation(false, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

    /*********************************************************************
     * @function Function Operation
     *
     * @brief   brief Used to encrypt/decrypt data using sec netflix calls
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
    uint32_t CipherNetflix::Operation(bool encrypt, const uint8_t ivLength, const uint8_t iv[], const uint32_t inputLength,
        const uint8_t input[], const uint32_t maxOutputLength, uint8_t output[]) const
    {

        uint32_t keyHandle;
        uint32_t retVal = -1;
        Sec_SocKeyHandle* pKey = NULL;
        SEC_SIZE            bytesWritten = 0;
        std::string outputbuf;

        if (inputLength % AES_128_BLOCK_SIZE != 0) {
            outputbuf.resize(inputLength + (AES_128_BLOCK_SIZE - (inputLength % AES_128_BLOCK_SIZE)));
            TRACE_L2(_T("SecNetflix_Aescbc adding pad to output buffer %d\n", outputbuf.size()));
        }
        else {
            outputbuf.resize(inputLength + AES_128_BLOCK_SIZE);
        }

        SEC_BYTE* out_buf = reinterpret_cast<uint8_t*>(&outputbuf[0]);
        retVal = _vaultNetflix->FindKey(_keyId, &pKey);
        if (retVal == RET_OK) {
            Sec_Result  result = SEC_RESULT_FAILURE;
            result = SecNetflix_Aescbc(_vaultNetflix->getNetflixHandle(), pKey, encrypt, iv, ivLength,
                input, inputLength, out_buf, outputbuf.size(), &bytesWritten);

            if (result != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_Aescbc FAILED : retVal = %d\n", result));
            }
        }
        else {
            TRACE_L1(_T("FindKey did not find key handle = %d\n", keyHandle));
        }

        TRACE_L2(_T("Encrypted message: encdatalen=%u\n", bytesWritten));
        memcpy(output, out_buf, bytesWritten);
        retVal = bytesWritten;

        return retVal;
    }

} // namespace Implementation


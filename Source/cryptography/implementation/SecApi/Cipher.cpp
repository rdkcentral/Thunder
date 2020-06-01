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

#include <cipher_implementation.h>
#include <core/core.h>
#include <cryptalgo/cryptalgo.h>
/*XRE-15569:Secapi headers */
#include <sec_security.h>
#include <sec_security_utils.h>

#include "Vault.h"
#include "../../Module.h"


struct CipherImplementation {
    virtual uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                             const uint32_t inputLength, const uint8_t input[],
                             const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                             const uint32_t inputLength, const uint8_t input[],
                             const uint32_t maxOutputLength, uint8_t output[]) const = 0;

    virtual ~CipherImplementation() { }
};

struct Crypto_Cipher
{
	Sec_CipherAlgorithm algorithm;
	Sec_CipherMode mode;
	uint16_t size;
};


namespace Implementation {

class Cipher : public CipherImplementation {
public:
    Cipher(const Cipher&) = delete;
    Cipher& operator=(const Cipher) = delete;
    Cipher() = delete;

    Cipher(const Implementation::Vault* vault,const Crypto_Cipher *cipher, const uint32_t keyId, const uint8_t keyLength, const uint8_t ivLength)
        : _vault(vault)
	, _cipher(cipher)
        , _keyId(keyId)
        , _keyLength(keyLength)
        , _ivLength(ivLength)
    {
        ASSERT(vault != nullptr);
        ASSERT(keyId != 0);
        ASSERT(keyLength != 0);
        ASSERT(ivLength != 0)
       
	TRACE_L1(_T("Constructor\n"));
    }

    ~Cipher() override
    {
        TRACE_L1(_T("Destructor\n"));
    }
	
	uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                     const uint32_t inputLength, const uint8_t input[],
                     const uint32_t maxOutputLength, uint8_t output[]) const override
    {
       
   	Sec_RandomHandle *random_handle = NULL;
        Sec_Result res;
        Sec_RandomAlgorithm  algorithm = SEC_RANDOMALGORITHM_TRUE;
        
        if (iv[0] != '\0')
        {
         TRACE_L1(_T("IV is not empty [%i]"), iv);
        }
        else
	    {
		/*XRE-15569:Obtain an instance of a random number generator.*/
			res = SecRandom_GetInstance(_vault->_secProcHandle, algorithm, &random_handle);
			if (res != SEC_RESULT_SUCCESS)
			{
				TRACE_L1("SecRandom_GetInstance failed");
			}
		/*XRE-15569:Generate random IV data */
			res = SecRandom_Process(random_handle, output,maxOutputLength);
			if (res != SEC_RESULT_SUCCESS)
			{
                TRACE_L1("SecRandom_Process failed\n");
			}
		/*XRE-15569:Release a random number generator handle*/
			SecRandom_Release(random_handle);
		}
        return (Operation(true, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

	uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                     const uint32_t inputLength, const uint8_t input[],
                     const uint32_t maxOutputLength, uint8_t output[]) const override
    {
       
	Sec_RandomHandle *random_handle = NULL;
	Sec_Result res;
	Sec_RandomAlgorithm  algorithm = SEC_RANDOMALGORITHM_TRUE;

        if (iv[0] != '\0')
        {
         TRACE_L1(_T("IV is not empty [%i]"), iv);
        }
        else
		{
	     /*XRE-15569:Obtain an instance of a random number generator.*/
             res = SecRandom_GetInstance(_vault->_secProcHandle, algorithm, &random_handle);
               if (res != SEC_RESULT_SUCCESS)
             {
               TRACE_L1("SecRandom_GetInstance failed");
             }

	    /*XRE-15569:Generate random IV data */
            res = SecRandom_Process(random_handle, output,maxOutputLength);
              if (res != SEC_RESULT_SUCCESS)
              {
                SEC_LOG_ERROR("SecRandom_Process failed\n");
              }
	/*XRE-15569:Release a random number generator handle*/
        SecRandom_Release(random_handle);
               }
        return (Operation(false, ivLength, iv, inputLength, input, maxOutputLength, output));
    }

private:
    uint32_t Operation(bool encrypt,
                       const uint8_t ivLength, const uint8_t iv[],
                       const uint32_t inputLength, const uint8_t input[],
                       const uint32_t maxOutputLength, uint8_t output[]) const
    {
        uint8_t result = 0;
        ASSERT(iv != nullptr);
        ASSERT(ivLength != 0);
        ASSERT(input != nullptr);
        ASSERT(inputLength != 0);
	    Sec_CipherHandle* cipher_handle=NULL;
        Sec_Result sec_res =SEC_RESULT_FAILURE;
        SEC_SIZE OutputLength = 0;

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
                   /*XRE-15569: Perform cipher operation */
		    sec_res = SecCipher_GetInstance(_vault->_secProcHandle,_cipher->algorithm, _cipher->mode,_vault->key_handle,
			      iv,&cipher_handle);
                    // ::memset(keyBuf, 0xFF, length); 
		      if (sec_res != SEC_RESULT_SUCCESS || cipher_handle==NULL)
                       {
                         TRACE_L1(_T("SecCipher_GetInstance failed"));
                           return 0;
                       }
		   /*XRE-15569:Processes data with the speciﬁed cipher and mode*/	
	            sec_res = SecCipher_Process(cipher_handle, input, inputLength, SEC_TRUE, output, maxOutputLength,
                                  &OutputLength);
                        if( sec_res!= SEC_RESULT_SUCCESS)
                                {
                                   TRACE_L1(_T("SecCipher_Process failed in Encrypt()"));
                                }
		   /*XRE-15569:release cipher handle */
		    SecCipher_Release(cipher_handle);
           
                   }
               }
        return (OutputLength);
    }

	
private:
    const Implementation::Vault* _vault;
    uint32_t _keyId;
    uint8_t _keyLength;
    uint8_t _ivLength;
    const Crypto_Cipher* _cipher;
};


/*XRE-15569:Mapping modes with cipher algorithm */
const Crypto_Cipher* AESCipher(const uint8_t keySize,const aes_mode mode)
{
   Crypto_Cipher* cipher = new Crypto_Cipher;
   
    switch (mode) {
    case aes_mode::AES_MODE_ECB:
	     cipher->algorithm = SEC_CIPHERALGORITHM_AES_ECB_NO_PADDING;
	     cipher->size = aes_mode::AES_MODE_ECB;
    break;	
    case aes_mode::AES_MODE_CBC:
         cipher->algorithm = SEC_CIPHERALGORITHM_AES_CBC_NO_PADDING;
	     cipher->size = aes_mode::AES_MODE_CBC;
    break;
    case aes_mode::AES_MODE_CTR:
         cipher->algorithm = SEC_CIPHERALGORITHM_AES_CTR;
         cipher->size = aes_mode::AES_MODE_CTR;
        break;
    case aes_mode::AES_MODE_OFB:
        TRACE_L1("Unsupported AES cipher block mode");
        break;
    case aes_mode::AES_MODE_CFB1:
        TRACE_L1("Unsupported AES cipher block mode");
        break;
    case aes_mode::AES_MODE_CFB8:
       TRACE_L1("Unsupported AES cipher block mode");
        break;
    case aes_mode::AES_MODE_CFB128:
        TRACE_L1("Unsupported AES cipher block mode");
        break;
    default:
        TRACE_L1(T_("not %i  implemented"),mode);
                }    
    return (cipher);
}
} // namespace Implementation


extern "C" {

struct CipherImplementation* cipher_create_aes(const struct VaultImplementation* vault, const aes_mode mode, const uint32_t key_id)
{
    ASSERT(vault != nullptr);

    CipherImplementation* cipher = nullptr;
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    uint16_t keyLength = vaultImpl->Size(key_id, true);
    if (keyLength == 0) {
        TRACE_L1(_T("Key 0x%08x does not exist"), key_id);
    } else {
       /*XRE-15569:Get the algorithm and mode from AESCipher() and create an instance with Implementation::Cipher*/
        const Crypto_Cipher* Cryptocipher = Implementation::AESCipher(keyLength, mode);   
        cipher = new Implementation::Cipher(vaultImpl, Cryptocipher, key_id, keyLength, 16);
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


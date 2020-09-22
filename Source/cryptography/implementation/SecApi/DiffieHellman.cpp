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

#include <diffiehellman_implementation.h>

#include "Vault.h"

namespace Implementation {
    /*********************************************************************
     * @function GenerateDiffieHellmanKeys
     *
     * @brief    Generate the DH  public-private key pair 
     *
     * @param[in] _vault  - VaultNetflix object
     * @param[in] generator - generator used by DH algorithm
     * @param[in] modulusSize -modulus size
     * @param[in] modulus - modulus array
     * @param[out] privateKeyId - id of the generated private key
     * @param[out] publicKeyId - id of the generated public key
     *
     * @return status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t GenerateDiffieHellmanKeys(Implementation::VaultNetflix* _vault, const uint8_t generator, 
        const uint16_t modulusSize, const uint8_t modulus[],uint32_t& privateKeyId, uint32_t& publicKeyId)
    {
        uint32_t result = -1;
        SEC_SIZE bytesWritten = 0;
        Sec_SocKeyHandle keyPublic;
        Sec_SocKeyHandle keyPrivate;
        uint8_t base_buf[sizeof(uint32_t)] = { 0 };

        ASSERT(generator >= 1);
        ASSERT(modulusSize != 0);
        ASSERT(modulus != nullptr);

        privateKeyId = 0;
        publicKeyId = 0;

        memcpy(base_buf, &generator, sizeof(uint8_t));
        Sec_Result  result_sec = SEC_RESULT_FAILURE;
        result_sec = SecNetflix_GenerateDHKeyPair(_vault->getNetflixHandle(), modulus, modulusSize, base_buf, sizeof(base_buf), &keyPublic, &keyPrivate);

        //convert the keypublic and keyprivate into a format that we can send to privateKeyId and publicKeyId
        //store in sockeymap
        if (result_sec == SEC_RESULT_SUCCESS) {
            privateKeyId = _vault->AddKey(keyPrivate);
            publicKeyId = _vault->AddKey(keyPublic);
            result = RET_OK;
        }
        else {
            TRACE_L1(_T("SEC :SecNetflix_GenerateDHKeyPair failed,retVal = %d \n"),result_sec);
        }

        return (result);
    }


    namespace Netflix {
        /*********************************************************************
         * @function  DiffieHellmanAuthenticatedDeriveSecret
         *
         * @brief Derive the DH secret (Netflix specific) 
         *
         * @param[in] _vault - VaultNetflix object
         * @param[in] privateKeyId -id of private key 
         * @param[in] peerPublicKeyId - id of peer public key 
         * @param[in] derivationKeyId -derivation key id
         * @param[out] encryptionKeyId -generated AES-128-CBC encryption key id
         * @param[out] hmacKeyId - generated HMAC-SHA256 hmac key id
         * @param[out] wrappingKeyId - generated AES-128  wrapping key id
         *
         * @return status success(0) or otherwise 
         *
         *********************************************************************/
        uint32_t DiffieHellmanAuthenticatedDeriveSecret(Implementation::VaultNetflix* _vault,
            const uint32_t privateKeyId, const uint32_t peerPublicKeyId, const uint32_t derivationKeyId,
            uint32_t& encryptionKeyId, uint32_t& hmacKeyId, uint32_t& wrappingKeyId)
        {
            uint32_t result = -1;
            Sec_SocKeyHandle* pKeyBase = NULL;
            Sec_SocKeyHandle* pKeyDerive = NULL;
            Sec_SocKeyHandle* pKeyPublic = NULL;
            Sec_SocKeyHandle  hKeyEnc;
            Sec_SocKeyHandle  hKeyHMAC;
            Sec_SocKeyHandle  hKeyWrap;
            uint8_t* peerKey = NULL;
            uint16_t pSize;

            //addkey function to coverto normal ids to seckey ids
            int retVal = ((_vault->FindKey(privateKeyId, &pKeyBase)) || (_vault->FindKey(derivationKeyId, &pKeyDerive)));
            ASSERT(retVal == RET_OK);

            peerKey = _vault->getPeerKey();
            pSize = _vault->getPeerSize();
            ASSERT(peerKey != NULL);

            //Need to export publickey data
            Sec_Result  result_sec = SEC_RESULT_FAILURE;
            result_sec = SecNetflix_NetflixDHDerive(_vault->getNetflixHandle(),pKeyBase,peerKey,pSize,pKeyDerive,
                        &hKeyEnc,&hKeyHMAC,&hKeyWrap);

            //use Addkey to covert sockey to normal keys ids
            if (result_sec == SEC_RESULT_SUCCESS) {
                encryptionKeyId = _vault->AddKey(hKeyEnc);
                hmacKeyId = _vault->AddKey(hKeyHMAC);
                wrappingKeyId = _vault->AddKey(hKeyWrap);
                result = RET_OK;
                TRACE_L2(_T("SEC:Netflix Derive  enc %d  hmac %d  wrapping handle %d \n", encryptionKeyId, hmacKeyId, wrappingKeyId));
            }
            else {
                TRACE_L1(_T("SEC:SecNetflix_NetflixDHDerive failed ,retval = %d \n"),result_sec);
            }

            return (result);
        }

    } // namespace Netflix

} // namespace Implementation


extern "C" {

    // Diffie-Hellman

    uint32_t diffiehellman_generate(struct VaultImplementation* vault,
        const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
        uint32_t* private_key_id, uint32_t* public_key_id)
    {
        ASSERT(vault != nullptr);
        ASSERT(modulus != nullptr);
        ASSERT(private_key_id != nullptr);
        ASSERT(public_key_id != nullptr);

        Implementation::VaultNetflix* _vault = reinterpret_cast<Implementation::VaultNetflix*>(vault);
        return (Implementation::GenerateDiffieHellmanKeys(_vault, generator, modulusSize, modulus, (*private_key_id), (*public_key_id)));
    }

    uint32_t diffiehellman_derive(struct VaultImplementation* vault, const uint32_t private_key_id, const uint32_t peer_public_key_id, uint32_t* secret_id)
    {
        ASSERT(vault != nullptr);
        ASSERT(secret_id != nullptr);

        //Not Implemented
        return 0;
    }

    // Netflix Security

    uint32_t netflix_security_derive_keys(const uint32_t private_dh_key_id, const uint32_t peer_public_dh_key_id, const uint32_t derivation_key_id,
        uint32_t* encryption_key_id, uint32_t* hmac_key_id, uint32_t* wrapping_key_id)
    {
        ASSERT(encryption_key_id != nullptr);
        ASSERT(hmac_key_id != nullptr);
        ASSERT(wrapping_key_id != nullptr);

        Implementation::VaultNetflix* _vault = &Implementation::VaultNetflix::NetflixInstance();
        return (Implementation::Netflix::DiffieHellmanAuthenticatedDeriveSecret(_vault, private_dh_key_id, peer_public_dh_key_id, derivation_key_id,
            (*encryption_key_id), (*hmac_key_id), (*wrapping_key_id)));

    }

} // extern "C"



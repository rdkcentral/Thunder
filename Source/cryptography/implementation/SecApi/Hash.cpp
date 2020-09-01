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

#include "Hash.h" 

namespace Implementation {

    /*********************************************************************
     * @function Algorithm
     *
     * @brief    choose the required hmac algorithm
     *
     * @param[in] type -enum value which specifies the hash type 
     *
     * @return struct which has chosen type of digest and mac algorithm 
     *
     *********************************************************************/
    const HashAlg* Algorithm(const hash_type type)
    {
        /* to select hash algorithms,
           Secpi suports only SHA1 and  SHA256,NOT SHA-224/384/512/ */
        HashAlg* algorithm = new HashAlg;
        switch (type) {
        case hash_type::HASH_TYPE_SHA1:
            algorithm->digest_alg = SEC_DIGESTALGORITHM_SHA1;
            algorithm->mac_alg = SEC_MACALGORITHM_HMAC_SHA1;
            algorithm->size = hash_type::HASH_TYPE_SHA1;
            break;
        case hash_type::HASH_TYPE_SHA256:
            algorithm->digest_alg = SEC_DIGESTALGORITHM_SHA256;
            algorithm->mac_alg = SEC_MACALGORITHM_HMAC_SHA256;
            algorithm->size = hash_type::HASH_TYPE_SHA256;
            break;
        default:
            TRACE_L1(_T("Hashing algorithm %i not supported"), type);
            //Default values 
            algorithm->digest_alg = SEC_DIGESTALGORITHM_NUM;
            algorithm->mac_alg = SEC_MACALGORITHM_NUM;
            algorithm->size = 0;
            break;
        }
        return (algorithm);

    }

    namespace Operation {

        // Wrappers to get around different function names for digest and HMAC calculation.
        struct Digest {
            /*********************************************************************
             * @function  Update (Digest)
             *
             * @brief    Wrapper for updating input data to calculate Digest
             *
             * @param[in] hndle - digest/mac handle
             * @param[in] data - data to be input
             * @param[in] cnt - size of input data
             *
             * @return Sec_Result indicating success or otherwise
             *
             *********************************************************************/
            static Sec_Result  Update(Handle* hndle, SEC_BYTE* data, SEC_SIZE cnt) {
                return SecDigest_Update(hndle->digest_handle, data, cnt);
            }

           /*********************************************************************
             * @function Final (Digest)
             *
             * @brief Wrapper for calculating the digest value
             *
             * @param[in] hndle - digest/mac handle
             * @param[out] digestOutput - calculated digest value
             * @param[out] digestSize - size of calculated digest
             *
             * @return Sec_Result indicating success or otherwise
             *
             *********************************************************************/
            static Sec_Result  Final(Handle* hndle, SEC_BYTE* digestOutput, SEC_SIZE* digestSize) {
                return SecDigest_Release(hndle->digest_handle, digestOutput, digestSize);
            }

        };

        struct HMAC {
            /*********************************************************************
             * @function Update (HMAC)
             *
             * @brief Wrapper for taking in data for HMAC calculation
             *
             * @param[in] hndle - digest/mac handle
             * @param[in] data - data to be input
             * @param[in] cnt - size of input data
             *
             * @return Sec_Result indicating success or otherwise
             *
             *******************************************************************/
            static Sec_Result Update(Handle* hndle, SEC_BYTE* data, SEC_SIZE cnt) {
                return SecMac_Update(hndle->mac_handle, data, cnt);

            }
            /*********************************************************************
             * @function Final (HMAC)
             *
             * @brief Wrapper to calculate the HMAC value
             *
             * @param[in] hndle - digest/mac handle
             * @param[out] hmacOutput - calculated hmac  value
             * @param[out] hmacSize - size of calculated hmac
             *
             * @return Sec_Result indicating success or otherwise
             *
             *********************************************************************/

            static Sec_Result Final(Handle* hndle, SEC_BYTE* hmacOutput, SEC_SIZE* hmacSize) {
                return SecMac_Release(hndle->mac_handle, hmacOutput, hmacSize);
            }

        };

    } // namespace Operation

    template<typename OPERATION>
    Implementation::HashType<OPERATION>::HashType()
    {
    }

    template<typename OPERATION>
    Implementation::HashType<OPERATION>::HashType(const Sec_DigestAlgorithm digestAlg, const uint16_t digestSize)
        : _vault_digest(nullptr)
        , _size(0)
        , _failure(false)
    {
        // get Digest Instance
        _vault_digest = new Implementation::Vault;
        if (_vault_digest == nullptr) {
            TRACE_L1(_T("Vault not initialized \n"));
            _failure = true;
        }
        else {
            if (_vault_digest->getSecProcHandle() != nullptr) {
                Sec_Result res = SecDigest_GetInstance(_vault_digest->getSecProcHandle(), digestAlg, &(handle->digest_handle));
                if (res != SEC_RESULT_SUCCESS) {
                    TRACE_L1(_T("SEC :SecDigest_GetInstance() failed retVal: %d\n"),res);
                    _failure = true;
                }
                else {
                    _size = digestSize;
                    ASSERT(_size != 0);
                }
            }
        }
    }

    template<typename OPERATION>
    Implementation::HashType<OPERATION>::HashType(const Implementation::Vault* vault, const Sec_MacAlgorithm  macAlg, const uint16_t macSize, const uint32_t secretId, const uint16_t secretLength)
        : _size(0)
        , _failure(false)
    {
        IdStore* ids;
        uint16_t length = 0;

        ASSERT(vault != nullptr);
        ASSERT(secretId != 0);
        ASSERT(secretLength != 0);

        _vault = vault;
        uint8_t* secret = reinterpret_cast<uint8_t*>(ALLOCA(sizeof(ids)));
        ASSERT(secret != nullptr);

        length = _vault->Export(secretId, secretLength, secret, true);
        if (length != 0) {
            if (_vault->getSecProcHandle() != nullptr) {
                std::memcpy(&ids, secret, sizeof(ids));
                ASSERT(ids->idHmac != 0);
                _id_sec = ids->idHmac;
                TRACE_L2(_T("SEC : the object id sec from export is %llu \n"), _id_sec);
                Sec_Result sec_res = SecKey_GetInstance(_vault->getSecProcHandle(), _id_sec, &sec_key);
                if (sec_key != nullptr && sec_res == SEC_RESULT_SUCCESS) {
                    Sec_Result res = SecMac_GetInstance(_vault->getSecProcHandle(), macAlg, sec_key, &(handle->mac_handle));
                    if (res != SEC_RESULT_SUCCESS) {
                        TRACE_L1(_T("SEC : SecMac_GetInstance() failed reval :%d \n"),res);
                        SecKey_Release(sec_key);
                        _failure = true;
                    }
                    else {
                        _size = macSize;
                        ASSERT(_size != 0);
                    }
                }
                else {
                    TRACE_L1(_T("SEC :Key instance failed retVal = %d \n"),sec_res);
                }
            }
            else {
                TRACE_L1(_T("SEC :Unable to get a valid proc handle from vault \n"));
            }
        }
        else {
            TRACE_L1(_T("SEC :Failed to export a valid key from vault at id %d\n"),secretId);
        }
    }

    template<typename OPERATION>
    Implementation::HashType<OPERATION>::~HashType()
    {
        delete handle;
        if (_vault_digest != nullptr) {
            delete _vault_digest;
        }
        if (sec_key != nullptr)
            SecKey_Release(sec_key);
    }

    template<typename OPERATION>
    /*********************************************************************
     * @function Ingest
     *
     * @brief    Ingest the data for which digest/hmac needs to be found
     *
     * @param[in] length - size of data
     * @param[in] data - data buffer
     *
     * @return status success/failure
     *
     *********************************************************************/
    uint32_t Implementation::HashType<OPERATION>::Ingest(const uint32_t length, const uint8_t* data)
    {
        ASSERT(data != nullptr);
        if (false == _failure) {
            SEC_BYTE* data_digest = const_cast<SEC_BYTE*>(data);
            Sec_Result retVal = OPERATION::Update(handle, data_digest, length);
            if ( SEC_RESULT_SUCCESS == retVal) {
                return length;
            }
            else {
                TRACE_L1(_T("SEC: Update() failed retval = %d"),retVal);
            }
        }
        return 0;

    }

    template<typename OPERATION>
    /*********************************************************************
     * @function Calculate
     *
     * @brief    Caluculate the digest/hmac value
     *
     * @param[in] maxLength - maximum length of data
     * @param[in] data - calculated digest/hmac 
     *
     * @return size of calculated digest/hmac 
     *
     *********************************************************************/
    uint8_t Implementation::HashType<OPERATION>::Calculate(const uint8_t maxLength, uint8_t* data)
    {
        uint8_t result = 0;
        if (true == _failure) {
            TRACE_L1(_T("SEC : Hash calculate() failed"));
        }
        else {
            if (maxLength < _size) {
                TRACE_L1(_T("Output buffer to small, need %i bytes, got %i bytes"), _size, maxLength);
            }
            else {
                size_t len = maxLength;
                Sec_Result res = OPERATION::Final(handle, data, &len);
                if (res != SEC_RESULT_SUCCESS) {
                    TRACE_L1(_T("Final() failed retVal = %d"),res);
                    _failure = true;
                }
                else {
                    TRACE_L2(_T("Calculated hash successfully, size: %i bytes"), len);
                    ASSERT(len == _size);
                    result = len;
                }
            }
        }
        return (result);

    }



} // namespace Implementation





extern "C" {
    HashImplementation* hash_create(const hash_type type)
    {
        HashImplementation* implementation = nullptr;
        // get algorithm,and pass this alg_hash to Hashtype COTR */
        const HashAlg* secalg = Implementation::Algorithm(type);

        if (secalg != nullptr) {
            Sec_DigestAlgorithm digest_alg = secalg->digest_alg;
            uint16_t size = secalg->size;
            implementation = new Implementation::HashType<Implementation::Operation::Digest>(digest_alg, size);
        }
        delete secalg;
        return (implementation);
    };

    HashImplementation* hash_create_hmac(const VaultImplementation* vault, const hash_type type, const uint32_t secret_id)
    {
        ASSERT(vault != nullptr);
        HashImplementation* implementation = nullptr;

        if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_DEFAULT) {
            const Implementation::Vault* vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
            uint16_t secretLength = vaultImpl->Size(secret_id, true);
            if (secretLength == 0) {
                TRACE_L1(_T("SEC: Failed to retrieve secret id 0x%08x"), secret_id);
            }
            else {
                // Pass digest and mac algorithm to HashType COTR
                const HashAlg* secalg = Implementation::Algorithm(type);
                if (secalg != nullptr) {
                    Sec_MacAlgorithm mac_alg = secalg->mac_alg;
                    uint16_t size = secalg->size;
                    implementation = new Implementation::HashType<Implementation::Operation::HMAC>(vaultImpl, mac_alg, size, secret_id, secretLength);
                    delete secalg;

                }
            }
        }
        else if (Implementation::vaultId == CRYPTOGRAPHY_VAULT_NETFLIX) {
            const Implementation::VaultNetflix* vaultImplNetflix = reinterpret_cast<const Implementation::VaultNetflix*>(vault);
            implementation = new Implementation::HashTypeNetflix(vaultImplNetflix, secret_id);
        }


        return (implementation);
    }

    void hash_destroy(HashImplementation* hash)
    {
        ASSERT(hash != nullptr);
        delete hash;
    }

    uint32_t hash_ingest(HashImplementation* hash, const uint32_t length, const uint8_t data[])
    {
        ASSERT(hash != nullptr);
        return (hash->Ingest(length, data));
    }

    uint8_t hash_calculate(HashImplementation* hash, const uint8_t max_length, uint8_t data[])
    {
        ASSERT(hash != nullptr);
        return (hash->Calculate(max_length, data));
    }

} // extern "C"


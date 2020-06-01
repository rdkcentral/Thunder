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

/*XRE-15581 Add sec api headers */
#include <sec_security.h>
#include <sec_security_datatype.h>
#include <sec_security_common.h>

#include "../../Module.h"

#include <hash_implementation.h>

#include <core/core.h>

#include "Vault.h"

struct HashImplementation {
    virtual uint32_t Ingest(const uint32_t length, const uint8_t data[]) = 0;
    virtual uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) = 0;

    virtual ~HashImplementation() { }
};

struct HashAlg {
	Sec_DigestAlgorithm digest_alg;
	Sec_MacAlgorithm mac_alg;
	uint16_t size;
};

struct Handle {
	Sec_DigestHandle* digest_handle;
        Sec_MacHandle* mac_handle;
};	

namespace Implementation {


const HashAlg* Algorithm(const hash_type type)
{
  #if 0
    const EVP_MD* md = nullptr;

    switch (type) {
    case hash_type::HASH_TYPE_SHA1:
        md = EVP_sha1();
        break;
    case hash_type::HASH_TYPE_SHA224:
        md = EVP_sha224();
        break;
    case hash_type::HASH_TYPE_SHA256:
        md = EVP_sha256();
        break;
    case hash_type::HASH_TYPE_SHA384:
        md = EVP_sha384();
        break;
    case hash_type::HASH_TYPE_SHA512:
        md = EVP_sha512();
        break;
    default:
        TRACE_L1(_T("Hashing algorithm %i not supported"), type);
        break;
    }

    return (md); 
  #endif
    /*XRE-15581 to select hash algorithms,
      TODO:Secpi suports only SHA1 and  SHA256,NOT SHA-224/384/512/ */
    HashAlg* algorithm = new HashAlg;
    switch (type) {
    case hash_type::HASH_TYPE_SHA1:
        algorithm->digest_alg = SEC_DIGESTALGORITHM_SHA1;
	algorithm->mac_alg = SEC_MACALGORITHM_HMAC_SHA1;
	algorithm->size=hash_type::HASH_TYPE_SHA1;
	TRACE_L1(_T("SEC :SHA1 \n"));
        break;
    case hash_type::HASH_TYPE_SHA256:
        algorithm->digest_alg = SEC_DIGESTALGORITHM_SHA256;
	algorithm->mac_alg = SEC_MACALGORITHM_HMAC_SHA256;
	algorithm->size=hash_type::HASH_TYPE_SHA256;
	TRACE_L1(_T("SEC:SHA256 \n"));
        break;
    default:
 	TRACE_L1(_T("Hashing algorithm %i not supported"), type);
        break;
    }
    return (algorithm);

}

namespace Operation {

    // Wrappers to get around different function names for digest and HMAC calculation.
    struct Digest {
	static Sec_Result  Update(Handle* hndle ,SEC_BYTE* d,SEC_SIZE cnt){
		return SecDigest_Update(hndle->digest_handle,d,cnt);
        }
	static Sec_Result  Final( Handle* hndle,SEC_BYTE* digestOutput, SEC_SIZE *digestSize){
		return SecDigest_Release(hndle->digest_handle,digestOutput,digestSize);
        }

};
    
    struct HMAC {
	static Sec_Result Update(Handle* hndle,SEC_BYTE* d,SEC_SIZE cnt){
		return SecMac_Update(hndle->mac_handle,d,cnt);

        }
	static Sec_Result Final(Handle* hndle,SEC_BYTE* hmacOutput, SEC_SIZE *hmacSize){
		return SecMac_Release(hndle->mac_handle,hmacOutput,hmacSize);
	}

    };

} // namespace Operation

template<typename OPERATION>
class HashType : public HashImplementation {
public:
    HashType(const HashType<OPERATION>&) = delete;
    HashType<OPERATION>& operator=(const HashType) = delete;

private:
    const Implementation::Vault* _vault;
    uint16_t _size;
    bool _failure;
    Handle* handle=new Handle;

public:
	HashType(const HashAlg* alg_hash)
        : _vault(nullptr)
        , _size(0)
        , _failure(false)
    {
	/*XRE-15581 get Digest Instance*/
	ASSERT(alg_hash!= nullptr);
	_vault = new Implementation::Vault;
	if(_vault->_secProcHandle!=nullptr) {
	Sec_Result res=SecDigest_GetInstance(_vault->_secProcHandle,alg_hash->digest_alg,&(handle->digest_handle));
	if(res!=SEC_RESULT_SUCCESS) {
	     TRACE_L1(_T("SecDigest_GetInstance() failed"));
            _failure = true;
	}else {
		_size=alg_hash->size;
		TRACE_L1(_T(" size is %d \n"),_size);
		ASSERT(_size != 0);
	
 	}

	} else {
	TRACE_L1(_T("SEC :proc handle null\n"));
	}
    }

    HashType(const Implementation::Vault* vault,const HashAlg* alg_hash, const uint32_t secretId, const uint16_t secretLength)
	: _size(0)
        , _failure(false)
    {
        ASSERT(vault != nullptr);
        ASSERT(secretId != 0);
        ASSERT(secretLength != 0);
        _vault = vault;

        if (false==_failure)  {
            uint8_t* secret = reinterpret_cast<uint8_t*>(ALLOCA(secretLength));
            ASSERT(secret != nullptr);

	    KeyInfo* keyinfo = _vault->Export(secretId, secretLength, secret, true);
	    ASSERT(keyinfo != nullptr);
	    if(keyinfo != nullptr){
	    /*XRE-15581 get a MAC instance .TODO:key to be properly got from vault*/
	         ASSERT(alg_hash!= nullptr);
	         if(_vault->_secProcHandle!=nullptr){
			if(_vault->key_handle != nullptr) {
				Sec_Result res =SecMac_GetInstance(_vault->_secProcHandle,alg_hash->mac_alg,keyinfo->key_handle,&(handle->mac_handle));	
				if(res!=SEC_RESULT_SUCCESS) {
	             			TRACE_L1(_T("SecMac_GetInstance() failed"));
           	       			_failure = true;
        			}else{
					 _size=alg_hash->size;
        	         		ASSERT(_size != 0);

				}		
			}
		}
	    }
	}
  }

    ~HashType() override
    {
	delete handle;
	delete _vault;
    }

public:

    uint32_t Ingest(const uint32_t length, const uint8_t* data) override
    {
	ASSERT(data != nullptr);
	if (false == _failure) {
		SEC_BYTE* data_digest = const_cast<SEC_BYTE *>(data);
		if (OPERATION::Update(handle, data_digest, length) != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("Update() failed"));
                _failure = true;
            }
        }

        return (_failure? 0 : length);

    }

    uint8_t Calculate(const uint8_t maxLength, uint8_t* data) override
    {
	 uint8_t result = 0;
	if (true == _failure) {
            TRACE_L1(_T("Hash calculation failure"));
        } else {
            if (maxLength < _size) {
                TRACE_L1(_T("Output buffer to small, need %i bytes, got %i bytes"), _size, maxLength);
            } else {
                size_t len = maxLength;
		  if (OPERATION::Final(handle, data, &len) != SEC_RESULT_SUCCESS) {
                    TRACE_L1(_T("Final() failed"));
                    _failure = true;
                } else {
                    TRACE_L2(_T("Calculated hash successfully, size: %i bytes"), len);
                    ASSERT(len == _size);
                    result = len;
                }
            }
        }

        return (result);

    }
  
};

} // namespace Implementation

extern "C" {
HashImplementation* hash_create(const hash_type type)
{
    HashImplementation* implementation = nullptr;
    /*XRE-15581 get algorithm,and pass this alg_hash to Hashtype COTR */
     const HashAlg* secalg=Implementation::Algorithm(type);
     if(secalg != nullptr) {
	implementation = new Implementation::HashType<Implementation::Operation::Digest>(secalg);
     }
    return (implementation);
};
HashImplementation* hash_create_hmac(const VaultImplementation* vault, const hash_type type, const uint32_t secret_id)
{
    ASSERT(vault != nullptr);
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    HashImplementation* implementation = nullptr;

    uint16_t secretLength = vaultImpl->Size(secret_id, true);
    if (secretLength == 0) {
        TRACE_L1(_T("Failed to retrieve secret id 0x%08x"), secret_id);
    } else {
	/*XRE-15581 Pass digest and mac algorithm to HashType COTR*/
	const HashAlg* secalg=Implementation::Algorithm(type);
	if(secalg != nullptr ) {
	implementation = new Implementation::HashType<Implementation::Operation::HMAC>(vaultImpl,secalg,secret_id, secretLength);
	}
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

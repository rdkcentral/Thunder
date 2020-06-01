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

#include <vault_implementation.h>

#include <cryptalgo/cryptalgo.h>


#include "Vault.h"
//#include "Derive.h"

namespace Implementation {

static constexpr uint8_t IV_SIZE = 16;

namespace Netflix {

static constexpr uint32_t KPE_ID = 1;
static constexpr uint32_t KPH_ID = 2;
static constexpr uint32_t KPW_ID = 3;
static constexpr uint32_t ESN_ID = 4;

static constexpr uint8_t MAX_ESN_SIZE = 64;

} // namespace Netflix


/* static */ Vault& Vault::NetflixInstance()
{
     /*XRE-15581 Need to be implemented for Netflix Instance */
}

/*XRE-15581 Added a default COTR */

Vault::Vault()
    : _lock()
    , _items()
    , _lastHandle(0)
{
     Sec_Result sec_res =  SecProcessor_GetInstance(&_secProcHandle, NULL);
     if (sec_res != SEC_RESULT_SUCCESS) {
         printf("proccesor instance failed\n");
     }

    _lastHandle = 0x80000000;

}



Vault::Vault(const string key, const Callback& ctor, const Callback& dtor)
    : _lock()
    , _items()
    , _lastHandle(0)
    , _vaultKey(key)
    , _dtor(dtor)
{
    if (ctor != nullptr) {
        ctor(*this);
    }
     Sec_Result sec_res =  SecProcessor_GetInstance(&_secProcHandle, NULL);
     if (sec_res != SEC_RESULT_SUCCESS) {
            printf("processor instance failed \n");
     }

    _lastHandle = 0x80000000;
    
}

Vault::~Vault()
{
    if (_dtor != nullptr) {
        _dtor(*this);
    }
   if (_secProcHandle != nullptr)
	Sec_Result res =  SecProcessor_Release(_secProcHandle);
}

/*XRE-15581 Take the key ,provision it and get key instance.
  use the key instance for cipher functions.Changes to be expected here based on netflix instance */
uint16_t Vault::Cipher(bool encrypt, const uint16_t inSize, const uint8_t input[], const uint16_t maxOutSize, uint8_t output[]) const
{
    uint16_t totalLen = 0;
    unsigned  int outLen = 0;

    ASSERT(maxOutSize >= (inSize + (encrypt? IV_SIZE : -IV_SIZE)));

    if (maxOutSize >= (inSize + (encrypt? IV_SIZE : -IV_SIZE))) {
        uint8_t newIv[IV_SIZE];
        const  uint8_t* iv = nullptr;
        const uint8_t* inputBuffer = nullptr;
        uint16_t inputSize = 0;
        uint8_t* outputBuffer = nullptr;

	Sec_Result sec_res = SEC_RESULT_FAILURE;
	Sec_CipherMode mode = SEC_CIPHERMODE_ENCRYPT; /*XRE-15581 Set to enc mode by default*/
	Sec_RandomHandle* sec_random;

        if (encrypt) {
	    SEC_BYTE iv_val[IV_SIZE]; 
	    sec_res = SecRandom_GetInstance(_secProcHandle, SEC_RANDOMALGORITHM_TRUE, &sec_random);
	    if (sec_res != SEC_RESULT_SUCCESS) { 
	    	return outLen;
            }
	    sec_res = SecRandom_Process(sec_random, iv_val, sizeof(iv_val)); 
	    if (sec_res != SEC_RESULT_SUCCESS) { 
		return outLen;
            } 
	    iv=iv_val;
	    SecRandom_Release(sec_random); 
		
            //iv = newIv;
            inputBuffer = input;
            inputSize = inSize;
            outputBuffer = output + IV_SIZE;
            ::memcpy(output, iv, IV_SIZE);
            totalLen += IV_SIZE;
        } else {
            iv = input;
            inputBuffer = (input + IV_SIZE);
            inputSize = (inSize - IV_SIZE);
            outputBuffer = output;
	    mode= SEC_CIPHERMODE_DECRYPT;
        }

       /* XRE-15581  Generate a key handle for key got from COTR*/
	Sec_KeyHandle* sec_key_cipher = nullptr;
	SEC_OBJECTID id_sec_key = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE,  SEC_OBJECTID_USER_TOP);
        SEC_BYTE* data = (SEC_BYTE*)(_vaultKey.data());
	SEC_SIZE sz = sizeof(_vaultKey.data());
        Sec_Result res =  SecKey_Provision(_secProcHandle,id_sec_key,SEC_STORAGELOC_RAM,SEC_KEYCONTAINER_RAW_AES_128,data,sz);
	if( res == SEC_RESULT_SUCCESS) {
		Sec_Result res_instance = SecKey_GetInstance(_secProcHandle,id_sec_key,&sec_key_cipher);	
		if(res_instance == SEC_RESULT_SUCCESS) {
			Sec_CipherHandle* cipher_handle=nullptr;
			SEC_BYTE* iv_value = const_cast<SEC_BYTE *>(iv);					    	
			sec_res = SecCipher_GetInstance(_secProcHandle,SEC_CIPHERALGORITHM_AES_CTR,mode,sec_key_cipher,iv_value,&cipher_handle);
			if(sec_res!= SEC_RESULT_SUCCESS) {
				TRACE_L1(_T("SecCipher_GetInstance failed in cipher()"));
        		}
			SEC_BYTE* inputBuf = const_cast<SEC_BYTE *>(inputBuffer);	
        		sec_res = SecCipher_Process(cipher_handle, inputBuf, inputSize, SEC_TRUE, outputBuffer, maxOutSize, &outLen);
        		if( sec_res!= SEC_RESULT_SUCCESS) {
				TRACE_L1(_T("SecCipher_Process failed in cipher()"));
	        	}
        		SecCipher_Release(cipher_handle);
		}
		 SecKey_Release(sec_key_cipher);
	}
	SecKey_Delete(_secProcHandle,id_sec_key);
    }

	return (outLen);
}

/*XRE-15581 :Return key or bundle size */
uint16_t Vault::Size(const uint32_t id, bool allowSealed) const
{
    uint16_t size = 0;

    _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
	if ((allowSealed == true) || (*it).second.IsExportable() == true) {
		//SEC_OBJECTID  id_sec=(*it).second.Buffer();
		const uint8_t* id_sec_val = (*it).second.Buffer();
		SEC_OBJECTID  id_sec = reinterpret_cast<unsigned int>(id_sec_val);
                Sec_KeyHandle* sec_key;
                Sec_Result sec_res = SecKey_GetInstance(_secProcHandle,id_sec,&sec_key);
		if( sec_res == SEC_RESULT_SUCCESS)
		size=SecKey_GetKeyLen(sec_key);
        } else {
            TRACE_L2(_T("Blob id 0x%08x is sealed"), id);
            size = USHRT_MAX;
        }
    } else {
        TRACE_L1(_T("Failed to look up blob id 0x%08x"), id);
    }
    _lock.Unlock();

    return (size);
}

/* XRE-15581 Import key and stor(provision) it */
uint32_t Vault::Import(const uint16_t size, const uint8_t blob[], bool exportable)
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);

        if (id != 0) {
            uint8_t* buf = reinterpret_cast<uint8_t*>(ALLOCA(USHRT_MAX));
	    SEC_OBJECTID id_sec = SecKey_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE,  SEC_OBJECTID_USER_TOP); 
	    SEC_BYTE* data = (SEC_BYTE*)blob;
	    Sec_Result sec_res =  SecKey_Provision(_secProcHandle,id_sec,SEC_STORAGELOC_RAM,SEC_KEYCONTAINER_RAW_AES_128,data,size);
	    if(sec_res == SEC_RESULT_SUCCESS) {
	    _items.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(exportable,id_sec));
	
            _lastHandle = id;
		TRACE_L2(_T("Added a %s data blob"), (exportable? "clear": "sealed"));
	     } else
		 TRACE_L2(_T("cannot provision key "));

        }
        _lock.Unlock();
    }

    return (id);
}

/* XRE-15581 Export key .chaged return typr from uint16_t to KeyInfo* */
KeyInfo*  Vault::Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed) const
{
    uint16_t outSize = 0;
    KeyInfo* keyinfo =new KeyInfo;
    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            if ((allowSealed == true) || ((*it).second.IsExportable() == true)) {
		const uint8_t* id_sec_val = (*it).second.Buffer();
		SEC_OBJECTID id_sec = reinterpret_cast<unsigned int>(id_sec_val);
		Sec_KeyHandle* sec_key;
		Sec_Result sec_res = SecKey_GetInstance(_secProcHandle,id_sec,&sec_key);
		if (sec_res != SEC_RESULT_SUCCESS) {
			TRACE_L1(_T("Key handle not generated "));
		}else{
			outSize=SecKey_GetKeyLen(sec_key);
			if(keyinfo != nullptr) {
			keyinfo->key_handle=sec_key;
			keyinfo->key_size=outSize;	
			}	
		}
            } else {
                TRACE_L1(_T("Blob id 0x%08x is sealed, can't export"), id);
            }
        } else {
            TRACE_L1(_T("Failed to look up blob id 0x%08x"), id);
        }
        _lock.Unlock();
    }

	
	return (keyinfo);
}

/*XRE-15581 Put/store encrypted key */
uint32_t Vault::Put(const uint16_t size, const uint8_t blob[])
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);
        if (id != 0) {
	SEC_OBJECTID id_sec = SecBundle_ObtainFreeObjectId(_secProcHandle, SEC_OBJECTID_USER_BASE,  SEC_OBJECTID_USER_TOP);
	SEC_BYTE* data = (SEC_BYTE*)blob;
        Sec_Result sec_res =  SecBundle_Provision(_secProcHandle,id_sec,SEC_STORAGELOC_RAM,data,size);
	if(sec_res == SEC_RESULT_SUCCESS) {
        _items.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(false,id_sec));

         _lastHandle = id;
	} else
		TRACE_L2(_T("cannot provision bundle "));
		
        }
        _lock.Unlock();
    }

    return (id);
}


/* XRE-15581 Get the key ,changed return type to KeyInfo* from uint16_t*/
KeyInfo* Vault::Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const
{
    uint16_t result = 0;
     KeyInfo* keyinfo =new KeyInfo;
    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
	    const uint8_t* id_sec_val = (*it).second.Buffer();
	    SEC_OBJECTID id_sec = reinterpret_cast<unsigned int>(id_sec_val);
            Sec_BundleHandle* sec_bundle;
            Sec_Result sec_res = SecBundle_GetInstance(_secProcHandle,id_sec,&sec_bundle);
	    if (sec_res != SEC_RESULT_SUCCESS) {
                        TRACE_L1(_T("Bundle handle not generated "));
	    }
	    else {
		SEC_SIZE* written = nullptr; 
		SEC_BYTE* buffer = nullptr;
		SEC_SIZE bufferLen;
		Sec_Result res_export = SecBundle_Export(sec_bundle,buffer,bufferLen,written);
		if( (keyinfo != nullptr) && (res_export == SEC_RESULT_SUCCESS)) {
                        keyinfo->bundle_handle=sec_bundle;
                        keyinfo->key_size=*written;
                        }

	    }
        }
        _lock.Unlock();
    }
	
	return keyinfo;
}

/* XRE-15581 Delte the key or bundle ids */
bool Vault::Delete(const uint32_t id)
{
    bool result = false;
    Sec_Result sec_res = SEC_RESULT_FAILURE;
   _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
	//SEC_OBJECTID id_sec=(*it).second.Buffer();
	const uint8_t* id_sec_val = (*it).second.Buffer();
	SEC_OBJECTID id_sec = reinterpret_cast<unsigned int>(id_sec_val);
	if((*it).second.IsExportable() == true)
		sec_res = SecKey_Delete(_secProcHandle,id_sec);
	else
		sec_res = SecBundle_Delete(_secProcHandle,id_sec);
	_items.erase(it);
	if (sec_res == SEC_RESULT_SUCCESS)
        result = true;
    }
    _lock.Unlock();

    return (result);
}

} // namespace Implementation


extern "C" {

// Vault

VaultImplementation* vault_instance(const cryptographyvault id)
{
    Implementation::Vault* vault = nullptr;

    switch(id) {
        case CRYPTOGRAPHY_VAULT_NETFLIX:
            vault = &Implementation::Vault::NetflixInstance();
            break;
	case CRYPTOGRAPHY_VAULT_DEFAULT:
	    vault = new  Implementation::Vault; /*XRE-15581 generic case,changes expected*/
        default:
            TRACE_L1(_T("Vault not supported: %d"), static_cast<uint32_t>(id));
            break;
    }

    return reinterpret_cast<VaultImplementation*>(vault);
}

uint16_t vault_size(const VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
    return (vaultImpl->Size(id));
}

uint32_t vault_import(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
{
    ASSERT(vault != nullptr);
    Implementation::Vault *vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Import(length, data, true /* imported in clear is always exportable */));
}

uint16_t vault_export(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
{
    ASSERT(vault != nullptr);
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
   // return (vaultImpl->Export(id, max_length, data));
	KeyInfo* keyinfo = vaultImpl->Export(id, max_length, data);
	return(keyinfo->key_size);
}

uint32_t vault_set(VaultImplementation* vault, const uint16_t length, const uint8_t data[])
{
    ASSERT(vault != nullptr);
    Implementation::Vault *vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Put(length, data));
}

uint16_t vault_get(const VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t data[])
{
    ASSERT(vault != nullptr);
    const Implementation::Vault *vaultImpl = reinterpret_cast<const Implementation::Vault*>(vault);
   // return (vaultImpl->Get(id, max_length, data));
     KeyInfo* keyinfo = vaultImpl->Get(id, max_length, data);
     return (keyinfo->key_size);
}

bool vault_delete(VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    Implementation::Vault *vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Delete(id));
}


// Netflix Security

uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[])
{
    if (data == nullptr) {
        return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::ESN_ID));
    } else {
       // return (Implementation::Vault::NetflixInstance().Export(Implementation::Netflix::ESN_ID, max_length, data));
	KeyInfo* keyinfo = Implementation::Vault::NetflixInstance().Export(Implementation::Netflix::ESN_ID, max_length, data);
	return (keyinfo->key_size);
    }
}

uint32_t netflix_security_encryption_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPE_ID) != 0? Implementation::Netflix::KPE_ID : 0);
}

uint32_t netflix_security_hmac_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPH_ID) != 0? Implementation::Netflix::KPH_ID : 0);
}

uint32_t netflix_security_wrapping_key(void)
{
    return (Implementation::Vault::NetflixInstance().Size(Implementation::Netflix::KPW_ID) != 0? Implementation::Netflix::KPW_ID : 0);
}

} // extern "C"

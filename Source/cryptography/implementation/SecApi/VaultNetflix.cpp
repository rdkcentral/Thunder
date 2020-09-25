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
#include "Vault.h"

namespace Implementation {

    
    /*********************************************************************
     * @function  NetflixInstance
     *
     * @brief    creates an instance of vault netflix and loads the keys
     *
     * @return  VaultNetflix Object
     *
     *********************************************************************/
    VaultNetflix& VaultNetflix::NetflixInstance()
    {
        auto ctor = [](VaultNetflix& netflix)
        {
            std::string path;
            WPEFramework::Core::SystemInfo::GetEnvironment(_T("NETFLIX_VAULT"), path);
            WPEFramework::Core::File file(path.c_str(), true);
            if (file.Open(true) == true) {
                uint64_t fileSize = file.Size();
                uint8_t buf[fileSize];
                uint16_t inSize = file.Read(buf, fileSize);
                file.Close();
                bool result = netflix.loadBoundKeys(buf, inSize);
                ASSERT(result == true);
            }
        };

        auto dtor = [](VaultNetflix& netflix) {
        };

        static VaultNetflix instance(ctor, dtor);
        return (instance);
    }

    
    /*********************************************************************
     * @function loadBoundKeys
     *
     * @brief    Loads esn and named keys and stores its ids in map
     *
     * @param[in] input - boundKeyMsg.bin file's key data
     * @param[in] inputSize - size of input
     *
     * @return status of operation as true  or false
     *
     *********************************************************************/
    bool VaultNetflix::loadBoundKeys(const uint8_t  input[], uint16_t inputSize)
    {
        Sec_Result result = SEC_RESULT_FAILURE;
        bool retVal = false; 
        result = SecNetflix_LoadBoundKeys(_netflixHandle,input,inputSize);
        if (result  == SEC_RESULT_SUCCESS) {
            // ESN
            esn();
            // Named  Keys 
            namedKeys();
            retVal = true;
        }
        else {
            TRACE_L1(_T("SecNetflix_LoadBoundKeys FAILED : retVal = %d\n"), result);
        }
        return retVal;
    }

    /*********************************************************************
     * @function esn
     *
     * @brief    Retreive the ESN value
     *
     * @return ESN string
     *
     *********************************************************************/
    const std::string& VaultNetflix::esn()
    {
        Sec_Result retVal = SEC_RESULT_FAILURE;
        SEC_SIZE bytesWritten = 0;
        if (_esn.size() == 0) {
            Sec_Result  retVal = SEC_RESULT_FAILURE;
            uint8_t esn[ESN_SIZE]= {0};
            retVal = SecNetflix_GetESN(_netflixHandle,esn,ESN_SIZE,&bytesWritten);
            if (retVal != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_GetESN FAILED : retVal = %d\n"), retVal);
            }
            _esn.append((char*)esn, 0, (int)bytesWritten);
        }
        TRACE_L2(_T("SEC : the esn value got is %s \n"), _esn.c_str());
        return _esn;
    }
    
    /*********************************************************************
     * @function namedKeys
     *
     * @brief    Retrieve namedkeys DKE,DKH and DKW and store in a map
     *
     * @return map storing the namedkeys with their ids
     *
     *********************************************************************/
    const VaultNetflix::NamedKeyMap& VaultNetflix::namedKeys()
    {
        if (_namedKeys.empty()) {
            uint32_t result = (GetNamedKey(std::string("DKE")) || GetNamedKey(std::string("DKH")) || GetNamedKey(std::string("DKW")));
            ASSERT(result != RET_FAIL);
        }
        return _namedKeys;
    }

    /*********************************************************************
     * @function GetNamedKey
     *
     * @brief    get sockey handle for named key and populate the map
     *
     * @param[in] keyName - named key string
     *
     * @return status success(0) or otherwise 
     *
     *********************************************************************/
    uint32_t VaultNetflix::GetNamedKey(std::string keyName)
    {
        uint32_t retVal = RET_FAIL;
        Sec_Result result = SEC_RESULT_FAILURE;
        // Get the named keys.
        Sec_SocKeyHandle hKey;
        result = SecNetflix_GetNamedKey(_netflixHandle,(SEC_BYTE*)keyName.c_str(),keyName.size(),&hKey);
        if (result == SEC_RESULT_SUCCESS) {
            uint32_t  keyID = AddKey(hKey);
            _namedKeys.insert(std::make_pair(keyName, keyID));
            TRACE_L2(_T("Key %s found and inserted into NamedKeys table using %d\n"), keyName.c_str(), keyID);
            _socKeyMap.insert(std::make_pair(keyID, hKey));
            TRACE_L2(_T("KeyID %d found and inserted into SocKeyMap table using %p\n"), keyID, hKey);
            retVal = RET_OK;
        }
        else {
            TRACE_L1(_T("SecNetflix_GetNamedKey FAILED for %s: result = %d\n"), keyName.c_str(), retVal);
        }
        return retVal;
    }

    /*********************************************************************
     * @function AddKey
     *
     * @brief    Add the sockey handle to the soc key map
     *
     * @param[in] socKey - Sec_SocKeyHandle of a key
     *
     * @return id across which socKey  of a key is stores in socKeyMap
     *
     *********************************************************************/
    uint32_t  VaultNetflix::AddKey(Sec_SocKeyHandle socKey)
    {
        uint32_t  hKey = GetNewKeyHandle();
        _socKeyMap.insert(std::pair<uint32_t, Sec_SocKeyHandle>(hKey, socKey));
        return hKey;
    }

    /*********************************************************************
     * @function GetNewKeyHandle
     *
     * @brief   Get the unused handle/id which can be used in the map
     *
     * @return the new id value 
     *
     *********************************************************************/
    uint32_t  VaultNetflix::GetNewKeyHandle()
    {
        uint32_t retVal = 1; //starting key index
        SocKeyMap::iterator it;
        it = _socKeyMap.find(retVal);
        TRACE_L2(_T("Looking for open key handle at %d slot %s\n"), retVal, it != _socKeyMap.end() ? "USED" : "OPEN");
        while (it != _socKeyMap.end()) {
            retVal++;
            it = _socKeyMap.find(retVal);
            TRACE_L2(_T("Looking for open key handle at %d slot %s\n"), retVal, it != _socKeyMap.end() ? "USED" : "OPEN");
        }
        TRACE_L2(_T("Found open key handle at %d \n"), retVal);
        return retVal;
    }

    /*********************************************************************
     * @function deleteKey
     *
     * @brief    delete the key and its entry from the Map 
     *
     * @param[in] keyHandle - id of the key
     *
     * @return status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t VaultNetflix::deleteKey(uint32_t  keyHandle)
    {
        Sec_Result result = SEC_RESULT_FAILURE;
        uint32_t retVal = RET_FAIL;
        Sec_SocKeyHandle* pKey = NULL;
        retVal = FindKey(keyHandle, &pKey);
        if (retVal == RET_OK) {
            result = SecNetflix_DeleteKey(_netflixHandle, pKey);
            if (result != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_DeleteKey FAILED : result = %d\n"));
            }
            retVal = DeleteKeyFromMap(keyHandle);
        }
        return retVal;
    }

    /*********************************************************************
     * @function FindKey
     *
     * @brief    Find the sockey handle of a key stores in map with id keyHandle
     *
     * @param[in] keyHandle - id of key in soc key map
     * @param[out] ppKey - the socKey handle for the key
     *
     * @return status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t VaultNetflix::FindKey(uint32_t  keyHandle, Sec_SocKeyHandle** ppKey) const
    {
        uint32_t result = RET_FAIL;
        SocKeyMap::const_iterator it;
        it = _socKeyMap.find(keyHandle);
        if (it != _socKeyMap.end()) {
            // Found a match
            *ppKey = const_cast<Sec_SocKeyHandle*>(&it->second);
            TRACE_L2(_T("Found SocKey for key handle %d key = %p\n"), keyHandle, *ppKey);
            result = RET_OK;
        }
        else {
            // No match
            *ppKey = NULL;
            TRACE_L1(_T("Did not find SocKey for key handle %d \n"), keyHandle);
        }
        return (result);
    }

    /*********************************************************************
     * @function FindNamedKey
     *
     * @brief    Find id of named key in namedKey map
     *
     * @param[in] keyName -  named key string
     * @param[out] keyHandle - id of named key in namedKey map
     *
     * @return status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t VaultNetflix::FindNamedKey(std::string keyName, uint32_t& keyHandle)
    {
        uint32_t retVal = RET_FAIL;
        NamedKeyMap::iterator it;
        it = _namedKeys.find(keyName);
        if (it != _namedKeys.end()) {
            keyHandle = it->second;
            TRACE_L1(_T("SEC :named key id found %d \n"), keyHandle);
            retVal = RET_OK;
        }
        else {
            TRACE_L1(_T("SEC :named key not found\n"));
        }
        return (retVal);
    }

    /*********************************************************************
     * @function DeleteKeyFromMap
     *
     * @brief    Delete the key info from the socKeyMap map 
     *
     * @param[in] keyHandle - id of the key to be deleted  
     *
     * @return status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t VaultNetflix::DeleteKeyFromMap(uint32_t keyHandle)
    {
        uint32_t result = RET_FAIL;
        if (_socKeyMap.erase(keyHandle)) {
            result = RET_OK;
            TRACE_L1(_T("Erased SocKey for key handle %d \n"), keyHandle);
        }
        else {
            TRACE_L1(_T("Could not find SocKey for key handle %d \n"), keyHandle);
        }
        return result;
    }

    /*********************************************************************
     * @function getKeyInfo
     *
     * @brief    get the meta data about the key
     *
     * @param[in] keyHandle - id of the key
     * @param[out] outKeyType - key type (secret,private,public etc)
     * @param[out] outAlgo - algorithm of key (aes,hmac,nflx_dh etc)
     * @param[out] outUsage - usage of key (encrypt,signing,verify etc)
     *
     * @return  status success(0) or otherwise
     *
     *********************************************************************/
    uint32_t VaultNetflix::getKeyInfo(uint32_t keyHandle,uint32_t& outKeyType,uint32_t& outAlgo,uint32_t& outUsage) const
    {
        Sec_SocKeyHandle* pKey = NULL;
        outKeyType =  0;
        outAlgo = 0;
        outUsage = 0;
        uint32_t retVal = FindKey(keyHandle, &pKey);
        TRACE_L2(_T("Found SocKey for key handle %d key = %p\n"), keyHandle, pKey);
        if (retVal == RET_OK) {
            Sec_Result  result = SEC_RESULT_FAILURE;
            result = SecNetflix_GetKeyInfo(_netflixHandle,pKey,(SEC_LONG*)&outKeyType,(SEC_LONG*)&outAlgo,(SEC_LONG*)&outUsage);
            if (result != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_GetKeyInfo FAILED for id  %d\n"), keyHandle);
                retVal = RET_FAIL;
            }
        }
        else {
            TRACE_L1(_T("FindKey did not find key handle = %d\n"), keyHandle);
        }
        return retVal;
    }

    /* COTR */
    VaultNetflix::VaultNetflix(const Callback& ctor, const Callback& dtor)
        : _lock()
        , _lastHandle(0)
        , _dtor(dtor)
        , _esn("")
        , _netflixHandle(NULL)
    {
        _namedKeys.clear();
        _socKeyMap.clear();
        if (ctor != nullptr) {
            Sec_Result retVal = SEC_RESULT_FAILURE;
            retVal = SecNetflix_GetInstance(NULL, &_netflixHandle);
            if (retVal != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_GetInstance FAILED : retVal = %d\n"), retVal);
            }
            ctor(*this);
        }
        _lastHandle = 0x80000000;
    }

    /* COTR */
    VaultNetflix::VaultNetflix()
        : _lock()
        , _lastHandle(0)
        , _netflixHandle(NULL)
    {
        Sec_Result retVal = SEC_RESULT_FAILURE;
        retVal = SecNetflix_GetInstance(NULL, &_netflixHandle);
        if (retVal != SEC_RESULT_SUCCESS) {
            TRACE_L1(_T("SecNetflix_GetInstance FAILED : retVal = %d\n"), retVal);
        }
        _lastHandle = 0x80000000;

    }

    /* DOTR */
    VaultNetflix::~VaultNetflix()
    {
        if (_pubKeyStore != NULL)
            delete[] _pubKeyStore;
        if (_dtor != nullptr) {
            Sec_Result retVal = SecNetflix_Release(_netflixHandle);
            if (retVal != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_Release FAILED : retVal = %d\n"), retVal);
            }
            _netflixHandle = NULL;
            _dtor(*this);
        }

    }

    /*********************************************************************
     * @function Size 
     *
     * @brief Gives Size of a key 
     *
     * @param[in] id -  id of key
     * @param[in] allowSealed -  wether the blob of data is sealed or not
     *
     * @return  size of key(return USHRT_MAX id data is sealed )
     *
     *********************************************************************/
    uint16_t VaultNetflix::Size(const uint32_t id, bool allowSealed) const
    {
        uint16_t size = 0;
        Sec_SocKeyHandle* keyPublic = NULL;
        SEC_SIZE bytesWritten = 0;
        uint32_t retVal = FindKey(id, &keyPublic);
        if (retVal == RET_OK) {
            uint32_t outKeyType = 0;
            uint32_t outAlgo = 0;
            uint32_t outUsage = 0;
            uint32_t keyInfo =  getKeyInfo(id, outKeyType, outAlgo, outUsage);
            if (keyInfo == RET_OK) {
                TRACE_L2(_T("SEC: the getinfo values keyid %d :  outKeyType %d outAlgo %d outUsage %d \n"), id, outKeyType, outAlgo, outUsage);
                if ((outKeyType == KEY_PUBLIC) && (outAlgo == NFLX_DH_ALG)) {
                    uint8_t keybuf[DH_PUBLIC_KEY_MAX] = {0};
                    uint16_t  result = SecNetflix_ExportKey(_netflixHandle,keyPublic,keybuf,DH_PUBLIC_KEY_MAX,&bytesWritten);
                    size = bytesWritten;
                } 
                else {
                    TRACE_L1(_T("SEC: key of id %d is sealed \n"),id);
                    size = USHRT_MAX;
                }
            }
        }
        else {
            TRACE_L1(_T("SEC:Could not find id  %d \n"),id);
        }
        return size;
    }


    /*********************************************************************
     * @function Import
     *
     * @brief   Store the key(DH public key)  used for DH operations
     *
     * @param[in] size - size of key
     * @param[in] blob - key to be stored
     * @param[in] exportable - blob expotable(true/false)
     *
     * @return id of 1 for storing successfully ,else 0
     *
     *********************************************************************/
    uint32_t VaultNetflix::Import(const uint16_t size, const uint8_t blob[], bool exportable)
    {
        uint32_t id = 0;
        //memcpy the data to a private variable location
        if (size > 0) {
            _pubKeyStore = new uint8_t[size];
            memcpy(_pubKeyStore, blob, size);
            _peerPubSize = size;
            TRACE_L2(_T("SEC :DH  pub key import and size  %d \n"), size);
            id =1;
        }
        return id;
    }

    /*********************************************************************
     * @function Export
     *
     * @brief   Export the key
     *
     * @param[in] id - id of the key
     * @param[in] size - size of the key
     * @param[out] blob - key blob written out
     * @param[in] allowSealed - blob sealed (true/false)
     *
     * @return size of key blob got
     *
     *********************************************************************/
    uint16_t VaultNetflix::Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed) const
    {
        Sec_SocKeyHandle* keyPublic = NULL;
        SEC_SIZE bytesWritten = 0;
        uint32_t retVal = FindKey(id, &keyPublic);
        if (retVal == RET_OK) {
            uint8_t blob_data[DH_PUBLIC_KEY_MAX];
            uint16_t  result = SecNetflix_ExportKey(_netflixHandle,keyPublic,blob_data,DH_PUBLIC_KEY_MAX,&bytesWritten);
            if (result != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_ExportKey FAILED : retVal = %d\n"), result);
            }
            else {
                TRACE_L2(_T("Exported Key data size is %d"), bytesWritten);
                memcpy(blob, blob_data, DH_PUBLIC_KEY_MAX);
            }
        }

        return bytesWritten;
    }

    /*********************************************************************
     * @function Put
     *
     * @brief    import/store a sealed key
     *
     * @param[in] size - size of the key
     * @param[in] blob - sealed key  that needs to be stored
     *
     * @return id across which the key  is stored in soc key map
     *
     *********************************************************************/
    uint32_t VaultNetflix::Put(const uint16_t size, const uint8_t blob[])
    {
        uint32_t id = 0;
        Sec_Result result = SEC_RESULT_FAILURE;
        Sec_SocKeyHandle hKey;
        result = SecNetflix_ImportSealedKey(_netflixHandle,blob,size,&hKey);
        if (result != SEC_RESULT_SUCCESS) {
            TRACE_L1(_T("SecNetflix_ImportSealedKey FAILED : result = %d\n"), result);
            id = (uint32_t)-1;
        }
        else {
            id = AddKey(hKey);
        }
        return id;
    }

    /*********************************************************************
     * @function Get 
     *
     * @brief    Get back the sealed key 
     *
     * @param[in] id - id of key
     * @param[in] size - size of the key
     * @param[out] blob - sealed key exported
     *
     * @return size of the key exported
     *
     *********************************************************************/
    uint16_t VaultNetflix::Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const
    {
        Sec_SocKeyHandle* pKey = NULL;
        SEC_SIZE bytesWritten = 0;
        std::string  exportedKey;
        uint32_t retVal = FindKey(id, &pKey);
        if (retVal == RET_OK)
        {
            Sec_Result  result = SEC_RESULT_FAILURE;
            result = SecNetflix_ExportSealedKey(_netflixHandle,pKey,blob,size,&bytesWritten);
            if (result != SEC_RESULT_SUCCESS) {
                TRACE_L1(_T("SecNetflix_ExportSealedKey FAILED : retVal = %d\n"), result);
            }
        }
        else {
            TRACE_L1(_T("FindKey did not find key handle = %d\n"), id);
        }
        return bytesWritten;
    }


    /*********************************************************************
     * @function Delete
     *
     * @brief    Delete the key itself and from map
     *
     * @param[in] id - id of the key to be deleted 
     *
     * @return true upon succesful delete,else false.
     *
     *********************************************************************/
    bool VaultNetflix::Delete(const uint32_t id)
    {
        bool result = false;
        uint32_t nfType = 0;
        uint32_t nfAlgorithm = 0;
        uint32_t nfUsage = 0;
        uint32_t rc = getKeyInfo(id, nfType, nfAlgorithm, nfUsage);
        if (rc != RET_OK) {
            TRACE_L1(_T("sec netflix: KeyInfo() failed [0x%08x]"), rc);
        }
        else {
            if (id == ENC_ID) {
                TRACE_L1(_T("SEC: not deleted enc  key\n"));
                return true;
            }
            result = (deleteKey(id) == RET_OK);
        }
        return (result);
    }

}// namespace implementation

extern "C" {
    uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[])
    {
        uint16_t length = 0;
        Implementation::VaultNetflix* netflix =  &Implementation::VaultNetflix::NetflixInstance();
        string esnVal = netflix->esn();
        TRACE_L2(_T("SEC :the esnVal value is %s \n"), esnVal.c_str());
        length = esnVal.size();
        TRACE_L2(_T("SEC: the esnval size is %d \n"), length);
        if (data != nullptr) {
            uint8_t* esn_data = reinterpret_cast<uint8_t*>(&esnVal[0]);
            memcpy(data, esn_data, length);
        }
        else { 
            TRACE_L1(_T("esn buffer null\n"));
        }
        return (length);
    }

    uint32_t netflix_security_encryption_key(void)
    {
        uint32_t  id = 0;
        Implementation::VaultNetflix* netflix = &Implementation::VaultNetflix::NetflixInstance();
        uint32_t retVal = netflix->FindNamedKey("DKE", id);
        if (retVal == RET_OK) {
            TRACE_L2(_T("SEC:dke FOUND at id %d \n"), id);
        }
        return (id);
    }

    uint32_t netflix_security_hmac_key(void)
    {
        uint32_t  id = 0;
        Implementation::VaultNetflix* netflix = &Implementation::VaultNetflix::NetflixInstance();
        uint32_t retVal = netflix->FindNamedKey("DKH", id);
        if (retVal == RET_OK) {
            TRACE_L2(_T("SEC:dkH FOUND at id %d \n"), id);
        }
        return (id);
    }

    uint32_t netflix_security_wrapping_key(void)
    {
        uint32_t  id = 0;
        Implementation::VaultNetflix* netflix;
        netflix = &Implementation::VaultNetflix::NetflixInstance();
        uint32_t retVal = netflix->FindNamedKey("DKW", id);
        if (retVal == RET_OK) {
            TRACE_L2(_T("SEC:dkh FOUND at id %d \n"), id);
        }
        return (id);
    }
} //extern c

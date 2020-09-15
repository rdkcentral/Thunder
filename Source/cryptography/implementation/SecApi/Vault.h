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

#include <map>
#include<cstring>

/* Add Secapi headers*/
#include <sec_security.h>
#include <sec_security_datatype.h>
#include <sec_security_common.h>
#include <sec_security_store.h>
#include <sec_security_utils.h>
#include <sec_security_netflix.h>

#include "../../Module.h"
#include "cryptography_vault_ids.h"

#define globalDir "/opt/drm/"
#define appDir "/tmp"

#define DH_PUBLIC_KEY_MAX    (129)
#define KEYLEN_AES_HMAC_128  (16)
#define KEYLEN_AES_HMAC_256  (32)
#define KEYLEN_HMAC_160      (20)
#define RET_OK               (0)
#define RET_FAIL             (1)
#define ESN_SIZE             (43)
#define KEY_PUBLIC	     (2)
#define NFLX_DH_ALG          (5)
#define ENC_ID           (1)        

namespace Implementation {

    extern "C" cryptographyvault  vaultId;

    struct IdStore {
        SEC_OBJECTID idHmac;
        SEC_OBJECTID idAes;
    };

    class Vault {
    public:
        static Vault& NetflixInstance();
        Vault();
        ~Vault();

    private:
        using Callback = std::function<void(Vault&)>;

    public:
        Vault(Vault const&) = delete;
        void operator=(Vault const&) = delete;

    public:
        // To obtain sec processor handle 
        Sec_ProcessorHandle* getSecProcHandle() const {
            return  _secProcHandle;
        }

        class MapStore :public WPEFramework::Core::DataStore {
        public:
            MapStore(bool exportable, struct IdStore ids, uint16_t keyLength)
                : _exportable(exportable)
                , _ids(ids)
                , _keyLength(keyLength)
            {
                TRACE_L2(_T("SEC:MAP for import and export \n"));
            }

            MapStore(bool exportable, uint8_t* buffer, uint16_t size)
                :WPEFramework::Core::DataStore(size)
                , _exportable(exportable)
            {
                Copy(buffer, size);
                TRACE_L2(_T("SEC:Map store for set and get\n"));
            }

            bool IsExportable() const { return _exportable; }

            uint16_t KeyLength() const { return _keyLength; }

            const IdStore* getIdStore() const { return &(_ids); }


        private:
            bool _exportable;
            struct IdStore _ids = { 0,0 };
            uint16_t _keyLength;
        };


    public:
        uint16_t Size(const uint32_t id, bool allowSealed = false) const;
        uint32_t Import(const uint16_t size, const uint8_t blob[], bool exportable = false);
        uint16_t Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed = false) const;
        uint32_t Put(const uint16_t size, const uint8_t blob[]);
        uint16_t Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const;
        bool Delete(const uint32_t id);

    private:
        mutable WPEFramework::Core::CriticalSection _lock;
        Sec_ProcessorHandle* _secProcHandle;
        std::map<uint32_t, MapStore> _items;
        uint32_t _lastHandle;
    };

    class VaultNetflix : public Vault
    {
    public:
        static VaultNetflix& NetflixInstance();

    public:
        VaultNetflix();
        ~VaultNetflix();

    private:
        using Callback = std::function<void(VaultNetflix&)>;
        VaultNetflix(const Callback& ctor = nullptr, const Callback& dtor = nullptr);

    public:
        VaultNetflix(VaultNetflix const&) = delete;
        void operator=(VaultNetflix const&) = delete;

        uint16_t Size(const uint32_t id, bool allowSealed = false) const;
        uint32_t Import(const uint16_t size, const uint8_t blob[], bool exportable = false);
        uint16_t Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed = false) const;
        uint32_t Put(const uint16_t size, const uint8_t blob[]);
        uint16_t Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const;
        bool Delete(const uint32_t id);
        typedef std::map<std::string, uint32_t> NamedKeyMap;
        typedef std::map<uint32_t, Sec_SocKeyHandle> SocKeyMap;
        const NamedKeyMap& namedKeys();
        bool loadBoundKeys(const uint8_t buf[], uint16_t inSize);
        uint32_t ESN(uint8_t buffer[], uint32_t bufferLength) const;
        uint32_t GetNamedKey(std::string keyName);
        uint32_t AddKey(Sec_SocKeyHandle socKey);
        uint32_t DeleteKeyFromMap(uint32_t keyHandle);
        uint32_t GetNewKeyHandle();

        uint32_t getKeyInfo(uint32_t handle,uint32_t& type,uint32_t& algorithm,uint32_t& usages) const;

        uint32_t deleteKey(uint32_t  keyHandle);
        uint32_t FindKey(uint32_t  keyHandle, Sec_SocKeyHandle** pKey) const;
        const std::string& esn();
        uint32_t FindNamedKey(std::string keyName, uint32_t& keyHandle);

    private:
        mutable WPEFramework::Core::CriticalSection _lock;
        uint32_t _lastHandle;
        uint8_t* _pubKeyStore = NULL;
        uint16_t _peerPubSize = 0;
        Callback _dtor;
        std::string _esn;
        NamedKeyMap _namedKeys;
        SocKeyMap _socKeyMap;

    public:
        Sec_NetflixHandle* getNetflixHandle() const { return _netflixHandle; }

        uint8_t* getPeerKey() { return _pubKeyStore; }

        uint16_t getPeerSize() { return _peerPubSize; }

    private:
        Sec_NetflixHandle* _netflixHandle;

    };

} // namespace Implementation

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

#include <sec_security.h>
#include <sec_security_datatype.h>
#include <sec_security_common.h>
#include<string>

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

    template<typename OPERATION>
    class HashType : public HashImplementation {
    public:
        HashType(const HashType<OPERATION>&) = delete;
        HashType<OPERATION>& operator=(const HashType) = delete;
        HashType();
        HashType(const Sec_DigestAlgorithm digestAlg, const uint16_t digestSize);
        HashType(const Implementation::Vault* vault, const Sec_MacAlgorithm  macAlg, const uint16_t macSize, const uint32_t secretId, const uint16_t secretLength);
        ~HashType() override;

    private:
        const Implementation::Vault* _vault;
        const Implementation::Vault* _vault_digest = nullptr;
        uint16_t _size;
        bool _failure;
        Handle* handle = new Handle;
        Sec_KeyHandle* sec_key = nullptr;
        SEC_OBJECTID _id_sec;

    public:
        uint32_t Ingest(const uint32_t length, const uint8_t* data) override;
        uint8_t Calculate(const uint8_t maxLength, uint8_t* data) override;

    };

    class HashTypeNetflix : public HashImplementation {
    private:
        const Implementation::VaultNetflix* _vaultNetflix;
        bool _failure;
        Sec_NetflixHandle* _netflixHandle;
        Sec_SocKeyHandle* _secretKey = NULL;
        std::string _buffer = "";

    public:
        HashTypeNetflix(const Implementation::VaultNetflix* vault, const uint32_t secretId);
        uint32_t Ingest(const uint32_t length, const uint8_t* data) override;
        uint8_t Calculate(const uint8_t maxLength, uint8_t* data) override;
    };

} // namespace Implementation






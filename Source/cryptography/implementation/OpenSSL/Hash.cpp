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

#include <hash_implementation.h>

#include <core/core.h>

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#include "Vault.h"


struct HashImplementation {
    virtual uint32_t Ingest(const uint32_t length, const uint8_t data[]) = 0;
    virtual uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) = 0;

    virtual ~HashImplementation() { }
};

namespace Implementation {

const EVP_MD* Algorithm(const hash_type type)
{
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
}

namespace Operation {

    // Wrappers to get around different function names for digest and HMAC calculation.

    struct Digest {
        static int Init(EVP_MD_CTX* ctx, EVP_PKEY_CTX **pctx, const EVP_MD *type, EVP_PKEY *pkey) {
            return (1);
        }
        static int Update(EVP_MD_CTX* ctx, const void* d, size_t cnt) {
            return (EVP_DigestUpdate(ctx, d, cnt));
        }
        static int Final(EVP_MD_CTX* ctx, unsigned char* sig, size_t* siglen) {
            uint32_t siglen32 = (*siglen);
            int rv = EVP_DigestFinal(ctx, sig, &siglen32);
            (*siglen) = siglen32;
            return (rv);
        }
    };

    struct HMAC {
        static int Init(EVP_MD_CTX* ctx, EVP_PKEY_CTX **pctx, const EVP_MD *type, EVP_PKEY *pkey) {
            return (EVP_DigestSignInit(ctx, pctx, type, nullptr, pkey));
        }
        static int Update(EVP_MD_CTX* ctx, const void* d, size_t cnt) {
            return (EVP_DigestSignUpdate(ctx, d, cnt));
        }
        static int Final(EVP_MD_CTX* ctx, unsigned char* sig, size_t* siglen) {
            return (EVP_DigestSignFinal(ctx, sig, siglen));
        }
    };

} // namespace Operation

template<typename OPERATION>
class HashType : public HashImplementation {
public:
    HashType(const HashType<OPERATION>&) = delete;
    HashType<OPERATION>& operator=(const HashType) = delete;

    HashType(const EVP_MD* digest)
        : _ctx(nullptr)
        , _pkey(nullptr)
        , _vault(nullptr)
        , _size(0)
        , _failure(false)
    {
        ASSERT(digest != nullptr);

        _ctx = EVP_MD_CTX_create();
        ASSERT(_ctx != nullptr);

        if (EVP_DigestInit_ex(_ctx, digest, NULL) == 0) {
            TRACE_L1(_T("EVP_DigestInit_ex() failed"));
            _failure = true;
        } else {
            _size = EVP_MD_size(digest);
            ASSERT(_size != 0);
        }
    }

    HashType(const Implementation::Vault* vault, const EVP_MD* digest, const uint32_t secretId, const uint16_t secretLength)
        : HashType(digest)
    {
        ASSERT(vault != nullptr);
        ASSERT(secretId != 0);
        ASSERT(secretLength != 0);
        _vault = vault;

        if (_failure == false) {
            uint8_t* secret = reinterpret_cast<uint8_t*>(ALLOCA(secretLength));
            ASSERT(secret != nullptr);

            uint16_t secretLen = _vault->Export(secretId, secretLength, secret, true);
            ASSERT(secretLen != 0);

            if (secretLen != 0) {
                _pkey = EVP_PKEY_new_mac_key(EVP_PKEY_HMAC, nullptr, secret, secretLen);
                ASSERT(_pkey != nullptr);

                ::memset(secret, 0xFF, secretLen);

                if (OPERATION::Init(_ctx, nullptr, digest, _pkey) == 0) {
                    TRACE_L1(_T("Init() failed"));
                    _failure = true;
                }
            }
        }
    }

    ~HashType() override
    {
        if (_ctx != nullptr) {
            EVP_MD_CTX_destroy(_ctx);
        }
    }

public:
    uint32_t Ingest(const uint32_t length, const uint8_t* data) override
    {
        ASSERT(data != nullptr);

        if (_failure == false) {
            if (OPERATION::Update(_ctx, data, length) == 0) {
                TRACE_L1(_T("Update() failed"));
                _failure = true;
            }
        }

        return (_failure? 0 : length);
    }

    uint8_t Calculate(const uint8_t maxLength, uint8_t* data) override
    {
        uint8_t result = 0;

        if (_failure == true) {
            TRACE_L1(_T("Hash calculation failure"));
        } else {
            if (maxLength < _size) {
                TRACE_L1(_T("Output buffer to small, need %i bytes, got %i bytes"), _size, maxLength);
            } else {
                size_t len = maxLength;
                if (OPERATION::Final(_ctx, data, &len) == 0) {
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

private:
    EVP_MD_CTX* _ctx;
    EVP_PKEY* _pkey;
    const Implementation::Vault* _vault;
    uint16_t _size;
    bool _failure;
};

} // namespace Implementation

extern "C" {

HashImplementation* hash_create(const hash_type type)
{
    HashImplementation* implementation = nullptr;

    const EVP_MD *md = Implementation::Algorithm(type);
    if (md != nullptr) {
        implementation = new Implementation::HashType<Implementation::Operation::Digest>(md);
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
        const EVP_MD* md = Implementation::Algorithm(type);
        if (md != nullptr) {
            implementation = new Implementation::HashType<Implementation::Operation::HMAC>(vaultImpl, md, secret_id, secretLength);
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

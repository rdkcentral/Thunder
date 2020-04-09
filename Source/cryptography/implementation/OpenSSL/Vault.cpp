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

#include <openssl/evp.h>

#include "Vault.h"


namespace Implementation {

const uint32_t encryptionKeyId = 1;
const uint32_t encryptionIVId = 2;

/* static */ Vault& Vault::Instance()
{
    static Vault instance;
    return (instance);
}

Vault::Vault()
    : _lock()
    , _items()
    , _lastHandle(0x80000000)
{
    typedef uint8_t pkey[16];

    // ideally these should be derived from device ID
    static const pkey privateKeys[] = {
        { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 },
        { 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11, 0x22 },
    };

    _lock.Lock();
    for (uint8_t i = 0; i < (sizeof(privateKeys) / sizeof(pkey)); i++) {
        _items.emplace(std::piecewise_construct,
                    std::forward_as_tuple(i + 1),
                    std::forward_as_tuple(false, sizeof(privateKeys[i]), privateKeys[i]));
    }
    _lock.Unlock();
}

uint16_t Vault::Cipher(bool encrypt, const uint16_t inSize, const uint8_t input[], const uint16_t maxOutSize, uint8_t output[]) const
{
    uint16_t totalLen = 0;

    ASSERT(maxOutSize >= inSize);

    if (maxOutSize >= inSize) {
        auto keyIt = _items.find(encryptionKeyId);
        ASSERT(keyIt != _items.end());
        auto ivIt = _items.find(encryptionIVId);
        ASSERT(ivIt != _items.end());

        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        ASSERT(ctx != nullptr);

        int outLen = 0;
        // AES-CTR ensures same buffer size after encryption
        EVP_CipherInit_ex(ctx, EVP_aes_128_ctr(), nullptr, (*keyIt).second.Buffer(), (*ivIt).second.Buffer(), encrypt);
        EVP_CipherUpdate(ctx, output, &outLen, input, inSize);
        totalLen += outLen;
        EVP_EncryptFinal_ex(ctx, output + totalLen, &outLen);
        totalLen += outLen;

        EVP_CIPHER_CTX_cleanup(ctx);
    }

    return (totalLen);
}


uint16_t Vault::Size(const uint32_t id, bool allowSealed) const
{
    uint16_t size = 0;

    _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
        if ((allowSealed == true) || (*it).second.IsExportable() == true) {
            size = (*it).second.Size();
            TRACE_L2(_T("Blob id 0x%08x size: %i"), id, size);
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

uint32_t Vault::Import(const uint16_t size, const uint8_t blob[], bool exportable)
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);

        if (id != 0) {
            uint8_t* buf = reinterpret_cast<uint8_t*>(ALLOCA(USHRT_MAX));
            uint16_t len = Cipher(true, size, blob, USHRT_MAX, buf);

            _items.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(exportable, len, buf));

            _lastHandle = id;

            TRACE_L2(_T("Added a %s data blob of size %i as id 0x%08x"), (exportable? "clear": "sealed"), len, id);
        }
        _lock.Unlock();
    }

    return (id);
}

uint16_t Vault::Export(const uint32_t id, const uint16_t size, uint8_t blob[], bool allowSealed) const
{
    uint16_t outSize = 0;

    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            if ((allowSealed == true) || ((*it).second.IsExportable() == true)) {
                outSize = Cipher(false, (*it).second.Size(), (*it).second.Buffer(), size, blob);

                TRACE_L2(_T("Exported %i bytes from blob id 0x%08x"), outSize, id);
            } else {
                TRACE_L1(_T("Blob id 0x%08x is sealed, can't export"), id);
            }
        } else {
            TRACE_L1(_T("Failed to look up blob id 0x%08x"), id);
        }
        _lock.Unlock();
    }

    return (outSize);
}

uint32_t Vault::Put(const uint16_t size, const uint8_t blob[])
{
    uint32_t id = 0;

    if (size > 0) {
        _lock.Lock();
        id = (_lastHandle + 1);
        if (id != 0) {
            _items.emplace(std::piecewise_construct,
                           std::forward_as_tuple(id),
                           std::forward_as_tuple(false, size, blob));

            _lastHandle = id;

            TRACE_L2(_T("Inserted a sealed data blob of size %i as id 0x%08x"), size, id);
        }
        _lock.Unlock();
    }

    return (id);
}

uint16_t Vault::Get(const uint32_t id, const uint16_t size, uint8_t blob[]) const
{
    uint16_t result = 0;

    if (size > 0) {
        _lock.Lock();
        auto it = _items.find(id);
        if (it != _items.end()) {
            result = std::min(size, static_cast<uint16_t>((*it).second.Size()));
            ::memcpy(blob, (*it).second.Buffer(), result);
            TRACE_L2(_T("Retrieved a sealed data blob id 0x%08x of size %i bytes"), id, result);
        }
        _lock.Unlock();
    }

    return (result);
}

bool Vault::Delete(const uint32_t id)
{
    bool result = false;

   _lock.Lock();
    auto it = _items.find(id);
    if (it != _items.end()) {
        _items.erase(it);
        result = true;
    }
    _lock.Unlock();

    return (result);
}

} // namespace Implementation


extern "C" {

VaultImplementation* vault_instance(const cryptographyvault id)
{
    Implementation::Vault* vault = nullptr;

    switch(id) {
        case CRYPTOGRAPHY_VAULT_PLATFORM:
        case CRYPTOGRAPHY_VAULT_NETFLIX:
            vault = &Implementation::Vault::Instance();
        default:
            TRACE_L1(_T("Vault not supported: %i", static_cast<uint32_t>(id)));
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
    return (vaultImpl->Export(id, max_length, data));
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
    return (vaultImpl->Get(id, max_length, data));
}

bool vault_delete(VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    Implementation::Vault *vaultImpl = reinterpret_cast<Implementation::Vault*>(vault);
    return (vaultImpl->Delete(id));
}

uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[])
{
    return (0);
}

uint32_t netflix_security_encryption_key(void)
{
    return (0);
}

uint32_t netflix_security_hmac_key(void)
{
    return (0);
}

uint32_t netflix_security_wrapping_key(void)
{
    return (0);
}

} // extern "C"

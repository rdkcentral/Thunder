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
#include <hash_implementation.h>
#include <cipher_implementation.h>
#include <diffiehellman_implementation.h>

#include "Administrator.h"


extern "C" {

using namespace Implementation;

// -------------------------------------------
// Vaults
//

VaultImplementation* vault_instance(const cryptographyvault id)
{
    Platform::IVaultImplementation* instance = nullptr;

    IVaultFactory* factory = Administrator::Instance().Factory<IVaultFactory>(id);
    if (factory != nullptr) {
        instance = factory->Create(id);
        ASSERT(instance != nullptr);
    } else {
        TRACE_L1(_T("Vault not available: %i"), id);
    }

    return (Handle(instance));
}

uint16_t vault_size(const struct VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Size(id));
}

uint32_t vault_import(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[])
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Import(length, blob));
}

uint16_t vault_export(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[])
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Export(id, max_length, blob));
}

uint32_t vault_set(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[])
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Set(length, blob));
}

uint16_t vault_get(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[])
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Get(id, max_length, blob));
}

bool vault_delete(struct VaultImplementation* vault, const uint32_t id)
{
    ASSERT(vault != nullptr);
    return (Platform::Implementation(vault)->Delete(id));
}

// -------------------------------------------
// Hashes/HMACs
//

HashImplementation* hash_create(const hash_type type)
{
    Platform::IHashImplementation* impl = nullptr;

    IHashFactory* factory = Administrator::Instance().Factory<IHashFactory>(CRYPTOGRAPHY_VAULT_PLATFORM);
    if (factory != nullptr) {
        impl = factory->Create(type);
        if (impl == nullptr) {
            TRACE_L1(_T("Hash type %i not supported"), type);
        }
    } else {
        TRACE_L1(_T("Hash calculation not supported"));
    }

    return (Handle(impl));
};

HashImplementation* hash_create_hmac(const VaultImplementation* vault, const hash_type type, const uint32_t secret_id)
{
    ASSERT(vault != nullptr);

    Platform::IHashImplementation* impl = nullptr;

    IHMACFactory* factory = Administrator::Instance().Factory<IHMACFactory>(Platform::Implementation(vault)->Id());
    if (factory != nullptr) {
        impl = factory->Create(type, secret_id);
        if (impl == nullptr) {
            TRACE_L1(_T("HMAC type %i not supported on vault %i"), type, Platform::Implementation(vault)->Id());
        }
    } else {
        TRACE_L1(_T("HMAC calculation not supported on vault %i"), Platform::Implementation(vault)->Id());
    }

    return (Handle(impl));
}

void hash_destroy(struct HashImplementation* hash)
{
    ASSERT(hash != nullptr);
    delete Platform::Implementation(hash);
}

uint32_t hash_ingest(struct HashImplementation* hash, const uint32_t length, const uint8_t data[])
{
    ASSERT(hash != nullptr);
    return (Platform::Implementation(hash)->Ingest(length, data));
}

uint8_t hash_calculate(struct HashImplementation* hash, const uint8_t max_length, uint8_t data[])
{
    ASSERT(hash != nullptr);
    return (Platform::Implementation(hash)->Calculate(max_length, data));
}

// -------------------------------------------
// Ciphers
//

struct CipherImplementation* cipher_create_aes(const VaultImplementation* vault, const aes_mode mode, const uint32_t key_id)
{
    ASSERT(vault != nullptr);

    Platform::ICipherImplementation* impl = nullptr;

    IAESFactory* factory = Administrator::Instance().Factory<IAESFactory>(Platform::Implementation(vault)->Id());
    if (factory != nullptr) {
        impl = factory->Create(mode, key_id);
        if (impl == nullptr) {
            TRACE_L1(_T("AES mode %i not supported on vault %i"), mode, Platform::Implementation(vault)->Id());
        }
    } else {
        TRACE_L1(_T("AES cipher not supported on vault %i"), Platform::Implementation(vault)->Id());
    }

    return (Handle(impl));
}

void cipher_destroy(struct CipherImplementation* cipher)
{
    ASSERT(cipher != nullptr);
    delete Platform::Implementation(cipher);
}

uint32_t cipher_encrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[])
{
    ASSERT(cipher != nullptr);
    return (Platform::Implementation(cipher)->Encrypt(iv_length, iv, input_length, input, max_output_length, output));
}

uint32_t cipher_decrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[])
{
    ASSERT(cipher != nullptr);
    return (Platform::Implementation(cipher)->Decrypt(iv_length, iv, input_length, input, max_output_length, output));
}

// -------------------------------------------
// Diffie-Hellman algorithms
//

uint32_t diffiehellman_generate(struct VaultImplementation* vault,
                                const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                                uint32_t* private_key_id, uint32_t* public_key_id)
{
    uint32_t result = -1;

    ASSERT(vault != nullptr);
    ASSERT(private_key_id != nullptr);
    ASSERT(public_key_id != nullptr);

    IDHFactory* factory = Administrator::Instance().Factory<IDHFactory>(Platform::Implementation(vault)->Id());
    if (factory != nullptr) {
        result = factory->Create()->Generate(generator, modulusSize, modulus, (*private_key_id), (*public_key_id));
    } else {
        TRACE_L1(_T("diffiehellman not implemented"));
    }

    return (result);
}

uint32_t diffiehellman_derive(struct VaultImplementation* vault,
                              const uint32_t private_key_id, const uint32_t peer_public_key_id, uint32_t* secret_id)
{
    uint32_t result = -1;

    ASSERT(vault != nullptr);
    ASSERT(secret_id != nullptr);

    IDHFactory* factory = Administrator::Instance().Factory<IDHFactory>(Platform::Implementation(vault)->Id());
    if (factory != nullptr) {
        result = factory->Create()->Derive(private_key_id, peer_public_key_id, (*secret_id));
    } else {
        TRACE_L1(_T("diffiehellman not implemented"));
    }

    return (result);
}

} // extern "C"

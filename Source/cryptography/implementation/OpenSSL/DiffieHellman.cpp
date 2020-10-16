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


#include <openssl/dh.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "Vault.h"
#include "Derive.h"


namespace Implementation {

class KeyStore {
public:
    KeyStore(const KeyStore&) = delete;
    KeyStore& operator=(const KeyStore) = delete;

    KeyStore(Implementation::Vault* vault)
        : _vault(vault)
    {
        ASSERT(vault != nullptr);
    }
    ~KeyStore() = default;

public:
    uint32_t Serialize(const BIGNUM* key, bool exportable = false)
    {
        ASSERT(key != nullptr);

        uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(BN_num_bytes(key)));
        ASSERT(keyBuf != nullptr);

        const uint16_t keySize = BN_bn2bin(key, keyBuf);
        ASSERT(keySize < USHRT_MAX);

        return (_vault->Import(keySize, keyBuf, exportable));
    }

    uint32_t Serialize(const uint8_t keyBuf[], const uint16_t keySize, bool exportable = false)
    {
        ASSERT(keyBuf != nullptr);

        return (_vault->Import(keySize, keyBuf, exportable));
    }

    uint32_t Serialize(const DH* key)
    {
        ASSERT(key != nullptr);

        DHKeyHeader header;
        header.primeSize = BN_num_bytes(key->p);
        header.generatorSize =  BN_num_bytes(key->g);
        header.privateKeySize = BN_num_bytes(key->priv_key);
        header.publicKeySize = BN_num_bytes(key->pub_key);

        uint32_t keySize = (sizeof(header) + header.primeSize + header.generatorSize + header.privateKeySize + header.publicKeySize);
        ASSERT(keySize < USHRT_MAX);

        uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(keySize));
        ASSERT(keyBuf != nullptr);

        ::memcpy(keyBuf, &header, sizeof(header));
        uint16_t offset = sizeof(header);
        offset += BN_bn2bin(key->p, keyBuf + offset);
        offset += BN_bn2bin(key->g, keyBuf + offset);
        offset += BN_bn2bin(key->priv_key, keyBuf + offset);
        offset += BN_bn2bin(key->pub_key, keyBuf + offset);

        return (_vault->Import(keySize, keyBuf, false /* DH private key always sealed */));
    }

    void Deserialize(const uint32_t keyId, DH*& key)
    {
        ASSERT(key == nullptr);

        uint16_t keySize = _vault->Size(keyId, true);
        if (keySize == 0) {
            TRACE_L1(_T("Key 0x%08x does not exist"), keyId);
        } else {
            uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(keySize));
            ASSERT(keyBuf != nullptr);

            keySize = _vault->Export(keyId, keySize, reinterpret_cast<uint8_t*>(keyBuf), true);
            if (keySize == 0) {
                TRACE_L1(_T("Failed to acces key 0x%08x"), keyId);
            } else {
                key = DH_new();
                ASSERT(key != nullptr);

                DHKeyHeader* header = reinterpret_cast<DHKeyHeader*>(keyBuf);
                key->p = BN_bin2bn(header->data, header->primeSize, nullptr);
                key->g = BN_bin2bn((header->data + header->primeSize), header->generatorSize, nullptr);
                key->priv_key = BN_bin2bn((header->data + header->primeSize + header->generatorSize), header->privateKeySize, nullptr);
                key->pub_key = BN_bin2bn((header->data + header->primeSize + header->generatorSize + header->privateKeySize), header->publicKeySize, nullptr);

                ASSERT(key->p != nullptr);
                ASSERT(key->g != nullptr);
                ASSERT(key->priv_key != nullptr);
                ASSERT(key->pub_key != nullptr);
            }
        }
    }

    void Deserialize(const uint32_t keyId, BIGNUM*& key)
    {
        ASSERT(key == nullptr);

        uint16_t keySize = _vault->Size(keyId, true);
        if (keySize == 0) {
            TRACE_L1(_T("Key 0x%08x does not exist"), keyId);
        } else {
            uint8_t* keyBuf = reinterpret_cast<uint8_t*>(ALLOCA(keySize));
            ASSERT(keyBuf != nullptr);

            keySize = _vault->Export(keyId, keySize, keyBuf, true);
            if (keySize == 0) {
                TRACE_L1(_T("Failed to access key 0x%08x"), keyId);
            } else {
                key = BN_bin2bn(keyBuf, keySize, nullptr);
                ASSERT(key != nullptr);
            }
        }
    }

private:
    struct DHKeyHeader {
        uint16_t primeSize;
        uint16_t generatorSize;
        uint16_t privateKeySize;
        uint16_t publicKeySize;
        uint8_t data[0];
    };

    Implementation::Vault* _vault;
}; //class KeyStore

uint32_t GenerateDiffieHellmanKeys(KeyStore& store,
                                   const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                                   uint32_t& privateKeyId, uint32_t& publicKeyId)
{
    uint32_t result = -1;

    ASSERT(generator >= 1);
    ASSERT(modulusSize != 0);
    ASSERT(modulus != nullptr);

    privateKeyId = 0;
    publicKeyId = 0;

    TRACE_L2(_T("Generator: %i"), generator);
    TRACE_L2(_T("Modulus: %02x %02x %02x... (%i bytes)"), modulus[0], modulus[1], modulus[2], modulusSize);

    DH* dh = DH_new();
    ASSERT(dh != nullptr);

    if (dh == nullptr) {
        TRACE_L1(_T("DH_new() failed"));
    } else {
        dh->p = BN_bin2bn(modulus, modulusSize, NULL);
        dh->g = BN_new();
        ASSERT(dh->p != nullptr);
        ASSERT(dh->g != nullptr);

        BN_set_word(dh->g, generator);

        int codes = 0;
        if ((DH_check(dh, &codes) == 0) || (codes != 0)) {
            TRACE_L1(_T("DH parameters are invalid [0x%08x]!"), codes);
        } else {
            if (DH_generate_key(dh) == 0) {
                TRACE_L1(_T("DH_generate_key() failed"));
            } else {
                privateKeyId = store.Serialize(dh);
                publicKeyId = store.Serialize(dh->pub_key, true /* public key shall not be sealed */);

                ASSERT(privateKeyId != 0);
                ASSERT(publicKeyId != 0);

                if ((privateKeyId != 0) && (publicKeyId != 0)) {
                    result = 0;
                }
            }
        }

        DH_free(dh);
    }

    return (result);
}

void DiffieHellmanDeriveSecret(DH* privateKey, const BIGNUM* peerPublicKey, BIGNUM*& secret)
{
    ASSERT(privateKey != nullptr);
    ASSERT(peerPublicKey != nullptr);
    ASSERT(secret == nullptr);

    int flags = 0;
    if ((DH_check_pub_key(privateKey, peerPublicKey, &flags) == 0) || (flags != 0)) {
        TRACE_L1(_T("Peer public key is invalid"))
    } else {
        uint16_t secretSize = DH_size(privateKey);
        ASSERT(secretSize != 0);

        uint8_t* secretBuf = reinterpret_cast<uint8_t*>(ALLOCA(secretSize));
        ASSERT(secretBuf != nullptr);

        secretSize = DH_compute_key(secretBuf, peerPublicKey, privateKey);
        if (secretSize == 0) {
            TRACE_L1(_T("DH_compute_key() failed"));
        } else {
            secret = BN_bin2bn(secretBuf, secretSize, nullptr);
            ASSERT(secret != nullptr);
        }
    }
}

uint32_t DiffieHellmanDeriveSecret(KeyStore& store, const uint32_t privateKeyId, const uint32_t peerPublicKeyId, uint32_t& secretId)
{
    uint32_t result = -1;

    DH* privateKey = nullptr;
    store.Deserialize(privateKeyId, privateKey);
    ASSERT(privateKey != nullptr);

    BIGNUM* peerPublicKey = nullptr;
    store.Deserialize(peerPublicKeyId, peerPublicKey);
    ASSERT(peerPublicKey != nullptr);

    if ((privateKey == nullptr) || (peerPublicKey == nullptr)) {
        TRACE_L1(_T("Failed to retrieve source keys from the vault"));
    } else {
        BIGNUM* secret = nullptr;
        DiffieHellmanDeriveSecret(privateKey, peerPublicKey, secret);

        if (secret == nullptr) {
            TRACE_L1(_T("Failed to compute a Diffie-Hellman secret"));
        } else {
            secretId = store.Serialize(secret);
            if (secretId == 0) {
                TRACE_L1(_T("Failed to store computed Diffie-Hellman secret"));
            } else {
                TRACE_L2(_T("Computed Diffie-Hellman secret as 0x%08x"), secretId);
                result = 0;
            }

            BN_free(secret);
        }
    }

    if (privateKey != nullptr) {
        // This will free all BIGNUMs inside too
        DH_free(privateKey);
    }

    if (peerPublicKey != nullptr) {
        BN_free(peerPublicKey);
    }

    return (result);
}


namespace Netflix {

uint32_t DiffieHellmanAuthenticatedDeriveSecret(KeyStore& store,
                                                const uint32_t privateKeyId, const uint32_t peerPublicKeyId, const uint32_t derivationKeyId,
                                                uint32_t& encryptionKeyId, uint32_t& hmacKeyId, uint32_t& wrappingKeyId)
{
    uint32_t result = -1;

    DH* privateKey = nullptr;
    store.Deserialize(privateKeyId, privateKey);
    ASSERT(privateKey != nullptr);

    BIGNUM* peerPublicKey = nullptr;
    store.Deserialize(peerPublicKeyId, peerPublicKey);
    ASSERT(peerPublicKey != nullptr);

    BIGNUM* derivationKey = nullptr;
    store.Deserialize(derivationKeyId, derivationKey);
    ASSERT(derivationKey != nullptr);

    if ((privateKey == nullptr) || (peerPublicKey == nullptr) || (derivationKey == nullptr)) {
        TRACE_L1(_T("Failed to retrieve source keys from the vault"));
    } else {
        BIGNUM* secret = nullptr;
        DiffieHellmanDeriveSecret(privateKey, peerPublicKey, secret);

        if (secret == nullptr) {
            TRACE_L1(_T("Failed to compute a (standard) Diffie-Hellman secret"));
        } else {
            // Load the standard DH shared secret
            uint16_t secretSize = BN_num_bytes(secret);
            ASSERT(secretSize != 0);
            uint8_t* secretBuf = reinterpret_cast<uint8_t*>(ALLOCA(secretSize + 1));
            ASSERT(secretBuf != nullptr);
            secretSize = BN_bn2bin(secret, secretBuf + 1);
            ASSERT(secretSize != 0);

            // Make sure the secret is prepended with a 0
            // See: https://github.com/Netflix/msl/wiki/Authenticated-Diffie-Hellman-Key-Exchange
            if (secretBuf[1] != 0) {
                secretBuf[0] = 0;
                secretSize++;
            } else {
                secretBuf++;
            }

            // Let the derivation key be an AES key
            uint16_t derivationKeySize = BN_num_bytes(derivationKey);
            ASSERT(derivationKeySize != 0);
            uint8_t* derivationKeyBuf = reinterpret_cast<uint8_t*>(ALLOCA(derivationKeySize));
            ASSERT(derivationKeyBuf != nullptr);
            derivationKeySize = BN_bn2bin(derivationKey, derivationKeyBuf);
            ASSERT(derivationKeySize == 16);

            // Derive the encryption and HMAC keys:
            //   a) SHA the derivation key into a HMAC key
            uint8_t hmacKey[SHA384_DIGEST_LENGTH];
            SHA384(derivationKeyBuf, derivationKeySize, hmacKey);

            //   b) HMAC the DH secret with the HMAC key
            uint8_t hmac[SHA384_DIGEST_LENGTH];
            uint32_t hmacSize = 0;
            HMAC(EVP_sha384(), hmacKey, sizeof(hmacKey), secretBuf, secretSize, hmac, &hmacSize);
            ASSERT(hmacSize == SHA384_DIGEST_LENGTH);

            //   c) Take the first 16 bytes of the HMAC vector as the "encryption key" (AES 128-bit)...
            encryptionKeyId = store.Serialize(hmac, 16);
            ASSERT(encryptionKeyId != 0);
            //      ... and the remaining 32 bytes as the "HMAC key" (SHA256)
            hmacKeyId = store.Serialize(hmac + 16 , SHA256_DIGEST_LENGTH);
            ASSERT(hmacKeyId != 0);

            // Derive the wrapping key
            uint8_t wrappingKeyBuf[SHA256_DIGEST_LENGTH];
            uint32_t wrappingKeyBufSize = DeriveWrappingKey(hmac, hmacSize, sizeof(wrappingKeyBuf), wrappingKeyBuf);
            ASSERT(wrappingKeyBufSize == SHA256_DIGEST_LENGTH);

            // Take the first 16 bytes as the "wrapping key" (AES 128-bit)
            wrappingKeyId = store.Serialize(wrappingKeyBuf, 16);
            ASSERT(wrappingKeyId != 0);

            result = !((encryptionKeyId != 0) && (hmacKeyId != 0) && (wrappingKeyId != 0));
            if (result != 0) {
                TRACE_L1(_T("Failed to store computed keys into the vault"));
            } else {
                TRACE_L2(_T("Computed authenticated Diffie-Hellman keys (encryption: 0x%08x, hmac: 0x%08x, wrapping: 0x%08x)"),
                            encryptionKeyId, hmacKeyId, wrappingKeyId);
                result = 0;
            }

            BN_free(secret);
        }
    }

    if (privateKey != nullptr) {
        DH_free(privateKey);
    }

    if (peerPublicKey != nullptr) {
        BN_free(peerPublicKey);
    }

    if (derivationKey != nullptr) {
        BN_free(derivationKey);
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

    Implementation::KeyStore store(reinterpret_cast<Implementation::Vault*>(vault));
    return (Implementation::GenerateDiffieHellmanKeys(store, generator, modulusSize, modulus, (*private_key_id), (*public_key_id)));
}

uint32_t diffiehellman_derive(struct VaultImplementation* vault, const uint32_t private_key_id, const uint32_t peer_public_key_id, uint32_t* secret_id)
{
    ASSERT(vault != nullptr);
    ASSERT(secret_id != nullptr);

    Implementation::KeyStore store(reinterpret_cast<Implementation::Vault*>(vault));
    return (Implementation::DiffieHellmanDeriveSecret(store, private_key_id, peer_public_key_id, (*secret_id)));
}


// Netflix Security

uint32_t netflix_security_derive_keys(const uint32_t private_dh_key_id, const uint32_t peer_public_dh_key_id, const uint32_t derivation_key_id,
                                      uint32_t* encryption_key_id, uint32_t* hmac_key_id, uint32_t* wrapping_key_id)
{
    ASSERT(encryption_key_id != nullptr);
    ASSERT(hmac_key_id != nullptr);
    ASSERT(wrapping_key_id != nullptr);

    Implementation::KeyStore store(&Implementation::Vault::NetflixInstance());
    return (Implementation::Netflix::DiffieHellmanAuthenticatedDeriveSecret(store, private_dh_key_id, peer_public_dh_key_id, derivation_key_id,
                                                                           (*encryption_key_id), (*hmac_key_id), (*wrapping_key_id)));
}

} // extern "C"

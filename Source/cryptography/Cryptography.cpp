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

#include "Module.h"
#include <ICryptography.h>

#include "implementation/vault_implementation.h"
#include "implementation/hash_implementation.h"
#include "implementation/cipher_implementation.h"
#include "implementation/diffiehellman_implementation.h"


namespace WPEFramework {

namespace Implementation {

    class HashImpl : virtual public WPEFramework::Cryptography::IHash {
    public:
        HashImpl() = delete;
        HashImpl(const HashImpl&) = delete;
        HashImpl& operator=(const HashImpl&) = delete;

        HashImpl(HashImplementation* impl)
            : _implementation(impl)
        {
            ASSERT(_implementation != nullptr);
        }

        ~HashImpl() override
        {
            hash_destroy(_implementation);
        }

    public:
        uint32_t Ingest(const uint32_t length, const uint8_t data[]) override
        {
            return (hash_ingest(_implementation, length, data));
        }

        uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) override
        {
            return (hash_calculate(_implementation, maxLength, data));
        }

    public:
        BEGIN_INTERFACE_MAP(HashImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::IHash)
        END_INTERFACE_MAP

    private:
        HashImplementation* _implementation;
    }; // class HashImpl

    class VaultImpl : virtual public WPEFramework::Cryptography::IVault {
    public:
        VaultImpl() = delete;
        VaultImpl(const VaultImpl&) = delete;
        VaultImpl& operator=(const VaultImpl&) = delete;

        VaultImpl(VaultImplementation* impl)
            : _implementation(impl)
        {
            ASSERT(_implementation != nullptr);
        }

        ~VaultImpl() = default;

    public:
        uint16_t Size(const uint32_t id) const override
        {
            return (vault_size(_implementation, id));
        }

        uint32_t Import(const uint16_t length, const uint8_t blob[]) override
        {
            return (vault_import(_implementation, length, blob));
        }

        uint16_t Export(const uint32_t id, const uint16_t maxLength, uint8_t blob[]) const override
        {
            return (vault_export(_implementation, id, maxLength, blob));
        }

        uint32_t Set(const uint16_t length, const uint8_t blob[]) override
        {
            return (vault_set(_implementation, length, blob));
        }

        uint16_t Get(const uint32_t id, const uint16_t maxLength, uint8_t blob[]) const override
        {
            return (vault_get(_implementation, id, maxLength, blob));
        }

        bool Delete(const uint32_t id) override
        {
            return (vault_delete(_implementation, id));
        }

        VaultImplementation* Implementation()
        {
            return _implementation;
        }

        const VaultImplementation* Implementation() const
        {
            return _implementation;
        }

        class HMACImpl : public HashImpl {
        public:
            HMACImpl() = delete;
            HMACImpl(const HMACImpl&) = delete;
            HMACImpl& operator=(const HMACImpl&) = delete;

            HMACImpl(VaultImpl* vault, HashImplementation* implementation)
                : HashImpl(implementation)
                , _vault(vault)
            {
                ASSERT(vault != nullptr);
                _vault->AddRef();
            }

            ~HMACImpl() override
            {
                _vault->Release();
            }

        private:
            VaultImpl* _vault;
        }; // class HMACImpl

        class CipherImpl : virtual public WPEFramework::Cryptography::ICipher {
        public:
            CipherImpl() = delete;
            CipherImpl(const CipherImpl&) = delete;
            CipherImpl& operator=(const CipherImpl&) = delete;

            CipherImpl(VaultImpl* vault, CipherImplementation* implementation)
                : _vault(vault)
                , _implementation(implementation)
            {
                ASSERT(_implementation != nullptr);
                ASSERT(_vault != nullptr);
                _vault->AddRef();
            }

            ~CipherImpl() override
            {
                cipher_destroy(_implementation);
                _vault->Release();
            }

        public:
            uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                            const uint32_t inputLength, const uint8_t input[],
                            const uint32_t maxOutputLength, uint8_t output[]) const override
            {
                return (cipher_encrypt(_implementation, ivLength, iv, inputLength, input, maxOutputLength, output));
            }

            uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                            const uint32_t inputLength, const uint8_t input[],
                            const uint32_t maxOutputLength, uint8_t output[]) const override
            {
                return (cipher_decrypt(_implementation, ivLength, iv, inputLength, input, maxOutputLength, output));
            }

        public:
            BEGIN_INTERFACE_MAP(CipherImpl)
            INTERFACE_ENTRY(WPEFramework::Cryptography::ICipher)
            END_INTERFACE_MAP

        private:
            VaultImpl* _vault;
            CipherImplementation* _implementation;
        }; // class CipherImpl

        class DiffieHellmanImpl : virtual public WPEFramework::Cryptography::IDiffieHellman {
        public:
            DiffieHellmanImpl() = delete;
            DiffieHellmanImpl(const DiffieHellmanImpl&) = delete;
            DiffieHellmanImpl& operator=(const DiffieHellmanImpl&) = delete;

            DiffieHellmanImpl(VaultImpl* vault)
                : _vault(vault)
            {
                ASSERT(_vault != nullptr);
                _vault->AddRef();
            }

            ~DiffieHellmanImpl() override
            {
                _vault->Release();
            }

        public:
            uint32_t Generate(const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                            uint32_t& privKeyId, uint32_t& pubKeyId) override
            {
                return (diffiehellman_generate(_vault->Implementation(), generator, modulusSize, modulus, &privKeyId, &pubKeyId));
            }

            uint32_t Derive(const uint32_t privateKeyId, const uint32_t peerPublicKeyId, uint32_t& secretId) override
            {
                return (diffiehellman_derive(_vault->Implementation(), privateKeyId, peerPublicKeyId, &secretId));
            }

        public:
            BEGIN_INTERFACE_MAP(DiffieHellmanImpl)
            INTERFACE_ENTRY(WPEFramework::Cryptography::IDiffieHellman)
            END_INTERFACE_MAP

        private:
            VaultImpl* _vault;
        }; // class DiffieHellmanImpl

        WPEFramework::Cryptography::IHash* HMAC(const WPEFramework::Cryptography::hashtype hashType,
                                                const uint32_t secretId) override
        {
            WPEFramework::Cryptography::IHash* hmac(nullptr);

            HashImplementation* impl = hash_create_hmac(_implementation, static_cast<hash_type>(hashType), secretId);

            if (impl != nullptr) {
                hmac = WPEFramework::Core::Service<HMACImpl>::Create<WPEFramework::Cryptography::IHash>(this, impl);
                ASSERT(hmac != nullptr);

                if (hmac == nullptr) {
                    hash_destroy(impl);
                }
            }

            return (hmac);
        }

        WPEFramework::Cryptography::ICipher* AES(const WPEFramework::Cryptography::aesmode aesMode,
                                                 const uint32_t keyId) override
        {
            WPEFramework::Cryptography::ICipher* cipher(nullptr);

            CipherImplementation* impl = cipher_create_aes(_implementation, static_cast<aes_mode>(aesMode), keyId);

            if (impl != nullptr) {
                cipher = Core::Service<CipherImpl>::Create<WPEFramework::Cryptography::ICipher>(this, impl);
                ASSERT(cipher != nullptr);

                if (cipher == nullptr) {
                    cipher_destroy(impl);
                }
            }

            return (cipher);
        }

        WPEFramework::Cryptography::IDiffieHellman* DiffieHellman() override
        {
            WPEFramework::Cryptography::IDiffieHellman* dh = Core::Service<DiffieHellmanImpl>::Create<WPEFramework::Cryptography::IDiffieHellman>(this);
            ASSERT(dh != nullptr);
            return (dh);
        }

    public:
        BEGIN_INTERFACE_MAP(VaultImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::IVault)
        END_INTERFACE_MAP

    private:
        VaultImplementation* _implementation;
    }; // class VaultImpl

    class CryptographyImpl : virtual public WPEFramework::Cryptography::ICryptography {
    public:
        CryptographyImpl(const CryptographyImpl&) = delete;
        CryptographyImpl& operator=(const CryptographyImpl&) = delete;
        CryptographyImpl() = default;
        ~CryptographyImpl() = default;

    public:
        WPEFramework::Cryptography::IHash* Hash(const WPEFramework::Cryptography::hashtype hashType) override
        {
            WPEFramework::Cryptography::IHash* hash(nullptr);

            HashImplementation* impl = hash_create(static_cast<hash_type>(hashType));
            if (impl != nullptr) {
                hash = WPEFramework::Core::Service<Implementation::HashImpl>::Create<WPEFramework::Cryptography::IHash>(impl);
                ASSERT(hash != nullptr);

                if (hash == nullptr) {
                    hash_destroy(impl);
                }
            }

            return (hash);
        }

        WPEFramework::Cryptography::IVault* Vault(const cryptographyvault id) override
        {
            WPEFramework::Cryptography::IVault* vault(nullptr);
            VaultImplementation* impl = vault_instance(id);

            if (impl != nullptr) {
                vault = WPEFramework::Core::Service<Implementation::VaultImpl>::Create<WPEFramework::Cryptography::IVault>(impl);
                ASSERT(vault != nullptr);
            }

            return (vault);
        }

    public:
        BEGIN_INTERFACE_MAP(CryptographyImpl)
        INTERFACE_ENTRY(WPEFramework::Cryptography::ICryptography)
        END_INTERFACE_MAP
    }; // class CryptographyImpl

} // namespace Implementation


/* static */ Cryptography::ICryptography* Cryptography::ICryptography::Instance(const std::string& connectionPoint)
{
    Cryptography::ICryptography* result(nullptr);

    if (connectionPoint.empty() == true) {
        result = Core::Service<Implementation::CryptographyImpl>::Create<Cryptography::ICryptography>();
    }

    return result;
}

}

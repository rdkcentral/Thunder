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

#pragma once

#include <cstdint>

#include <vault_implementation.h>
#include <hash_implementation.h>
#include <cipher_implementation.h>
#include <diffiehellman_implementation.h>


namespace Implementation {

namespace Platform {

    struct IVaultImplementation {
        virtual ~IVaultImplementation() { }

        virtual cryptographyvault Id() const { return (CRYPTOGRAPHY_VAULT_PLATFORM); }

        virtual uint16_t Size(const uint32_t id) const = 0;
        virtual uint32_t Import(const uint16_t length, const uint8_t data[]) = 0;
        virtual uint16_t Export(const uint32_t id, const uint16_t max_length, uint8_t data[]) const = 0;
        virtual uint32_t Set(const uint16_t length, const uint8_t data[]) = 0;
        virtual uint16_t Get(const uint32_t id, const uint16_t max_length, uint8_t data[]) const = 0;
        virtual bool Delete(const uint32_t id) = 0;
    };

    inline VaultImplementation* Handle(IVaultImplementation* impl) { return reinterpret_cast<VaultImplementation*>(impl); }
    inline IVaultImplementation* Implementation(VaultImplementation* handle) { return reinterpret_cast<IVaultImplementation*>(handle); }
    inline const IVaultImplementation* Implementation(const VaultImplementation* handle) { return reinterpret_cast<const IVaultImplementation*>(handle); }

    struct IHashImplementation {
        virtual ~IHashImplementation() { }

        virtual uint32_t Ingest(const uint32_t length, const uint8_t data[]) = 0;
        virtual uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) = 0;
    };

    inline HashImplementation* Handle(IHashImplementation* impl) { return reinterpret_cast<HashImplementation*>(impl); }
    inline IHashImplementation* Implementation(HashImplementation* handle) { return reinterpret_cast<IHashImplementation*>(handle); }
    inline const IHashImplementation* Implementation(const HashImplementation* handle) { return reinterpret_cast<const IHashImplementation*>(handle); }

    struct ICipherImplementation {
        virtual ~ICipherImplementation() { }

        virtual uint32_t Encrypt(const uint8_t ivLength, const uint8_t iv[],
                                const uint32_t inputLength, const uint8_t input[],
                                const uint32_t maxOutputLength, uint8_t output[]) const = 0;
        virtual uint32_t Decrypt(const uint8_t ivLength, const uint8_t iv[],
                                const uint32_t inputLength, const uint8_t input[],
                                const uint32_t maxOutputLength, uint8_t output[]) const = 0;
    };

    inline CipherImplementation* Handle(ICipherImplementation* impl) { return reinterpret_cast<CipherImplementation*>(impl); }
    inline ICipherImplementation* Implementation(CipherImplementation* handle) { return reinterpret_cast<ICipherImplementation*>(handle); }
    inline const ICipherImplementation* Implementation(const CipherImplementation* handle) { return reinterpret_cast<const ICipherImplementation*>(handle); }

    struct IDiffieHellmanImplementation {
        virtual ~IDiffieHellmanImplementation() { }

        virtual uint32_t Generate(const uint8_t generator, const uint16_t modulusSize, const uint8_t modulus[],
                                  uint32_t& privKeyId, uint32_t& pubKeyId) = 0;
        virtual uint32_t Derive(const uint32_t privateKey, const uint32_t peerPublicKeyId, uint32_t& secretId) = 0;
    };

} // namespace Platform

} // namespace Implementation

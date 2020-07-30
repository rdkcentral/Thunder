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

#include "../TEE.h"
#include "../TEECommand.h"

#include <cryptography_vault_ids.h>


namespace Implementation {

namespace Platform {

namespace Netflix {

    class TEE : public Platform::TEE  {
    private:
        TEE();
        ~TEE();

    public:
        TEE(const TEE&) = delete;
        TEE& operator=(const TEE&) = delete;

    public:
        static TEE& Instance()
        {
            static TEE instance;
            return (instance);
        }

        static constexpr cryptographyvault ID = CRYPTOGRAPHY_VAULT_NETFLIX;

    public:
        static const uint16_t MAX_DATA_SIZE = 16384; // NetflixTA won't accept larger data blobs
        static const uint16_t MAX_CLEAR_KEY_SIZE = 512;
        static const uint32_t USER_KEY = (1 << 31);

        enum namedkeys : uint32_t {
            KDE = 1,
            KDH = 2,
            KDW = 3,
#ifdef __DEBUG__
            KDW_TEST = 4,
#endif
            LOCAL_KEY_MAX
        };

        enum class hashtype : uint32_t {
            UNDEFINED = 0,
            SHA256 = 9
        };

        enum class cryptop : uint32_t {
            ENCRYPT = 0,
            DECRYPT = 1
        };

        enum class keyformat : uint32_t {
            RAW = 0
        };

        enum class keytype : uint32_t {
            SECRET = 0,
            PUBLIC = 1,
            PRIVATE = 2
        };

        uint32_t ESNLength(uint16_t& length) const;

        uint32_t ESN(uint8_t buffer[], uint16_t& bufferLength) const;

        uint32_t HMAC(const uint32_t secretId, const hashtype hashType, const uint8_t buffer[], const uint32_t bufferSize,
                    uint8_t hmacBuffer[], uint32_t& hmacSize) const;

        uint32_t KeyInfo(const uint32_t id, keytype& type, bool& exportable, uint32_t& algorithm, uint32_t& usage) const;

        uint32_t ImportClearKey(const uint8_t buffer[], const uint32_t length,
                                const keyformat format, const uint32_t algorithm, const uint32_t usage, uint32_t& id, keytype& type);

        uint32_t ExportClearKey(const uint32_t id, const keyformat format, uint8_t buffer[], uint32_t& length) const;

        uint32_t ImportSealedKey(const uint8_t buffer[], const uint32_t length,
                                uint32_t& id, uint32_t& algorithm, uint32_t& usage, keytype& type, uint32_t& keySize);

        uint32_t ExportSealedKey(const uint32_t id, uint8_t buffer[], uint32_t& size) const;

        uint32_t DeleteKey(const uint32_t id);

        uint32_t DHGenerateKeys(const uint8_t generator[], const uint32_t generatorSize,
                                const uint8_t modulus[], const uint32_t modulusSize,
                                uint32_t& dhKey);

        uint32_t DHAuthenticatedDeriveKeys(const uint32_t privateKeyId, const uint32_t derivationKeyId,
                                        const uint8_t peerPublicKey[], const uint32_t peerPublicKeySize,
                                        uint32_t& encryptionKeyId, uint32_t& hmacKeyId, uint32_t& wrappingKeyId);

        uint32_t AESCBC(const uint32_t keyId, const cryptop operation,
                        const uint8_t iv[], const uint32_t ivLength,
                        const uint8_t input[], const uint32_t inputLength,
                        uint8_t output[], uint32_t& outputLength) const;

    private:
        uint32_t Initialize(const std::string& drmBinPath);

        uint32_t Deinitialize();

        uint32_t NamedKey(const uint8_t keyName[], const uint32_t keyNameLength, uint32_t& keyHandle) const;

    private:
        uint32_t InId(const uint32_t id) const
        {
            if (id == 0) {
                return (0);
            } else if (id & USER_KEY) {
                return (id & ~USER_KEY);
            } else {
                ASSERT((id <= sizeof(_localIds)/sizeof(uint32_t)) && "Invalid internal key id");
                return (_localIds[id - 1]);
            }
        }

        uint32_t OutId(const uint32_t id) const
        {
            return (USER_KEY | id);
        }

    private:
        uint32_t _localIds[LOCAL_KEY_MAX - 1];
    };

} // namespace Netflix

} // namespace Platform

} // namespace Implementation

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

#include "../../../Module.h"

#include "../Administrator.h"
#include "TEE.h"


namespace Implementation {

namespace Platform {

namespace Netflix {

    class Vault : public Platform::IVaultImplementation {
    private:
        Vault(const Vault&) = delete;
        Vault& operator=(const Vault) = delete;
        Vault() = default;
        ~Vault() = default;

    public:
        static Vault& Instance()
        {
            static Vault instance;
            return (instance);
        }

        cryptographyvault Id() const override
        {
            return TEE::ID;
        }

        uint16_t Size(const uint32_t id) const override
        {
            uint16_t size = 0;

            TEE::keytype type;
            bool extractable = false;
            uint32_t algorithm;
            uint32_t usageFlags;

            uint32_t rc = TEE::Instance().KeyInfo(id, type, extractable, algorithm, usageFlags);
            if (rc != 0) {
                TRACE_L1(_T("NetflixTA: KeyInfo() failed [0x%08x]"), rc);
            } else {
                size = USHRT_MAX;

                if (extractable == true) {
                    uint32_t outSize = TEE::MAX_CLEAR_KEY_SIZE;
                    uint8_t* buffer = reinterpret_cast<uint8_t*>(ALLOCA(outSize));
                    ASSERT(buffer != nullptr);

                    uint32_t rc = TEE::Instance().ExportClearKey(id, TEE::keyformat::RAW, buffer, outSize);
                    if (rc == 0) {
                        size = outSize;
                    }
                } else {
                    TRACE_L1(_T("NetflixTA: Data is not exportable, size not known"));
                }
            }

            return (size);
        }

        uint32_t Import(const uint16_t length, const uint8_t data[]) override
        {
            uint32_t id = 0;

            if ((length > 0) && (length <= TEE::MAX_CLEAR_KEY_SIZE)) {
                TEE::keytype type = TEE::keytype::SECRET;
                uint32_t algorithm = 0;
                uint32_t usage = 0xC;

#ifdef __DEBUG__
                // For testing purposes only
                if (length == 16) {
                    algorithm = 1;
                    usage = 3;
                }
#endif // __DEBUG__

                uint32_t rc = TEE::Instance().ImportClearKey(data, length, TEE::keyformat::RAW, algorithm, usage, id, type);
                if (rc != 0) {
                    TRACE_L1(_T("NetflixTA: ImportClearKey() failed [0x%08x]"), rc);
                    id = 0;
                }
            } else {
                TRACE_L1(_T("NetflixTA: Invalid buffer length given [%i]"), length);
            }

            return (id);
        }

        uint16_t Export(const uint32_t id, const uint16_t max_length, uint8_t data[]) const override
        {
            uint32_t outSize = 0;

            if (max_length > 0) {
                TEE::keytype type;
                uint32_t algorithm;
                uint32_t usageFlags;
                bool extractable = false;

                uint32_t rc = TEE::Instance().KeyInfo(id, type, extractable, algorithm, usageFlags);
                if (rc != 0) {
                    TRACE_L1(_T("NetflixTA: KeyInfo() failed [0x%08x]"), rc);
                } else if (extractable == true) {
                    outSize = std::min(max_length, TEE::MAX_CLEAR_KEY_SIZE);

                    uint32_t rc = TEE::Instance().ExportClearKey(id, TEE::keyformat::RAW, data, outSize);
                    if (rc != 0) {
                        outSize = 0;
                    }
                } else {
                    TRACE_L1(_T("NetflixTA: Data is not exportable in clear"));
                }
            }

            return (outSize);
        }

        uint32_t Set(const uint16_t length, const uint8_t data[]) override
        {
            uint32_t id = 0;

            if ((length > 0) && (length <= TEE::MAX_DATA_SIZE)) {
                uint32_t algorithm;
                uint32_t usage;
                TEE::keytype type;
                uint32_t size = 0;

                uint32_t rc = TEE::Instance().ImportSealedKey(data, length, id, algorithm, usage, type, size);
                if ((rc != 0) && (size > 0)) {
                    TRACE_L1(_T("NetflixTA: ImportSealedKey() failed [0x%08x]"), rc);
                    id = 0;
                }
            } else {
                TRACE_L1(_T("NetflixTA: Invalid buffer length given [%i]"), length);
            }

            return (id);
        }

        uint16_t Get(const uint32_t id, const uint16_t max_length, uint8_t data[]) const override
        {
            uint32_t outSize = 0;

            if (max_length > 0) {
                outSize = std::min(max_length, TEE::MAX_DATA_SIZE);
                uint32_t rc = TEE::Instance().ExportSealedKey(id, data, outSize);
                if (rc != 0) {
                    outSize = 0;
                }
            }

            return (outSize);
        }

        bool Delete(const uint32_t id) override
        {
            bool result = false;

            TEE::keytype type;
            bool extractable;
            uint32_t algorithm;
            uint32_t usage;

            /* DeleteKey() succeeds even if the key is not present, hence check if the key exists first. */
            uint32_t rc = TEE::Instance().KeyInfo(id, type, extractable, algorithm, usage);
            if (rc != 0) {
                TRACE_L1(_T("NetflixTA: KeyInfo() failed [0x%08x]"), rc);
            } else {
                result = (TEE::Instance().DeleteKey(id) == 0);
            }

            return (result);
        }
    }; // class VaultImplementation

    class VaultFactory : public VaultFactoryType<TEE::ID, Vault> {
        Platform::IVaultImplementation* Create(const cryptographyvault id) override
        {
            ASSERT(id == TEE::ID);
            return (&Vault::Instance());
        }
    }; // class VaultFactory

} // namespace Netflix


static PlatformRegistrationType<IVaultFactory, Netflix::VaultFactory> registration(/* default vault */ true);

} // namespace Platform

} // namespace Implementation

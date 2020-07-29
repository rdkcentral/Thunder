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

#include "../Nexus.h"

#if (defined NEXUS_HAS_SECURITY) && (NEXUS_SECURITY_API_VERSION == 1)
#include <nexus_hmac_sha_cmd.h>
#include <nexus_base_mmap.h>
#include <nexus_base_os.h>
#endif

namespace Implementation {

namespace Platform {

#if (defined NEXUS_HAS_SECURITY)

#if (NEXUS_SECURITY_API_VERSION == 1)

namespace NexusSecurity {

    class SHA : public Platform::IHashImplementation {
    public:
        SHA(const SHA&) = delete;
        SHA& operator=(const SHA) = delete;
        SHA() = delete;

        SHA(const hash_type type)
            : _buffer()
            , _ingestFailed(false)
        {
            ::memset(&_hashSettings, 0, sizeof(_hashSettings));

            /* Configure for hash calculation (not HMAC) */
            _hashSettings.keySource = NEXUS_HMACSHA_KeySource_eKeyVal;
            _hashSettings.keyIncMode = NEXUS_HMACSHA_KeyInclusion_Op_eNo;
            _hashSettings.context = NEXUS_HMACSHA_BSPContext_eContext0;
            _hashSettings.dataSrc = NEXUS_HMACSHA_DataSource_eDRAM;
            _hashSettings.keySource = NEXUS_HMACSHA_KeySource_eKeyVal;
            _hashSettings.keyLength = 0;

            /* Operation is SHA */
            _hashSettings.opMode = NEXUS_HMACSHA_Op_eSHA;
            _hashSettings.shaType = HashType(type);
            ASSERT(_hashSettings.shaType != NEXUS_HMACSHA_SHAType_eMax);

            /* The data needs to be fed in big-endian long words, so a byte-order swap is required. */
            _hashSettings.byteSwap = TRUE;

            /* Partial ingest is supported, however limited to data blocks of 64..512 bytes only.
               TODO: Consider revisiting this implementation if memory usage is a concern. */
            _hashSettings.dataMode = NEXUS_HMACSHA_DataMode_eAllIn;
        }

        ~SHA() override = default;

    public:
        uint32_t Ingest(const uint32_t length, const uint8_t data[]) override
        {
            _buffer.append(reinterpret_cast<const char*>(data), length);
            return (length);
        }

        uint8_t Calculate(const uint8_t maxLength, uint8_t data[]) override
        {
            uint8_t outLength = 0;

            if ((Nexus::Initialized() == true) && (_buffer.empty() == false) && (maxLength != 0)) {
                uint8_t* address = nullptr;

                NEXUS_HMACSHA_DigestOutput output;
                memset(&output, 0, sizeof(output));

                NEXUS_MemoryAllocationSettings allocSettings;
                NEXUS_Memory_GetDefaultAllocationSettings(&allocSettings);
                allocSettings.alignment = 32;

                NEXUS_Error err = NEXUS_Memory_Allocate(RoundUp4(_buffer.length()), &allocSettings, reinterpret_cast<void**>(&address));
                if (err != NEXUS_SUCCESS) {
                    TRACE_L1(_T("NEXUS_Memory_Allocate() failed [0x%08x]"), err);
                } else if (address != nullptr) {
                    BKNI_Memcpy(address, _buffer.data(), _buffer.length());
                    NEXUS_FlushCache(address, RoundUp4(_buffer.length()));

                    _hashSettings.dataSize = _buffer.length();
                    _hashSettings.dataAddress = address;

                    err = NEXUS_HMACSHA_PerformOp(&_hashSettings, &output);
                    if (err != NEXUS_SUCCESS) {
                        TRACE_L1(_T("NEXUS_HMACSHA_PerformOp() failed [0x%08x]"), err);
                    } else {
                        if (output.digestSize <= maxLength) {
                            outLength = output.digestSize;
                            ::memcpy(data, output.digestData, outLength);
                        } else {
                            TRACE_L1(_T("Output buffer too small for digest, need at least %i bytes"), output.digestSize);
                        }
                    }

                    NEXUS_Memory_Free(address);

                    _buffer.clear();
                }
            }

            return (outLength);
        }

        static inline uint32_t RoundUp4(const uint32_t val)
        {
            return ((val & 3)? (val + (4 - (val & 3))) : val);
        }

        static NEXUS_HMACSHA_SHAType HashType(const hash_type type)
        {
            NEXUS_HMACSHA_SHAType teeType = NEXUS_HMACSHA_SHAType_eMax;

            switch (type) {
            case hash_type::HASH_TYPE_SHA1:
                teeType = NEXUS_HMACSHA_SHAType_eSha160;
                break;
            case hash_type::HASH_TYPE_SHA224:
                teeType = NEXUS_HMACSHA_SHAType_eSha224;
                break;
            case hash_type::HASH_TYPE_SHA256:
                teeType = NEXUS_HMACSHA_SHAType_eSha256;
                break;
            default:
                break;
            }

            return (teeType);
        }

    private:
        NEXUS_HMACSHA_OpSettings _hashSettings;
        std::string _buffer;
        bool _ingestFailed;
    };

    class HashFactory : public HashFactoryType<CRYPTOGRAPHY_VAULT_PLATFORM, SHA> {
        Platform::IHashImplementation* Create(const hash_type type) override
        {
            Platform::IHashImplementation* impl = nullptr;

            if (SHA::HashType(type) != NEXUS_HMACSHA_SHAType_eMax) {
                impl =  new SHA(type);
                ASSERT(impl != nullptr);
            } else {
                TRACE_L1(_T("NexusSecurity: Hash type not supported [%i]"), type);
            }

            return (impl);
        }
    };

} // namespace NexusSecurity

static PlatformRegistrationType<IHashFactory, NexusSecurity::HashFactory> registration;

#else // (NEXUS_SECURITY_API_VERSION == 1)

#warning Nexus security v1 not available

#endif // (NEXUS_SECURITY_API_VERSION == 1)

#endif // (defined NEXUS_HAS_SECURITY)

} // namespace Platform

} // namespace Implementation

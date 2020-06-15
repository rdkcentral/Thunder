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

#include "TEE.h"

#include "../Nexus.h"


#if (defined NEXUS_HAS_SAGE)

#include <drm_types.h>
#include <drm_types_tl.h>
#include <drm_data.h>
#include <drm_common_tl.h>
#include <drm_metadata.h>

#include "../TEECommand.h"


namespace Implementation {

namespace Platform {

namespace Netflix {

    const uint32_t CLEAR_KEY = 0x80;

    TEE::TEE()
        : Platform::TEE()
    {
        ::memset(_localIds, 0, sizeof(_localIds));

        if (Nexus::Initialized() == true) {
            std::string binPath;
            if (WPEFramework::Core::SystemInfo::GetEnvironment(_T("NETFLIX_VAULT"), binPath) == false) {
                binPath = _T("drm.bin");
            }

            uint32_t result = Initialize(binPath);
            ASSERT(ModuleHandle() != nullptr);
            ASSERT(DRMHandle() != 0);

            if ((result != 0) || (ModuleHandle() == nullptr) || (DRMHandle() == 0)) {
                TRACE_L1(_T("Initialize() failed [0x%08x]"), result);
            } else {
                // Map private TEE keys to IDs
                result = NamedKey(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("DKE")), 3, _localIds[KDE - 1]);
                if (result != 0) {
                    TRACE_L1(_T("NamedKey('DKE') failed [0x%08x]"), result);
                }

                result = NamedKey(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("DKH")), 3, _localIds[KDH - 1]);
                if (result != 0) {
                    TRACE_L1(_T("NamedKey('DKH') failed [0x%08x]"), result);
                }

                result = NamedKey(const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("DKW")), 3, _localIds[KDW - 1]);
                if (result != 0) {
                    TRACE_L1(_T("NamedKey('DKW') failed [0x%08x]"), result);
                }

#ifdef __DEBUG__
                // Add an extra wrapping key (for testing purpose only)
                static const uint8_t testKey[16] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };
                TEE::keytype testKeyType;
                if (ImportClearKey(testKey, sizeof(testKey), TEE::keyformat::RAW, 0xC, 0x40, _localIds[KDW_TEST - 1], testKeyType) != 0) {
                    TRACE_L1(_T("Failed to install the test wrapping key!"));
                } else {
                    _localIds[KDW_TEST - 1] &= ~USER_KEY; // force it to be an internal key
                }
#endif // __DEBUG__

#ifdef __DEBUG__
                // Dump the private keys (for dev purpose only)
                for (uint8_t i = 0; i < sizeof(_localIds)/sizeof(uint32_t); i++) {
                    TEE::keytype type;
                    bool exportable;
                    uint32_t algorithm;
                    uint32_t usage;
                    KeyInfo(_localIds[i], type, exportable, algorithm, usage);
                }
#endif // __DEBUG__
            }
        }

        if ((ModuleHandle() == nullptr) || (DRMHandle() == 0)) {
            TRACE_L1(_T("Failed to initialize Sage/Netflix Cryptography. Subsequent cryptography calls are expected to fail."));
        } else {
            TRACE_L1(_T("Sage/Netflix Cryptography initialized successfully!"));
        }
    }

    TEE::~TEE()
    {
        if (ModuleHandle() != nullptr) {
            uint32_t result = Deinitialize();
            if (result != 0)  {
                TRACE_L1(_T("Deinitialize() failed [0x%08x]"), result);
            }
        }
    }

    uint32_t TEE::Initialize(const std::string& drmBinPath)
    {
        ASSERT(ModuleHandle() == nullptr);
        ASSERT(DRMHandle() == 0);

        DRMVersion(1);

        uint32_t result = -1;

        DrmCommonInit_TL_t init;
        ::memset(&init, 0, sizeof(init));
        init.drmCommonInit.heap = nullptr;
        init.ta_bin_file_path = ((DRM_Common_GetChipType() == ChipType_eZS)? bdrm_get_ta_nflx_dev_bin_file_path() : bdrm_get_ta_nflx_bin_file_path());
        init.drmType = BSAGElib_BinFileDrmType_eNetflix;
        uint32_t drmBinFileSize = 0;

        DrmRC rc = DRM_Common_P_GetFileSize(const_cast<char*>(drmBinPath.c_str()), &drmBinFileSize);
        if ((rc != Drm_Success) || (drmBinFileSize == 0)) {
            TRACE_L2(_T("Bin file '%s' not found"), drmBinPath.c_str());
        } else {
            DrmRC rc = DRM_Common_TL_Initialize(&init);
            if (rc != Drm_Success) {
                TRACE_L1(_T("DRM_Common_TL_Initialize() failed [0x%08x]"), rc);
            } else {
                TEECommand command;
                command.Block(1, drmBinFileSize); // out

                TRACE_L1(_T("Initializing Sage/Netflix module '%s' with bin file '%s'..."), init.ta_bin_file_path, drmBinPath.c_str());

                SRAI_ModuleHandle moduleHandle = nullptr;
                DrmRC rc = DRM_Common_TL_ModuleInitialize_TA(Common_Platform_Netflix, 1, const_cast<char*>(drmBinPath.c_str()),
                                                            (*command), &moduleHandle);
                if (rc != Drm_Success) {
                    TRACE_L1(_T("DRM_Common_TL_ModuleInitialize_TA() failed [0x%08x]"), rc);
                } else {
                    ModuleHandle(moduleHandle);
                    DRMHandle(command.Basic(2));
                    result = 0;

                    TRACE_L2(_T("Successfully initialized Sage/Netflix module (moduleHandle: 0x%08x, drmHandle: 0x%08x)"),
                                reinterpret_cast<uint32_t>(ModuleHandle()), DRMHandle());
                }
            }
        }

        return (result);
    }

    uint32_t TEE::Deinitialize()
    {
        ASSERT(ModuleHandle() != nullptr);

        uint32_t result = 0;

        DrmRC rc = DRM_Common_TL_ModuleFinalize_TA(Common_Platform_Netflix, reinterpret_cast<SRAI_ModuleHandle>(ModuleHandle()));
        if (rc != Drm_Success) {
            TRACE_L1(_T("DRM_Common_TL_ModuleFinalize_TA() failed [0x%08x]"), rc);
            result = -1;
        }

        rc = DRM_Common_TL_Finalize_TA(Common_Platform_Netflix);
        if (rc != Drm_Success) {
            TRACE_L1(_T("DRM_Common_TL_Finalize_TA() failed [0x%08x]"), rc);
            result = -1;
        }

        ModuleHandle(nullptr);
        DRMHandle(0);
        DRMVersion(0);

        return (result);
    }

    uint32_t TEE::ESNLength(uint16_t& length) const
    {
        TEECommandOld command(17, this); // pre4.1 NRD call

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                length = command.Basic(1);

                TRACE_L2(_T("Retrieved ESN length: %i"), length);
            } else {
                TRACE_L1(_T("Failed to retrieve ESN length [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::ESN(uint8_t buffer[], uint16_t& bufferLength) const
    {
        ASSERT(buffer != nullptr);
        ASSERT(bufferLength != 0);

        TEECommandOld command(18, this); // pre4.1 NRD call
        command.Block(0, bufferLength); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                bufferLength = command.Block(0, 1, buffer);

                TRACE_L2(_T("Retrieved ESN: '%s'"), std::string(reinterpret_cast<char*>(buffer), bufferLength).c_str());
            } else {
                TRACE_L1(_T("Failed to retrieve ESN [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::HMAC(const uint32_t secretId, const hashtype hashType,
                       const uint8_t data[], const uint32_t dataLength, uint8_t hmac[], uint32_t& hmacLength) const
    {
        ASSERT(data != nullptr);
        ASSERT(dataLength != 0);
        ASSERT(hmac != nullptr);
        ASSERT(hmacLength == 32);
        ASSERT(hashType == hashtype::SHA256); // only SHA256 supported

        TEECommand command(26, this);
        command.Basic(2, InId(secretId));
        command.Basic(3, static_cast<uint32_t>(hashType));
        command.Block(0, data, dataLength); // in
        command.Block(1, hmacLength); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                hmacLength = command.Block(1, 1, hmac);

                TRACE_L2(_T("Calculated HMAC using secret 0x%08x (hash type: %i, digest size: %i bytes)"),
                            secretId, static_cast<uint32_t>(hashType), hmacLength);
            } else {
                TRACE_L1(_T("Failed to calculate HMAC using key 0x%08x [0x%08x]"), secretId, command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::ImportClearKey(const uint8_t buffer[], const uint32_t length, const keyformat format, const uint32_t algorithm,
                                 const uint32_t usage, uint32_t& id, keytype& type)
    {
        ASSERT(buffer != nullptr);
        ASSERT(length != 0);
        ASSERT(length <= MAX_CLEAR_KEY_SIZE);

        TEECommand command(19, this);
        command.Basic(2, static_cast<uint32_t>(format));
        command.Basic(3, algorithm);
        /* Since the key has already been once present in the clear, there's no point in protecting it, hence make it always extractable. */
        command.Basic(4, (usage | CLEAR_KEY));
        command.Block(0, buffer, length); // in

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                id = OutId(command.Basic(1));
                type = static_cast<keytype>(command.Basic(2));

                TRACE_L2(_T("Imported clear key 0x%08x (size: %i bytes, type: %i)"), id, length, static_cast<uint32_t>(type));
            } else {
                TRACE_L1(_T("Failed to import clear key [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::ImportSealedKey(const uint8_t buffer[], const uint32_t length,
                                  uint32_t& id, uint32_t& algorithm, uint32_t& usage, keytype& type, uint32_t& keySize)
    {
        ASSERT(buffer != nullptr);
        ASSERT(length != 0);
        ASSERT(length <= MAX_DATA_SIZE);

        struct KeyInfo {
            uint32_t handle;
            uint32_t type;
            uint32_t algorithm;
            uint32_t usage;
        } __attribute__((packed));

        KeyInfo info;
        ::memset(&info, 0, sizeof(info));

        TEECommand command(20, this);
        command.Block(0, buffer, length); // in
        command.Block(1, sizeof(info)); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                uint32_t size = command.Block(1, 2, reinterpret_cast<uint8_t*>(&info));
                if (size != sizeof(info)) {
                    TRACE_L1(_T("Incorrect output from Sage (returned size: %i bytes, expected: %i bytes)"), size, sizeof(info));
                    result = -1;
                } else {
                    id = OutId(info.handle);
                    algorithm = info.algorithm;
                    type = static_cast<keytype>(info.type);
                    usage = info.usage;
                    keySize = command.Basic(1);

                    TRACE_L2(_T("Imported sealed key 0x%08x (size: %i bytes)"), id, length);
                }
            } else {
                TRACE_L1(_T("Failed to import sealed key [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::ExportClearKey(const uint32_t id, const keyformat format, uint8_t buffer[], uint32_t& length) const
    {
        ASSERT(buffer != nullptr);
        ASSERT(length != 0);

        TEECommand command(21, this);
        command.Basic(2, InId(id));
        command.Basic(3, static_cast<uint32_t>(format));
        command.Block(0, length); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                length = command.Block(0, 1, buffer);

                TRACE_L2(_T("Exported clear key 0x%08x (size: %i bytes, format: %i)"), id, length, static_cast<uint32_t>(format));
            } else {
                TRACE_L1(_T("Failed to export clear key 0x%08x [0x%08x]"), id, command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::ExportSealedKey(const uint32_t id, uint8_t buffer[], uint32_t& length) const
    {
        ASSERT(buffer != nullptr);
        ASSERT(length != 0);

        TEECommand command(22, this);
        command.Basic(2, InId(id));
        command.Block(0, length); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                length = command.Block(0, 1, buffer);

                TRACE_L2(_T("Exported sealed key 0x%08x (size: %i bytes)"), id, length);
            } else {
                TRACE_L1(_T("Failed to export sealed key 0x%08x [0x%08x]"), id, command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::KeyInfo(const uint32_t id, keytype& type, bool& exportable, uint32_t& algorithm, uint32_t& usage) const
    {
        struct KeyInfo {
            uint32_t handle;
            uint32_t type;
            uint32_t algorithm;
            uint32_t usage;
        } __attribute__((packed));

        KeyInfo info;
        ::memset(&info, 0, sizeof(info));

        TEECommand command(23, this);
        command.Basic(2, InId(id));
        command.Block(0, sizeof(info)); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                uint32_t size = command.Block(0, 1, reinterpret_cast<uint8_t*>(&info));
                if (size != sizeof(info)) {
                    TRACE_L1(_T("Incorrect output from Sage (returned size: %i bytes, expected: %i bytes)"), size, sizeof(info));
                } else {
                    type = static_cast<keytype>(info.type);
                    algorithm = info.algorithm;
                    usage = info.usage;
                    exportable = (info.usage & CLEAR_KEY);

                    TRACE_L2(_T("Retrieved key 0x%08x information (type: 0x%08x, exportable: %s, algorithm: 0x%08x, usage: 0x%08x)"),
                                id, static_cast<uint32_t>(type), (exportable == 0? "false" : "true"), algorithm, usage);
                }
            } else {
                TRACE_L1(_T("Failed to retrieve key 0x%08x information [0x%08x]"), id, command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::NamedKey(const uint8_t name[], const uint32_t nameLength, uint32_t& id) const
    {
        ASSERT(name != nullptr);
        ASSERT(nameLength != 0);

        TEECommand command(24, this);
        command.Block(0, name, nameLength); // in

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                id = command.Basic(1); // internal key id!
                ASSERT((id & USER_KEY) == 0);

                TRACE_L2(_T("Retrieved named key '%s' as 0x%08x"),
                            string(reinterpret_cast<const char*>(name), nameLength).c_str(), id);
            } else {
                TRACE_L1(_T("Failed to retrieve named key '%s' [0x%08x]"),
                            string(reinterpret_cast<const char*>(name), nameLength).c_str(), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::DeleteKey(const uint32_t id)
    {
        TEECommand command(30, this);
        command.Basic(2, InId(id));

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                TRACE_L2(_T("Deleted key 0x%08x"), id);
            } else {
                TRACE_L1(_T("Failed to delete key 0x%08x [0x%08x]"), id, command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::DHGenerateKeys(const uint8_t generator[], const uint32_t generatorLength,
                                 const uint8_t modulus[], const uint32_t modulusLength,
                                 uint32_t& keyId)
    {
        ASSERT(generator != nullptr);
        ASSERT(generatorLength == sizeof(uint8_t)); // In fact always either a value of 2, 3 or 5
        ASSERT(modulus != nullptr);
        ASSERT(modulusLength != 0);

        TEECommand command(28, this);
        command.Block(0, modulus, modulusLength); // in
        command.Block(1, generator, generatorLength); // in

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                keyId = OutId(command.Basic(1));
                TRACE_L2(_T("Successfully generated Diffie-Hellman key (id: 0x%08x)"), keyId);
            } else {
                TRACE_L1(_T("Failed to generate Diffie-Hellman keypair [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::DHAuthenticatedDeriveKeys(const uint32_t privateKeyId, const uint32_t derivationKeyId,
                                            const uint8_t peerPublicKey[], const uint32_t peerPublicKeyLength,
                                            uint32_t& encryptionKeyId, uint32_t& hmacKeyId, uint32_t& wrappingKeyId)
    {
        ASSERT(peerPublicKey != nullptr);
        ASSERT(peerPublicKeyLength != 0);

        TEECommand command(29, this);
        command.Basic(2, InId(privateKeyId));
        command.Basic(3, InId(derivationKeyId));
        command.Block(0, peerPublicKey, peerPublicKeyLength); // in

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                encryptionKeyId = OutId(command.Basic(1));
                hmacKeyId = OutId(command.Basic(2));
                wrappingKeyId = OutId(command.Basic(3));

                TRACE_L2(_T("Successfully derived Diffie-Hellman authenticated secret keys (encryption: 0x%08x, hmac: 0x%08x, wrapping: 0x%08x)"),
                            encryptionKeyId, hmacKeyId, wrappingKeyId);
            } else {
                TRACE_L1(_T("Failed to derive Diffie-Hellman authenticated secret keys [0x%08x]"), command.Result());
                result = -1;
            }
        }

        return (result);
    }

    uint32_t TEE::AESCBC(const uint32_t keyId, const cryptop operation,
                         const uint8_t iv[], const uint32_t ivLength,
                         const uint8_t input[], const uint32_t inputLength,
                         uint8_t output[], uint32_t& outputLength) const
    {
        ASSERT(iv != nullptr);
        ASSERT(ivLength == 16); // Fixed for all AES modes and key sizes
        ASSERT(input != nullptr);
        ASSERT(inputLength != 0);
        ASSERT(output != nullptr);
        ASSERT(outputLength != 0);
        ASSERT((operation == cryptop::DECRYPT) || (outputLength >= inputLength + 32 - inputLength % 16)); // Accommodate for PKCS5 padding
        ASSERT((operation == cryptop::ENCRYPT) || (outputLength >= inputLength)); // Required, but in fact not really necessary

        TEECommand command(25, this);
        command.Basic(2, InId(keyId));
        command.Basic(3, static_cast<uint32_t>(operation));
        command.Block(0, iv, ivLength); // in
        command.Block(1, input, inputLength); // in
        command.Block(2, outputLength); // out

        uint32_t result = command.Execute();
        if (result == 0) {
            if (command.Result() == 0) {
                outputLength = command.Block(2, 1, output);

                TRACE_L2(_T("Successfully %scrypted data blob of size %i bytes to %i bytes using key 0x%08x"),
                            (operation == cryptop::ENCRYPT? "en" : "de"), inputLength, outputLength, keyId);
            } else {
                TRACE_L1(_T("Failed %scrypt %i bytes of data using key 0x%08x [0x%08x]"),
                            (operation == cryptop::ENCRYPT? "en" : "de"), inputLength, keyId, command.Result());
                result = -1;
            }
        }

        return (result);
    }

} // namespace Netflix

} // namespace Platform

} // namespace Implementation

#else // (defined NEXUS_HAS_SAGE)

#error SAGE not available on this platform

#endif // (defined NEXUS_HAS_SAGE)

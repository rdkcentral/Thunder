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

#include <netflix_security_implementation.h>

#include "TEE.h"


extern "C" {

using TEE = Implementation::Platform::Netflix::TEE;

uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[])
{
    uint16_t length = 0;

    uint32_t rc = TEE::Instance().ESNLength(length);
    if (rc != 0) {
        TRACE_L1(_T("ESNLength() failed [0x%08x]"), rc);
    } else {
        if ((max_length >= length) && (data != nullptr)) {
            uint32_t rc = TEE::Instance().ESN(data, length);
            if (rc != 0) {
                TRACE_L1(_T("ESN() failed [%08x]"), rc);
            }
        }
    }

    return (length);
}

uint32_t netflix_security_encryption_key(void)
{
    return (TEE::namedkeys::KDE);
}

uint32_t netflix_security_hmac_key(void)
{
    return (TEE::namedkeys::KDH);
}

uint32_t netflix_security_wrapping_key(void)
{
    return (TEE::namedkeys::KDW);
}

uint32_t netflix_security_derive_keys(const uint32_t private_dh_key_id, const uint32_t peer_public_dh_key_id, const uint32_t derivation_key_id,
                                      uint32_t* encryption_key_id, uint32_t* hmac_key_id, uint32_t* wrapping_key_id)
{
    ASSERT(private_dh_key_id != 0);
    ASSERT(peer_public_dh_key_id != 0);
    ASSERT(derivation_key_id != 0);
    ASSERT(encryption_key_id != nullptr);
    ASSERT(hmac_key_id != nullptr);
    ASSERT(wrapping_key_id != nullptr);

    uint32_t result = -1;

    uint32_t peerPublicKeySize = TEE::MAX_CLEAR_KEY_SIZE;
    uint8_t* peerPublicKey = reinterpret_cast<uint8_t*>(ALLOCA(peerPublicKeySize));

    if (TEE::Instance().ExportClearKey(peer_public_dh_key_id, TEE::keyformat::RAW, peerPublicKey, peerPublicKeySize) != 0) {
        TRACE_L1(_T("NetflixTA: Failed to extract public key for DH authenticated derive"));
    } else {
        uint32_t rc = TEE::Instance().DHAuthenticatedDeriveKeys(private_dh_key_id, derivation_key_id, peerPublicKey, peerPublicKeySize,
                                                                *encryption_key_id, *hmac_key_id, *wrapping_key_id);
        if (rc != 0) {
            TRACE_L1(_T("NetflixTA: DHAuthenticatedDeriveKeys() failed [0x%08x]"), rc);
        } else if ((encryption_key_id != 0) && (hmac_key_id != 0) && (wrapping_key_id != 0)) {
            result = 0;
        }
    }

    return (result);
}

} // extern "C"


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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Note: These calls adhere to the CRYPTOGRAPHY_VAULT_NETFLIX vault only. */

uint16_t netflix_security_esn(const uint16_t max_length, uint8_t data[]);

uint32_t netflix_security_encryption_key(void);

uint32_t netflix_security_hmac_key(void);

uint32_t netflix_security_wrapping_key(void);

/* Derive encryption keys based on an authenticated Diffie-Hellman procedure :
        1) secret = dh_shared_secret(private_key, peer_public_key)
        2) vector = HMAC-384((SHA-384(derivation_key), secret)           ; HMAC-384
        3) encryptionKey = vector[0..15]                                 ; AES-128
        4) hmacKey = vector[16..47]                                      ; HMAC-256
        5) wrappingKey = HMAC-256(809f82a7addf548d3ea9dd067ff9bb91,
                                  HMAC-256(vector, 027617984f6227539a630b897c017d69))[0..15]  ; AES-128 */
uint32_t netflix_security_derive_keys(const uint32_t private_dh_key_id, const uint32_t peer_public_dh_key_id, const uint32_t derivation_key_id,
                                      uint32_t* encryption_key_id, uint32_t* hmac_key_id, uint32_t* wrapping_key_id);

#ifdef __cplusplus
} // extern "C"
#endif

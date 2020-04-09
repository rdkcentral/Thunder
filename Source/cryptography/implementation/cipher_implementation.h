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
#include "vault_implementation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AES_MODE_ECB,
    AES_MODE_CBC,
    AES_MODE_OFB,
    AES_MODE_CFB1,
    AES_MODE_CFB8,
    AES_MODE_CFB128,
    AES_MODE_CTR,
} aes_mode;

struct CipherImplementation;


struct CipherImplementation* cipher_create_aes(const struct VaultImplementation* vault, const aes_mode mode, const uint32_t key_id);

void cipher_destroy(struct CipherImplementation* cipher);


uint32_t cipher_encrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[]);

uint32_t cipher_decrypt(const struct CipherImplementation* cipher, const uint8_t iv_length, const uint8_t iv[],
                        const uint32_t input_length, const uint8_t input[], const uint32_t max_output_length, uint8_t output[]);

#ifdef __cplusplus
} // extern "C"
#endif

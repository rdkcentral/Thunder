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
    HASH_TYPE_SHA1 = 20,
    HASH_TYPE_SHA224 = 28,
    HASH_TYPE_SHA256 = 32,
    HASH_TYPE_SHA384 = 48,
    HASH_TYPE_SHA512 = 64
} hash_type;

struct HashImplementation;


struct HashImplementation* hash_create(const hash_type type);

struct HashImplementation* hash_create_hmac(const struct VaultImplementation* vault, const hash_type type, const uint32_t secret_id);

void hash_destroy(struct HashImplementation* signing);


uint32_t hash_ingest(struct HashImplementation* signing, const uint32_t length, const uint8_t data[]);

uint8_t hash_calculate(struct HashImplementation* signing, const uint8_t max_length, uint8_t data[]);

#ifdef __cplusplus
} // extern "C"
#endif

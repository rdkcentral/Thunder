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
#include <stdbool.h>
#include "cryptography_vault_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

struct VaultImplementation;


struct VaultImplementation* vault_instance(const enum cryptographyvault id);

uint16_t vault_size(const struct VaultImplementation* vault, const uint32_t id);

uint32_t vault_import(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[]);

uint16_t vault_export(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[]);

uint32_t vault_set(struct VaultImplementation* vault, const uint16_t length, const uint8_t blob[]);

uint16_t vault_get(const struct VaultImplementation* vault, const uint32_t id, const uint16_t max_length, uint8_t blob[]);

bool vault_delete(struct VaultImplementation* vault, const uint32_t id);

#ifdef __cplusplus
} // extern "C"
#endif

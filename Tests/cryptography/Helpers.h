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

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/sha.h>


#ifdef __cplusplus
extern "C" {
#endif

void DumpBuffer(const uint8_t buf[], const uint16_t size);

DH* DHGenerate(const uint32_t generator, const uint8_t modulus[], const uint16_t modulusSize);

uint8_t* DHDerive(DH* dh, const BIGNUM* publicKey);

bool DHAuthenticatedDerive(DH* dh, const uint16_t secretSize, const uint8_t secret[],  const uint16_t derivationKeySize, const uint8_t derivationKey[],
                           const uint16_t saltSize, const uint8_t salt[], const uint16_t dataSize, const uint8_t data[],
                           uint8_t* encKeyOut[], uint8_t* hmacKeyOut[], uint8_t* wrapKeyOut[]);

void Teardown();

#ifdef __cplusplus
}
#endif


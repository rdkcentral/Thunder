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

#include "../../Module.h"

#include <openssl/sha.h>
#include <openssl/hmac.h>


namespace Implementation {

namespace Netflix {

uint16_t DeriveWrappingKey(const uint8_t input[], const uint16_t inputSize, const uint16_t maxSize, uint8_t output[])
{
    // As per https://github.com/Netflix/msl/wiki/Pre-shared-Keys-or-Model-Group-Keys-Entity-Authentication

    static const uint8_t salt[] = { 0x02, 0x76, 0x17, 0x98, 0x4f, 0x62, 0x27, 0x53, 0x9a, 0x63, 0x0b, 0x89, 0x7c, 0x01, 0x7d, 0x69 };
    static const uint8_t data[] = { 0x80, 0x9f, 0x82, 0xa7, 0xad, 0xdf, 0x54, 0x8d, 0x3e, 0xa9, 0xdd, 0x06, 0x7f, 0xf9, 0xbb, 0x91 };

    ASSERT(maxSize >= SHA256_DIGEST_LENGTH);

    //   a) HMAC the HMAC vector with the salt into a HMAC key
    uint8_t hmac[SHA256_DIGEST_LENGTH];
    uint32_t hmacSize = 0;
    HMAC(EVP_sha256(), salt, sizeof(salt), input, inputSize, hmac, &hmacSize);
    ASSERT(hmacSize == SHA256_DIGEST_LENGTH);

    //   b) HMAC the constant data with the HMAC key
    uint32_t outputSize = 0;
    HMAC(EVP_sha256(), hmac, hmacSize, data, sizeof(data), output, &outputSize);
    ASSERT(outputSize == SHA256_DIGEST_LENGTH);

    return (outputSize);
}

} // namespace Netflix

} // namespace Implementation

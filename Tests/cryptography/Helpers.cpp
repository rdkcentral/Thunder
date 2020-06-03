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

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "core/core.h"

#include "Helpers.h"
#include "Test.h"

extern "C" {

void DumpBuffer(const uint8_t buf[], const uint16_t size)
{
    if (size > 0) {
        printf("    ");
        for (uint16_t i = 0; i < size; i++) {
            if ((i != 0) && (i % 32) == 0) {
                printf("\n    ");
            }
            printf("%02x ", buf[i]);
        }
        printf("\n");
    }
}

void DumpBignum(const BIGNUM* bn)
{
    uint8_t* buf = (uint8_t*)alloca(BN_num_bytes(bn));
    BN_bn2bin(bn, buf);
    DumpBuffer(buf, BN_num_bytes(bn));
}

DH* DHGenerate(const uint32_t generator, const uint8_t modulus[], const uint16_t modulusSize)
{
    DH* dh = DH_new();
    if (dh != NULL) {
        dh->p = BN_bin2bn(modulus, modulusSize, NULL);
        dh->g = BN_new();
        BN_set_word(dh->g, generator);

        int codes = 0;
        if ((DH_check(dh, &codes) != 0) && (codes == 0)) {
            if (DH_generate_key(dh) != 0) {
                printf("Prerequisite: generated public key\n");
                DumpBignum(dh->pub_key);
            }
        } else {
            DH_free(dh);
            dh = NULL;
        }
    }

    return (dh);
}

uint8_t* DHDerive(DH* dh, const BIGNUM* peerPublicKey)
{
    uint8_t* secret = NULL;

    int flags = 0;
    if ((DH_check_pub_key(dh, peerPublicKey, &flags) == 0) || (flags != 0)) {
        printf("public key is invalid\n");
    } else {
        uint16_t secretSize = DH_size(dh);
        secret = (uint8_t*)::malloc(secretSize);

        secretSize = DH_compute_key(secret, peerPublicKey, dh);
        assert(secretSize == 128);

        if (secretSize == 0) {
            printf("DH_compute_key() failed\n");
        } else {
            printf("Prerequisite: derived shared secret\n");
            DumpBuffer(secret, secretSize);
        }
    }

    return (secret);
}


bool DHAuthenticatedDerive(DH* dh, const uint16_t secretSize, const uint8_t secret[],  const uint16_t derivationKeySize, const uint8_t derivationKey[],
                           const uint16_t saltSize, const uint8_t salt[], const uint16_t dataSize, const uint8_t data[],
                           uint8_t* encKeyOut[], uint8_t* hmacKeyOut[], uint8_t* wrapKeyOut[])
{
    assert(dh);
    assert(encKeyOut);
    assert(hmacKeyOut);
    assert(wrapKeyOut);
    assert(secret);
    assert(secretSize == 128);
    assert(derivationKey);
    assert(derivationKeySize == 16);
    assert(salt);
    assert(saltSize == 16);
    assert(data);
    assert(dataSize == 16);

    const uint16_t AES_KEY_SIZE = 16;

    printf("Prerequisite: derivation key:\n");
    DumpBuffer(derivationKey, derivationKeySize);

    // important: prepend the secret if neccessessary
    uint8_t* paddedSecret = (uint8_t*) secret;
    uint16_t paddedSecretSize = secretSize;
    if (secret[0] != 0) {
        paddedSecret = (uint8_t*) alloca(secretSize + 1);
        paddedSecret[0] = 0;
        memcpy(&paddedSecret[1], secret, secretSize);
        paddedSecretSize++;
    }

    printf("Prerequisite: padded shared secret:\n");
    DumpBuffer(paddedSecret, paddedSecretSize);

    // SHA the pre-shared derviation key into a HMAC key
    uint8_t hmacKey[SHA384_DIGEST_LENGTH];
    SHA384(derivationKey, derivationKeySize, hmacKey);

    // Calculate HMAC of the secret using the HMAC key
    uint8_t hmac[SHA384_DIGEST_LENGTH];
    uint32_t hmacSize = 0;
    HMAC(EVP_sha384(), hmacKey, sizeof(hmacKey), paddedSecret, paddedSecretSize, hmac, &hmacSize);
    assert(hmacSize == SHA384_DIGEST_LENGTH);

    // Extract the encryption key from the caluclated HMAC
    (*encKeyOut) = (uint8_t*)::malloc(AES_KEY_SIZE);
    memcpy(*encKeyOut, hmac, AES_KEY_SIZE);
    printf("Prerequisite: derived encryption key:\n");
    DumpBuffer(*encKeyOut, AES_KEY_SIZE);

    // Extract the HMAC key from the calculated HMAC
    (*hmacKeyOut) = (uint8_t*)::malloc(SHA256_DIGEST_LENGTH);
    memcpy(*hmacKeyOut, hmac + 16, SHA256_DIGEST_LENGTH);
    printf("Prerequisite: derived HMAC key\n");
    DumpBuffer(*hmacKeyOut, SHA256_DIGEST_LENGTH);

    // Calculate a HMAC' of the previous HMAC using salt as secret
    uint8_t hmac2[SHA256_DIGEST_LENGTH];
    uint32_t hmac2Size = 0;
    HMAC(EVP_sha256(), salt, saltSize, hmac, hmacSize, hmac2, &hmac2Size);

    // Calculate a HMAC'' of the const data by using HMAC' as secret
    uint8_t wrappingKeyBuf[SHA256_DIGEST_LENGTH];
    uint32_t wrappingKeyBufSize = 0;
    HMAC(EVP_sha256(), hmac2, hmac2Size, data, dataSize, wrappingKeyBuf, &wrappingKeyBufSize);

    // Extract the wrapping key from the calculated HMAC''
    (*wrapKeyOut) = (uint8_t*)::malloc(AES_KEY_SIZE);
    memcpy(*wrapKeyOut, wrappingKeyBuf, AES_KEY_SIZE);
    printf("Prerequisite: derived wrapping key:\n");
    DumpBuffer(*wrapKeyOut, AES_KEY_SIZE);

    return (true);
}

void Teardown()
{
    WPEFramework::Core::Singleton::Dispose();
}

} // extern "C"

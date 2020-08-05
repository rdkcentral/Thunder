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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include <openssl/dh.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <implementation/vault_implementation.h>
#include <implementation/hash_implementation.h>
#include <implementation/cipher_implementation.h>
#include <implementation/diffiehellman_implementation.h>

#include "Test.h"
#include "Helpers.h"


struct VaultImplementation* vault = NULL;

/*
  ===================================
    VAULT
  ===================================
*/

static const uint8_t testVector1[] = { 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x7 };
static const uint8_t testVector2[] = { 0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69,};
static const uint8_t testVector3[] = { 0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a };
static const uint8_t testVector4[] = { 0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a,
                                        0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a,
                                        0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a,
                                        0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a,
                                        0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a,
                                        0x74, 0x65, 0x73, 0x74, 0x64, 0x61, 0x74, 0x61, 0x66, 0x6f, 0x72, 0x74, 0x65, 0x73, 0x74, 0x69, 0x6e, 0x67, 0x0a };

static uint32_t TestVaultImport(const uint8_t vector[], const uint16_t vectorSize, bool clear)
{
    uint8_t* output = malloc(vectorSize);

    if (clear) {
        printf("> Testing vault import with blob of size %i\n", vectorSize);
    }

    uint32_t id = vault_import(vault, vectorSize, vector);
    EXPECT_GT(id, 0x80000000U);
    EXPECT_EQ(vault_size(vault, id), vectorSize);
    if (vault_size(vault, id) != 0) {
        EXPECT_EQ(vault_export(vault, id, 0, NULL), 0);
        EXPECT_EQ(vault_export(vault, id, vectorSize, output), vectorSize);
        EXPECT_EQ(memcmp(vector, output, vectorSize), 0);
        if (clear) {
            EXPECT_NE(vault_delete(vault, id), false);
            EXPECT_EQ(vault_size(vault, id), 0);
        }
    } else {
        printf("  FATAL: Failed to import data into vault, vault export test of size %i will be skipped\n", vectorSize);
    }

    free(output);

    return (id);
}

TEST(Vault, Common)
{
    EXPECT_EQ(vault_size(vault, 0), 0);
    EXPECT_EQ(vault_size(vault, 0x80000000U), 0);
    EXPECT_EQ(vault_delete(vault, 0), false);
    EXPECT_EQ(vault_delete(vault, 0x80000000), false);
}

TEST(Vault, ImportExport)
{
    EXPECT_EQ(vault_import(vault, 0, testVector1), 0);
    uint8_t buf;
    EXPECT_EQ(vault_export(vault, 0, 1, &buf), 0);

    /* Single key in vault of different sizes */
    TestVaultImport(testVector1, sizeof(testVector1), 1);
    TestVaultImport(testVector1, sizeof(testVector1), 1);
    TestVaultImport(testVector3, sizeof(testVector3), 1);
    TestVaultImport(testVector4, sizeof(testVector4), 1);

    /* Multiple keys in the vault */
    printf("> Testing vault with multiple blobs\n");
    uint32_t id1 = TestVaultImport(testVector1, sizeof(testVector1), 0);
    uint32_t id2 = TestVaultImport(testVector1, sizeof(testVector2), 0);
    uint32_t id3 = TestVaultImport(testVector1, sizeof(testVector3), 0);
    uint32_t id4 = TestVaultImport(testVector1, sizeof(testVector4), 0);
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id3, id4);
    EXPECT_NE(vault_delete(vault, id1), false);
    EXPECT_NE(vault_delete(vault, id2), false);
    EXPECT_NE(vault_delete(vault, id3), false);
    EXPECT_NE(vault_delete(vault, id4), false);
    EXPECT_EQ(vault_size(vault, id1), 0);
    EXPECT_EQ(vault_size(vault, id2), 0);
    EXPECT_EQ(vault_size(vault, id3), 0);
    EXPECT_EQ(vault_size(vault, id4), 0);
}


static uint32_t TestVaultSet(const uint8_t vector[], const uint16_t vectorSize, bool clear)
{
    uint8_t* sealed = malloc(USHRT_MAX);
    uint8_t* sealed2 = malloc(USHRT_MAX);
    uint32_t sealedId = 0;

    if (clear) {
        printf("> Testing vault set with blob of size %i\n", vectorSize);
    }

    /* Import the clear blob into the vault */
    uint32_t id = vault_import(vault, vectorSize, vector);
    EXPECT_GT(id, 0x80000000U);
    EXPECT_EQ(vault_size(vault, id), vectorSize);
    if (vault_size(vault, id) != 0) {
        EXPECT_EQ(vault_get(vault, id, 0, NULL), 0);
        /* Get the blob sealed */
        uint16_t sealedSize = vault_get(vault, id, USHRT_MAX, sealed);
        EXPECT_NE(sealedSize, 0);
        EXPECT_NE(memcmp(vector, sealed, MIN(sealedSize, vectorSize)), 0);
        EXPECT_NE(vault_delete(vault, id), false);
        EXPECT_EQ(vault_size(vault, id), 0);
        /* Set the sealed blob back to the vault */
        sealedId = vault_set(vault, sealedSize, sealed);
        EXPECT_GT(sealedId, 0x80000000U);
        EXPECT_EQ(vault_size(vault, sealedId), USHRT_MAX);
        /* Get the blob sealed again*/
        uint16_t sealedSize2 = vault_get(vault, sealedId, USHRT_MAX, sealed2);
        EXPECT_EQ(sealedSize, sealedSize2);
        EXPECT_EQ(memcmp(sealed, sealed2, MIN(sealedSize, sealedSize2)), 0);
        if (clear) {
            EXPECT_NE(vault_delete(vault, sealedId), false);
            EXPECT_EQ(vault_size(vault, sealedId), 0);
        }
    } else {
        printf("  FATAL: Failed to import data into vault, vault set test of size %i will be skipped\n", vectorSize);
    }

    free(sealed);
    free(sealed2);

    return (sealedId);
}

TEST(Vault, SetGet)
{
    /* Empty vault */
    EXPECT_EQ(vault_set(vault, 0, testVector1), 0);
    uint8_t buf;
    EXPECT_EQ(vault_get(vault, 0, 1, &buf), 0);

    /* Single key in vault of different sizes */
    TestVaultSet(testVector1, sizeof(testVector1), 1);
    TestVaultSet(testVector1, sizeof(testVector1), 1);
    TestVaultSet(testVector3, sizeof(testVector3), 1);
    TestVaultSet(testVector4, sizeof(testVector4), 1);

    /* Multiple keys in the vault */
    printf("> Testing vault with multiple blobs\n");
    uint32_t id1 = TestVaultSet(testVector1, sizeof(testVector1), 0);
    uint32_t id2 = TestVaultSet(testVector1, sizeof(testVector2), 0);
    uint32_t id3 = TestVaultSet(testVector1, sizeof(testVector3), 0);
    uint32_t id4 = TestVaultSet(testVector1, sizeof(testVector4), 0);
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id3, id4);
    EXPECT_NE(vault_delete(vault, id1), false);
    EXPECT_NE(vault_delete(vault, id2), false);
    EXPECT_NE(vault_delete(vault, id3), false);
    EXPECT_NE(vault_delete(vault, id4), false);
    EXPECT_EQ(vault_size(vault, id1), 0);
    EXPECT_EQ(vault_size(vault, id2), 0);
    EXPECT_EQ(vault_size(vault, id3), 0);
    EXPECT_EQ(vault_size(vault, id4), 0);
}

/*
  ===================================
    HASH
  ===================================
*/

static void TestHash(const char *name, const hash_type type, const uint8_t data[], const uint16_t length, const uint8_t expected[], const uint16_t expectedLength)
{
    printf("> Testing hash %s\n", name);
    struct HashImplementation* hash = hash_create(type);

    if (hash != NULL) {
        uint8_t* output = malloc(expectedLength);
        memset(output, 0, expectedLength);
        EXPECT_EQ(hash_ingest(hash, length, data), length);
        EXPECT_EQ(hash_calculate(hash, 0, NULL), 0);
        EXPECT_EQ(hash_calculate(hash, expectedLength, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        free(output);

        hash_destroy(hash);
    } else {
        printf("  FATAL: Failed to create signing implementation, hash %s test will be skipped\n", name);
    }

    printf("> Testing hash %s (partial)\n", name);
    struct HashImplementation* hashp = hash_create(type);

    if (hashp != NULL) {
        uint8_t* output = malloc(expectedLength);
        memset(output, 0, expectedLength);
        EXPECT_EQ(hash_ingest(hashp, length/2, data), length/2);
        EXPECT_EQ(hash_ingest(hashp, (length - length/2), data + length/2), (length - length/2));
        EXPECT_EQ(hash_calculate(hashp, expectedLength, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        free(output);

        hash_destroy(hashp);
    } else {
        printf("  FATAL: Failed to create signing implementation, hash %s test will be skipped\n", name);
    }
}

static void TestHMAC(const char *name, const hash_type type, const uint32_t secret, const uint8_t data[], const uint16_t length, const uint8_t expected[], const uint16_t expectedLength)
{
    printf("> Testing HMAC %s\n", name);
    struct HashImplementation* hash = hash_create_hmac(vault, type, secret);

    if (hash != NULL) {
        uint8_t* output = malloc(128);
        memset(output, 0, 128);
        hash_ingest(hash, length, data);
        EXPECT_EQ(hash_calculate(hash, 0, NULL), 0);
        memset(output, 0, 128);
        EXPECT_EQ(hash_calculate(hash, expectedLength, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        memset(output, 0, 128);
        EXPECT_EQ(hash_calculate(hash, 128, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        free(output);

        hash_destroy(hash);
    } else {
        printf("  FATAL: Failed to create signing implementation, HMAC %s test will be skipped\n", name);
    }

    printf("> Testing HMAC %s (partial)\n", name);
    struct HashImplementation* hashp = hash_create_hmac(vault, type, secret);

    if (hashp != NULL) {
        uint8_t* output = malloc(128);
        memset(output, 0, 128);
        hash_ingest(hashp, length/2, data);
        hash_ingest(hashp, length - length/2, data + length/2);
        EXPECT_EQ(hash_calculate(hashp, expectedLength, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        memset(output, 0, 128);
        EXPECT_EQ(hash_calculate(hashp, 128, output), expectedLength);
        DumpBuffer(output, expectedLength);
        EXPECT_EQ(memcmp(output, expected, expectedLength), 0);
        free(output);

        hash_destroy(hashp);
    } else {
        printf("  FATAL: Failed to create signing implementation, HMAC %s test will be skipped\n", name);
    }

}

TEST(Signing, Hash)
{
    const uint8_t data[] = "Etaoin Shrldu";

    // const uint8_t hash_md5[] = { 0xac, 0x1c, 0x0d, 0x68, 0xe8, 0x59, 0xda, 0x1f, 0x6e, 0x13, 0x56, 0x9e, 0x00, 0xe0, 0xe0, 0x9b };
    // TestHash("MD5", HASH_TYPE_MD5, data, (sizeof(data) - 1), hash_md5, sizeof(hash_md5));

    const uint8_t hash_sha1[] =  { 0x12, 0xCD, 0xF3, 0xFF, 0x6B, 0x67, 0x1D, 0x77, 0x07, 0x59, 0x9F, 0x3C,
                                   0x63, 0x59, 0xD9, 0x94, 0x95, 0x63, 0x94, 0x70 };
    TestHash("SHA1", HASH_TYPE_SHA1, data, (sizeof(data) - 1), hash_sha1, sizeof(hash_sha1));

    const uint8_t hash_sha224[] = { 0x59, 0x0F, 0x4A, 0xF2, 0x01, 0x95, 0xBF, 0x65, 0xB6, 0xD3, 0xD3, 0x48,
	                                0x57, 0x06, 0x2C, 0xD3, 0x51, 0x02, 0x3E, 0x4B, 0x62, 0x97, 0x40, 0xBF,
	                                0xC7, 0xAD, 0xA3, 0x00 };
    TestHash("SHA224", HASH_TYPE_SHA224, data, (sizeof(data) - 1), hash_sha224, sizeof(hash_sha224));

    const uint8_t hash_sha256[] =  { 0x80, 0x72, 0xA8, 0x3C, 0x2C, 0xFB, 0xF3, 0x67, 0xA1, 0x64, 0x1C, 0x22,
	                                 0x03, 0xCD, 0x78, 0x1D, 0x2E, 0x85, 0x13, 0x11, 0x72, 0x7D, 0xCE, 0x8E,
	                                 0xD7, 0x25, 0x51, 0x0F, 0xE1, 0x3B, 0x78, 0x35 };
    TestHash("SHA256", HASH_TYPE_SHA256, data, (sizeof(data) - 1), hash_sha256, sizeof(hash_sha256));

    const uint8_t hash_sha384[] = { 0x11, 0xD9, 0x05, 0xFD, 0x6A, 0x63, 0xCF, 0xE6, 0xBD, 0x71, 0xD4, 0x9C,
	                                0x17, 0xED, 0x5C, 0xB2, 0x07, 0xE5, 0x3A, 0xC3, 0x10, 0x4F, 0x65, 0x47,
	                                0xD9, 0xD0, 0xE1, 0xD2, 0xDD, 0x49, 0xEC, 0x10, 0x08, 0x1B, 0xF3, 0xD8,
	                                0xFB, 0xB6, 0xE0, 0xDD, 0xDB, 0x73, 0xE7, 0x1E, 0xC6, 0xE0, 0xFF, 0xCE };
    TestHash("SHA384", HASH_TYPE_SHA384, data, (sizeof(data) - 1), hash_sha384, sizeof(hash_sha384));

    const uint8_t hash_sha512[] = { 0x36, 0x00, 0x9C, 0x7A, 0xA3, 0x25, 0x93, 0xBD, 0xF7, 0x02, 0x5B, 0xD7,
                                    0x7A, 0x1F, 0x62, 0xB8, 0xCD, 0xB0, 0xF2, 0x16, 0xD1, 0xE2, 0x35, 0x49,
                                    0xA5, 0x3D, 0xF8, 0x05, 0x9D, 0x0F, 0x33, 0x3E, 0x3D, 0x41, 0xDD, 0x20,
                                    0x7F, 0xB8, 0xA4, 0x39, 0xC2, 0x66, 0xD7, 0xB8, 0x8F, 0x29, 0x81, 0xD1,
                                    0xC3, 0xB5, 0xA8, 0x4D, 0x25, 0xE1, 0xF8, 0x40, 0x78, 0x0B, 0xF6, 0xFF,
                                    0x87, 0xE3, 0x53, 0x1A };
    TestHash("SHA512", HASH_TYPE_SHA512, data, (sizeof(data) - 1), hash_sha512, sizeof(hash_sha512));
}

TEST(Signing, HMAC)
{
    const uint8_t data[] = "Etaoin Shrldu";
    const uint8_t password[] = "Thunder";

    uint32_t secret = vault_import(vault, (sizeof(password) - 1), password);
    if (secret != 0) {
        // const uint8_t hash_md5[] = { 0x94, 0x68, 0x9D, 0xF2, 0xA8, 0x83, 0xE3, 0xE3, 0x8D, 0xED, 0x41, 0x83,
        //                              0x3A, 0x27, 0xD2, 0x93 };
        // TestHMAC("MD5", HASH_TYPE_MD5, secret, data, (sizeof(data) - 1), hash_md5, sizeof(hash_md5));

        const uint8_t hash_sha1[] = { 0x2F, 0xD0, 0x08, 0x86, 0x53, 0xE2, 0x1B, 0xFE, 0x27, 0x88, 0xC3, 0xCB,
	                                  0x11, 0x90, 0x4E, 0x5D, 0x87, 0xFF, 0x61, 0x25 };
        TestHMAC("SHA1", HASH_TYPE_SHA1, secret, data, (sizeof(data) - 1), hash_sha1, sizeof(hash_sha1));

        const uint8_t hash_sha224[] = { 0x10, 0x10, 0xC2, 0x4D, 0xAF, 0x1E, 0x1E, 0xE9, 0x5E, 0xD9, 0x90, 0xD9,
	                                    0xD5, 0x00, 0x78, 0x41, 0x95, 0x9E, 0x60, 0xD2, 0xE9, 0xCF, 0x72, 0xDB,
	                                    0xC5, 0x33, 0x5A, 0x97 };
        TestHMAC("SHA224", HASH_TYPE_SHA224, secret, data, (sizeof(data) - 1), hash_sha224, sizeof(hash_sha224));

        const uint8_t hash_sha256[] = { 0x2D, 0xF5, 0x9C, 0xBE, 0x61, 0x59, 0x7F, 0x14, 0xEC, 0xD2, 0x85, 0x6F,
	                                    0xAB, 0xF1, 0x12, 0xFC, 0xF4, 0x68, 0x6D, 0xFE, 0x93, 0x5F, 0xDB, 0xB7,
	                                    0x34, 0x8C, 0x6C, 0x6B, 0xF1, 0x64, 0xE9, 0x27 };
        TestHMAC("SHA256", HASH_TYPE_SHA256, secret, data, (sizeof(data) - 1), hash_sha256, sizeof(hash_sha256));

        const uint8_t hash_sha384[] = { 0x01, 0xD3, 0xFA, 0xC7, 0xCF, 0x43, 0xA1, 0x91, 0xBA, 0x71, 0x60, 0xF4,
	                                    0x78, 0xB7, 0x8D, 0x4C, 0x84, 0x76, 0xC0, 0xE4, 0xCE, 0x38, 0x67, 0xCE,
	                                    0xFB, 0x12, 0xED, 0x62, 0xC2, 0xA4, 0x78, 0x50, 0x70, 0xD4, 0x3F, 0xC5,
	                                    0x04, 0x77, 0xB9, 0xAF, 0x8D, 0xE0, 0x80, 0x0C, 0x5E, 0x53, 0x9B, 0xDA };
        TestHMAC("SHA384", HASH_TYPE_SHA384, secret, data, (sizeof(data) - 1), hash_sha384, sizeof(hash_sha384));

        const uint8_t hash_sha512[] = { 0xAC, 0x1B, 0xC5, 0x95, 0xB2, 0x14, 0x33, 0xA4, 0xD7, 0xF7, 0xC6, 0x88,
	                                    0x08, 0xF1, 0xEA, 0x90, 0x8A, 0xEB, 0x2C, 0xA9, 0xAE, 0x2F, 0x13, 0xAE,
	                                    0x66, 0xFE, 0x0C, 0x4E, 0x93, 0x9B, 0xEF, 0xDD, 0x79, 0x59, 0xE0, 0xE2,
                                        0x31, 0xA1, 0xA2, 0x5D, 0x0A, 0xD8, 0x96, 0xA4, 0x04, 0xF9, 0x7A, 0x29,
                                        0xEC, 0x47, 0x89, 0x62, 0x89, 0xBF, 0x25, 0x0D, 0x1B, 0x11, 0x28, 0xA6,
                                        0x48, 0xD5, 0x77, 0xF2 };
        TestHMAC("SHA512", HASH_TYPE_SHA512, secret, data, (sizeof(data) - 1), hash_sha512, sizeof(hash_sha512));
    } else {
        printf("FATAL: Failed to store secret into vault, HMAC tests are skipped\n");
    }
}

/*
  ===================================
    CIPHER
  ===================================
*/

static const uint32_t testGenerator = 5;

static const uint8_t testPrime1024[] = {
    0x96, 0x94, 0xe9, 0xd8, 0xd9, 0x3a, 0x5a, 0xc7, 0x4c, 0x50, 0x9b, 0x4b, 0xbc, 0xe8, 0x5e, 0x92,
    0x13, 0x2c, 0xd1, 0x9c, 0xce, 0x47, 0x7d, 0x1a, 0x7e, 0x47, 0xd5, 0x27, 0xd9, 0xec, 0x29, 0x15,
    0x15, 0xf0, 0xb8, 0xb3, 0xe1, 0xea, 0xed, 0x50, 0x06, 0xe1, 0xb1, 0xb9, 0x1e, 0xa2, 0x5b, 0x91,
    0xa0, 0x1b, 0x10, 0xe2, 0xe8, 0x34, 0xb8, 0xd6, 0x60, 0xb2, 0xe3, 0x21, 0xad, 0x64, 0x4c, 0xe1,
    0xa8, 0x3b, 0x32, 0x8d, 0x90, 0x14, 0xee, 0x7e, 0x16, 0xf1, 0xe4, 0x4f, 0xfe, 0x89, 0x57, 0x9a,
    0xc3, 0xee, 0x47, 0xd6, 0x68, 0xb6, 0xb7, 0x66, 0x87, 0xc2, 0xfe, 0x90, 0xa3, 0x5b, 0x5e, 0x60,
    0x28, 0xfd, 0x04, 0xef, 0xea, 0x88, 0x23, 0x73, 0xec, 0xf6, 0x0b, 0xa2, 0xf6, 0x37, 0xe4, 0xcd,
    0xaa, 0x1b, 0x60, 0x89, 0xd6, 0xc0, 0xb5, 0x61, 0xa8, 0xe5, 0x20, 0xe7, 0x96, 0xde, 0x27, 0xdf
};

static bool GenerateDHKeyPair(const uint32_t generator, const uint8_t modulus[], const uint16_t modulusSize, uint32_t *privKey, uint32_t *pubKey)
{
    bool result = false;

    uint32_t dhPrivateKey = 0;
    uint32_t dhPublicKey = 0;

    uint32_t DHStatus = diffiehellman_generate(vault, generator, modulusSize, modulus, &dhPrivateKey, &dhPublicKey);
    EXPECT_EQ(DHStatus, 0);
    EXPECT_GT(dhPrivateKey, 0x80000000U);
    EXPECT_GT(dhPublicKey, 0x80000000U);
    EXPECT_NE(dhPrivateKey, dhPublicKey);

    (*privKey) = dhPrivateKey;
    (*pubKey) = dhPublicKey;

    EXPECT_NE(vault_size(vault, dhPrivateKey), 0);
    EXPECT_NE(vault_size(vault, dhPublicKey), 0);

    result = true;

    return (result);
}

TEST(DH, Generate)
{
    uint32_t privateKeyId;
    uint32_t publicKeyId;

    GenerateDHKeyPair(testGenerator, testPrime1024, sizeof(testPrime1024), &privateKeyId, &publicKeyId);

    EXPECT_EQ(vault_size(vault, privateKeyId), USHRT_MAX);
    EXPECT_NE(privateKeyId, publicKeyId);
    uint16_t publicKeySize = vault_size(vault, publicKeyId);
    EXPECT_NE(publicKeySize, 0);
    EXPECT_NE(publicKeySize, USHRT_MAX);
    if (publicKeySize != 0) {
        uint8_t* publicKeyBuf = malloc(publicKeySize);
        EXPECT_EQ(vault_export(vault, publicKeyId, publicKeySize, publicKeyBuf), publicKeySize);
        DumpBuffer(publicKeyBuf, publicKeySize);
        free(publicKeyBuf);
    }

    EXPECT_NE(vault_delete(vault, privateKeyId), false);
    EXPECT_NE(vault_delete(vault, publicKeyId), false);

    EXPECT_EQ(vault_size(vault, privateKeyId), 0);
    EXPECT_EQ(vault_size(vault, publicKeyId), 0);

}

TEST(DH, DeriveStandard)
{
    uint32_t privateKeyId = 0;;
    uint32_t publicKeyId = 0;
    uint32_t peerPublicKeyId = 0;
    uint32_t secretId = 0;

    DH* dh = DHGenerate(testGenerator, testPrime1024, sizeof(testPrime1024));
    assert(dh != NULL);
    if (dh != NULL) {
        uint32_t peerPublicKeySize = BN_num_bytes(dh->pub_key);
        uint8_t* peerPublicKeyBuf = (uint8_t*) alloca(peerPublicKeySize);
        BN_bn2bin(dh->pub_key, peerPublicKeyBuf);

        peerPublicKeyId = vault_import(vault, peerPublicKeySize, peerPublicKeyBuf);
        EXPECT_GT(peerPublicKeyId, 0x80000000U);
        EXPECT_EQ(vault_size(vault, peerPublicKeyId), peerPublicKeySize);

        EXPECT_EQ(diffiehellman_generate(vault, testGenerator, sizeof(testPrime1024), testPrime1024, &privateKeyId, &publicKeyId), 0);
        EXPECT_GT(privateKeyId, 0x80000000U);
        EXPECT_GT(publicKeyId, 0x80000000U);
        EXPECT_NE(privateKeyId, publicKeyId);

        if (privateKeyId != 0) {
            uint16_t publicKeySize = 0;
            EXPECT_NE(publicKeySize = vault_size(vault, publicKeyId), 0);
            EXPECT_NE(publicKeySize = vault_size(vault, publicKeyId), USHRT_MAX);
            uint8_t* publicKeyBuf = (uint8_t*) alloca(publicKeySize);
            EXPECT_EQ(vault_export(vault, publicKeyId, publicKeySize, publicKeyBuf), publicKeySize);
            printf("TEE public key:\n");
            DumpBuffer(publicKeyBuf, publicKeySize);

            EXPECT_EQ(diffiehellman_derive(vault, privateKeyId, peerPublicKeyId, &secretId), 0);
            EXPECT_GT(secretId, 0x80000000L);
            EXPECT_EQ(vault_size(vault, secretId), USHRT_MAX);

            BIGNUM *publicKeyBn = BN_bin2bn(publicKeyBuf, publicKeySize, NULL);
            assert(publicKeyBn != NULL);
            uint8_t* peerSecret = DHDerive(dh, publicKeyBn);

            // Verify secret by calculating a HMAC of a common buffer
            uint8_t teeHmac[SHA256_DIGEST_LENGTH] = { 0 };
            uint8_t hostHmac[SHA256_DIGEST_LENGTH] = { 0 };
            const char testStr[] = "Thunder";

            HMAC(EVP_sha256(), peerSecret, sizeof(testPrime1024), (uint8_t*)testStr, sizeof(testStr), hostHmac, NULL);
            printf("Host HMAC:\n");
            DumpBuffer(hostHmac, sizeof(hostHmac));

            struct HashImplementation* himp = hash_create_hmac(vault, HASH_TYPE_SHA256, secretId);
            EXPECT_NE(himp, NULL);
            if (himp) {
                EXPECT_EQ(hash_ingest(himp, sizeof(testStr), (uint8_t*)testStr), sizeof(testStr));
                EXPECT_EQ(hash_calculate(himp, sizeof(teeHmac), teeHmac), sizeof(teeHmac));
                hash_destroy(himp);
                himp = NULL;

                printf("TEE HMAC:\n");
                DumpBuffer(teeHmac, sizeof(teeHmac));
            }

            EXPECT_EQ(memcmp(teeHmac, hostHmac, SHA256_DIGEST_LENGTH), 0);

            BN_free(publicKeyBn);
        }

        DH_free(dh);
    }
}

static void TestCryptAES(const char *name, const aes_mode mode, const uint32_t key,
                         const uint8_t iv[], const uint16_t ivLength,
                         const uint8_t data[], const uint16_t length,
                         const uint8_t expected[], const uint16_t expectedLength,
                         const uint16_t bufferSize)
{
    printf("> Testing %s encryption\n", name);
    struct CipherImplementation* cipher = cipher_create_aes(vault, mode, key);

    if (cipher != NULL) {
        uint8_t* output = malloc(bufferSize);
        memset(output, 0, bufferSize);

        uint8_t* input = malloc(bufferSize);
        memset(input, 0, bufferSize);

        DumpBuffer(data, length);

        EXPECT_EQ(cipher_encrypt(cipher, ivLength, iv, length, data, bufferSize, output), expectedLength);
        DumpBuffer(output, expectedLength);

        if (expected != NULL) {
            EXPECT_EQ(memcmp(output, expected, length), 0);
        }

        EXPECT_EQ(cipher_decrypt(cipher, ivLength, iv, expectedLength, output, bufferSize, input), length);
        DumpBuffer(input, length);
        EXPECT_EQ(memcmp(input, data, length), 0);

        EXPECT_NE(memcmp(input, output, MIN(expectedLength, length)), 0);

        free(input);
        free(output);
    } else {
        printf("  FATAL: Failed to create cryptor implementations, encryption test %s test will be skipped\n", name);
    }

    if (cipher) {
        cipher_destroy(cipher);
    }
}

TEST(Cipher, AES_Padded)
{
    const uint8_t data[] = "Look behind you, a Three-Headed Monkey!";
    const uint16_t dataSize = sizeof(data) - 1;
    const uint16_t expectedSize = dataSize + (16 - (dataSize % 16));
    const uint16_t bufferSize = expectedSize + 16;

    const uint8_t iv[]   =  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };


    // 128-bit AES

    const uint8_t key128[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    const uint8_t expected_AES_CBC_128[] = {
	    0xB0, 0xF2, 0x9F, 0xB5, 0x55, 0xB2, 0x48, 0x08, 0x3A, 0x0D, 0xD9, 0xDB,
	    0x6E, 0xD3, 0x37, 0x13, 0x5F, 0x91, 0x92, 0xBB, 0x33, 0xCE, 0x22, 0x21,
	    0xDB, 0xEC, 0x8C, 0x55, 0xD7, 0xB9, 0xD9, 0x96, 0xEA, 0x5E, 0xE0, 0x84,
	    0x9F, 0xBA, 0x44, 0x13, 0xCE, 0x2C, 0x63, 0x8A, 0x0A, 0x1D, 0x60, 0xBC
    };

    const size_t paddedLength = sizeof(expected_AES_CBC_128);

    uint32_t key128Id = vault_import(vault, sizeof(key128), key128);
    EXPECT_NE(key128Id, 0);
    if (key128Id != 0) {
        TestCryptAES("128-bit AES/CBC", AES_MODE_CBC, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, expected_AES_CBC_128, expectedSize, bufferSize);
        TestCryptAES("128-bit AES/ECB", AES_MODE_ECB, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, expectedSize, bufferSize);
        TestCryptAES("128-bit AES/OFB", AES_MODE_OFB, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB1", AES_MODE_CFB1, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB8", AES_MODE_CFB8, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB128", AES_MODE_CFB128, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CTR", AES_MODE_CTR, key128Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        EXPECT_NE(vault_delete(vault, key128Id), false);
        EXPECT_EQ(vault_size(vault, key128Id), 0);
    } else {
        printf("  FATAL: Failed to store key to vault, 128-bit AES tests will be skipped\n");
    }


    // 192-bit AES

    const uint8_t key192[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11,
                               0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

    const uint8_t expected_AES_CBC_192[] = {
	    0x6D, 0x0E, 0xBA, 0x8C, 0xF9, 0x51, 0x80, 0xFB, 0x58, 0xE2, 0x3C, 0x91,
	    0xC3, 0x5C, 0x18, 0xBB, 0xED, 0x14, 0xB0, 0xF1, 0xA9, 0x97, 0x4D, 0xC7,
	    0x31, 0xF4, 0xF5, 0x45, 0x5B, 0x19, 0x89, 0x7F, 0xDC, 0xBA, 0x66, 0x4B,
	    0xFD, 0x0F, 0xD5, 0xB5, 0x84, 0x9A, 0x13, 0x58, 0x3B, 0x6D, 0xD7, 0xF1
    };

    uint32_t key192Id = vault_import(vault, sizeof(key192), key192);
    EXPECT_NE(key192Id, 0);
    if (key192Id != 0) {
        TestCryptAES("192-bit AES/CBC", AES_MODE_CBC, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, expected_AES_CBC_192, paddedLength, bufferSize);
        TestCryptAES("192-bit AES/ECB", AES_MODE_ECB, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, paddedLength, bufferSize);
        TestCryptAES("192-bit AES/OFB", AES_MODE_OFB, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("192-bit AES/CFB1", AES_MODE_CFB1, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("192-bit AES/CFB8", AES_MODE_CFB8, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("192-bit AES/CFB128", AES_MODE_CFB128, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("192-bit AES/CTR", AES_MODE_CTR, key192Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        EXPECT_NE(vault_delete(vault, key192Id), false);
        EXPECT_EQ(vault_size(vault, key192Id), 0);
    } else {
        printf("  FATAL: Failed to store key to vault, 192-bit AES tests will be skipped\n");
    }


    // 256-bit AES

    const uint8_t key256[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11,
                               0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    const uint8_t expected_AES_CBC_256[] = {
	    0x18, 0x7F, 0xE4, 0x55, 0x2F, 0x96, 0xB0, 0x81, 0x58, 0x03, 0xA4, 0xB6,
	    0xC7, 0x79, 0x90, 0x23, 0xA0, 0xFD, 0x37, 0xF3, 0x1F, 0x92, 0x17, 0xFC,
	    0x90, 0x4A, 0xB3, 0x22, 0x43, 0x64, 0x18, 0x18, 0x0D, 0x0F, 0x42, 0x9C,
	    0x97, 0x8F, 0x98, 0xB0, 0x09, 0x40, 0x4D, 0x2E, 0x3E, 0x7F, 0x4A, 0xBD
    };

    uint32_t key256Id = vault_import(vault, sizeof(key256), key256);
    EXPECT_NE(key256Id, 0);
    if (key256Id != 0) {
        TestCryptAES("256-bit AES/CBC", AES_MODE_CBC, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, expected_AES_CBC_256, paddedLength, bufferSize);
        TestCryptAES("256-bit AES/ECB", AES_MODE_ECB, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, paddedLength, bufferSize);
        TestCryptAES("256-bit AES/OFB", AES_MODE_OFB, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("256-bit AES/CFB1", AES_MODE_CFB1, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("256-bit AES/CFB8", AES_MODE_CFB8, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("256-bit AES/CFB128", AES_MODE_CFB128, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        TestCryptAES("256-bit AES/CTR", AES_MODE_CTR, key256Id, iv, sizeof(iv), data, sizeof(data) - 1, NULL, sizeof(data) - 1, bufferSize);
        EXPECT_NE(vault_delete(vault, key256Id), false);
        EXPECT_EQ(vault_size(vault, key256Id), 0);
    } else {
        printf("  FATAL: Failed to store key to vault, 256-bit AES tests will be skipped\n");
    }
}

TEST(Cipher, AES_Unpadded)
{
    const uint8_t data[] = "0123456789abcdef";
    const uint16_t dataSize = sizeof(data) - 1;
    const uint16_t expectedSize = dataSize + (16 - (dataSize % 16));
    const uint16_t bufferSize = expectedSize + 16;

    const uint8_t iv[]   =  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

    const uint8_t key128[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    const uint8_t expected_AES_CBC_128[] = {
	    0x5C, 0xCE, 0x87, 0xB7, 0xD3, 0xCB, 0xA8, 0xA1, 0xD2, 0xCD, 0xAE, 0xD9,
	    0xA1, 0x4A, 0x34, 0x53, 0x99, 0x1A, 0x27, 0xD5, 0xBE, 0x52, 0x78, 0xDA,
	    0x09, 0x91, 0xC9, 0x78, 0x49, 0x3C, 0xCE, 0x42
    };

    uint32_t key128Id = vault_import(vault, sizeof(key128), key128);
    EXPECT_NE(key128Id, 0);
    if (key128Id != 0) {
        /* padding only needed on CBC an ECB */
        TestCryptAES("128-bit AES/CBC", AES_MODE_CBC, key128Id, iv, sizeof(iv), data, dataSize, expected_AES_CBC_128, expectedSize, bufferSize);
        TestCryptAES("128-bit AES/ECB", AES_MODE_ECB, key128Id, iv, sizeof(iv), data, dataSize, NULL, expectedSize, bufferSize);

        TestCryptAES("128-bit AES/OFB", AES_MODE_OFB, key128Id, iv, sizeof(iv), data, dataSize, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB1", AES_MODE_CFB1, key128Id, iv, sizeof(iv), data, dataSize, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB8", AES_MODE_CFB8, key128Id, iv, sizeof(iv), data, dataSize, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CFB128", AES_MODE_CFB128, key128Id, iv, sizeof(iv), data, dataSize, NULL, dataSize, bufferSize);
        TestCryptAES("128-bit AES/CTR", AES_MODE_CTR, key128Id, iv, sizeof(iv), data, dataSize, NULL, dataSize, bufferSize);
        EXPECT_NE(vault_delete(vault, key128Id), false);
        EXPECT_EQ(vault_size(vault, key128Id), 0);
    } else {
        printf("  FATAL: Failed to store key to vault, 128-bit AES tests will be skipped\n");
    }
}

/*
  ===================================
*/

int main(void)
{
    CALL(Signing, Hash);

    vault = vault_instance(CRYPTOGRAPHY_VAULT_NETFLIX);
    if (vault != NULL) {
        CALL(Vault, Common);
        CALL(Vault, ImportExport);
        CALL(Vault, SetGet); // Will not work on Sage

        CALL(Signing, Hash);
        CALL(Signing, HMAC);

        CALL(DH, Generate);
        CALL(DH, DeriveStandard); // Will not work on Sage

        CALL(Cipher, AES_Padded);
        CALL(Cipher, AES_Unpadded);
    }

    printf("TOTAL: %i tests; %i PASSED, %i FAILED\n", TotalTests, TotalTestsPassed, (TotalTests - TotalTestsPassed));

    Teardown();

    return (TotalTests - TotalTestsPassed);
}


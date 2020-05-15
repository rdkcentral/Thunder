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

#include <climits>

#include <cryptography.h>
#include <core/core.h>

#include "Test.h"
#include "Helpers.h"


static WPEFramework::Cryptography::ICryptography* cg;
static WPEFramework::Cryptography::IVault *vault = nullptr;

static const uint8_t testVector[] = { 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x74, 0x7 };


TEST(Vault, ImportExport)
{
    EXPECT_EQ(vault->Size(0), 0);
    EXPECT_EQ(vault->Size(0x80000000U), 0);
    EXPECT_NE(vault->Delete(0), true);
    EXPECT_NE(vault->Delete(0x80000000U), true);

    uint32_t id = vault->Import(sizeof(testVector), testVector);
    EXPECT_GT(id, 0x80000000U);
    EXPECT_EQ(vault->Size(id), sizeof(testVector));
    if (id != 0) {
        uint8_t* output = new uint8_t[sizeof(testVector)];
        EXPECT_EQ(vault->Export(id, sizeof(testVector), output), sizeof(testVector));
        EXPECT_EQ(::memcmp(testVector, output, sizeof(testVector)), 0);
        EXPECT_EQ(vault->Delete(id), true);
        EXPECT_EQ(vault->Size(id), 0);
        delete[] output;
    }
}

TEST(Vault, SetGet)
{
    const uint32_t MAX_SIZE = 16384;
    uint32_t id = vault->Import(sizeof(testVector), testVector);
    EXPECT_GT(id, 0x80000000U);
    EXPECT_EQ(vault->Size(id), sizeof(testVector));
    if (id != 0) {
        uint8_t* sealed = new uint8_t[MAX_SIZE];
        uint8_t* sealed2 = new uint8_t[MAX_SIZE];
        uint16_t sealedSize = vault->Get(id, MAX_SIZE, sealed);
        DumpBuffer(sealed, sealedSize);
        EXPECT_NE(sealedSize, 0);
        EXPECT_NE(::memcmp(testVector, sealed, sizeof(testVector)), 0);
        EXPECT_EQ(vault->Delete(id), true);
        EXPECT_EQ(vault->Size(id), 0);
        uint32_t sealedId = vault->Set(sealedSize, sealed);
        EXPECT_GT(sealedId, 0x80000000U);
        EXPECT_EQ(vault->Size(sealedId), USHRT_MAX);
        uint16_t sealedSize2 = vault->Get(sealedId, MAX_SIZE, sealed2);
        EXPECT_EQ(sealedSize, sealedSize2);
        EXPECT_EQ(memcmp(sealed, sealed2, MIN(sealedSize, sealedSize2)), 0);
        EXPECT_EQ(vault->Delete(sealedId), true);
        EXPECT_EQ(vault->Size(sealedId), 0);
        delete[] sealed;
        delete[] sealed2;
    }
}


TEST(Hash, Hash)
{
    const uint8_t data[] = "Etaoin Shrldu";

    static const uint8_t hash_sha256[] =  { 0x80, 0x72, 0xA8, 0x3C, 0x2C, 0xFB, 0xF3, 0x67, 0xA1, 0x64, 0x1C, 0x22,
	                                        0x03, 0xCD, 0x78, 0x1D, 0x2E, 0x85, 0x13, 0x11, 0x72, 0x7D, 0xCE, 0x8E,
	                                        0xD7, 0x25, 0x51, 0x0F, 0xE1, 0x3B, 0x78, 0x35 };

    WPEFramework::Cryptography::IHash* hashImpl = cg->Hash(WPEFramework::Cryptography::hashtype::SHA256);
    EXPECT_NE(hashImpl, nullptr);
    if (hashImpl != nullptr) {
        uint8_t* output = new uint8_t[sizeof(hash_sha256)];
        ::memset(output, 0, sizeof(hash_sha256));

        EXPECT_EQ(hashImpl->Ingest(sizeof(data) - 1, data), sizeof(data) - 1);
        EXPECT_EQ(hashImpl->Calculate(0, output), 0);
        EXPECT_EQ(hashImpl->Calculate(sizeof(hash_sha256), output), sizeof(hash_sha256));
        DumpBuffer(output, sizeof(hash_sha256));
        EXPECT_EQ(::memcmp(output, hash_sha256, sizeof(hash_sha256)), 0);

        hashImpl->Release();

        delete[] output;
    }


    const uint8_t ldata[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aliquam efficitur pretium magna id imperdiet. Aliquam ac venenatis sapien. "
                            "Quisque consequat elit non orci consectetur mattis in dictum odio. Cras placerat turpis id nibh sagittis fringilla. "
                            "Nam eget elit bibendum ante vehicula euismod eu pulvinar metus. Fusce egestas lacus ut facilisis pellentesque. "
                            "Nulla fermentum, mauris at vestibulum cursus, eros dui suscipit erat, sit amet blandit ex nisi ut neque. Suspendisse tempus sollicitudin odio. "
                            "Sed interdum dapibus libero, eget venenatis massa sodales sed. Suspendisse eget hendrerit risus, fringilla tempus ligula. Nam augue lacus, "
                            "iaculis eu varius vel, interdum eu ligula. Etiam gravida nisi eget odio auctor luctus.";

    static const uint8_t lhash_sha256[] =  { 0x69, 0xE7, 0x21, 0x53, 0xEC, 0x05, 0xBD, 0x2D, 0x5D, 0x24, 0xC1, 0x64,
	                                         0xED, 0xD3, 0x47, 0x09, 0x7B, 0xC2, 0x4C, 0xB1, 0x78, 0x32, 0x41, 0xF8,
	                                         0x8A, 0x9D, 0x3D, 0x91, 0xB6, 0xD6, 0x4D, 0x60};

    WPEFramework::Cryptography::IHash* lhashImpl = cg->Hash(WPEFramework::Cryptography::hashtype::SHA256);
    EXPECT_NE(lhashImpl, nullptr);
    if (lhashImpl != nullptr) {
        uint8_t* output = new uint8_t[sizeof(lhash_sha256)];
        ::memset(output, 0, sizeof(lhash_sha256));

        EXPECT_EQ(hashImpl->Ingest(sizeof(ldata) - 1, ldata), sizeof(ldata) - 1);
        EXPECT_EQ(hashImpl->Calculate(sizeof(lhash_sha256), output), sizeof(lhash_sha256));
        DumpBuffer(output, sizeof(lhash_sha256));
        EXPECT_EQ(::memcmp(output, lhash_sha256, sizeof(lhash_sha256)), 0);

        hashImpl->Release();

        delete[] output;
    }
}

TEST(Hash, HMAC)
{
    static const uint8_t data[] = "Etaoin Shrldu";
    static const uint8_t password[] = "Thunder";

    static const uint8_t hash_sha256[] =  { 0x2D, 0xF5, 0x9C, 0xBE, 0x61, 0x59, 0x7F, 0x14, 0xEC, 0xD2, 0x85, 0x6F,
	                                        0xAB, 0xF1, 0x12, 0xFC, 0xF4, 0x68, 0x6D, 0xFE, 0x93, 0x5F, 0xDB, 0xB7,
	                                        0x34, 0x8C, 0x6C, 0x6B, 0xF1, 0x64, 0xE9, 0x27 };

    uint32_t keyId = vault->Import(sizeof(password) - 1, password);
    EXPECT_NE(keyId, 0);
    if (keyId != 0) {
        WPEFramework::Cryptography::IHash* hashImpl = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, keyId);
        EXPECT_NE(hashImpl, nullptr);
        if (hashImpl != nullptr) {
            uint8_t* output = new uint8_t[128];
            ::memset(output, 0, 128);

            EXPECT_EQ(hashImpl->Ingest(sizeof(data) - 1, data), sizeof(data) - 1);
            EXPECT_EQ(hashImpl->Calculate(0, output), 0);
            EXPECT_EQ(hashImpl->Calculate(128, output), sizeof(hash_sha256));
            DumpBuffer(output, sizeof(hash_sha256));
            EXPECT_EQ(::memcmp(output, hash_sha256, sizeof(hash_sha256)), 0);

            hashImpl->Release();

            delete[] output;

            EXPECT_NE(vault->Delete(keyId), false);
            EXPECT_EQ(vault->Size(keyId), 0);
        }
    } else {
        printf("FATAL: Failed to put key into vault, HMAC tests can't be performed\n");
    }
}


TEST(Cipher, AES)
{
    const uint8_t data[] = "Look behind you, a Three-Headed Monkey!";
    const uint16_t dataSize = sizeof(data) - 1;
    const uint16_t expectedSize = dataSize + (16 - (dataSize % 16));
    const uint16_t bufferSize = expectedSize + 16;

    const uint8_t iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

    const uint8_t key128[] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    const uint8_t expected_AES_CBC_128[expectedSize] = {
	    0xB0, 0xF2, 0x9F, 0xB5, 0x55, 0xB2, 0x48, 0x08, 0x3A, 0x0D, 0xD9, 0xDB,
	    0x6E, 0xD3, 0x37, 0x13, 0x5F, 0x91, 0x92, 0xBB, 0x33, 0xCE, 0x22, 0x21,
	    0xDB, 0xEC, 0x8C, 0x55, 0xD7, 0xB9, 0xD9, 0x96, 0xEA, 0x5E, 0xE0, 0x84,
	    0x9F, 0xBA, 0x44, 0x13, 0xCE, 0x2C, 0x63, 0x8A, 0x0A, 0x1D, 0x60, 0xBC
    };

    uint32_t key128Id = vault->Import(sizeof(key128), key128);
    EXPECT_NE(key128Id, 0);
    if (key128Id != 0) {
        WPEFramework::Cryptography::ICipher* aes = vault->AES(WPEFramework::Cryptography::aesmode::CBC, key128Id);
        EXPECT_NE(aes, nullptr);
        if (aes) {
                uint8_t* output = new uint8_t[bufferSize];
                ::memset(output, 0, bufferSize);
                uint8_t* input = new uint8_t[bufferSize];
                ::memset(input, 0, bufferSize);

                DumpBuffer(data, dataSize);

                EXPECT_EQ(aes->Encrypt(sizeof(iv), iv, dataSize, data, bufferSize, output), expectedSize);
                DumpBuffer(output, expectedSize);
                EXPECT_EQ(::memcmp(output, expected_AES_CBC_128, expectedSize), 0);

                EXPECT_EQ(aes->Decrypt(sizeof(iv), iv, expectedSize, output, bufferSize, input), dataSize);
                DumpBuffer(input, dataSize);
                EXPECT_EQ(::memcmp(input, data, dataSize), 0);

                delete[] output;
                delete[] input;
        } else {
            printf("FATAL: Failed to create cryptors, AES tests can't be performed\n");
        }

        if (aes != nullptr) {
            aes->Release();
        }

        EXPECT_NE(vault->Delete(key128Id), false);
        EXPECT_EQ(vault->Size(key128Id), 0);
    } else {
        printf("FATAL: Failed to put key into vault, AES tests can't be performed\n");
    }
}


static bool GenerateDHKeyPair(const uint32_t generator, const uint8_t modulus[], const uint16_t modulusSize, uint32_t *privKey, uint32_t *pubKey)
{
    bool result = false;

    uint32_t dhPrivateKey = 0;
    uint32_t dhPublicKey = 0;

    WPEFramework::Cryptography::IDiffieHellman* dh = vault->DiffieHellman();
    EXPECT_NE(dh, nullptr);
    if (dh != nullptr) {
        uint32_t DHStatus = dh->Generate(generator, modulusSize, modulus, dhPrivateKey, dhPublicKey);
        EXPECT_EQ(DHStatus, 0);
        EXPECT_GT(dhPrivateKey, 0x80000000U);
        EXPECT_GT(dhPublicKey, 0x80000000U);
        EXPECT_NE(dhPublicKey, dhPrivateKey);

        (*privKey) = dhPrivateKey;
        (*pubKey) = dhPublicKey;

        result = true;
    } else {
        printf("FATAL: Failed to get Diffie-Hellman interface, DH tests are skipped\n");
    }

    return (result);
}

TEST(DH, Generate)
{
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

    uint32_t privateKeyId;
    uint32_t publicKeyId;

    GenerateDHKeyPair(testGenerator, testPrime1024, sizeof(testPrime1024), &privateKeyId, &publicKeyId);

    uint16_t publicKeySize = vault->Size(publicKeyId);
    uint16_t privateKeySize = vault->Size(privateKeyId);
    EXPECT_NE(publicKeySize, 0);
    EXPECT_NE(publicKeySize, USHRT_MAX);
    EXPECT_EQ(privateKeySize, USHRT_MAX);
    if (publicKeySize != 0) {
        uint8_t* publicKeyBuf = new uint8_t[publicKeySize];
        ::memset(publicKeyBuf, 0, publicKeySize);
        EXPECT_EQ(vault->Export(publicKeyId, publicKeySize, publicKeyBuf), publicKeySize);
        DumpBuffer(publicKeyBuf, publicKeySize);
        delete[] publicKeyBuf;
    }

    EXPECT_NE(vault->Delete(privateKeyId), false);
    EXPECT_NE(vault->Delete(publicKeyId), false);

    EXPECT_EQ(vault->Size(privateKeyId), 0);
    EXPECT_EQ(vault->Size(publicKeyId), 0);

}

int main()
{
    cg = WPEFramework::Cryptography::ICryptography::Instance("");
    if (cg != nullptr) {
        vault = cg->Vault(cryptographyvault::CRYPTOGRAPHY_VAULT_NETFLIX);
        if (vault != nullptr) {
            CALL(Vault, ImportExport);
            CALL(Vault, SetGet); // Will not work on Sage

            CALL(Hash, Hash);
            CALL(Hash, HMAC);

            CALL(Cipher, AES);

            CALL(DH, Generate);
        } else {
            printf("FATAL: Failed to acquire IVault, Vault tests can't be performed\n");
        }
   } else {
        printf("FATAL: Failed to acquire ICryptographic, no tests can't be performed\n");
    }

    printf("TOTAL: %i tests; %i PASSED, %i FAILED\n", TotalTests, TotalTestsPassed, (TotalTests - TotalTestsPassed));

    Teardown();

    return (TotalTests - TotalTestsPassed);
}

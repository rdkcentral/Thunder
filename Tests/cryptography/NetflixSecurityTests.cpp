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

#include <cryptography.h>
#include <core/core.h>
#include <string.h>
#include <climits>

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bn.h>

#include "Helpers.h"
#include "Test.h"


static WPEFramework::Cryptography::IVault* vault = nullptr;
static WPEFramework::Cryptography::INetflixSecurity* nfSecurity = nullptr;


TEST(NetflixSecurity, Security)
{
    std::string esn;
    EXPECT_NE(nfSecurity->EncryptionKey(), 0);
    EXPECT_NE(nfSecurity->HMACKey(), 0);
    EXPECT_NE(nfSecurity->WrappingKey(), 0);
    esn = nfSecurity->ESN();
    EXPECT_NE(esn.length(), 0);
    printf("ESN: %s\n", esn.c_str());
}

static void TestAuthenticatedDerive(const uint16_t hostPskSize, const uint8_t hostPsk[], uint32_t teePskKeyId, uint8_t* hostNewWrapKey[], uint32_t& teeNewWrapKeyId)
{
    static const uint32_t generator = 5;

    static const uint8_t prime1024[128] = {
        0x96, 0x94, 0xe9, 0xd8, 0xd9, 0x3a, 0x5a, 0xc7, 0x4c, 0x50, 0x9b, 0x4b, 0xbc, 0xe8, 0x5e, 0x92,
        0x13, 0x2c, 0xd1, 0x9c, 0xce, 0x47, 0x7d, 0x1a, 0x7e, 0x47, 0xd5, 0x27, 0xd9, 0xec, 0x29, 0x15,
        0x15, 0xf0, 0xb8, 0xb3, 0xe1, 0xea, 0xed, 0x50, 0x06, 0xe1, 0xb1, 0xb9, 0x1e, 0xa2, 0x5b, 0x91,
        0xa0, 0x1b, 0x10, 0xe2, 0xe8, 0x34, 0xb8, 0xd6, 0x60, 0xb2, 0xe3, 0x21, 0xad, 0x64, 0x4c, 0xe1,
        0xa8, 0x3b, 0x32, 0x8d, 0x90, 0x14, 0xee, 0x7e, 0x16, 0xf1, 0xe4, 0x4f, 0xfe, 0x89, 0x57, 0x9a,
        0xc3, 0xee, 0x47, 0xd6, 0x68, 0xb6, 0xb7, 0x66, 0x87, 0xc2, 0xfe, 0x90, 0xa3, 0x5b, 0x5e, 0x60,
        0x28, 0xfd, 0x04, 0xef, 0xea, 0x88, 0x23, 0x73, 0xec, 0xf6, 0x0b, 0xa2, 0xf6, 0x37, 0xe4, 0xcd,
        0xaa, 0x1b, 0x60, 0x89, 0xd6, 0xc0, 0xb5, 0x61, 0xa8, 0xe5, 0x20, 0xe7, 0x96, 0xde, 0x27, 0xdf
    };

    static const uint8_t salt[16] = { 0x02, 0x76, 0x17, 0x98, 0x4f, 0x62, 0x27, 0x53, 0x9a, 0x63, 0x0b, 0x89, 0x7c, 0x01, 0x7d, 0x69 };
    static const uint8_t data[16] = { 0x80, 0x9f, 0x82, 0xa7, 0xad, 0xdf, 0x54, 0x8d, 0x3e, 0xa9, 0xdd, 0x06, 0x7f, 0xf9, 0xbb, 0x91 };
    const char testStr[] = "Thunder";

    uint8_t* hostEncKey = NULL;
    uint8_t* hostHmacKey = NULL;
    uint8_t* hostWrapKey = NULL;

    // Generate host DH keypair
    DH *hostPrivKey = DHGenerate(generator, prime1024, sizeof(prime1024));
    assert(hostPrivKey != nullptr);

    WPEFramework::Cryptography::IDiffieHellman *idh = vault->DiffieHellman();
    EXPECT_NE(idh, nullptr);

    if (idh != nullptr) {
        // Import host public DH key into vault
        uint16_t pubKeySize = BN_num_bytes(hostPrivKey->pub_key);
        uint8_t* pubKeySizeBuf = (uint8_t*)alloca(pubKeySize);
        pubKeySize = BN_bn2bin(hostPrivKey->pub_key, pubKeySizeBuf);
        assert(pubKeySize != 0);
        uint32_t hostPubKeyId = vault->Import(pubKeySize, pubKeySizeBuf);
        EXPECT_NE(hostPubKeyId, 0);
        EXPECT_EQ(vault->Size(hostPubKeyId), pubKeySize);

        // Generate TEE DH keypair
        uint32_t teePrivKeyId = 0;
        uint32_t teePubKeyId = 0;
        EXPECT_EQ(idh->Generate(generator, sizeof(prime1024), prime1024, teePrivKeyId, teePubKeyId), 0);
        EXPECT_NE(teePrivKeyId, 0);
        EXPECT_NE(teePubKeyId, 0);
        EXPECT_NE(teePrivKeyId, teePubKeyId);

        if (teePrivKeyId && teePubKeyId) {
            // Extract TEE's public DH key
            uint16_t teePubKeySize = vault->Size(teePubKeyId);
            EXPECT_NE(teePubKeySize, 0);
            EXPECT_NE(teePubKeySize, USHRT_MAX);
            uint8_t* teePubKey = (uint8_t*)alloca(teePubKeySize);
            EXPECT_EQ(vault->Export(teePubKeyId, teePubKeySize, teePubKey), teePubKeySize);
            BIGNUM* teePubKeyBn = BN_bin2bn(teePubKey, teePubKeySize, NULL);
            assert(teePubKeyBn);

            // Derive a shared secret on host
            uint8_t* hostSecret = DHDerive(hostPrivKey, teePubKeyBn);
            assert(hostSecret);

            if (hostSecret) {
                // Derive authenticated keys on host
                if (DHAuthenticatedDerive(hostPrivKey, sizeof(prime1024), hostSecret, hostPskSize, hostPsk,
                                        sizeof(salt), salt, sizeof(data), data, &hostEncKey, &hostHmacKey, &hostWrapKey)) {
                    assert(hostEncKey);
                    assert(hostHmacKey);
                    assert(hostWrapKey);

                    uint32_t teeEncKeyId = 0;
                    uint32_t teeHmacKeyId = 0;
                    uint32_t teeWrapKeyId = 0;

                    // Derive authenticated keys on TEE
                    EXPECT_EQ(nfSecurity->DeriveKeys(teePrivKeyId, hostPubKeyId, teePskKeyId, teeEncKeyId, teeHmacKeyId, teeWrapKeyId), 0);
                    EXPECT_NE(teeEncKeyId, 0);
                    EXPECT_NE(teeHmacKeyId, 0);
                    EXPECT_NE(teeWrapKeyId, 0);
                    EXPECT_EQ(vault->Size(teeEncKeyId), USHRT_MAX);
                    EXPECT_EQ(vault->Size(teeHmacKeyId), USHRT_MAX);
                    EXPECT_EQ(vault->Size(teeWrapKeyId), USHRT_MAX);

                    // Verify encryption with AES cipher of a common buffer
                    printf("Clear data to be encrypted on host:\n");
                    DumpBuffer((uint8_t*)testStr, sizeof(testStr));

                    uint8_t encBuf[32] = { 0 };
                    int encLen = 0;
                    EVP_CIPHER_CTX* cctx = EVP_CIPHER_CTX_new();
                    assert(cctx);
                    EVP_CipherInit_ex(cctx, EVP_aes_128_cbc(), nullptr, (const unsigned char*)hostEncKey, (const unsigned char*)salt, 1);
                    EVP_CipherUpdate(cctx, encBuf, &encLen, (const unsigned char*)testStr, (int)sizeof(testStr));
                    EVP_CipherFinal_ex(cctx, (encBuf + encLen), &encLen);
                    EVP_CIPHER_CTX_free(cctx);
                    printf("Encrypted data on host:\n");
                    DumpBuffer(encBuf, (uint32_t)encLen);

                    uint8_t decBuf[32] = { 0 };
                    WPEFramework::Cryptography::ICipher *cipher = vault->AES(WPEFramework::Cryptography::aesmode::CBC, teeEncKeyId);
                    EXPECT_NE(cipher, nullptr);
                    if (cipher != nullptr) {
                        uint32_t decLen = 0;
                        EXPECT_EQ(decLen = cipher->Decrypt(sizeof(salt), salt, encLen, encBuf, sizeof(decBuf), decBuf), sizeof(testStr));
                        if (decLen != (uint32_t) -1L) {
                            printf("Decrypted data on TEE:\n");
                            DumpBuffer(decBuf, decLen);
                        }
                    }

                    EXPECT_EQ(memcmp(testStr, decBuf, sizeof(testStr)), 0);

                    // Verify HMAC key by calculating a HMAC of a common buffer
                    uint8_t teeHmac[SHA256_DIGEST_LENGTH] = { 0 };
                    uint8_t hostHmac[SHA256_DIGEST_LENGTH] = { 0 };

                    HMAC(EVP_sha256(), hostHmacKey, SHA256_DIGEST_LENGTH, (uint8_t*)testStr, sizeof(testStr), hostHmac, nullptr);
                    printf("Host HMAC using hmacKey:\n");
                    DumpBuffer(hostHmac, sizeof(hostHmac));

                    WPEFramework::Cryptography::IHash *hash = vault->HMAC(WPEFramework::Cryptography::hashtype::SHA256, teeHmacKeyId);
                    EXPECT_NE(hash, nullptr);
                    if (hash != nullptr) {
                        EXPECT_EQ(hash->Ingest(sizeof(testStr), (uint8_t*)testStr), sizeof(testStr));
                        EXPECT_EQ(hash->Calculate(sizeof(teeHmac), teeHmac), sizeof(teeHmac));
                        hash->Release();
                        hash = nullptr;

                        printf("TEE HMAC using hmacKey:\n");
                        DumpBuffer(teeHmac, sizeof(teeHmac));
                    }

                    EXPECT_EQ(memcmp(teeHmac, hostHmac, SHA256_DIGEST_LENGTH), 0);

                    EXPECT_EQ(vault->Delete(teeEncKeyId), true);
                    EXPECT_EQ(vault->Delete(teeHmacKeyId), true);

                    EXPECT_EQ(vault->Size(teeEncKeyId), 0);
                    EXPECT_EQ(vault->Size(teeHmacKeyId), 0);

                    free(hostEncKey);
                    free(hostHmacKey);

                    teeNewWrapKeyId = teeWrapKeyId;
                    (*hostNewWrapKey) = hostWrapKey;
                }

                free(hostSecret);
            }

            BN_free(teePubKeyBn);
        }

        DH_free(hostPrivKey);

        EXPECT_EQ(vault->Delete(teePrivKeyId), true);
        EXPECT_EQ(vault->Delete(teePubKeyId), true);

        idh->Release();
    }
}

TEST(NetflixSecurity, AuthenticatedDerive)
{
    // This test will attempt to exercise the authenticated DH exchange procedure using a substituted wrapping key.

    uint32_t teePskKeyId = 4; // hardcoded!
    static const uint8_t psk[16]  = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x11 };

    if (vault->Size(teePskKeyId) == 0) {
        // The vault does not include the test wrapping key, so try to install it now.
        teePskKeyId = vault->Import(sizeof(psk), psk);
    }

    EXPECT_NE(teePskKeyId, 0);
    EXPECT_NE(vault->Size(teePskKeyId), 0);

    // Do the authentication twice to verify the wrap key too!
    uint32_t teeWrapKeyId = 0;
    uint8_t* hostWrapKey = nullptr;

    printf("\nAUTHENTICATION WITH PRE-SHARED KEY\n");
    TestAuthenticatedDerive(sizeof(psk), psk, teePskKeyId, &hostWrapKey, teeWrapKeyId);
    if (teeWrapKeyId != 0) {
        uint32_t teeWrapKeyId2 = 0;
        uint8_t* hostWrapKey2 = nullptr;

        printf("\nAUTHENTICATION WITH DERIVED KEY\n");
        TestAuthenticatedDerive(sizeof(psk), hostWrapKey, teeWrapKeyId, &hostWrapKey2, teeWrapKeyId2);
        EXPECT_EQ(vault->Delete(teeWrapKeyId), true);
        EXPECT_EQ(vault->Delete(teeWrapKeyId2), true);

        free(hostWrapKey);
        free(hostWrapKey2);
    }

    EXPECT_EQ(vault->Delete(teePskKeyId), true);
}

int main()
{
    nfSecurity = WPEFramework::Cryptography::INetflixSecurity::Instance();
    if (nfSecurity != nullptr) {
        CALL(NetflixSecurity, Security);

        WPEFramework::Cryptography::ICryptography* cg = WPEFramework::Cryptography::ICryptography::Instance("");
        if (cg != nullptr) {
            vault = cg->Vault(CRYPTOGRAPHY_VAULT_NETFLIX);
            if (vault != nullptr) {
                CALL(NetflixSecurity, AuthenticatedDerive);
                vault->Release();
            } else {
                printf("FATAL: Failed to acquire IVault\n");
            }
        } else {
            printf("FATAL: Failed to acquire ICryptography\n");
        }
    } else {
        printf("FATAL: Failed to acquire INetflixSecurity, NetflixSecurity tests can't be performed\n");
    }

    printf("TOTAL: %i tests; %i PASSED, %i FAILED\n", TotalTests, TotalTestsPassed, (TotalTests - TotalTestsPassed));

    Teardown();

    return (TotalTests - TotalTestsPassed);
}

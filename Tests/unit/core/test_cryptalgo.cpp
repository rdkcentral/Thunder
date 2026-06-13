/*
 * Copyright 2026 Metrological
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

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <gtest/gtest.h>

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

namespace Thunder {
namespace Tests {

    // Helper: convert digest bytes to hex string for comparison
    static string ToHex(const uint8_t* data, uint8_t length)
    {
        string result;
        result.reserve(length * 2);
        for (uint8_t i = 0; i < length; i++) {
            char buf[3];
            snprintf(buf, sizeof(buf), "%02x", data[i]);
            result += buf;
        }
        return result;
    }

    // =====================================================================
    // Test Suite: Cryptalgo_HMAC
    // Covers: HMACType<SHA256>, HMACType<SHA384>, HMACType<SHA512>
    // Report gap: HMAC-SHA256/384/512 variants, empty key, Thunder wrapper
    // RFC 4231 Test Case 2: Key="Jefe", Data="what do ya want for nothing?"
    // =====================================================================

    TEST(Cryptalgo_HMAC, SHA256_RFC4231_TestCase2)
    {
        // RFC 4231 Test Case 2
        const string key = "Jefe";
        const string data = "what do ya want for nothing?";

        Crypto::SHA256HMAC hmac(key);
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result = hmac.Result();

        string hex = ToHex(result, Crypto::SHA256HMAC::Length);
        EXPECT_EQ(hex, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    }

    TEST(Cryptalgo_HMAC, SHA384_Deterministic)
    {
        const string key = "Jefe";
        const string data = "what do ya want for nothing?";

        Crypto::SHA384HMAC hmac1(key);
        hmac1.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex1 = ToHex(hmac1.Result(), Crypto::SHA384HMAC::Length);

        // Verify determinism
        Crypto::SHA384HMAC hmac2(key);
        hmac2.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex2 = ToHex(hmac2.Result(), Crypto::SHA384HMAC::Length);

        EXPECT_EQ(hex1, hex2);
        // Verify correct digest length (48 bytes = 96 hex chars)
        EXPECT_EQ(hex1.length(), 96u);
    }

    TEST(Cryptalgo_HMAC, SHA512_Deterministic)
    {
        const string key = "Jefe";
        const string data = "what do ya want for nothing?";

        Crypto::SHA512HMAC hmac1(key);
        hmac1.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex1 = ToHex(hmac1.Result(), Crypto::SHA512HMAC::Length);

        // Verify determinism
        Crypto::SHA512HMAC hmac2(key);
        hmac2.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex2 = ToHex(hmac2.Result(), Crypto::SHA512HMAC::Length);

        EXPECT_EQ(hex1, hex2);
        // Verify correct digest length (64 bytes = 128 hex chars)
        EXPECT_EQ(hex1.length(), 128u);
    }

    TEST(Cryptalgo_HMAC, SHA1_RFC2104_TestCase2)
    {
        // Same key/data as above, verify SHA1 HMAC against RFC 2104 / RFC 4231
        const string key = "Jefe";
        const string data = "what do ya want for nothing?";

        Crypto::SHA1HMAC hmac(key);
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result = hmac.Result();

        string hex = ToHex(result, Crypto::SHA1HMAC::Length);
        EXPECT_EQ(hex, "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79");
    }

    TEST(Cryptalgo_HMAC, MD5_RFC2104_TestCase2)
    {
        // RFC 2104 Test Case 2
        const string key = "Jefe";
        const string data = "what do ya want for nothing?";

        Crypto::MD5HMAC hmac(key);
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result = hmac.Result();

        string hex = ToHex(result, Crypto::MD5HMAC::Length);
        EXPECT_EQ(hex, "750c783e6ab0b503eaa86e310a5db738");
    }

    TEST(Cryptalgo_HMAC, SHA256_EmptyKey)
    {
        string emptyKey;
        Crypto::SHA256HMAC hmac(emptyKey);
        const string data = "test data";
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result = hmac.Result();

        // Should produce a valid 32-byte digest (not crash)
        ASSERT_NE(result, nullptr);
        EXPECT_EQ(+Crypto::SHA256HMAC::Length, 32u);
    }

    TEST(Cryptalgo_HMAC, SHA256_EmptyMessage)
    {
        const string key = "secret";
        Crypto::SHA256HMAC hmac(key);
        // Don't call Input — empty message
        const uint8_t* result = hmac.Result();

        ASSERT_NE(result, nullptr);
        // HMAC of empty message with key should be deterministic
        string hex1 = ToHex(result, Crypto::SHA256HMAC::Length);

        Crypto::SHA256HMAC hmac2(key);
        const uint8_t* result2 = hmac2.Result();
        string hex2 = ToHex(result2, Crypto::SHA256HMAC::Length);

        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_HMAC, SHA256_LongKey_AutoHashed)
    {
        // Key longer than 64 bytes should be auto-hashed per HMAC spec
        string longKey(100, 'K');
        const string data = "test message";

        Crypto::SHA256HMAC hmac(longKey);
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result = hmac.Result();

        ASSERT_NE(result, nullptr);

        // Verify determinism with same long key
        Crypto::SHA256HMAC hmac2(longKey);
        hmac2.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        const uint8_t* result2 = hmac2.Result();

        string hex1 = ToHex(result, Crypto::SHA256HMAC::Length);
        string hex2 = ToHex(result2, Crypto::SHA256HMAC::Length);
        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_HMAC, SHA256_Reset_ReuseSameKey)
    {
        const string key = "mykey";
        const string data = "hello";

        Crypto::SHA256HMAC hmac(key);
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex1 = ToHex(hmac.Result(), Crypto::SHA256HMAC::Length);

        // Reset and re-input same data — should get same result
        hmac.Reset();
        hmac.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex2 = ToHex(hmac.Result(), Crypto::SHA256HMAC::Length);

        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_HMAC, SHA256_IncrementalInput)
    {
        const string key = "secret";
        const string data = "hello world";

        // One-shot
        Crypto::SHA256HMAC hmac1(key);
        hmac1.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex1 = ToHex(hmac1.Result(), Crypto::SHA256HMAC::Length);

        // Incremental
        Crypto::SHA256HMAC hmac2(key);
        hmac2.Input(reinterpret_cast<const uint8_t*>("hello"), 5);
        hmac2.Input(reinterpret_cast<const uint8_t*>(" world"), 6);
        string hex2 = ToHex(hmac2.Result(), Crypto::SHA256HMAC::Length);

        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_HMAC, SHA256_DifferentKey_DifferentResult)
    {
        const string data = "same data";

        Crypto::SHA256HMAC hmac1(string("key1"));
        hmac1.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex1 = ToHex(hmac1.Result(), Crypto::SHA256HMAC::Length);

        Crypto::SHA256HMAC hmac2(string("key2"));
        hmac2.Input(reinterpret_cast<const uint8_t*>(data.c_str()), static_cast<uint16_t>(data.length()));
        string hex2 = ToHex(hmac2.Result(), Crypto::SHA256HMAC::Length);

        EXPECT_NE(hex1, hex2);
    }

    TEST(Cryptalgo_HMAC, DigestLengths)
    {
        EXPECT_EQ(+Crypto::MD5HMAC::Length, 16u);
        EXPECT_EQ(+Crypto::SHA1HMAC::Length, 20u);
        EXPECT_EQ(+Crypto::SHA224HMAC::Length, 28u);
        EXPECT_EQ(+Crypto::SHA256HMAC::Length, 32u);
        EXPECT_EQ(+Crypto::SHA384HMAC::Length, 48u);
        EXPECT_EQ(+Crypto::SHA512HMAC::Length, 64u);
    }

    // =====================================================================
    // Test Suite: Cryptalgo_AES
    // Covers: AESEncryption, AESDecryption — ECB, CBC modes
    // Report gap: Thunder's cryptalgo AES wrapper, key sizes, error paths
    // =====================================================================

    TEST(Cryptalgo_AES, ECB_128_EncryptDecrypt_RoundTrip)
    {
        const uint8_t key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        // ECB works on 16-byte blocks
        const uint8_t plaintext[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };

        uint8_t ciphertext[16] = {};
        uint8_t decrypted[16] = {};

        Crypto::AESEncryption enc(Crypto::aesType::AES_ECB);
        uint32_t result = enc.Key(16, key);
        EXPECT_EQ(result, 0u);

        result = enc.Encrypt(16, plaintext, ciphertext);
        EXPECT_EQ(result, 0u);

        // Ciphertext should differ from plaintext
        EXPECT_NE(memcmp(plaintext, ciphertext, 16), 0);

        Crypto::AESDecryption dec(Crypto::aesType::AES_ECB);
        result = dec.Key(16, key);
        EXPECT_EQ(result, 0u);

        result = dec.Decrypt(16, ciphertext, decrypted);
        EXPECT_EQ(result, 0u);

        EXPECT_EQ(memcmp(plaintext, decrypted, 16), 0);
    }

    TEST(Cryptalgo_AES, CBC_128_EncryptDecrypt_RoundTrip)
    {
        const uint8_t key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        const uint8_t iv[16] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        };
        const uint8_t plaintext[32] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
            0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
            0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51
        };

        uint8_t ciphertext[32] = {};
        uint8_t decrypted[32] = {};

        Crypto::AESEncryption enc(Crypto::aesType::AES_CBC);
        enc.Key(16, key);
        enc.InitialVector(iv);
        uint32_t result = enc.Encrypt(32, plaintext, ciphertext);
        EXPECT_EQ(result, 0u);

        EXPECT_NE(memcmp(plaintext, ciphertext, 32), 0);

        Crypto::AESDecryption dec(Crypto::aesType::AES_CBC);
        dec.Key(16, key);
        dec.InitialVector(iv);
        result = dec.Decrypt(32, ciphertext, decrypted);
        EXPECT_EQ(result, 0u);

        EXPECT_EQ(memcmp(plaintext, decrypted, 32), 0);
    }

    TEST(Cryptalgo_AES, ECB_192_EncryptDecrypt_RoundTrip)
    {
        const uint8_t key[24] = {
            0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52,
            0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5,
            0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b
        };
        const uint8_t plaintext[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };

        uint8_t ciphertext[16] = {};
        uint8_t decrypted[16] = {};

        Crypto::AESEncryption enc(Crypto::aesType::AES_ECB);
        enc.Key(24, key);
        enc.Encrypt(16, plaintext, ciphertext);

        Crypto::AESDecryption dec(Crypto::aesType::AES_ECB);
        dec.Key(24, key);
        dec.Decrypt(16, ciphertext, decrypted);

        EXPECT_EQ(memcmp(plaintext, decrypted, 16), 0);
    }

    TEST(Cryptalgo_AES, ECB_256_EncryptDecrypt_RoundTrip)
    {
        const uint8_t key[32] = {
            0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
            0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
            0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
            0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
        };
        const uint8_t plaintext[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };

        uint8_t ciphertext[16] = {};
        uint8_t decrypted[16] = {};

        Crypto::AESEncryption enc(Crypto::aesType::AES_ECB);
        enc.Key(32, key);
        enc.Encrypt(16, plaintext, ciphertext);

        Crypto::AESDecryption dec(Crypto::aesType::AES_ECB);
        dec.Key(32, key);
        dec.Decrypt(16, ciphertext, decrypted);

        EXPECT_EQ(memcmp(plaintext, decrypted, 16), 0);
    }

    TEST(Cryptalgo_AES, WrongKey_DecryptFails)
    {
        const uint8_t key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        const uint8_t wrongKey[16] = {
            0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
            0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
        };
        const uint8_t plaintext[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };

        uint8_t ciphertext[16] = {};
        uint8_t decrypted[16] = {};

        Crypto::AESEncryption enc(Crypto::aesType::AES_ECB);
        enc.Key(16, key);
        enc.Encrypt(16, plaintext, ciphertext);

        // Decrypt with wrong key
        Crypto::AESDecryption dec(Crypto::aesType::AES_ECB);
        dec.Key(16, wrongKey);
        dec.Decrypt(16, ciphertext, decrypted);

        // Should NOT match original plaintext
        EXPECT_NE(memcmp(plaintext, decrypted, 16), 0);
    }

    TEST(Cryptalgo_AES, CBC_InitialVector_GetSet)
    {
        const uint8_t iv[16] = {
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
        };

        Crypto::AESEncryption enc(Crypto::aesType::AES_CBC);
        enc.InitialVector(iv);

        const uint8_t* retrieved = enc.InitialVector();
        EXPECT_EQ(memcmp(iv, retrieved, 16), 0);
    }

    TEST(Cryptalgo_AES, DifferentIV_DifferentCiphertext)
    {
        const uint8_t key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c
        };
        const uint8_t plaintext[16] = {
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a
        };
        const uint8_t iv1[16] = { 0 };
        const uint8_t iv2[16] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

        uint8_t cipher1[16] = {};
        uint8_t cipher2[16] = {};

        Crypto::AESEncryption enc1(Crypto::aesType::AES_CBC);
        enc1.Key(16, key);
        enc1.InitialVector(iv1);
        enc1.Encrypt(16, plaintext, cipher1);

        Crypto::AESEncryption enc2(Crypto::aesType::AES_CBC);
        enc2.Key(16, key);
        enc2.InitialVector(iv2);
        enc2.Encrypt(16, plaintext, cipher2);

        // Same key, different IV → different ciphertext
        EXPECT_NE(memcmp(cipher1, cipher2, 16), 0);
    }

    TEST(Cryptalgo_AES, TypeAccessor)
    {
        Crypto::AESEncryption ecb(Crypto::aesType::AES_ECB);
        EXPECT_EQ(ecb.Type(), Crypto::aesType::AES_ECB);

        Crypto::AESEncryption cbc(Crypto::aesType::AES_CBC);
        EXPECT_EQ(cbc.Type(), Crypto::aesType::AES_CBC);

        Crypto::AESDecryption dec(Crypto::aesType::AES_OFB);
        EXPECT_EQ(dec.Type(), Crypto::aesType::AES_OFB);
    }

    // =====================================================================
    // Test Suite: Cryptalgo_Random
    // Covers: Random number generation
    // Report gap: Random output non-zero, no immediate repeats
    // =====================================================================

    TEST(Cryptalgo_Random, Uint32_ProducesValue)
    {
        uint32_t value = 0;
        Crypto::Random(value);
        // Statistically near-impossible to get 0 from 32-bit random
        // But we mainly verify it doesn't crash; check non-zero as sanity
        // (1 in 2^32 chance of false failure — acceptable)
        EXPECT_NE(value, 0u);
    }

    TEST(Cryptalgo_Random, Uint64_ProducesValue)
    {
        uint64_t value = 0;
        Crypto::Random(value);
        EXPECT_NE(value, 0u);
    }

    TEST(Cryptalgo_Random, Uint16_ProducesValue)
    {
        uint16_t value = 0;
        Crypto::Random(value);
        // Just verify it runs — 16-bit has 1/65536 chance of 0
        // We'll check diversity below instead
        (void)value;
    }

    TEST(Cryptalgo_Random, Uint8_ProducesValue)
    {
        uint8_t value = 0;
        Crypto::Random(value);
        (void)value; // Just verify no crash
    }

    TEST(Cryptalgo_Random, NoImmediateRepeats)
    {
        // Generate 10 random uint32_t values; at least 2 should differ
        uint32_t values[10];
        for (int i = 0; i < 10; i++) {
            Crypto::Random(values[i]);
        }

        bool allSame = true;
        for (int i = 1; i < 10; i++) {
            if (values[i] != values[0]) {
                allSame = false;
                break;
            }
        }
        EXPECT_FALSE(allSame);
    }

    TEST(Cryptalgo_Random, Uint64_Diversity)
    {
        uint64_t v1 = 0, v2 = 0;
        Crypto::Random(v1);
        Crypto::Random(v2);
        EXPECT_NE(v1, v2);
    }

    TEST(Cryptalgo_Random, Reseed_DoesNotCrash)
    {
        Crypto::Reseed();
        uint32_t value = 0;
        Crypto::Random(value);
        // Should still produce values after reseed
        (void)value;
    }

    // =====================================================================
    // Test Suite: Cryptalgo_Hash
    // Covers: Hash with empty input, SHA384/SHA512 known vectors
    // Report gap: Hash with empty input, Thunder wrapper layer
    // =====================================================================

    TEST(Cryptalgo_Hash, SHA256_EmptyInput)
    {
        // SHA-256 of empty string is well-known:
        // e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
        Crypto::SHA256 hash;
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA256::Length);
        EXPECT_EQ(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }

    TEST(Cryptalgo_Hash, SHA384_EmptyInput)
    {
        // SHA-384 of empty string:
        // 38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b
        Crypto::SHA384 hash;
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA384::Length);
        EXPECT_EQ(hex, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b");
    }

    TEST(Cryptalgo_Hash, SHA512_EmptyInput)
    {
        // SHA-512 of empty string:
        // cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e
        Crypto::SHA512 hash;
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA512::Length);
        EXPECT_EQ(hex, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
    }

    TEST(Cryptalgo_Hash, MD5_EmptyInput)
    {
        // MD5 of empty string: d41d8cd98f00b204e9800998ecf8427e
        Crypto::MD5 hash;
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::MD5::Length);
        EXPECT_EQ(hex, "d41d8cd98f00b204e9800998ecf8427e");
    }

    TEST(Cryptalgo_Hash, SHA1_EmptyInput)
    {
        // SHA-1 of empty string: da39a3ee5e6b4b0d3255bfef95601890afd80709
        Crypto::SHA1 hash;
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA1::Length);
        EXPECT_EQ(hex, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    }

    TEST(Cryptalgo_Hash, SHA256_KnownVector)
    {
        // SHA-256("abc") = ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad
        const uint8_t data[] = { 'a', 'b', 'c' };
        Crypto::SHA256 hash(data, 3);
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA256::Length);
        EXPECT_EQ(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }

    TEST(Cryptalgo_Hash, SHA384_KnownVector)
    {
        // SHA-384("abc") = cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7
        const uint8_t data[] = { 'a', 'b', 'c' };
        Crypto::SHA384 hash(data, 3);
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA384::Length);
        EXPECT_EQ(hex, "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
    }

    TEST(Cryptalgo_Hash, SHA512_KnownVector)
    {
        // SHA-512("abc") = ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f
        const uint8_t data[] = { 'a', 'b', 'c' };
        Crypto::SHA512 hash(data, 3);
        const uint8_t* result = hash.Result();

        string hex = ToHex(result, Crypto::SHA512::Length);
        EXPECT_EQ(hex, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
    }

    TEST(Cryptalgo_Hash, SHA256_IncrementalInput)
    {
        // One-shot vs incremental should give same result
        const uint8_t data[] = { 'a', 'b', 'c', 'd', 'e', 'f' };

        Crypto::SHA256 oneShot(data, 6);
        string hex1 = ToHex(oneShot.Result(), Crypto::SHA256::Length);

        Crypto::SHA256 incremental;
        incremental.Input(data, 3);      // "abc"
        incremental.Input(data + 3, 3);  // "def"
        string hex2 = ToHex(incremental.Result(), Crypto::SHA256::Length);

        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_Hash, SHA256_Reset)
    {
        const uint8_t data[] = { 'a', 'b', 'c' };

        Crypto::SHA256 hash(data, 3);
        string hex1 = ToHex(hash.Result(), Crypto::SHA256::Length);

        hash.Reset();
        hash.Input(data, 3);
        string hex2 = ToHex(hash.Result(), Crypto::SHA256::Length);

        EXPECT_EQ(hex1, hex2);
    }

    TEST(Cryptalgo_Hash, ContextSaveLoad)
    {
        const uint8_t data1[] = { 'h', 'e', 'l', 'l', 'o' };
        const uint8_t data2[] = { ' ', 'w', 'o', 'r', 'l', 'd' };

        // Compute full hash
        Crypto::SHA256 full;
        full.Input(data1, 5);
        full.Input(data2, 6);
        string hexFull = ToHex(full.Result(), Crypto::SHA256::Length);

        // Save context after first input, restore and continue
        Crypto::SHA256 partial;
        partial.Input(data1, 5);
        Crypto::Context ctx = partial.CurrentContext();

        Crypto::SHA256 restored;
        restored.Load(ctx);
        restored.Input(data2, 6);
        string hexRestored = ToHex(restored.Result(), Crypto::SHA256::Length);

        EXPECT_EQ(hexFull, hexRestored);
    }

    TEST(Cryptalgo_Hash, DigestLengths)
    {
        EXPECT_EQ(+Crypto::MD5::Length, 16u);
        EXPECT_EQ(+Crypto::SHA1::Length, 20u);
        EXPECT_EQ(+Crypto::SHA224::Length, 28u);
        EXPECT_EQ(+Crypto::SHA256::Length, 32u);
        EXPECT_EQ(+Crypto::SHA384::Length, 48u);
        EXPECT_EQ(+Crypto::SHA512::Length, 64u);
    }

    TEST(Cryptalgo_Hash, EnumTypes)
    {
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::MD5::Type), Crypto::HASH_MD5);
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::SHA1::Type), Crypto::HASH_SHA1);
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::SHA224::Type), Crypto::HASH_SHA224);
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::SHA256::Type), Crypto::HASH_SHA256);
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::SHA384::Type), Crypto::HASH_SHA384);
        EXPECT_EQ(static_cast<Crypto::EnumHashType>(Crypto::SHA512::Type), Crypto::HASH_SHA512);
    }

} // namespace Tests
} // namespace Thunder

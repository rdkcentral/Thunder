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
#include <websocket/websocket.h>

namespace Thunder {
namespace Tests {

    // Test key: 32 bytes for HMAC-SHA256
    static const uint8_t TestKey[32] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20
    };

    // Different key for wrong-key tests
    static const uint8_t WrongKey[32] = {
        0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
        0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0,
        0xef, 0xee, 0xed, 0xec, 0xeb, 0xea, 0xe9, 0xe8,
        0xe7, 0xe6, 0xe5, 0xe4, 0xe3, 0xe2, 0xe1, 0xe0
    };

    // =====================================================================
    // Test Suite: JWT_Encode
    // Covers: JSONWebToken::Encode — token creation
    // =====================================================================

    TEST(JWT_Encode, ProducesThreePartToken)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"sub\":\"1234\",\"name\":\"Test\"}";
        string token;
        uint16_t len = jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        EXPECT_GT(len, 0u);
        EXPECT_EQ(token.length(), len);

        // JWT must have exactly 3 parts separated by dots
        size_t dot1 = token.find('.');
        ASSERT_NE(dot1, string::npos);
        size_t dot2 = token.find('.', dot1 + 1);
        ASSERT_NE(dot2, string::npos);
        size_t dot3 = token.find('.', dot2 + 1);
        EXPECT_EQ(dot3, string::npos); // No fourth part
    }

    TEST(JWT_Encode, HeaderContainsHS256)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "test";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        // Extract and decode header (first part before first dot)
        size_t dot1 = token.find('.');
        ASSERT_NE(dot1, string::npos);
        string headerB64 = token.substr(0, dot1);

        uint8_t decoded[256] = {};
        uint16_t decodedLen = Core::URL::Base64Decode(
            headerB64.c_str(), static_cast<uint16_t>(headerB64.length()),
            decoded, sizeof(decoded), nullptr);

        string header(reinterpret_cast<const char*>(decoded), decodedLen);
        EXPECT_NE(header.find("HS256"), string::npos);
        EXPECT_NE(header.find("JWT"), string::npos);
    }

    TEST(JWT_Encode, DeterministicOutput)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"data\":\"hello\"}";
        string token1, token2;
        jwt.Encode(token1, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));
        jwt.Encode(token2, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        EXPECT_EQ(token1, token2);
    }

    TEST(JWT_Encode, DifferentPayloadDifferentToken)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload1 = "{\"a\":1}";
        const string payload2 = "{\"a\":2}";
        string token1, token2;
        jwt.Encode(token1, static_cast<uint16_t>(payload1.length()),
            reinterpret_cast<const uint8_t*>(payload1.c_str()));
        jwt.Encode(token2, static_cast<uint16_t>(payload2.length()),
            reinterpret_cast<const uint8_t*>(payload2.c_str()));

        EXPECT_NE(token1, token2);
    }

    TEST(JWT_Encode, DifferentKeyDifferentToken)
    {
        Web::JSONWebToken jwt1(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);
        Web::JSONWebToken jwt2(Web::JSONWebToken::SHA256, sizeof(WrongKey), WrongKey);

        const string payload = "{\"data\":\"same\"}";
        string token1, token2;
        jwt1.Encode(token1, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));
        jwt2.Encode(token2, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        // Headers and payloads should be same, but signatures should differ
        size_t dot1a = token1.find_last_of('.');
        size_t dot1b = token2.find_last_of('.');
        EXPECT_EQ(token1.substr(0, dot1a), token2.substr(0, dot1b));
        EXPECT_NE(token1.substr(dot1a), token2.substr(dot1b));
    }

    TEST(JWT_Encode, EmptyPayload)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        string token;
        uint16_t len = jwt.Encode(token, 0, reinterpret_cast<const uint8_t*>(""));

        // Should produce a valid token even with empty payload
        EXPECT_GT(len, 0u);
        size_t dot1 = token.find('.');
        ASSERT_NE(dot1, string::npos);
        size_t dot2 = token.find('.', dot1 + 1);
        ASSERT_NE(dot2, string::npos);
    }

    // =====================================================================
    // Test Suite: JWT_Decode
    // Covers: JSONWebToken::Decode — token validation and payload extraction
    // =====================================================================

    TEST(JWT_Decode, RoundTrip)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"sub\":\"1234\",\"name\":\"Test\",\"admin\":true}";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);

        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        string result(reinterpret_cast<const char*>(decoded), decodedLen);
        EXPECT_EQ(result, payload);
    }

    TEST(JWT_Decode, RoundTrip_BinaryPayload)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const uint8_t binaryPayload[] = { 0x00, 0x01, 0x02, 0xff, 0xfe, 0x80, 0x7f };
        string token;
        jwt.Encode(token, sizeof(binaryPayload), binaryPayload);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);

        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(decodedLen, sizeof(binaryPayload));
        EXPECT_EQ(memcmp(decoded, binaryPayload, sizeof(binaryPayload)), 0);
    }

    TEST(JWT_Decode, WrongKey_Fails)
    {
        Web::JSONWebToken jwtEncode(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);
        Web::JSONWebToken jwtDecode(Web::JSONWebToken::SHA256, sizeof(WrongKey), WrongKey);

        const string payload = "{\"secret\":\"data\"}";
        string token;
        jwtEncode.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwtDecode.Decode(token, sizeof(decoded), decoded);

        // Should fail — wrong key means invalid signature
        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, TamperedPayload_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"role\":\"user\"}";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        // Tamper: find the payload section and flip a character
        size_t dot1 = token.find('.');
        size_t dot2 = token.find('.', dot1 + 1);
        ASSERT_NE(dot1, string::npos);
        ASSERT_NE(dot2, string::npos);

        // Flip a character in the payload section
        size_t tamperPos = dot1 + 1;
        if (token[tamperPos] == 'A') {
            token[tamperPos] = 'B';
        } else {
            token[tamperPos] = 'A';
        }

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);

        // Should fail — signature mismatch
        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, TamperedSignature_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"ok\":true}";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        // Tamper a character in the middle of the signature
        // (Avoid last char: its low bits are Base64 padding and may not
        //  affect the decoded 32-byte HMAC.)
        size_t lastDot = token.find_last_of('.');
        ASSERT_NE(lastDot, string::npos);
        size_t tamperPos = lastDot + 1 + 5; // 5 chars into signature
        ASSERT_LT(tamperPos, token.length());
        if (token[tamperPos] == 'A') {
            token[tamperPos] = 'Z';
        } else {
            token[tamperPos] = 'A';
        }

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);

        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, EmptyString_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(string(), sizeof(decoded), decoded);

        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, NoDots_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode("nodotsatall", sizeof(decoded), decoded);

        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, OneDot_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode("header.payloadonly", sizeof(decoded), decoded);

        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, GarbageToken_Fails)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode("abc.def.ghi", sizeof(decoded), decoded);

        EXPECT_EQ(decodedLen, static_cast<uint16_t>(~0));
    }

    TEST(JWT_Decode, EmptyPayload_RoundTrip)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        string token;
        jwt.Encode(token, 0, reinterpret_cast<const uint8_t*>(""));

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);

        // Empty payload decode should succeed with length 0
        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(decodedLen, 0u);
    }

    // =====================================================================
    // Test Suite: JWT_PayloadLength
    // Covers: JSONWebToken::PayloadLength — length estimation
    // =====================================================================

    TEST(JWT_PayloadLength, MatchesActualPayload)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        const string payload = "{\"data\":\"hello world\"}";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        uint16_t estimatedLen = jwt.PayloadLength(token);
        EXPECT_NE(estimatedLen, static_cast<uint16_t>(~0));
        // Estimated length should be >= actual payload length (Base64 padding can overshoot)
        EXPECT_GE(estimatedLen, payload.length());
    }

    TEST(JWT_PayloadLength, InvalidToken_ReturnsError)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        uint16_t len = jwt.PayloadLength("no-dots-here");
        EXPECT_EQ(len, static_cast<uint16_t>(~0));
    }

    // =====================================================================
    // Test Suite: JWT_KeyVariations
    // Covers: Different key sizes and edge cases
    // =====================================================================

    TEST(JWT_KeyVariations, ShortKey)
    {
        const uint8_t shortKey[4] = { 0x01, 0x02, 0x03, 0x04 };
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(shortKey), shortKey);

        const string payload = "{\"test\":true}";
        string token;
        uint16_t len = jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));
        EXPECT_GT(len, 0u);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);
        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(string(reinterpret_cast<const char*>(decoded), decodedLen), payload);
    }

    TEST(JWT_KeyVariations, LongKey)
    {
        // Key longer than HMAC block size (64 bytes) — gets auto-hashed
        uint8_t longKey[128];
        memset(longKey, 0xAA, sizeof(longKey));
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(longKey), longKey);

        const string payload = "{\"test\":true}";
        string token;
        uint16_t len = jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));
        EXPECT_GT(len, 0u);

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);
        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(string(reinterpret_cast<const char*>(decoded), decodedLen), payload);
    }

    TEST(JWT_KeyVariations, SingleByteKey)
    {
        const uint8_t singleKey[1] = { 0x42 };
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, 1, singleKey);

        const string payload = "minimal";
        string token;
        jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));

        uint8_t decoded[512] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);
        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(string(reinterpret_cast<const char*>(decoded), decodedLen), payload);
    }

    // =====================================================================
    // Test Suite: JWT_LargePayload
    // Covers: Larger payloads to test Base64 encoding edge cases
    // =====================================================================

    TEST(JWT_LargePayload, OneKBPayload)
    {
        Web::JSONWebToken jwt(Web::JSONWebToken::SHA256, sizeof(TestKey), TestKey);

        string payload(1024, 'X');
        string token;
        uint16_t len = jwt.Encode(token, static_cast<uint16_t>(payload.length()),
            reinterpret_cast<const uint8_t*>(payload.c_str()));
        EXPECT_GT(len, 0u);

        uint8_t decoded[2048] = {};
        uint16_t decodedLen = jwt.Decode(token, sizeof(decoded), decoded);
        ASSERT_NE(decodedLen, static_cast<uint16_t>(~0));
        EXPECT_EQ(string(reinterpret_cast<const char*>(decoded), decodedLen), payload);
    }

} // namespace Tests
} // namespace Thunder

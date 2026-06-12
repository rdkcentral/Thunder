/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>
#include <websocket/websocket.h>

namespace Thunder {
namespace Tests {
namespace Core {

    // =========================================================================
    // URL Parsing — Scheme Detection
    // =========================================================================

    TEST(URL_Parse, HTTP_FullURL)
    {
        ::Thunder::Core::URL url("http://example.com:8080/path/to/resource?q=1&r=2#section");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_HTTP);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "example.com");
        EXPECT_TRUE(url.Port().IsSet());
        EXPECT_EQ(url.Port().Value(), 8080);
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "path/to/resource");
        EXPECT_TRUE(url.Query().IsSet());
        EXPECT_STREQ(url.Query().Value().c_str(), "q=1&r=2");
        EXPECT_TRUE(url.Ref().IsSet());
        EXPECT_STREQ(url.Ref().Value().c_str(), "section");
    }

    TEST(URL_Parse, HTTPS_DefaultPort)
    {
        ::Thunder::Core::URL url("https://secure.example.com/api/v1");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_HTTPS);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "secure.example.com");
        EXPECT_FALSE(url.Port().IsSet());
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "api/v1");
    }

    TEST(URL_Parse, WebSocket_WS)
    {
        ::Thunder::Core::URL url("ws://localhost:9998/jsonrpc");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_WS);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "localhost");
        EXPECT_TRUE(url.Port().IsSet());
        EXPECT_EQ(url.Port().Value(), 9998);
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "jsonrpc");
    }

    TEST(URL_Parse, WebSocket_WSS)
    {
        ::Thunder::Core::URL url("wss://secure.host:443/ws");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_WSS);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "secure.host");
        EXPECT_TRUE(url.Port().IsSet());
        EXPECT_EQ(url.Port().Value(), 443);
    }

    TEST(URL_Parse, FTP)
    {
        ::Thunder::Core::URL url("ftp://files.example.com/pub/data.tar.gz");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_FTP);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "files.example.com");
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "pub/data.tar.gz");
    }

    TEST(URL_Parse, File)
    {
        ::Thunder::Core::URL url("file:///tmp/local/file.txt");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_FILE);
    }

    TEST(URL_Parse, NTP)
    {
        ::Thunder::Core::URL url("ntp://pool.ntp.org");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_NTP);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "pool.ntp.org");
    }

    // =========================================================================
    // Default Port Mapping
    // =========================================================================

    TEST(URL_Port, DefaultPortHTTP)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_HTTP), 80);
    }

    TEST(URL_Port, DefaultPortHTTPS)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_HTTPS), 443);
    }

    TEST(URL_Port, DefaultPortFTP)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_FTP), 21);
    }

    TEST(URL_Port, DefaultPortNTP)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_NTP), 123);
    }

    TEST(URL_Port, DefaultPortWS)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_WS), 80);
    }

    TEST(URL_Port, DefaultPortWSS)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_WSS), 443);
    }

    TEST(URL_Port, DefaultPortFile)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_FILE), 0);
    }

    TEST(URL_Port, DefaultPortUnknown)
    {
        EXPECT_EQ(::Thunder::Core::URL::Port(::Thunder::Core::URL::SCHEME_UNKNOWN), 0);
    }

    // =========================================================================
    // UserInfo Parsing
    // =========================================================================

    TEST(URL_Parse, UserInfoWithPassword)
    {
        ::Thunder::Core::URL url("http://user:pass@host.com/path");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.UserName().IsSet());
        EXPECT_STREQ(url.UserName().Value().c_str(), "user");
        EXPECT_TRUE(url.Password().IsSet());
        EXPECT_STREQ(url.Password().Value().c_str(), "pass");
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "host.com");
    }

    TEST(URL_Parse, UserInfoWithoutPassword)
    {
        // NOTE: Thunder's URL parser only extracts UserName when there's a ':'
        // separator (user:pass@host). For user@host without password, neither
        // username nor password is set — the "anonymous@" portion becomes part
        // of the host parsing. Verify this actual behavior.
        ::Thunder::Core::URL url("ftp://anonymous@ftp.example.com/pub");

        EXPECT_TRUE(url.IsValid());
        // UserName is NOT set because there's no ':' before '@'
        EXPECT_FALSE(url.UserName().IsSet());
        EXPECT_TRUE(url.Host().IsSet());
    }

    // =========================================================================
    // Edge Cases
    // =========================================================================

    TEST(URL_Parse, HostOnly)
    {
        ::Thunder::Core::URL url("http://example.com");

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_HTTP);
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "example.com");
        EXPECT_FALSE(url.Port().IsSet());
        EXPECT_FALSE(url.Query().IsSet());
        EXPECT_FALSE(url.Ref().IsSet());
    }

    TEST(URL_Parse, HostWithTrailingSlash)
    {
        ::Thunder::Core::URL url("http://example.com/");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "example.com");
    }

    TEST(URL_Parse, QueryWithoutPath)
    {
        ::Thunder::Core::URL url("http://example.com?key=value");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_TRUE(url.Query().IsSet());
        EXPECT_STREQ(url.Query().Value().c_str(), "key=value");
    }

    TEST(URL_Parse, FragmentOnly)
    {
        ::Thunder::Core::URL url("http://example.com#top");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_TRUE(url.Ref().IsSet());
        EXPECT_STREQ(url.Ref().Value().c_str(), "top");
    }

    TEST(URL_Parse, EmptyString)
    {
        ::Thunder::Core::URL url("");

        EXPECT_FALSE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_UNKNOWN);
    }

    TEST(URL_Parse, NoScheme)
    {
        ::Thunder::Core::URL url("example.com/path");

        // Without "scheme://" the parse should not produce a valid URL
        EXPECT_FALSE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_UNKNOWN);
    }

    TEST(URL_Parse, NumericPort)
    {
        ::Thunder::Core::URL url("http://192.168.1.1:3000/api");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "192.168.1.1");
        EXPECT_TRUE(url.Port().IsSet());
        EXPECT_EQ(url.Port().Value(), 3000);
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "api");
    }

    TEST(URL_Parse, MultipleQueryParams)
    {
        ::Thunder::Core::URL url("http://host/search?a=1&b=2&c=3");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Query().IsSet());
        EXPECT_STREQ(url.Query().Value().c_str(), "a=1&b=2&c=3");
    }

    TEST(URL_Parse, QueryAndFragment)
    {
        ::Thunder::Core::URL url("http://host/path?q=1#frag");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Query().IsSet());
        EXPECT_STREQ(url.Query().Value().c_str(), "q=1");
        EXPECT_TRUE(url.Ref().IsSet());
        EXPECT_STREQ(url.Ref().Value().c_str(), "frag");
    }

    TEST(URL_Parse, DeepPath)
    {
        ::Thunder::Core::URL url("http://host/a/b/c/d/e");

        EXPECT_TRUE(url.IsValid());
        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "a/b/c/d/e");
    }

    // =========================================================================
    // Text() Serialization / Round-trip
    // =========================================================================

    TEST(URL_Text, HTTPRoundTrip)
    {
        const string original = "http://example.com:8080/path?q=1#ref";
        ::Thunder::Core::URL url(original);

        string serialized = url.Text();
        EXPECT_FALSE(serialized.empty());
        EXPECT_TRUE(url.IsValid());

        // Parse back and verify
        ::Thunder::Core::URL reparsed(serialized);
        EXPECT_EQ(reparsed.Type(), url.Type());
        EXPECT_EQ(reparsed.Host().Value(), url.Host().Value());
        EXPECT_EQ(reparsed.Port().Value(), url.Port().Value());
        EXPECT_EQ(reparsed.Path().Value(), url.Path().Value());
        EXPECT_EQ(reparsed.Query().Value(), url.Query().Value());
        EXPECT_EQ(reparsed.Ref().Value(), url.Ref().Value());
    }

    TEST(URL_Text, UserInfoRoundTrip)
    {
        ::Thunder::Core::URL url("http://user:pass@host.com:80/path");
        string serialized = url.Text();

        ::Thunder::Core::URL reparsed(serialized);
        EXPECT_EQ(reparsed.UserName().Value(), "user");
        EXPECT_EQ(reparsed.Password().Value(), "pass");
        EXPECT_EQ(reparsed.Host().Value(), "host.com");
    }

    TEST(URL_Text, MinimalURL)
    {
        ::Thunder::Core::URL url("http://h");
        EXPECT_TRUE(url.IsValid());
        string serialized = url.Text();
        EXPECT_FALSE(serialized.empty());
    }

    // =========================================================================
    // Setters
    // =========================================================================

    TEST(URL_Setters, SetHost)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTP);
        url.Host(::Thunder::Core::OptionalType<string>("newhost.com"));

        EXPECT_TRUE(url.Host().IsSet());
        EXPECT_STREQ(url.Host().Value().c_str(), "newhost.com");
    }

    TEST(URL_Setters, SetPort)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTP);
        url.Host(::Thunder::Core::OptionalType<string>("host.com"));
        url.Port(::Thunder::Core::OptionalType<unsigned short>(9090));

        EXPECT_TRUE(url.Port().IsSet());
        EXPECT_EQ(url.Port().Value(), 9090);
    }

    TEST(URL_Setters, SetPath)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTP);
        url.Host(::Thunder::Core::OptionalType<string>("host.com"));
        url.Path(::Thunder::Core::OptionalType<string>("api/v2/data"));

        EXPECT_TRUE(url.Path().IsSet());
        EXPECT_STREQ(url.Path().Value().c_str(), "api/v2/data");
    }

    TEST(URL_Setters, SetQuery)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTP);
        url.Host(::Thunder::Core::OptionalType<string>("host.com"));
        url.Query(::Thunder::Core::OptionalType<string>("key=value"));

        EXPECT_TRUE(url.Query().IsSet());
        EXPECT_STREQ(url.Query().Value().c_str(), "key=value");
    }

    TEST(URL_Setters, SetRef)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTP);
        url.Host(::Thunder::Core::OptionalType<string>("host.com"));
        url.Ref(::Thunder::Core::OptionalType<string>("section"));

        EXPECT_TRUE(url.Ref().IsSet());
        EXPECT_STREQ(url.Ref().Value().c_str(), "section");
    }

    TEST(URL_Setters, SetUserNameAndPassword)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_FTP);
        url.UserName(::Thunder::Core::OptionalType<string>("admin"));
        url.Password(::Thunder::Core::OptionalType<string>("secret"));

        EXPECT_STREQ(url.UserName().Value().c_str(), "admin");
        EXPECT_STREQ(url.Password().Value().c_str(), "secret");
    }

    // =========================================================================
    // Copy / Move
    // =========================================================================

    TEST(URL_CopyMove, CopyConstructor)
    {
        ::Thunder::Core::URL original("http://host:80/path?q=1#ref");
        ::Thunder::Core::URL copy(original);

        EXPECT_EQ(copy.Type(), original.Type());
        EXPECT_EQ(copy.Host().Value(), original.Host().Value());
        EXPECT_EQ(copy.Port().Value(), original.Port().Value());
        EXPECT_EQ(copy.Path().Value(), original.Path().Value());
        EXPECT_EQ(copy.Query().Value(), original.Query().Value());
        EXPECT_EQ(copy.Ref().Value(), original.Ref().Value());
    }

    TEST(URL_CopyMove, CopyAssignment)
    {
        ::Thunder::Core::URL original("https://host/path");
        ::Thunder::Core::URL copy;
        copy = original;

        EXPECT_EQ(copy.Type(), original.Type());
        EXPECT_EQ(copy.Host().Value(), original.Host().Value());
    }

    TEST(URL_CopyMove, MoveConstructor)
    {
        ::Thunder::Core::URL original("ws://host:9998/ws");
        string origHost = original.Host().Value();

        ::Thunder::Core::URL moved(std::move(original));

        EXPECT_TRUE(moved.IsValid());
        EXPECT_EQ(moved.Type(), ::Thunder::Core::URL::SCHEME_WS);
        EXPECT_EQ(moved.Host().Value(), origHost);
    }

    TEST(URL_CopyMove, MoveAssignment)
    {
        ::Thunder::Core::URL original("ftp://host/file");
        string origHost = original.Host().Value();

        ::Thunder::Core::URL moved;
        moved = std::move(original);

        EXPECT_TRUE(moved.IsValid());
        EXPECT_EQ(moved.Type(), ::Thunder::Core::URL::SCHEME_FTP);
        EXPECT_EQ(moved.Host().Value(), origHost);
    }

    // =========================================================================
    // Clear
    // =========================================================================

    TEST(URL_Clear, ClearResetsAllFields)
    {
        ::Thunder::Core::URL url("http://user:pass@host:80/path?q=1#ref");
        ASSERT_TRUE(url.IsValid());

        url.Clear();

        EXPECT_FALSE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_UNKNOWN);
        EXPECT_FALSE(url.Host().IsSet());
        EXPECT_FALSE(url.Port().IsSet());
        EXPECT_FALSE(url.Path().IsSet());
        EXPECT_FALSE(url.Query().IsSet());
        EXPECT_FALSE(url.Ref().IsSet());
        EXPECT_FALSE(url.UserName().IsSet());
        EXPECT_FALSE(url.Password().IsSet());
    }

    // =========================================================================
    // IsDomain
    // =========================================================================

    TEST(URL_IsDomain, MatchesDomain)
    {
        ::Thunder::Core::URL url("http://api.example.com/resource");

        EXPECT_TRUE(url.IsDomain(_T("example.com"), 11));
        EXPECT_TRUE(url.IsDomain(_T("api.example.com"), 15));
    }

    TEST(URL_IsDomain, DoesNotMatchDifferentDomain)
    {
        ::Thunder::Core::URL url("http://example.com/resource");

        EXPECT_FALSE(url.IsDomain(_T("other.com"), 9));
    }

    TEST(URL_IsDomain, PartialDomainNoMatch)
    {
        ::Thunder::Core::URL url("http://notexample.com/resource");

        // "example.com" should NOT match "notexample.com" — the code checks
        // for a dot or exact-length boundary before the suffix.
        EXPECT_FALSE(url.IsDomain(_T("example.com"), 11));
    }

    // =========================================================================
    // Default Constructor
    // =========================================================================

    TEST(URL_Default, DefaultIsInvalid)
    {
        ::Thunder::Core::URL url;

        EXPECT_FALSE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_UNKNOWN);
        EXPECT_FALSE(url.Host().IsSet());
        EXPECT_FALSE(url.Port().IsSet());
        EXPECT_FALSE(url.Path().IsSet());
    }

    TEST(URL_Default, SchemeConstructor)
    {
        ::Thunder::Core::URL url(::Thunder::Core::URL::SCHEME_HTTPS);

        EXPECT_TRUE(url.IsValid());
        EXPECT_EQ(url.Type(), ::Thunder::Core::URL::SCHEME_HTTPS);
        EXPECT_FALSE(url.Host().IsSet());
    }

    // =========================================================================
    // URL Encoding / Decoding
    // =========================================================================

    TEST(URL_Encode, SpaceEncodedAsPlus)
    {
        const TCHAR source[] = "hello world";
        TCHAR dest[64] = {};

        uint16_t len = ::Thunder::Core::URL::Encode(source, static_cast<uint16_t>(strlen(source)), dest, sizeof(dest));
        EXPECT_GT(len, 0u);

        string encoded(dest, len);
        EXPECT_NE(encoded.find('+'), string::npos);
    }

    TEST(URL_Encode, AlphanumericPassThrough)
    {
        const TCHAR source[] = "abc123XYZ";
        TCHAR dest[64] = {};

        uint16_t len = ::Thunder::Core::URL::Encode(source, static_cast<uint16_t>(strlen(source)), dest, sizeof(dest));
        string encoded(dest, len);

        EXPECT_STREQ(encoded.c_str(), "abc123XYZ");
    }

    TEST(URL_Encode, SpecialCharsEncoded)
    {
        const TCHAR source[] = "a=b&c";
        TCHAR dest[64] = {};

        uint16_t len = ::Thunder::Core::URL::Encode(source, static_cast<uint16_t>(strlen(source)), dest, sizeof(dest));
        string encoded(dest, len);

        // '=' and '&' should be percent-encoded
        EXPECT_NE(encoded.find('%'), string::npos);
        EXPECT_EQ(encoded.find('='), string::npos);
        EXPECT_EQ(encoded.find('&'), string::npos);
    }

    TEST(URL_Decode, DecodePercentEncoded)
    {
        const TCHAR source[] = "hello%20world";
        TCHAR dest[64] = {};

        uint16_t len = ::Thunder::Core::URL::Decode(source, static_cast<uint16_t>(strlen(source)), dest, sizeof(dest));
        string decoded(dest, len);

        EXPECT_STREQ(decoded.c_str(), "hello world");
    }

    TEST(URL_Decode, DecodePlusAsSpace)
    {
        const TCHAR source[] = "hello+world";
        TCHAR dest[64] = {};

        uint16_t len = ::Thunder::Core::URL::Decode(source, static_cast<uint16_t>(strlen(source)), dest, sizeof(dest));
        string decoded(dest, len);

        EXPECT_STREQ(decoded.c_str(), "hello world");
    }

    TEST(URL_Encode, RoundTrip)
    {
        const TCHAR source[] = "key=hello world&other=a/b";
        TCHAR encoded[128] = {};
        TCHAR decoded[128] = {};

        uint16_t encLen = ::Thunder::Core::URL::Encode(source, static_cast<uint16_t>(strlen(source)), encoded, sizeof(encoded));
        ASSERT_GT(encLen, 0u);

        uint16_t decLen = ::Thunder::Core::URL::Decode(encoded, encLen, decoded, sizeof(decoded));
        string result(decoded, decLen);

        EXPECT_STREQ(result.c_str(), source);
    }

    // =========================================================================
    // URL-safe Base64 Encoding / Decoding
    // =========================================================================

    TEST(URL_Base64, EncodeDecodeRoundTrip)
    {
        const uint8_t data[] = { 0x00, 0x01, 0x02, 0xFF, 0xFE, 0x80, 0x7F };
        TCHAR encoded[64] = {};
        uint8_t decoded[64] = {};

        uint16_t encLen = ::Thunder::Core::URL::Base64Encode(data, sizeof(data), encoded, sizeof(encoded));
        ASSERT_GT(encLen, 0u);

        uint16_t decLen = ::Thunder::Core::URL::Base64Decode(encoded, encLen, decoded, sizeof(decoded));
        ASSERT_EQ(decLen, sizeof(data));

        EXPECT_EQ(memcmp(data, decoded, sizeof(data)), 0);
    }

    TEST(URL_Base64, URLSafeCharsUsed)
    {
        // Use data that would produce + and / in standard Base64
        const uint8_t data[] = { 0xFB, 0xFF, 0xFE };
        TCHAR encoded[64] = {};

        uint16_t encLen = ::Thunder::Core::URL::Base64Encode(data, sizeof(data), encoded, sizeof(encoded));
        string encodedStr(encoded, encLen);

        // URL-safe base64 uses '-' and '_' instead of '+' and '/'
        EXPECT_EQ(encodedStr.find('+'), string::npos);
        EXPECT_EQ(encodedStr.find('/'), string::npos);
    }

    TEST(URL_Base64, EmptyInput)
    {
        TCHAR encoded[64] = {};
        uint16_t encLen = ::Thunder::Core::URL::Base64Encode(nullptr, 0, encoded, sizeof(encoded));
        EXPECT_EQ(encLen, 0u);
    }

    TEST(URL_Base64, WithPadding)
    {
        const uint8_t data[] = { 0x41 }; // 1 byte -> 2 base64 chars + padding
        TCHAR encoded[64] = {};
        uint8_t decoded[64] = {};

        uint16_t encLen = ::Thunder::Core::URL::Base64Encode(data, sizeof(data), encoded, sizeof(encoded), true);
        ASSERT_GT(encLen, 0u);

        uint16_t decLen = ::Thunder::Core::URL::Base64Decode(encoded, encLen, decoded, sizeof(decoded));
        ASSERT_EQ(decLen, sizeof(data));
        EXPECT_EQ(decoded[0], 0x41);
    }

    TEST(URL_Base64, LargerPayload)
    {
        // 32 bytes (like a SHA256 hash)
        uint8_t data[32];
        for (int i = 0; i < 32; i++) {
            data[i] = static_cast<uint8_t>(i * 7 + 3);
        }

        TCHAR encoded[128] = {};
        uint8_t decoded[64] = {};

        uint16_t encLen = ::Thunder::Core::URL::Base64Encode(data, sizeof(data), encoded, sizeof(encoded));
        ASSERT_GT(encLen, 0u);

        uint16_t decLen = ::Thunder::Core::URL::Base64Decode(encoded, encLen, decoded, sizeof(decoded));
        ASSERT_EQ(decLen, 32u);
        EXPECT_EQ(memcmp(data, decoded, 32), 0);
    }

    // =========================================================================
    // KeyValue Query Parameter Parsing
    // =========================================================================

    TEST(URL_KeyValue, SingleKeyValue)
    {
        ::Thunder::Core::URL::KeyValue kv("key=value");

        EXPECT_EQ(kv.HasKey(_T("key")), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
        EXPECT_STREQ(kv.Value(_T("key")).Text().c_str(), "value");
    }

    TEST(URL_KeyValue, MultipleKeyValues)
    {
        ::Thunder::Core::URL::KeyValue kv("a=1&b=2&c=3");

        EXPECT_EQ(kv.HasKey(_T("a")), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
        EXPECT_EQ(kv.HasKey(_T("b")), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
        EXPECT_EQ(kv.HasKey(_T("c")), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
        EXPECT_STREQ(kv.Value(_T("a")).Text().c_str(), "1");
        EXPECT_STREQ(kv.Value(_T("b")).Text().c_str(), "2");
        EXPECT_STREQ(kv.Value(_T("c")).Text().c_str(), "3");
    }

    TEST(URL_KeyValue, KeyOnly)
    {
        ::Thunder::Core::URL::KeyValue kv("flag&key=value");

        EXPECT_EQ(kv.HasKey(_T("flag")), ::Thunder::Core::URL::KeyValue::status::KEY_ONLY);
        EXPECT_EQ(kv.HasKey(_T("key")), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
    }

    TEST(URL_KeyValue, MissingKey)
    {
        ::Thunder::Core::URL::KeyValue kv("a=1");

        EXPECT_EQ(kv.HasKey(_T("b")), ::Thunder::Core::URL::KeyValue::status::UNAVAILABLE);
    }

    TEST(URL_KeyValue, NumberExtraction)
    {
        ::Thunder::Core::URL::KeyValue kv("count=42&name=test");

        uint32_t val = kv.Number<uint32_t>(_T("count"));
        EXPECT_EQ(val, 42u);
    }

    TEST(URL_KeyValue, BooleanTrue)
    {
        ::Thunder::Core::URL::KeyValue kv("enabled=true&flag=1&yes=T");

        EXPECT_TRUE(kv.Boolean(_T("enabled"), false));
        EXPECT_TRUE(kv.Boolean(_T("flag"), false));
        EXPECT_TRUE(kv.Boolean(_T("yes"), false));
    }

    TEST(URL_KeyValue, BooleanFalse)
    {
        ::Thunder::Core::URL::KeyValue kv("disabled=false&flag=0&no=F");

        EXPECT_FALSE(kv.Boolean(_T("disabled"), true));
        EXPECT_FALSE(kv.Boolean(_T("flag"), true));
        EXPECT_FALSE(kv.Boolean(_T("no"), true));
    }

    TEST(URL_KeyValue, BooleanDefault)
    {
        ::Thunder::Core::URL::KeyValue kv("other=xyz");

        EXPECT_TRUE(kv.Boolean(_T("missing"), true));
        EXPECT_FALSE(kv.Boolean(_T("missing"), false));
    }

    TEST(URL_KeyValue, CaseInsensitive)
    {
        ::Thunder::Core::URL::KeyValue kv("MyKey=hello");

        EXPECT_EQ(kv.HasKey(_T("mykey"), false), ::Thunder::Core::URL::KeyValue::status::KEY_VALUE);
        EXPECT_STREQ(kv.Value(_T("mykey"), false).Text().c_str(), "hello");
    }

    TEST(URL_KeyValue, CaseSensitiveNoMatch)
    {
        ::Thunder::Core::URL::KeyValue kv("MyKey=hello");

        EXPECT_EQ(kv.HasKey(_T("mykey"), true), ::Thunder::Core::URL::KeyValue::status::UNAVAILABLE);
    }

    TEST(URL_KeyValue, OperatorBracket)
    {
        ::Thunder::Core::URL::KeyValue kv("x=42");

        EXPECT_STREQ(kv[_T("x")].Text().c_str(), "42");
    }

    TEST(URL_KeyValue, CopyAndAssign)
    {
        ::Thunder::Core::URL::KeyValue original("a=1&b=2");
        ::Thunder::Core::URL::KeyValue copy(original);

        EXPECT_STREQ(copy.Value(_T("a")).Text().c_str(), "1");
        EXPECT_STREQ(copy.Value(_T("b")).Text().c_str(), "2");

        ::Thunder::Core::URL::KeyValue assigned("x=0");
        assigned = original;
        EXPECT_STREQ(assigned.Value(_T("a")).Text().c_str(), "1");
    }

    // =========================================================================
    // Integration: Parse URL then use KeyValue on query
    // =========================================================================

    TEST(URL_Integration, ParseThenQueryKeyValue)
    {
        ::Thunder::Core::URL url("http://host/search?page=3&lang=en&debug");

        ASSERT_TRUE(url.IsValid());
        ASSERT_TRUE(url.Query().IsSet());

        ::Thunder::Core::URL::KeyValue kv(url.Query().Value());

        EXPECT_EQ(kv.Number<uint32_t>(_T("page")), 3u);
        EXPECT_STREQ(kv.Value(_T("lang")).Text().c_str(), "en");
        EXPECT_EQ(kv.HasKey(_T("debug")), ::Thunder::Core::URL::KeyValue::status::KEY_ONLY);
    }

} // Core
} // Tests
} // Thunder

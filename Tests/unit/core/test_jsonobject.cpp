/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifdef __APPLE__
#include <time.h>
#endif

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace WPEFramework {
namespace Tests {
namespace Core {

    TEST(JSONOBJECT, FirstKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("name"));

        EXPECT_TRUE(container.HasLabel("Name"));

        /* void */ container.Remove("Name");

        EXPECT_FALSE(container.HasLabel("Name"));
    }

    TEST(JSONOBJECT, KeyRemoveWithPartialMatch)
    {
        const std::string keyValues = R"({"Service": "Wifi", "Wifi_ssid": "ssid-name", "Wifi_key": 0, "status": "Connected"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("Wifi"));

        EXPECT_TRUE(container.HasLabel("Wifi_ssid"));

        /* void */ container.Remove("Wifi");

        EXPECT_TRUE(container.HasLabel("Wifi_ssid"));

    }


    TEST(JSONOBJECT, SecondKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("age"));

        EXPECT_TRUE(container.HasLabel("Age"));

        /* void */ container.Remove("Age");

        EXPECT_FALSE(container.HasLabel("Age"));
    }

    TEST(JSONOBJECT, ThirdKeyRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": 0, "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("gender"));

        EXPECT_TRUE(container.HasLabel("Gender"));

        /* void */ container.Remove("Gender");

        EXPECT_FALSE(container.HasLabel("Gender"));
    }

    TEST(JSONOBJECT, IdenticalKeyValueRemove)
    {
        const std::string keyValues = R"({"Name": "Your full name", "Age": "Age", "Gender": "Undisclosed"})";

        JsonObject container;

        EXPECT_TRUE(container.FromString(keyValues));

        EXPECT_FALSE(container.HasLabel("age"));

        EXPECT_TRUE(container.HasLabel("Age"));

        /* void */ container.Remove("Age");

        EXPECT_FALSE(container.HasLabel("Age"));
    }

    TEST(JSONOBJECT, KeyValueAccess)
    {
        JsonObject container;

        ::WPEFramework::Core::JSON::Variant intType{ 123 };
        ::WPEFramework::Core::JSON::Variant floatType{ 123.456f };
        ::WPEFramework::Core::JSON::Variant doubleType{ 123.456 }; // defaults to double with omitted suffix
        ::WPEFramework::Core::JSON::Variant boolType{ true };
        ::WPEFramework::Core::JSON::Variant stringType{ "scribble" };

        // An empty JsonObject is valid
        EXPECT_TRUE(container.IsValid());

        /* void */ container.Set("integer", intType);
        /* void */ container.Set("float", floatType);
        /* void */ container.Set("double", doubleType);
        /* void */ container.Set("boolean", boolType);
        /* void */ container.Set("string", stringType);

        // All contained elements are valid
        EXPECT_TRUE(container.IsValid());

        ::WPEFramework::Core::JSON::Variant intFound     = container.Get("integer");
        // Create and add if it does not exist otherwise return the existing element
        ::WPEFramework::Core::JSON::Variant& intRefFound = container["integer"];

        EXPECT_TRUE(intFound.Content() == ::WPEFramework::Core::JSON::Variant::type::NUMBER);
        EXPECT_EQ(intFound.Number(), 123);
        EXPECT_TRUE(intFound == intRefFound);

        ::WPEFramework::Core::JSON::Variant floatFound = container.Get("float");
        ::WPEFramework::Core::JSON::Variant& floatRefFound = container["float"];
        EXPECT_TRUE(floatFound.Content() == ::WPEFramework::Core::JSON::Variant::type::FLOAT);
        EXPECT_FLOAT_EQ(floatFound.Float(), 123.456);
        EXPECT_TRUE(floatFound == floatRefFound);

        ::WPEFramework::Core::JSON::Variant doubleFound = container.Get("double");
        ::WPEFramework::Core::JSON::Variant& doubleRefFound = container["double"];
        EXPECT_TRUE(doubleFound.Content() == ::WPEFramework::Core::JSON::Variant::type::DOUBLE);
        EXPECT_FLOAT_EQ(doubleFound.Float(), 123.456);
        EXPECT_TRUE(doubleFound == doubleRefFound);

        ::WPEFramework::Core::JSON::Variant boolFound = container.Get("boolean");
        ::WPEFramework::Core::JSON::Variant& boolRefFound = container["boolean"];
        EXPECT_TRUE(boolFound.Content() == ::WPEFramework::Core::JSON::Variant::type::BOOLEAN);
        EXPECT_EQ(boolFound.Boolean(), true);
        EXPECT_TRUE(boolFound == boolRefFound);

        ::WPEFramework::Core::JSON::Variant stringFound = container.Get("string");
        ::WPEFramework::Core::JSON::Variant& stringRefFound = container["string"];
        EXPECT_TRUE(stringFound.Content() == ::WPEFramework::Core::JSON::Variant::type::STRING);
        EXPECT_STREQ(stringFound.String().c_str(), "scribble");
        EXPECT_TRUE(stringFound == stringRefFound);

        container.Clear();

        // The container should have nothing to index
        ::WPEFramework::Core::JSON::VariantContainer::Iterator it = container.Variants();
        EXPECT_FALSE(it.IsValid());

        // It does not exist so a default constructed variant is returned
        // Create and add a default variant with the label 'integer'
        ::WPEFramework::Core::JSON::Variant& intRefCreated = container["integer"];
        EXPECT_TRUE(container.HasLabel("integer"));
        EXPECT_TRUE(intRefCreated.Content() == ::WPEFramework::Core::JSON::Variant::type::EMPTY);

        ::WPEFramework::Core::JSON::Variant& floatRefCreated = container["float"];
        EXPECT_TRUE(container.HasLabel("float"));
        EXPECT_TRUE(floatRefCreated.Content() == ::WPEFramework::Core::JSON::Variant::type::EMPTY);

        ::WPEFramework::Core::JSON::Variant& doubleRefCreated = container["double"];
        EXPECT_TRUE(container.HasLabel("double"));
        EXPECT_TRUE(doubleRefCreated.Content() == ::WPEFramework::Core::JSON::Variant::type::EMPTY);

        ::WPEFramework::Core::JSON::Variant& boolRefCreated = container["boolean"];
        EXPECT_TRUE(container.HasLabel("boolean"));
        EXPECT_TRUE(boolRefCreated.Content() == ::WPEFramework::Core::JSON::Variant::type::EMPTY);

        ::WPEFramework::Core::JSON::Variant& stringRefCreated = container["string"];
        EXPECT_TRUE(container.HasLabel("string"));
        EXPECT_TRUE(stringRefCreated.Content() == ::WPEFramework::Core::JSON::Variant::type::EMPTY);
    }

    TEST(JSONOBJECT, ParsedStringVariantKeepsStringSemantics)
    {
        const std::string keyValues = R"({"string":"scribble"})";

        JsonObject container;

        ASSERT_TRUE(container.FromString(keyValues));
        ASSERT_TRUE(container.HasLabel("string"));

        const ::Thunder::Core::JSON::Variant parsed = container.Get("string");
        const ::Thunder::Core::JSON::Variant constructed{ "scribble" };

        EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(parsed.String(), std::string("scribble"));
        EXPECT_TRUE(parsed == constructed);

        std::string serialized;
        EXPECT_TRUE(::Thunder::Core::JSON::IElement::ToString(parsed, serialized));
        EXPECT_EQ(serialized, std::string("\"scribble\""));
    }

    TEST(JSONOBJECT, ParsedStringVariantRoundTrip)
    {
        // Verify that a string variant parsed from JSON and then re-serialized produces
        // valid JSON with surrounding quotes (round-trip correctness).
        const std::string original = R"({"key":"hello world"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(original));

        std::string roundTripped;
        ASSERT_TRUE(container.ToString(roundTripped));

        // The re-serialized JSON must still contain the key with quoted value.
        EXPECT_NE(roundTripped.find("\"hello world\""), std::string::npos);
    }

    TEST(JSONOBJECT, ParsedStringVariantLooksLikeNumber)
    {
        // A quoted value that looks like a number must remain a STRING type, not NUMBER.
        // SetQuoted(quoted) ensures the type resolves to STRING when the token was quoted.
        const std::string keyValues = R"({"val":"42"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(keyValues));

        const ::Thunder::Core::JSON::Variant parsed = container.Get("val");

        EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(parsed.String(), std::string("42"));

        std::string serialized;
        EXPECT_TRUE(::Thunder::Core::JSON::IElement::ToString(parsed, serialized));
        // Must be serialized with quotes, not as a bare number.
        EXPECT_EQ(serialized, std::string("\"42\""));
    }

    TEST(JSONOBJECT, ParsedStringVariantLooksLikeBoolean)
    {
        // A quoted "true" must remain a STRING, not BOOLEAN.
        const std::string keyValues = R"({"flag":"true"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(keyValues));

        const ::Thunder::Core::JSON::Variant parsed = container.Get("flag");

        EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(parsed.String(), std::string("true"));

        std::string serialized;
        EXPECT_TRUE(::Thunder::Core::JSON::IElement::ToString(parsed, serialized));
        EXPECT_EQ(serialized, std::string("\"true\""));
    }

    TEST(JSONOBJECT, ParsedStringVariantLooksLikeNull)
    {
        // A quoted "null" must remain a STRING, not EMPTY/null.
        const std::string keyValues = R"({"nothing":"null"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(keyValues));

        const ::Thunder::Core::JSON::Variant parsed = container.Get("nothing");

        EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(parsed.String(), std::string("null"));

        std::string serialized;
        EXPECT_TRUE(::Thunder::Core::JSON::IElement::ToString(parsed, serialized));
        EXPECT_EQ(serialized, std::string("\"null\""));
    }

    TEST(JSONOBJECT, MultipleStringVariantsAllPreserveQuotedState)
    {
        // All string fields in a JSON object must retain their string type and
        // serialize with quotes after the fix.
        const std::string keyValues = R"({"a":"alpha","b":"beta","c":"gamma"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(keyValues));

        const std::vector<std::pair<std::string, std::string>> expected = {
            {"a", "alpha"}, {"b", "beta"}, {"c", "gamma"}
        };

        for (const auto& kv : expected) {
            const ::Thunder::Core::JSON::Variant parsed = container.Get(kv.first.c_str());

            EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING)
                << "Field '" << kv.first << "' should be STRING";
            EXPECT_EQ(parsed.String(), kv.second)
                << "Field '" << kv.first << "' has wrong value";

            std::string serialized;
            EXPECT_TRUE(::Thunder::Core::JSON::IElement::ToString(parsed, serialized));
            EXPECT_EQ(serialized, "\"" + kv.second + "\"")
                << "Field '" << kv.first << "' should serialize with quotes";
        }
    }

    TEST(JSONOBJECT, ParsedStringVariantEqualityWithConstructed)
    {
        // A Variant parsed from JSON {"key":"value"} must compare equal to
        // a Variant constructed directly with the same string.
        const std::string keyValues = R"({"key":"Thunder"})";

        JsonObject container;
        ASSERT_TRUE(container.FromString(keyValues));

        const ::Thunder::Core::JSON::Variant parsed    = container.Get("key");
        const ::Thunder::Core::JSON::Variant constructed{ "Thunder" };

        EXPECT_EQ(parsed.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(parsed.Content(), constructed.Content());
        EXPECT_TRUE(parsed == constructed);
    }

    TEST(JSONOBJECT, VariantReuseQuotedThenUnquoted)
    {
        // Regression test: reusing the same Variant for a second FromString() call must
        // not carry over the quoted/serialize flag from the first parse.  Without the fix,
        // parsing an unquoted number after a quoted string left IsQuoted()==true, causing
        // the number to be misclassified as STRING.
        ::Thunder::Core::JSON::Variant variant;

        // First parse: quoted string — variant must be STRING.
        ASSERT_TRUE(variant.FromString("\"hello\""));
        EXPECT_EQ(variant.Content(), ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_EQ(variant.String(), std::string("hello"));

        // Second parse on the same instance: unquoted integer — variant must be NUMBER.
        ASSERT_TRUE(variant.FromString("42"));
        EXPECT_EQ(variant.Content(), ::Thunder::Core::JSON::Variant::type::NUMBER);
        EXPECT_EQ(variant.Number(), static_cast<int64_t>(42));

        // Third parse on the same instance: unquoted boolean — must be BOOLEAN, not STRING.
        ASSERT_TRUE(variant.FromString("true"));
        EXPECT_EQ(variant.Content(), ::Thunder::Core::JSON::Variant::type::BOOLEAN);
    }
} // Core
} // Tests
} // WPEFramework

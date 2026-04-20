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

namespace Thunder {
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

        ::Thunder::Core::JSON::Variant intType{ 123 };
        ::Thunder::Core::JSON::Variant floatType{ 123.456f };
        ::Thunder::Core::JSON::Variant doubleType{ 123.456 }; // defaults to double with omitted suffix
        ::Thunder::Core::JSON::Variant boolType{ true };
        ::Thunder::Core::JSON::Variant stringType{ "scribble" };

        // An empty JsonObject is valid
        EXPECT_TRUE(container.IsValid());

        /* void */ container.Set("integer", intType);
        /* void */ container.Set("float", floatType);
        /* void */ container.Set("double", doubleType);
        /* void */ container.Set("boolean", boolType);
        /* void */ container.Set("string", stringType);

        // All contained elements are valid
        EXPECT_TRUE(container.IsValid());

        ::Thunder::Core::JSON::Variant intFound     = container.Get("integer");
        // Create and add if it does not exist otherwise return the existing element
        ::Thunder::Core::JSON::Variant& intRefFound = container["integer"];

        EXPECT_TRUE(intFound.Content() == ::Thunder::Core::JSON::Variant::type::NUMBER);
        EXPECT_EQ(intFound.Number(), 123);
        EXPECT_TRUE(intFound == intRefFound);

        ::Thunder::Core::JSON::Variant floatFound = container.Get("float");
        ::Thunder::Core::JSON::Variant& floatRefFound = container["float"];
        EXPECT_TRUE(floatFound.Content() == ::Thunder::Core::JSON::Variant::type::FLOAT);
        EXPECT_FLOAT_EQ(floatFound.Float(), 123.456);
        EXPECT_TRUE(floatFound == floatRefFound);

        ::Thunder::Core::JSON::Variant doubleFound = container.Get("double");
        ::Thunder::Core::JSON::Variant& doubleRefFound = container["double"];
        EXPECT_TRUE(doubleFound.Content() == ::Thunder::Core::JSON::Variant::type::DOUBLE);
        EXPECT_FLOAT_EQ(doubleFound.Float(), 123.456);
        EXPECT_TRUE(doubleFound == doubleRefFound);

        ::Thunder::Core::JSON::Variant boolFound = container.Get("boolean");
        ::Thunder::Core::JSON::Variant& boolRefFound = container["boolean"];
        EXPECT_TRUE(boolFound.Content() == ::Thunder::Core::JSON::Variant::type::BOOLEAN);
        EXPECT_EQ(boolFound.Boolean(), true);
        EXPECT_TRUE(boolFound == boolRefFound);

        ::Thunder::Core::JSON::Variant stringFound = container.Get("string");
        ::Thunder::Core::JSON::Variant& stringRefFound = container["string"];
        EXPECT_TRUE(stringFound.Content() == ::Thunder::Core::JSON::Variant::type::STRING);
        EXPECT_STREQ(stringFound.String().c_str(), "scribble");
        EXPECT_TRUE(stringFound == stringRefFound);

        container.Clear();

        // The container should have nothing to index
        ::Thunder::Core::JSON::VariantContainer::Iterator it = container.Variants();
        EXPECT_FALSE(it.IsValid());

        // It does not exist so a default constructed variant is returned
        // Create and add a default variant with the label 'integer'
        ::Thunder::Core::JSON::Variant& intRefCreated = container["integer"];
        EXPECT_TRUE(container.HasLabel("integer"));
        EXPECT_TRUE(intRefCreated.Content() == ::Thunder::Core::JSON::Variant::type::EMPTY);

        ::Thunder::Core::JSON::Variant& floatRefCreated = container["float"];
        EXPECT_TRUE(container.HasLabel("float"));
        EXPECT_TRUE(floatRefCreated.Content() == ::Thunder::Core::JSON::Variant::type::EMPTY);

        ::Thunder::Core::JSON::Variant& doubleRefCreated = container["double"];
        EXPECT_TRUE(container.HasLabel("double"));
        EXPECT_TRUE(doubleRefCreated.Content() == ::Thunder::Core::JSON::Variant::type::EMPTY);

        ::Thunder::Core::JSON::Variant& boolRefCreated = container["boolean"];
        EXPECT_TRUE(container.HasLabel("boolean"));
        EXPECT_TRUE(boolRefCreated.Content() == ::Thunder::Core::JSON::Variant::type::EMPTY);

        ::Thunder::Core::JSON::Variant& stringRefCreated = container["string"];
        EXPECT_TRUE(container.HasLabel("string"));
        EXPECT_TRUE(stringRefCreated.Content() == ::Thunder::Core::JSON::Variant::type::EMPTY);
    }


} // Core
} // Tests
} // Thunder

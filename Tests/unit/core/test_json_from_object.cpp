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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/core.h>

namespace WPEFramework {
namespace Tests {
namespace Core {

    struct DeviceInfo : public ::WPEFramework::Core::JSON::Container {
        DeviceInfo()
            : ::WPEFramework::Core::JSON::Container()
            , model()
            , firmware()
        {
            Add(_T("model"), &model);
            Add(_T("firmware"), &firmware);
        }

        ::WPEFramework::Core::JSON::String model;
        ::WPEFramework::Core::JSON::String firmware;
    };

    struct NestedDevice : public ::WPEFramework::Core::JSON::Container {
        NestedDevice()
            : ::WPEFramework::Core::JSON::Container()
            , model()
            , region()
        {
            Add(_T("model"), &model);
            Add(_T("region"), &region);
        }

        ::WPEFramework::Core::JSON::String model;
        ::WPEFramework::Core::JSON::String region;
    };

    struct DeviceResponse : public ::WPEFramework::Core::JSON::Container {
        DeviceResponse()
            : ::WPEFramework::Core::JSON::Container()
            , status()
            , device()
        {
            Add(_T("status"), &status);
            Add(_T("device"), &device);
        }

        ::WPEFramework::Core::JSON::String status;
        NestedDevice device;
    };

    struct ThreeFields : public ::WPEFramework::Core::JSON::Container {
        ThreeFields()
            : ::WPEFramework::Core::JSON::Container()
            , a()
            , b()
            , c()
        {
            Add(_T("a"), &a);
            Add(_T("b"), &b);
            Add(_T("c"), &c);
        }

        ::WPEFramework::Core::JSON::String a;
        ::WPEFramework::Core::JSON::String b;
        ::WPEFramework::Core::JSON::String c;
    };

    // 4.1: VariantContainer ← VariantContainer (replace semantics)
    TEST(JSONFromObject, VariantContainer_FromObject_VariantContainer_Replace)
    {
        JsonObject source;
        source.Set(_T("a"), JsonValue(1));
        source.Set(_T("b"), JsonValue(2));

        JsonObject target;
        target.Set(_T("b"), JsonValue(99));
        target.Set(_T("c"), JsonValue(3));

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_EQ(target.Get(_T("a")).Number(), 1);
        EXPECT_EQ(target.Get(_T("b")).Number(), 2);

        EXPECT_FALSE(target.HasLabel(_T("c")));
    }

    // 4.2: Typed Container — only registered slots updated
    TEST(JSONFromObject, TypedContainer_OnlyRegisteredSlotsUpdated)
    {
        JsonObject source;
        source.Set(_T("model"), JsonValue(_T("ES1-A")));
        source.Set(_T("firmware"), JsonValue(_T("R4.4.7")));
        source.Set(_T("extra"), JsonValue(_T("ignored")));

        DeviceInfo target;
        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-A"));
        EXPECT_STREQ(target.firmware.Value().c_str(), _T("R4.4.7"));

        EXPECT_FALSE(target.HasLabel(_T("extra")));
    }

    // 4.3: Typed Container — absent source keys leave target fields unset
    TEST(JSONFromObject, TypedContainer_AbsentSourceKeysAreUnset)
    {
        DeviceInfo source;
        source.model = _T("ES1-A");

        DeviceInfo target;
        target.model = _T("old-model");
        target.firmware = _T("old-firmware");

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-A"));
        EXPECT_FALSE(target.firmware.IsSet());
    }

    // 4.4: Deep recursive update — nested typed Container
    TEST(JSONFromObject, TypedContainer_DeepRecursiveNested)
    {
        DeviceResponse source;
        source.status = _T("ok");
        source.device.model = _T("ES1-B");

        DeviceResponse target;
        target.status = _T("old");
        target.device.model = _T("old-model");
        target.device.region = _T("EU");

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.status.Value().c_str(), _T("ok"));
        EXPECT_STREQ(target.device.model.Value().c_str(), _T("ES1-B"));

        EXPECT_FALSE(target.device.region.IsSet());
    }

    // 4.5: Cross-type: typed Container ← VariantContainer
    TEST(JSONFromObject, TypedContainer_FromObject_VariantContainer)
    {
        JsonObject source;
        source.Set(_T("model"), JsonValue(_T("ES1-B")));
        source.Set(_T("firmware"), JsonValue(_T("R5.0")));

        DeviceInfo target;
        bool result = target.FromObject(source);

        EXPECT_TRUE(result);
        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-B"));
        EXPECT_STREQ(target.firmware.Value().c_str(), _T("R5.0"));
    }

    // 4.6: Cross-type: VariantContainer ← typed Container
    TEST(JSONFromObject, VariantContainer_FromObject_TypedContainer)
    {
        DeviceInfo source;
        source.model = _T("ES1-A");
        source.firmware = _T("R4.4.7");

        JsonObject target;
        target.Set(_T("old_key"), JsonValue(_T("will-be-gone")));

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.Get(_T("model")).String().c_str(), _T("ES1-A"));
        EXPECT_STREQ(target.Get(_T("firmware")).String().c_str(), _T("R4.4.7"));

        EXPECT_FALSE(target.HasLabel(_T("old_key")));
    }

    // 4.7: Typed Container ← Typed Container (same type)
    TEST(JSONFromObject, TypedContainer_FromObject_TypedContainer)
    {
        DeviceInfo source;
        source.model = _T("ES1-A");
        source.firmware = _T("R4.4.7");

        DeviceInfo target;
        target.model = _T("old");

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-A"));
        EXPECT_STREQ(target.firmware.Value().c_str(), _T("R4.4.7"));
    }

    // 4.8: Null fields from source are imported
    TEST(JSONFromObject, TypedContainer_NullFieldsImported)
    {
        DeviceInfo source;
        source.model = _T("ES1-A");
        source.firmware.Null(true);

        DeviceInfo target;
        bool result = target.FromObject(source);

        EXPECT_TRUE(result);
        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-A"));
        EXPECT_TRUE(target.firmware.IsNull());
        EXPECT_TRUE(target.firmware.IsSet());
    }

    // 4.9: Returns false for invalid source (scalar as source)
    TEST(JSONFromObject, ReturnsFalse_ScalarSource)
    {
        ::WPEFramework::Core::JSON::String scalar;
        scalar = _T("just a string");

        DeviceInfo target;
        target.model = _T("will-be-cleared");

        bool result = target.FromObject(scalar);

        EXPECT_FALSE(result);
    }

    // 4.10: Empty source — default-constructed typed Container serialises to "{}".
    TEST(JSONFromObject, EmptySource_ReturnsFalse)
    {
        DeviceInfo source;

        DeviceInfo target;
        target.model = _T("old-model");
        target.firmware = _T("old-firmware");

        bool result = target.FromObject(source);

        EXPECT_FALSE(result);

        EXPECT_FALSE(target.model.IsSet());
        EXPECT_FALSE(target.firmware.IsSet());
    }

    // 4.11: Unset fields in source are discarded (not imported)
    TEST(JSONFromObject, UnsetFieldsDiscarded)
    {
        ThreeFields source;
        source.a = _T("hello");

        ThreeFields target;
        target.a = _T("old-a");
        target.b = _T("old-b");
        target.c = _T("old-c");

        bool result = target.FromObject(source);

        EXPECT_TRUE(result);

        EXPECT_STREQ(target.a.Value().c_str(), _T("hello"));
        EXPECT_FALSE(target.b.IsSet());
        EXPECT_FALSE(target.c.IsSet());
    }

    // 4.12: VariantContainer chaining — last FromObject wins (clear-first)
    TEST(JSONFromObject, VariantContainer_Chaining_LastWins)
    {
        JsonObject source1;
        source1.Set(_T("x"), JsonValue(1));

        JsonObject source2;
        source2.Set(_T("y"), JsonValue(2));

        JsonObject source3;
        source3.Set(_T("z"), JsonValue(3));

        JsonObject target;
        EXPECT_TRUE(target.FromObject(source1));
        EXPECT_TRUE(target.FromObject(source2));
        EXPECT_TRUE(target.FromObject(source3));

        EXPECT_FALSE(target.HasLabel(_T("x")));
        EXPECT_FALSE(target.HasLabel(_T("y")));

        EXPECT_TRUE(target.HasLabel(_T("z")));
        EXPECT_EQ(target.Get(_T("z")).Number(), 3);
    }

    // 4.13: ArrayType as source returns false
    TEST(JSONFromObject, ReturnsFalse_ArraySource)
    {
        ::WPEFramework::Core::JSON::ArrayType<::WPEFramework::Core::JSON::String> arr;
        arr.Add() = _T("item1");
        arr.Add() = _T("item2");

        DeviceInfo target;
        target.model = _T("will-be-cleared");

        bool result = target.FromObject(arr);

        EXPECT_FALSE(result);
    }

    // 4.14: Self import works because FromObject serializes before deserializing
    TEST(JSONFromObject, TypedContainer_FromObject_SelfPreservesValues)
    {
        DeviceInfo target;
        target.model = _T("ES1-A");
        target.firmware = _T("R4.4.7");

        bool result = target.FromObject(target);

        EXPECT_TRUE(result);
        EXPECT_STREQ(target.model.Value().c_str(), _T("ES1-A"));
        EXPECT_STREQ(target.firmware.Value().c_str(), _T("R4.4.7"));
    }

    // 4.15: Cross-type: VariantContainer <- typed Container with nested object
    TEST(JSONFromObject, VariantContainer_FromObject_TypedContainerNested)
    {
        DeviceResponse source;
        source.status = _T("ok");
        source.device.model = _T("ES1-B");
        source.device.region = _T("EU");

        JsonObject target;
        bool result = target.FromObject(source);

        EXPECT_TRUE(result);
        EXPECT_STREQ(target.Get(_T("status")).String().c_str(), _T("ok"));
        EXPECT_EQ(target.Get(_T("device")).Content(), JsonValue::type::OBJECT);

        JsonObject nested;
        ASSERT_TRUE(nested.FromString(target.Get(_T("device")).String()));
        EXPECT_STREQ(nested.Get(_T("model")).String().c_str(), _T("ES1-B"));
        EXPECT_STREQ(nested.Get(_T("region")).String().c_str(), _T("EU"));
    }

    // 4.16: Cross-type: typed Container <- VariantContainer with nested object
    TEST(JSONFromObject, TypedContainer_FromObject_VariantContainerNested)
    {
        JsonObject source;
        source.Set(_T("status"), JsonValue(_T("ok")));

        JsonObject nested;
        nested.Set(_T("model"), JsonValue(_T("ES1-C")));
        nested.Set(_T("region"), JsonValue(_T("US")));
        source.Set(_T("device"), JsonValue(nested));

        DeviceResponse target;
        bool result = target.FromObject(source);

        EXPECT_TRUE(result);
        EXPECT_STREQ(target.status.Value().c_str(), _T("ok"));
        EXPECT_STREQ(target.device.model.Value().c_str(), _T("ES1-C"));
        EXPECT_STREQ(target.device.region.Value().c_str(), _T("US"));
    }


} // Core
} // Tests
} // WPEFramework

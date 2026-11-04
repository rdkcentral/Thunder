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
#include <atomic>
#include <thread>

namespace Thunder {
namespace Tests {
namespace Core {

    // --- JSON parameter types used by tests ---

    class JsonStringParam : public ::Thunder::Core::JSON::Container {
    public:
        JsonStringParam()
            : ::Thunder::Core::JSON::Container()
            , Value()
        {
            Add(_T("value"), &Value);
        }
        JsonStringParam(const JsonStringParam& copy)
            : ::Thunder::Core::JSON::Container()
            , Value(copy.Value)
        {
            Add(_T("value"), &Value);
        }
        ~JsonStringParam() override = default;

        ::Thunder::Core::JSON::String Value;
    };

    class JsonNumberParam : public ::Thunder::Core::JSON::Container {
    public:
        JsonNumberParam()
            : ::Thunder::Core::JSON::Container()
            , Value(0)
        {
            Add(_T("value"), &Value);
        }
        JsonNumberParam(const JsonNumberParam& copy)
            : ::Thunder::Core::JSON::Container()
            , Value(copy.Value)
        {
            Add(_T("value"), &Value);
        }
        ~JsonNumberParam() override = default;

        ::Thunder::Core::JSON::DecUInt32 Value;
    };

    class JsonAddParams : public ::Thunder::Core::JSON::Container {
    public:
        JsonAddParams()
            : ::Thunder::Core::JSON::Container()
            , A(0)
            , B(0)
        {
            Add(_T("a"), &A);
            Add(_T("b"), &B);
        }
        JsonAddParams(const JsonAddParams& copy)
            : ::Thunder::Core::JSON::Container()
            , A(copy.A)
            , B(copy.B)
        {
            Add(_T("a"), &A);
            Add(_T("b"), &B);
        }
        ~JsonAddParams() override = default;

        ::Thunder::Core::JSON::DecUInt32 A;
        ::Thunder::Core::JSON::DecUInt32 B;
    };

    class JsonAddResult : public ::Thunder::Core::JSON::Container {
    public:
        JsonAddResult()
            : ::Thunder::Core::JSON::Container()
            , Sum(0)
        {
            Add(_T("sum"), &Sum);
        }
        JsonAddResult(const JsonAddResult& copy)
            : ::Thunder::Core::JSON::Container()
            , Sum(copy.Sum)
        {
            Add(_T("sum"), &Sum);
        }
        ~JsonAddResult() override = default;

        ::Thunder::Core::JSON::DecUInt32 Sum;
    };

    // --- Test service object for method-binding tests ---

    class TestService {
    public:
        TestService() : _value(0) {}

        uint32_t Add(const JsonAddParams& params, JsonAddResult& result)
        {
            result.Sum = params.A + params.B;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t GetValue(JsonNumberParam& result) const
        {
            result.Value = _value;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t SetValue(const JsonNumberParam& param)
        {
            _value = param.Value;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t GetIndexedValue(const string& index, JsonStringParam& result) const
        {
            result.Value = string("indexed_") + index;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t SetIndexedValue(const string& index, const JsonStringParam& param)
        {
            _lastIndex = index;
            _lastIndexedValue = param.Value.Value();
            return ::Thunder::Core::ERROR_NONE;
        }

        // --- Context-aware methods for typed registration ---
        uint32_t ActionWithContext(const ::Thunder::Core::JSONRPC::Context& context, const JsonNumberParam& param, JsonNumberParam& result)
        {
            result.Value = context.ChannelId() + param.Value;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t ActionWithContextOutbound(const ::Thunder::Core::JSONRPC::Context& context, JsonNumberParam& result)
        {
            result.Value = context.ChannelId();
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t ActionWithContextInbound(const ::Thunder::Core::JSONRPC::Context& context, const JsonNumberParam& param)
        {
            _value = context.ChannelId() + param.Value;
            return ::Thunder::Core::ERROR_NONE;
        }

        // --- Index-aware methods ---
        uint32_t ActionWithIndex(const string& index, const JsonNumberParam& param)
        {
            _lastIndex = index;
            _value = param.Value;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t ActionWithIndexOutbound(const string& index, JsonNumberParam& result)
        {
            _lastIndex = index;
            result.Value = _value;
            return ::Thunder::Core::ERROR_NONE;
        }

        uint32_t ActionWithContextIndex(const ::Thunder::Core::JSONRPC::Context& context, const string& index, const JsonNumberParam& param, JsonNumberParam& result)
        {
            _lastIndex = index;
            result.Value = context.ChannelId() + param.Value;
            return ::Thunder::Core::ERROR_NONE;
        }

        // --- Announce-style methods (void return, callback-like) ---
        void AnnounceVoid(const ::Thunder::Core::JSONRPC::Context& context)
        {
            _value = context.ChannelId();
        }

        void AnnounceWithInbound(const ::Thunder::Core::JSONRPC::Context& context, const JsonNumberParam& param)
        {
            _value = context.ChannelId() + param.Value;
        }

        uint32_t Value() const { return _value; }
        const string& LastIndex() const { return _lastIndex; }
        const string& LastIndexedValue() const { return _lastIndexedValue; }

    private:
        uint32_t _value;
        string _lastIndex;
        string _lastIndexedValue;
    };

    // --- Helper to create a default context ---

    static ::Thunder::Core::JSONRPC::Context MakeContext(uint32_t channelId = 1)
    {
        return ::Thunder::Core::JSONRPC::Context(channelId, 0, string());
    }

    // =========================================================================
    // JSONRPC::Handler tests — closes gap: Core JSONRPC::Handler register/invoke
    // (v2.1 gap: "JSONRPC::Handler — register, invoke, unregister")
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Invoke_RegisteredMethod_ReturnsCorrectResponse)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("add", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string& parameters, string& result) -> uint32_t {
            JsonAddParams params;
            params.FromString(parameters);
            JsonAddResult res;
            res.Sum = params.A + params.B;
            res.ToString(result);
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "add", "{\"a\":3,\"b\":7}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        JsonAddResult parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Sum.Value(), 10u);

        handler.Unregister("add");
    }

    // =========================================================================
    // Test: Invoke with unknown method returns ERROR_UNKNOWN_METHOD
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Invoke_UnknownMethod_ReturnsErrorUnknownMethod)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "nonExistentMethod", "{}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_UNKNOWN_METHOD);
        EXPECT_TRUE(response.empty());
    }

    // =========================================================================
    // Test: Register method with typed inbound+outbound via object binding
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_TypedMethodOnObject_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonAddParams, JsonAddResult>("add", &TestService::Add, &service);

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "add", "{\"a\":10,\"b\":20}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        JsonAddResult parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Sum.Value(), 30u);

        handler.Unregister("add");
    }

    // =========================================================================
    // Test: Duplicate method registration overwrites the previous handler
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_DuplicateMethod_OverwritesPrevious)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("getValue", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"first\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        // Overwrite with a different implementation
        handler.Register("getValue", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"second\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "getValue", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("second"), string::npos);

        handler.Unregister("getValue");
    }

    // =========================================================================
    // Test: Unregister method then invoke returns ERROR_UNKNOWN_METHOD
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Unregister_ThenInvoke_ReturnsUnknownMethod)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("temp", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        // Verify it works
        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "temp", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        // Unregister
        handler.Unregister("temp");

        // Now invoke should fail
        response.clear();
        code = handler.Invoke(context, "temp", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_UNKNOWN_METHOD);
    }

    // =========================================================================
    // Test: Property with get and set (1-argument, no index)
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Property_GetAndSet_WorksCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonNumberParam>("counter", &TestService::GetValue, &TestService::SetValue, &service);

        auto context = MakeContext();
        string response;

        // GET: invoke with empty parameters
        uint32_t code = handler.Invoke(context, "counter", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        {
            JsonNumberParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), 0u); // initial value
        }

        // SET: invoke with parameters
        response.clear();
        code = handler.Invoke(context, "counter", "{\"value\":42}", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        // Verify via the service object directly
        EXPECT_EQ(service.Value(), 42u);

        // GET again to verify
        response.clear();
        code = handler.Invoke(context, "counter", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        {
            JsonNumberParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), 42u);
        }

        handler.Unregister("counter");
    }

    // =========================================================================
    // Test: Property with get-only (set is nullptr)
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Property_GetOnly_SetReturnsError)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonNumberParam>("readOnlyCounter", &TestService::GetValue, nullptr, &service);

        auto context = MakeContext();
        string response;

        // GET should work
        uint32_t code = handler.Invoke(context, "readOnlyCounter", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        // SET should fail (property registered with no setter; sending params to a get-only → ERROR_UNAVAILABLE)
        response.clear();
        code = handler.Invoke(context, "readOnlyCounter", "{\"value\":99}", response);
        EXPECT_NE(code, ::Thunder::Core::ERROR_NONE);

        handler.Unregister("readOnlyCounter");
    }

    // =========================================================================
    // Test: Property with set-only (get is nullptr)
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Property_SetOnly_GetReturnsError)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonNumberParam>("writeOnlyCounter", nullptr, &TestService::SetValue, &service);

        auto context = MakeContext();
        string response;

        // SET should work
        uint32_t code = handler.Invoke(context, "writeOnlyCounter", "{\"value\":55}", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.Value(), 55u);

        // GET should fail (no getter; calling with empty params → ERROR_UNAVAILABLE)
        response.clear();
        code = handler.Invoke(context, "writeOnlyCounter", "", response);
        EXPECT_NE(code, ::Thunder::Core::ERROR_NONE);

        handler.Unregister("writeOnlyCounter");
    }

    // =========================================================================
    // Test: Exists() returns ERROR_NONE for registered, ERROR_UNKNOWN_METHOD for unregistered
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Exists_RegisteredAndUnregistered)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("myMethod", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            return ::Thunder::Core::ERROR_NONE;
        });

        EXPECT_EQ(handler.Exists("myMethod"), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(handler.Exists("noSuchMethod"), ::Thunder::Core::ERROR_UNKNOWN_METHOD);

        handler.Unregister("myMethod");

        EXPECT_EQ(handler.Exists("myMethod"), ::Thunder::Core::ERROR_UNKNOWN_METHOD);
    }

    // =========================================================================
    // Test: Version support check
    // =========================================================================

    TEST(Core_JSONRPC_Handler, HasVersionSupport_ReturnsCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1, 2});

        EXPECT_TRUE(handler.HasVersionSupport(1));
        EXPECT_TRUE(handler.HasVersionSupport(2));
        EXPECT_FALSE(handler.HasVersionSupport(3));
    }

    // =========================================================================
    // Test: Register alias (alternative name) for an existing method
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_Alias_DispatchesToOriginal)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("original", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"fromOriginal\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        // Register alias pointing to "original"
        handler.Register("alias", string("original"));

        auto context = MakeContext();
        string response;

        // Invoke via alias
        uint32_t code = handler.Invoke(context, "alias", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("fromOriginal"), string::npos);

        // Original still works
        response.clear();
        code = handler.Invoke(context, "original", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("fromOriginal"), string::npos);

        handler.Unregister("alias");
        handler.Unregister("original");
    }

    // =========================================================================
    // Test: EventIterator lists registered handlers
    // =========================================================================

    TEST(Core_JSONRPC_Handler, EventIterator_ListsRegisteredMethods)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("alpha", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            return ::Thunder::Core::ERROR_NONE;
        });
        handler.Register("beta", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            return ::Thunder::Core::ERROR_NONE;
        });

        auto iterator = handler.Events();
        std::set<string> found;

        while (iterator.Next()) {
            found.insert(iterator.Event());
        }

        EXPECT_EQ(found.size(), 2u);
        EXPECT_TRUE(found.count("alpha") > 0);
        EXPECT_TRUE(found.count("beta") > 0);

        handler.Unregister("alpha");
        handler.Unregister("beta");
    }

    // =========================================================================
    // Test: Handler correctly propagates error codes from implementation
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Invoke_HandlerReturnsCustomError_PropagatedToResult)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("failing", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            return ::Thunder::Core::ERROR_UNAVAILABLE;
        });

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "failing", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_UNAVAILABLE);

        handler.Unregister("failing");
    }

    // =========================================================================
    // Test: Context is passed through to the handler
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Invoke_ContextPassedToHandler)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        uint32_t capturedChannelId = 0;

        handler.Register("contextCheck", [&capturedChannelId](const ::Thunder::Core::JSONRPC::Context& context, const string&, const string&, string& result) -> uint32_t {
            capturedChannelId = context.ChannelId();
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext(42);
        string response;
        uint32_t code = handler.Invoke(context, "contextCheck", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(capturedChannelId, 42u);

        handler.Unregister("contextCheck");
    }

    // =========================================================================
    // Test: Invoke with typed inbound params that fail to parse returns error
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_TypedMethod_InvalidParams_ReturnsParseError)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonAddParams, JsonAddResult>("add", &TestService::Add, &service);

        auto context = MakeContext();
        string response;

        // Send completely invalid JSON as parameters — the typed registration
        // will attempt FromString which should report a parse failure
        uint32_t code = handler.Invoke(context, "add", "not-valid-json{{{", response);

        // The internal implementation returns ERROR_PARSE_FAILURE when FromString reports an error
        EXPECT_EQ(code, ::Thunder::Core::ERROR_PARSE_FAILURE);

        handler.Unregister("add");
    }

    // =========================================================================
    // Test: Register multiple methods, invoke each independently
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_MultipleMethods_EachInvokesIndependently)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("methodA", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"A\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        handler.Register("methodB", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"B\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        handler.Register("methodC", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"C\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;

        EXPECT_EQ(handler.Invoke(context, "methodA", "", response), ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("A"), string::npos);

        response.clear();
        EXPECT_EQ(handler.Invoke(context, "methodB", "", response), ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("B"), string::npos);

        response.clear();
        EXPECT_EQ(handler.Invoke(context, "methodC", "", response), ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("C"), string::npos);

        handler.Unregister("methodA");
        handler.Unregister("methodB");
        handler.Unregister("methodC");
    }

    // =========================================================================
    // Test: Invoke with empty method string returns ERROR_UNKNOWN_METHOD
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Invoke_EmptyMethodName_ReturnsUnknownMethod)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_UNKNOWN_METHOD);
    }

    // =========================================================================
    // Test: Indexed property with get and set (2-argument)
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Property_Indexed_GetAndSet)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonStringParam>("item", &TestService::GetIndexedValue, &TestService::SetIndexedValue, &service);

        auto context = MakeContext();
        string response;

        // GET with index via @index suffix in designator
        uint32_t code = handler.Invoke(context, "item@myKey", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        {
            JsonStringParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), string("indexed_myKey"));
        }

        // SET with index
        response.clear();
        code = handler.Invoke(context, "item@otherKey", "{\"value\":\"hello\"}", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.LastIndex(), string("otherKey"));
        EXPECT_EQ(service.LastIndexedValue(), string("hello"));

        handler.Unregister("item");
    }

    // =========================================================================
    // Test: Copy handler from another Handler instance
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Copy_MethodFromAnotherHandler)
    {
        ::Thunder::Core::JSONRPC::Handler source(std::vector<uint8_t>{1});
        ::Thunder::Core::JSONRPC::Handler target(std::vector<uint8_t>{1});

        source.Register("shared", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"copied\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        bool copied = target.Copy(source, "shared");
        EXPECT_TRUE(copied);

        auto context = MakeContext();
        string response;
        uint32_t code = target.Invoke(context, "shared", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("copied"), string::npos);

        // Copy non-existent method should fail
        EXPECT_FALSE(target.Copy(source, "doesNotExist"));

        source.Unregister("shared");
        target.Unregister("shared");
    }

    // =========================================================================
    // Test: CallbackFunction (async) registration and invoke
    // =========================================================================

    TEST(Core_JSONRPC_Handler, Register_CallbackFunction_InvokeReturnsAsync)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        bool callbackInvoked = false;

        ::Thunder::Core::JSONRPC::CallbackFunction cb = [&callbackInvoked](const ::Thunder::Core::JSONRPC::Context&, const string&, ::Thunder::Core::OptionalType<::Thunder::Core::JSON::Error>&) {
            callbackInvoked = true;
        };

        handler.Register("asyncMethod", cb);

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "asyncMethod", "", response);

        // Async callbacks return ~0 (no error report set)
        EXPECT_EQ(code, static_cast<uint32_t>(~0));
        EXPECT_TRUE(callbackInvoked);

        handler.Unregister("asyncMethod");
    }

    // =========================================================================
    // Test: Message class — deserialization from JSON string
    // =========================================================================

    TEST(Core_JSONRPC_Message, FromString_ValidRequest_ParsesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        string input = R"({"jsonrpc":"2.0","id":1,"method":"Controller.1.status","params":{"value":"test"}})";

        EXPECT_TRUE(msg.FromString(input));
        EXPECT_EQ(msg.JSONRPC.Value(), string("2.0"));
        EXPECT_EQ(msg.Id.Value(), 1u);
        EXPECT_EQ(msg.Designator.Value(), string("Controller.1.status"));
        EXPECT_TRUE(msg.Parameters.IsSet());
    }

    // =========================================================================
    // Test: Message class — Callsign/Method/Version extraction
    // =========================================================================

    TEST(Core_JSONRPC_Message, DesignatorParsing_ExtractsCallsignVersionMethod)
    {
        // Format: Callsign.version.method
        string designator = "Controller.1.status";

        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Callsign(designator), string("Controller"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Version(designator), 1u);
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Method(designator), string("status"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::FullMethod(designator), string("status"));
    }

    // =========================================================================
    // Test: Message class — designator with index
    // =========================================================================

    TEST(Core_JSONRPC_Message, DesignatorParsing_WithIndex)
    {
        string designator = "Plugin.1.property@myIndex";

        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Index(designator), string("myIndex"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Method(designator), string("property"));
    }

    // =========================================================================
    // Test: Message class — designator without callsign or version
    // =========================================================================

    TEST(Core_JSONRPC_Message, DesignatorParsing_MethodOnly)
    {
        string designator = "simpleMethod";

        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Callsign(designator), string());
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Method(designator), string("simpleMethod"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Index(designator), string());
    }

    // =========================================================================
    // Test: Message::Info SetError maps framework errors to JSON-RPC codes
    // =========================================================================

    TEST(Core_JSONRPC_Message, Info_SetError_MapsFrameworkCodes)
    {
        ::Thunder::Core::JSONRPC::Message::Info info;

        info.SetError(::Thunder::Core::ERROR_UNKNOWN_METHOD);
        EXPECT_EQ(info.Code.Value(), -32601); // Method not found

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_INVALID_ENVELOPPE);
        EXPECT_EQ(info.Code.Value(), -32600); // Invalid request

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_PARSING_ENVELOPPE);
        EXPECT_EQ(info.Code.Value(), -32700); // Parse error

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_INVALID_PARAMETER);
        EXPECT_EQ(info.Code.Value(), -32602); // Invalid params

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_INTERNAL_JSONRPC);
        EXPECT_EQ(info.Code.Value(), -32603); // Internal error

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_TIMEDOUT);
        EXPECT_EQ(info.Code.Value(), -32000); // Server defined: timed out

        info.Clear();
        info.SetError(::Thunder::Core::ERROR_PRIVILIGED_REQUEST);
        EXPECT_EQ(info.Code.Value(), -32604); // Privileged
    }

    // =========================================================================
    // Test: Message Clear resets all fields
    // =========================================================================

    TEST(Core_JSONRPC_Message, Clear_ResetsAllFields)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        string input = R"({"jsonrpc":"2.0","id":99,"method":"test","params":"hello","result":"world"})";
        msg.FromString(input);

        msg.Clear();

        EXPECT_EQ(msg.JSONRPC.Value(), string("2.0")); // reset to default version
        EXPECT_FALSE(msg.Id.IsSet());
        EXPECT_FALSE(msg.Designator.IsSet());
        EXPECT_FALSE(msg.Parameters.IsSet());
        EXPECT_FALSE(msg.Result.IsSet());
    }

    // =========================================================================
    // Test: Message Join/Split round-trip
    // =========================================================================

    TEST(Core_JSONRPC_Message, Join_ProducesCorrectDesignator)
    {
        string result = ::Thunder::Core::JSONRPC::Message::Join("Controller", "1", "", "", "status", "");
        EXPECT_EQ(result, string("Controller.1.status"));

        // With index
        result = ::Thunder::Core::JSONRPC::Message::Join("Plugin", "2", "", "", "prop", "idx");
        EXPECT_EQ(result, string("Plugin.2.prop@idx"));

        // With prefix and instance ID
        result = ::Thunder::Core::JSONRPC::Message::Join("", "", "org.rdk", "42", "method", "");
        EXPECT_EQ(result, string("org.rdk#42::method"));
    }

    // =========================================================================
    // VOID PARAMETER PERMUTATION TESTS
    // =========================================================================

    // Test: Register typed method with void INBOUND and void OUTBOUND via lambda
    TEST(Core_JSONRPC_Handler, Register_VoidInboundVoidOutbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        bool invoked = false;
        handler.Register("action", [&invoked](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            invoked = true;
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "action", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(invoked);

        handler.Unregister("action");
    }

    // Test: Register typed method with INBOUND only (void OUTBOUND) via object binding
    TEST(Core_JSONRPC_Handler, Register_InboundOnly_VoidOutbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam, void>("setVal", &TestService::ActionWithContextInbound, &service);

        auto context = MakeContext(10);
        string response;
        uint32_t code = handler.Invoke(context, "setVal", "{\"value\":5}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.Value(), 15u); // channelId(10) + param(5)

        handler.Unregister("setVal");
    }

    // Test: Register typed method with void INBOUND and OUTBOUND only via object binding
    TEST(Core_JSONRPC_Handler, Register_VoidInbound_OutboundOnly_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<void, JsonNumberParam>("getVal", &TestService::ActionWithContextOutbound, &service);

        auto context = MakeContext(42);
        string response;
        uint32_t code = handler.Invoke(context, "getVal", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        JsonNumberParam parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Value.Value(), 42u);

        handler.Unregister("getVal");
    }

    // Test: Context + INBOUND + OUTBOUND (full typed registration)
    TEST(Core_JSONRPC_Handler, Register_ContextInboundOutbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam, JsonNumberParam>("ctxAction", &TestService::ActionWithContext, &service);

        auto context = MakeContext(10);
        string response;
        uint32_t code = handler.Invoke(context, "ctxAction", "{\"value\":5}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);

        JsonNumberParam parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Value.Value(), 15u); // channelId(10) + param(5)

        handler.Unregister("ctxAction");
    }

    // =========================================================================
    // INDEX-AWARE TYPED METHOD TESTS
    // =========================================================================

    // Test: Index + inbound (void outbound)
    TEST(Core_JSONRPC_Handler, Register_IndexInbound_VoidOutbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam, void>("indexedSet", &TestService::ActionWithIndex, &service);

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "indexedSet@key1", "{\"value\":33}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.Value(), 33u);
        EXPECT_EQ(service.LastIndex(), string("key1"));

        handler.Unregister("indexedSet");
    }

    // Test: Index + outbound (void inbound)
    TEST(Core_JSONRPC_Handler, Register_IndexOutbound_VoidInbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        // Set known value
        JsonNumberParam setParam;
        setParam.Value = 44;
        service.SetValue(setParam);

        handler.Register<void, JsonNumberParam>("indexedGet", &TestService::ActionWithIndexOutbound, &service);

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "indexedGet@myIdx", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.LastIndex(), string("myIdx"));

        JsonNumberParam parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Value.Value(), 44u);

        handler.Unregister("indexedGet");
    }

    // Test: Context + Index + inbound + outbound
    TEST(Core_JSONRPC_Handler, Register_ContextIndexInboundOutbound_InvokesCorrectly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam, JsonNumberParam>("ctxIdx", &TestService::ActionWithContextIndex, &service);

        auto context = MakeContext(100);
        string response;
        uint32_t code = handler.Invoke(context, "ctxIdx@theKey", "{\"value\":7}", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.LastIndex(), string("theKey"));

        JsonNumberParam parsed;
        parsed.FromString(response);
        EXPECT_EQ(parsed.Value.Value(), 107u); // channelId(100) + param(7)

        handler.Unregister("ctxIdx");
    }

    // Test: Indexed property get-only
    TEST(Core_JSONRPC_Handler, Property_Indexed_GetOnly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonStringParam>("readItem", &TestService::GetIndexedValue, nullptr, &service);

        auto context = MakeContext();
        string response;

        uint32_t code = handler.Invoke(context, "readItem@abc", "", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        {
            JsonStringParam parsed;
            parsed.FromString(response);
            EXPECT_EQ(parsed.Value.Value(), string("indexed_abc"));
        }

        // SET should fail
        response.clear();
        code = handler.Invoke(context, "readItem@abc", "{\"value\":\"x\"}", response);
        EXPECT_NE(code, ::Thunder::Core::ERROR_NONE);

        handler.Unregister("readItem");
    }

    // Test: Indexed property set-only
    TEST(Core_JSONRPC_Handler, Property_Indexed_SetOnly)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Property<JsonStringParam>("writeItem", nullptr, &TestService::SetIndexedValue, &service);

        auto context = MakeContext();
        string response;

        // SET should work
        uint32_t code = handler.Invoke(context, "writeItem@key2", "{\"value\":\"hello\"}", response);
        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(service.LastIndex(), string("key2"));
        EXPECT_EQ(service.LastIndexedValue(), string("hello"));

        // GET should fail
        response.clear();
        code = handler.Invoke(context, "writeItem@key2", "", response);
        EXPECT_NE(code, ::Thunder::Core::ERROR_NONE);

        handler.Unregister("writeItem");
    }

    // =========================================================================
    // INSTANCE ID ROUTING TESTS
    // =========================================================================

    // Test: Instance ID routing — Invoke extracts instance ID from designator
    TEST(Core_JSONRPC_Handler, Invoke_InstanceIdRouting_ExtractsAndDispatches)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        string capturedInstanceId;
        string capturedMethod;

        // Method must be registered WITH prefix (Message::Method strips instanceId but keeps prefix)
        // Message::Method("prefix#42::doSomething") => "prefix::doSomething"
        handler.Register("prefix::doSomething", [&capturedInstanceId, &capturedMethod](
            const ::Thunder::Core::JSONRPC::Context&, const string& method, const string&, string& result) -> uint32_t {
            capturedMethod = method;
            capturedInstanceId = ::Thunder::Core::JSONRPC::Message::InstanceId(method);
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;

        // Invoke with prefix#instanceId::method format
        uint32_t code = handler.Invoke(context, "prefix#42::doSomething", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(capturedInstanceId, string("42"));
        // The full designator is passed to the handler lambda
        EXPECT_EQ(capturedMethod, string("prefix#42::doSomething"));

        handler.Unregister("prefix::doSomething");
    }

    // Test: Instance ID routing — different instance IDs dispatch to same handler
    TEST(Core_JSONRPC_Handler, Invoke_InstanceIdRouting_DifferentInstancesSameHandler)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        std::vector<string> capturedIds;

        // Register with prefix (Message::Method strips instanceId, keeps prefix)
        handler.Register("ns::action", [&capturedIds](
            const ::Thunder::Core::JSONRPC::Context&, const string& method, const string&, string& result) -> uint32_t {
            capturedIds.push_back(::Thunder::Core::JSONRPC::Message::InstanceId(method));
            result = "\"done\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;

        handler.Invoke(context, "ns#instance1::action", "", response);
        handler.Invoke(context, "ns#instance2::action", "", response);
        handler.Invoke(context, "ns#instance3::action", "", response);

        ASSERT_EQ(capturedIds.size(), 3u);
        EXPECT_EQ(capturedIds[0], string("instance1"));
        EXPECT_EQ(capturedIds[1], string("instance2"));
        EXPECT_EQ(capturedIds[2], string("instance3"));

        handler.Unregister("ns::action");
    }

    // Test: Instance ID + index routing
    TEST(Core_JSONRPC_Handler, Invoke_InstanceIdWithIndex_BothExtracted)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        string capturedId;
        string capturedIndex;

        // Message::Method("pre#abc::prop@myIdx") => "pre::prop" (strips instanceId and index)
        handler.Register("pre::prop", [&capturedId, &capturedIndex](
            const ::Thunder::Core::JSONRPC::Context&, const string& method, const string&, string& result) -> uint32_t {
            capturedId = ::Thunder::Core::JSONRPC::Message::InstanceId(method);
            capturedIndex = ::Thunder::Core::JSONRPC::Message::Index(method);
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;

        uint32_t code = handler.Invoke(context, "pre#abc::prop@myIdx", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(capturedId, string("abc"));
        EXPECT_EQ(capturedIndex, string("myIdx"));

        handler.Unregister("pre::prop");
    }

    // Test: prefix::method designator (no instance ID) dispatches correctly
    TEST(Core_JSONRPC_Handler, Invoke_PrefixNoInstanceId_StillDispatches)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        string capturedMethod;
        // Message::Method("prefix::myMethod") => "prefix::myMethod" (no instanceId to strip)
        handler.Register("prefix::myMethod", [&capturedMethod](
            const ::Thunder::Core::JSONRPC::Context&, const string& method, const string&, string& result) -> uint32_t {
            capturedMethod = method;
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        auto context = MakeContext();
        string response;

        uint32_t code = handler.Invoke(context, "prefix::myMethod", "", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::InstanceId(capturedMethod), string());
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Prefix(capturedMethod), string("prefix"));

        handler.Unregister("prefix::myMethod");
    }

    // =========================================================================
    // ANNOUNCE (CALLBACK) REGISTRATION TESTS
    // =========================================================================

    // Test: Announce with void INBOUND (just context)
    TEST(Core_JSONRPC_Handler, Register_AnnounceVoid_InvokesAsCallback)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<void>("announce", &TestService::AnnounceVoid, &service);

        auto context = MakeContext(77);
        string response;
        uint32_t code = handler.Invoke(context, "announce", "", response);

        // Announce/callback returns ~0 when no error is set
        EXPECT_EQ(code, static_cast<uint32_t>(~0));
        EXPECT_EQ(service.Value(), 77u);

        handler.Unregister("announce");
    }

    // Test: Announce with typed INBOUND
    TEST(Core_JSONRPC_Handler, Register_AnnounceWithInbound_InvokesAsCallback)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam>("announceIn", &TestService::AnnounceWithInbound, &service);

        auto context = MakeContext(10);
        string response;
        uint32_t code = handler.Invoke(context, "announceIn", "{\"value\":3}", response);

        EXPECT_EQ(code, static_cast<uint32_t>(~0));
        EXPECT_EQ(service.Value(), 13u); // channelId(10) + param(3)

        handler.Unregister("announceIn");
    }

    // Test: Announce with typed INBOUND — parse error
    TEST(Core_JSONRPC_Handler, Register_AnnounceWithInbound_ParseError)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});
        TestService service;

        handler.Register<JsonNumberParam>("announceBad", &TestService::AnnounceWithInbound, &service);

        auto context = MakeContext();
        string response;
        uint32_t code = handler.Invoke(context, "announceBad", "not-valid-json{{{", response);

        EXPECT_EQ(code, ::Thunder::Core::ERROR_PARSING_ENVELOPPE);

        handler.Unregister("announceBad");
    }

    // =========================================================================
    // HANDLER COPY CONSTRUCTOR TEST
    // =========================================================================

    TEST(Core_JSONRPC_Handler, CopyConstructor_CopiesAllMethods)
    {
        ::Thunder::Core::JSONRPC::Handler source(std::vector<uint8_t>{1, 2});

        source.Register("methodA", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"A\"";
            return ::Thunder::Core::ERROR_NONE;
        });
        source.Register("methodB", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            result = "\"B\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        // Copy constructor
        ::Thunder::Core::JSONRPC::Handler copy(std::vector<uint8_t>{3}, source);

        auto context = MakeContext();
        string response;

        // Methods from source should be available
        EXPECT_EQ(copy.Invoke(context, "methodA", "", response), ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("A"), string::npos);

        response.clear();
        EXPECT_EQ(copy.Invoke(context, "methodB", "", response), ::Thunder::Core::ERROR_NONE);
        EXPECT_NE(response.find("B"), string::npos);

        // Copy has its own version set
        EXPECT_FALSE(copy.HasVersionSupport(1));
        EXPECT_FALSE(copy.HasVersionSupport(2));
        EXPECT_TRUE(copy.HasVersionSupport(3));

        source.Unregister("methodA");
        source.Unregister("methodB");
        copy.Unregister("methodA");
        copy.Unregister("methodB");
    }

    // =========================================================================
    // EVENT ITERATOR ADDITIONAL TESTS
    // =========================================================================

    TEST(Core_JSONRPC_Handler, EventIterator_EmptyHandler_NoEvents)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        auto iterator = handler.Events();
        EXPECT_FALSE(iterator.Next());
        EXPECT_FALSE(iterator.IsValid());
    }

    TEST(Core_JSONRPC_Handler, EventIterator_Reset_AllowsReIteration)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        handler.Register("ev1", [](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string&) -> uint32_t {
            return ::Thunder::Core::ERROR_NONE;
        });

        auto iterator = handler.Events();

        // First iteration
        EXPECT_TRUE(iterator.Next());
        EXPECT_TRUE(iterator.IsValid());
        EXPECT_EQ(iterator.Event(), string("ev1"));
        EXPECT_FALSE(iterator.Next());

        // Reset and iterate again
        iterator.Reset();
        EXPECT_FALSE(iterator.IsValid());
        EXPECT_TRUE(iterator.Next());
        EXPECT_TRUE(iterator.IsValid());
        EXPECT_EQ(iterator.Event(), string("ev1"));

        handler.Unregister("ev1");
    }

    // =========================================================================
    // REMAINING ERROR CODE MAPPING TESTS
    // =========================================================================

    TEST(Core_JSONRPC_Message, Info_SetError_MapsRemainingFrameworkCodes)
    {
        ::Thunder::Core::JSONRPC::Message::Info info;
        constexpr int32_t base = ::Thunder::Core::JSONRPC::Message::ApplicationErrorCodeBase; // -31000

        // ERROR_NOT_EXIST (43) → -31043
        info.SetError(::Thunder::Core::ERROR_NOT_EXIST);
        EXPECT_EQ(info.Code.Value(), base - 43);
        EXPECT_NE(info.Text.Value().find("not available"), string::npos);

        // ERROR_INVALID_SIGNATURE (38) → -31038
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_INVALID_SIGNATURE);
        EXPECT_EQ(info.Code.Value(), base - 38);
        EXPECT_NE(info.Text.Value().find("version"), string::npos);

        // ERROR_INCORRECT_URL (15) → -31015
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_INCORRECT_URL);
        EXPECT_EQ(info.Code.Value(), base - 15);
        EXPECT_NE(info.Text.Value().find("Designator"), string::npos);

        // ERROR_ILLEGAL_STATE (5) → -31005
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_ILLEGAL_STATE);
        EXPECT_EQ(info.Code.Value(), base - 5);
        EXPECT_NE(info.Text.Value().find("illegal state"), string::npos);

        // ERROR_FAILED_REGISTERED (48) → -31048
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_FAILED_REGISTERED);
        EXPECT_EQ(info.Code.Value(), base - 48);
        EXPECT_NE(info.Text.Value().find("Registration failed"), string::npos);

        // ERROR_FAILED_UNREGISTERED (49) → -31049
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_FAILED_UNREGISTERED);
        EXPECT_EQ(info.Code.Value(), base - 49);
        EXPECT_NE(info.Text.Value().find("Unregister failed"), string::npos);

        // ERROR_HIBERNATED (46) → -31046
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_HIBERNATED);
        EXPECT_EQ(info.Code.Value(), base - 46);
        EXPECT_NE(info.Text.Value().find("hibernated"), string::npos);

        // ERROR_UNAVAILABLE (2) → -31002
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_UNAVAILABLE);
        EXPECT_EQ(info.Code.Value(), base - 2);
        EXPECT_NE(info.Text.Value().find("not active"), string::npos);

        // ERROR_NOT_SUPPORTED (44) → -31044
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_NOT_SUPPORTED);
        EXPECT_EQ(info.Code.Value(), base - 44);
        EXPECT_NE(info.Text.Value().find("not supported"), string::npos);

        // ERROR_PRIVILIGED_DEFERRED (51) → -32604
        info.Clear();
        info.SetError(::Thunder::Core::ERROR_PRIVILIGED_DEFERRED);
        EXPECT_EQ(info.Code.Value(), -32604);
        EXPECT_NE(info.Text.Value().find("deferred"), string::npos);
    }

    // Test: SetError default fallback path (unknown framework error maps to ApplicationErrorCodeBase - error)
    TEST(Core_JSONRPC_Message, Info_SetError_DefaultFallback)
    {
        ::Thunder::Core::JSONRPC::Message::Info info;
        constexpr int32_t base = ::Thunder::Core::JSONRPC::Message::ApplicationErrorCodeBase;

        // Use a framework error code that is not in the switch (e.g., ERROR_GENERAL = 1)
        info.SetError(::Thunder::Core::ERROR_GENERAL);
        EXPECT_EQ(info.Code.Value(), base - 1);
        EXPECT_TRUE(info.Text.IsSet());
    }

    // Test: SetError with COM_ERROR bit
    TEST(Core_JSONRPC_Message, Info_SetError_ComError)
    {
        ::Thunder::Core::JSONRPC::Message::Info info;
        constexpr int32_t base = ::Thunder::Core::JSONRPC::Message::ApplicationErrorCodeBase;

        // COM_ERROR | some value
        uint32_t comErr = COM_ERROR | 0x00000005;
        info.SetError(comErr);
        // Code = base - (comErr & 0x7FFFFFFF) - 500 = -31000 - 5 - 500 = -31505
        EXPECT_EQ(info.Code.Value(), base - 5 - 500);
    }

    // =========================================================================
    // MESSAGE DESIGNATOR PARSING — ADDITIONAL COVERAGE
    // =========================================================================

    // Test: Message::Split full designator
    TEST(Core_JSONRPC_Message, Split_FullDesignator_AllParts)
    {
        string callsign, version, prefix, instanceId, method, index;

        ::Thunder::Core::JSONRPC::Message::Split("Controller.1.org.rdk#42::status@myIdx",
            &callsign, &version, &prefix, &instanceId, &method, &index);

        EXPECT_EQ(method, string("status"));
        EXPECT_EQ(index, string("myIdx"));
        EXPECT_EQ(instanceId, string("42"));
    }

    // Test: Message::Split with nullptrs (should not crash)
    TEST(Core_JSONRPC_Message, Split_NullPtrs_DoesNotCrash)
    {
        // Just pass nullptrs for all output params — it should handle gracefully
        ::Thunder::Core::JSONRPC::Message::Split("Controller.1.status", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    }

    // Test: Message::Prefix
    TEST(Core_JSONRPC_Message, Prefix_Extraction)
    {
        // Prefix treats dots as callsign separators — prefix is after last dot before ::
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Prefix("org.rdk#42::method"), string("rdk"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Prefix("ns::method"), string("ns"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Prefix("simpleMethod"), string());
        // Prefix without dot — the whole part before :: is the prefix
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Prefix("myPrefix#id::method"), string("myPrefix"));
    }

    // Test: Message::InstanceId
    TEST(Core_JSONRPC_Message, InstanceId_Extraction)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::InstanceId("prefix#42::method"), string("42"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::InstanceId("prefix#abc::method@idx"), string("abc"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::InstanceId("simpleMethod"), string());
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::InstanceId("ns::method"), string());
    }

    // Test: Message::FullMethod (includes everything after last dot)
    TEST(Core_JSONRPC_Message, FullMethod_Extraction)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::FullMethod("Controller.1.status"), string("status"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::FullMethod("Controller.1.prop@idx"), string("prop@idx"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::FullMethod("method"), string("method"));
    }

    // Test: Message::VersionedFullMethod (includes version + everything after)
    TEST(Core_JSONRPC_Message, VersionedFullMethod_Extraction)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::VersionedFullMethod("Controller.1.status"), string("1.status"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::VersionedFullMethod("simpleMethod"), string("simpleMethod"));
    }

    // Test: Message::Version — invalid version returns ~0 (255)
    TEST(Core_JSONRPC_Message, Version_InvalidReturns255)
    {
        // No version at all
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Version("simpleMethod"), static_cast<uint8_t>(~0));

        // Just a method, no dots
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Version("method"), static_cast<uint8_t>(~0));
    }

    // Test: Message::Version — valid versions
    TEST(Core_JSONRPC_Message, Version_ValidVersions)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Version("Controller.1.status"), 1u);
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Version("Plugin.99.method"), 99u);
    }

    // Test: Message::VersionAsString
    TEST(Core_JSONRPC_Message, VersionAsString_Extraction)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::VersionAsString("Controller.1.status"), string("1"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::VersionAsString("Plugin.42.method"), string("42"));
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::VersionAsString("simpleMethod"), string());
    }

    // Test: Message::Index — missing index
    TEST(Core_JSONRPC_Message, Index_MissingReturnsEmpty)
    {
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Index("simpleMethod"), string());
        EXPECT_EQ(::Thunder::Core::JSONRPC::Message::Index("Controller.1.method"), string());
    }

    // Test: Message instance methods
    TEST(Core_JSONRPC_Message, InstanceMethods_WorkCorrectly)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        string input = R"({"jsonrpc":"2.0","id":5,"method":"Controller.2.status@idx1"})";
        msg.FromString(input);

        EXPECT_EQ(msg.Callsign(), string("Controller"));
        EXPECT_EQ(msg.Method(), string("status"));
        EXPECT_EQ(msg.FullMethod(), string("status@idx1"));
        EXPECT_EQ(msg.VersionedFullMethod(), string("2.status@idx1"));
        EXPECT_EQ(msg.Version(), 2u);
        EXPECT_EQ(msg.VersionAsString(), string("2"));
        EXPECT_EQ(msg.Index(), string("idx1"));
    }

    // Test: Message ImplicitCallsign
    TEST(Core_JSONRPC_Message, ImplicitCallsign_UsedWhenNoExplicitCallsign)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        string input = R"({"jsonrpc":"2.0","id":1,"method":"status"})";
        msg.FromString(input);

        // No callsign in designator
        EXPECT_EQ(msg.Callsign(), string());

        // Set implicit callsign
        msg.ImplicitCallsign("DefaultPlugin");
        EXPECT_EQ(msg.Callsign(), string("DefaultPlugin"));
    }

    // Test: Message ImplicitCallsign not used when explicit callsign present
    TEST(Core_JSONRPC_Message, ImplicitCallsign_NotUsedWhenExplicitPresent)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        string input = R"({"jsonrpc":"2.0","id":1,"method":"MyPlugin.1.status"})";
        msg.FromString(input);

        msg.ImplicitCallsign("DefaultPlugin");
        EXPECT_EQ(msg.Callsign(), string("MyPlugin"));
    }

    // Test: Join 3-arg overload (prefix, instanceId, method)
    TEST(Core_JSONRPC_Message, Join_ThreeArg_PrefixInstanceIdMethod)
    {
        string result = ::Thunder::Core::JSONRPC::Message::Join("org.rdk", "42", "doAction");
        EXPECT_EQ(result, string("org.rdk#42::doAction"));

        // Without instance ID
        result = ::Thunder::Core::JSONRPC::Message::Join("ns", "", "method");
        EXPECT_EQ(result, string("ns::method"));

        // Without prefix
        result = ::Thunder::Core::JSONRPC::Message::Join("", "", "method");
        EXPECT_EQ(result, string("method"));
    }

    // Test: Join 2-arg overload (prefix, method)
    TEST(Core_JSONRPC_Message, Join_TwoArg_PrefixMethod)
    {
        string result = ::Thunder::Core::JSONRPC::Message::Join("myPrefix", "method");
        EXPECT_EQ(result, string("myPrefix::method"));

        result = ::Thunder::Core::JSONRPC::Message::Join("", "method");
        EXPECT_EQ(result, string("method"));
    }

    // Test: Message serialization round-trip (ToString + FromString)
    TEST(Core_JSONRPC_Message, Serialization_RoundTrip)
    {
        ::Thunder::Core::JSONRPC::Message original;
        original.JSONRPC = "2.0";
        original.Id = 42;
        original.Designator = "Controller.1.status";
        original.Parameters = "{\"key\":\"val\"}";

        string serialized;
        original.ToString(serialized);

        ::Thunder::Core::JSONRPC::Message parsed;
        parsed.FromString(serialized);

        EXPECT_EQ(parsed.JSONRPC.Value(), string("2.0"));
        EXPECT_EQ(parsed.Id.Value(), 42u);
        EXPECT_EQ(parsed.Designator.Value(), string("Controller.1.status"));
        EXPECT_EQ(parsed.Parameters.Value(), string("{\"key\":\"val\"}"));
    }

    // Test: Message with Error field
    TEST(Core_JSONRPC_Message, ErrorField_SetAndSerialize)
    {
        ::Thunder::Core::JSONRPC::Message msg;
        msg.JSONRPC = "2.0";
        msg.Id = 1;
        msg.Error.SetError(::Thunder::Core::ERROR_UNKNOWN_METHOD);

        string serialized;
        msg.ToString(serialized);

        // Verify error code is in the serialized string
        EXPECT_NE(serialized.find("-32601"), string::npos);
        EXPECT_NE(serialized.find("Unknown method"), string::npos);
    }

    // =========================================================================
    // THREAD SAFETY TEST — concurrent invoke must not crash
    // =========================================================================

    TEST(Core_JSONRPC_Handler, ConcurrentInvoke_DoesNotCrash)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{1});

        std::atomic<uint32_t> invokeCount{0};

        handler.Register("counter", [&invokeCount](const ::Thunder::Core::JSONRPC::Context&, const string&, const string&, string& result) -> uint32_t {
            invokeCount++;
            result = "\"ok\"";
            return ::Thunder::Core::ERROR_NONE;
        });

        constexpr int kThreads = 4;
        constexpr int kIterations = 100;
        std::vector<std::thread> threads;

        for (int t = 0; t < kThreads; t++) {
            threads.emplace_back([&handler]() {
                auto ctx = MakeContext();
                for (int i = 0; i < kIterations; i++) {
                    string response;
                    handler.Invoke(ctx, "counter", "", response);
                }
            });
        }

        for (auto& t : threads) {
            t.join();
        }

        EXPECT_EQ(invokeCount.load(), static_cast<uint32_t>(kThreads * kIterations));

        handler.Unregister("counter");
    }

    // =========================================================================
    // CONTEXT CLASS TESTS
    // =========================================================================

    TEST(Core_JSONRPC_Context, DefaultConstructor_HasMaxValues)
    {
        ::Thunder::Core::JSONRPC::Context ctx;
        EXPECT_EQ(ctx.ChannelId(), static_cast<uint32_t>(~0));
        EXPECT_EQ(ctx.Sequence(), static_cast<uint32_t>(~0));
        EXPECT_TRUE(ctx.Token().empty());
    }

    TEST(Core_JSONRPC_Context, ParameterizedConstructor_SetsAllFields)
    {
        ::Thunder::Core::JSONRPC::Context ctx(100, 200, "myToken");
        EXPECT_EQ(ctx.ChannelId(), 100u);
        EXPECT_EQ(ctx.Sequence(), 200u);
        EXPECT_EQ(ctx.Token(), string("myToken"));
    }

    TEST(Core_JSONRPC_Context, SingleArgConstructor_SetsChannelOnly)
    {
        ::Thunder::Core::JSONRPC::Context ctx(42);
        EXPECT_EQ(ctx.ChannelId(), 42u);
        EXPECT_EQ(ctx.Sequence(), static_cast<uint32_t>(~0));
        EXPECT_TRUE(ctx.Token().empty());
    }

    TEST(Core_JSONRPC_Context, CopyConstructor_CopiesAllFields)
    {
        ::Thunder::Core::JSONRPC::Context original(10, 20, "tok");
        ::Thunder::Core::JSONRPC::Context copy(original);
        EXPECT_EQ(copy.ChannelId(), 10u);
        EXPECT_EQ(copy.Sequence(), 20u);
        EXPECT_EQ(copy.Token(), string("tok"));
    }

    TEST(Core_JSONRPC_Context, MoveConstructor_MovesToken)
    {
        ::Thunder::Core::JSONRPC::Context original(1, 2, "moveMe");
        ::Thunder::Core::JSONRPC::Context moved(std::move(original));
        EXPECT_EQ(moved.ChannelId(), 1u);
        EXPECT_EQ(moved.Sequence(), 2u);
        EXPECT_EQ(moved.Token(), string("moveMe"));
    }

    // =========================================================================
    // ERROR ALIAS TYPE TEST
    // =========================================================================

    TEST(Core_JSONRPC_Error, ErrorTypeAlias_IsMessageInfo)
    {
        // Error is an alias for Message::Info
        ::Thunder::Core::JSONRPC::Error err;
        err.SetError(::Thunder::Core::ERROR_TIMEDOUT);
        EXPECT_EQ(err.Code.Value(), -32000);
    }

} // namespace Core
} // namespace Tests
} // namespace Thunder

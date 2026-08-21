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

// Tests JSON-RPC handler versioning: CreateHandler/GetHandler dispatch and
// TokenCheckFunction validation, as defined in PluginHost::JSONRPC (R4).
//
// R4 dispatch specifics:
//   - IDispatcher::Validate(token, method, params) checks the TokenCheckFunction.
//   - IDispatcher::Invoke(nullptr, channelId, id, token, method, params, response)
//     preserves the version prefix in the method string for correct dispatch.
//   - ILocalDispatcher::Invoke(...) strips the version via FullMethod — do NOT
//     use it for versioned dispatch tests.
//   - Unknown version  -> Core::ERROR_INVALID_RANGE
//   - Unknown method   -> Core::ERROR_UNKNOWN_METHOD
//   - exists built-in  -> returns "0" (found) or "22" (not found) as strings.
//
// JSONRPCSupportsEventStatus uses virtual inheritance of PluginHost::JSONRPC.
// The most-derived class must explicitly initialize PluginHost::JSONRPC(...) in
// its member-initializer list; otherwise the virtual base is default-constructed
// giving _handlers={1} and _validate=nullptr regardless of what the intermediate
// constructor specifies.

#ifndef MODULE_NAME
#define MODULE_NAME ThunderUnitTests
#endif

#include <gtest/gtest.h>
#include <plugins/JSONRPC.h>

using namespace WPEFramework;

// =========================================================================
// Helpers
// =========================================================================

// Full dispatch path: Validate then Invoke via IDispatcher (not ILocalDispatcher).
// Validate checks the TokenCheckFunction; Invoke(nullptr,...) preserves the
// version prefix so Handler() can pick the correct versioned handler.
static uint32_t PluginCall(PluginHost::JSONRPC& plugin, const string& token,
    const string& method, const string& params, string& response)
{
    PluginHost::IDispatcher& disp = plugin;

    // Validate first (TokenCheckFunction); short-circuit on INVALID/DEFERRED.
    uint32_t result = disp.Validate(token, Core::JSONRPC::Message::Method(method), params);
    if (result != Core::ERROR_NONE) {
        return result;
    }

    // Invoke via IDispatcher so the version prefix is preserved for dispatch.
    return disp.Invoke(nullptr, 0, 1, token, method, params, response);
}

// Convenience overload: empty token (no validation).
static uint32_t PluginCall(PluginHost::JSONRPC& plugin,
    const string& method, const string& params, string& response)
{
    return PluginCall(plugin, _T(""), method, params, response);
}

// =========================================================================
// VersioningPlugin — base={2,3,4}, v1=clone+override
//
// Handler layout after construction:
//   _handlers[0] (front) — versions {2, 3, 4}
//       echo   -> "v2response"
//       shared -> "shared"
//       v2only -> "v2only"   (registered AFTER clone)
//   _handlers[1]          — version  {1}  (clone of front at construction time)
//       echo   -> "v1response"  (overridden)
//       shared -> "shared"      (inherited)
//       v2only -> ABSENT        (registered after clone)
// =========================================================================

class VersioningPlugin : public PluginHost::JSONRPCSupportsEventStatus {
public:
    VersioningPlugin()
        : PluginHost::JSONRPC({ 2, 3, 4 })  // Explicitly init virtual base
        , PluginHost::JSONRPCSupportsEventStatus({ 2, 3, 4 })
    {
        Register<Core::JSON::String, Core::JSON::String>(
            _T("echo"), &VersioningPlugin::EchoV2, this);
        Register<Core::JSON::String, Core::JSON::String>(
            _T("shared"), &VersioningPlugin::Shared, this);

        // Clone to v1; at this point v1 inherits echo and shared.
        Core::JSONRPC::Handler& v1 = JSONRPC::CreateHandler({ 1 }, *this);

        // Override echo on v1.
        v1.Register<Core::JSON::String, Core::JSON::String>(
            _T("echo"), &VersioningPlugin::EchoV1, this);

        // Register v2only AFTER the clone — v1 will not have it.
        Register<Core::JSON::String, Core::JSON::String>(
            _T("v2only"), &VersioningPlugin::V2Only, this);
    }

    ~VersioningPlugin() override
    {
        Core::JSONRPC::Handler* v1 = JSONRPC::GetHandler(1);
        if (v1 != nullptr) {
            v1->Unregister(_T("echo"));
        }
        Unregister(_T("v2only"));
        Unregister(_T("shared"));
        Unregister(_T("echo"));
    }

    uint32_t EchoV2(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("v2response");
        return Core::ERROR_NONE;
    }
    uint32_t EchoV1(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("v1response");
        return Core::ERROR_NONE;
    }
    uint32_t Shared(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("shared");
        return Core::ERROR_NONE;
    }
    uint32_t V2Only(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("v2only");
        return Core::ERROR_NONE;
    }

    BEGIN_INTERFACE_MAP(VersioningPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    uint32_t AddRef() const override { return Core::ERROR_COMPOSIT_OBJECT; }
    uint32_t Release() const override { return Core::ERROR_COMPOSIT_OBJECT; }
};

class TestVersioningMain : public ::testing::Test {
protected:
    void SetUp() override    { _plugin = new VersioningPlugin(); }
    void TearDown() override { delete _plugin; _plugin = nullptr; }

    uint32_t Call(const string& method, const string& params, string& response)
    {
        return PluginCall(*_plugin, method, params, response);
    }

    VersioningPlugin* _plugin{ nullptr };
};

// -------------------------------------------------------------------------
// Explicit versioned dispatch
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, ExplicitV2_RoutesToV2Handler)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.echo", "{}", response));
    EXPECT_EQ(response, "\"v2response\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, ExplicitV3_RoutesToSameHandlerAsV2)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("3.echo", "{}", response));
    EXPECT_EQ(response, "\"v2response\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, ExplicitV4_RoutesToSameHandlerAsV2)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("4.echo", "{}", response));
    EXPECT_EQ(response, "\"v2response\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, ExplicitV1_RoutesToV1Handler)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.echo", "{}", response));
    EXPECT_EQ(response, "\"v1response\"") << "Response: " << response;
}

// -------------------------------------------------------------------------
// Unversioned dispatch — must use the FIRST handler (base {2,3,4}), not v1
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, Unversioned_UsesFirstHandler_IsV2_NotV1)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("echo", "{}", response));
    EXPECT_EQ(response, "\"v2response\"")
        << "Unversioned must route to the first (base) handler. "
        << "Response: " << response;
}

// -------------------------------------------------------------------------
// Inherited method on v1 clone
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, V1_InheritedShared_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.shared", "{}", response));
    EXPECT_EQ(response, "\"shared\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, V2_Shared_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.shared", "{}", response));
    EXPECT_EQ(response, "\"shared\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, Unversioned_Shared_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("shared", "{}", response));
    EXPECT_EQ(response, "\"shared\"") << "Response: " << response;
}

// -------------------------------------------------------------------------
// Method registered only after the v1 clone was taken
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, V2Only_ExplicitV2_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.v2only", "{}", response));
    EXPECT_EQ(response, "\"v2only\"") << "Response: " << response;
}

TEST_F(TestVersioningMain, V2Only_ExplicitV1_Rejected)
{
    // v1 was cloned before v2only was registered.
    string response;
    EXPECT_EQ(Core::ERROR_UNKNOWN_METHOD, Call("1.v2only", "{}", response));
}

TEST_F(TestVersioningMain, V2Only_Unversioned_Works_Via_FirstHandler)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("v2only", "{}", response));
    EXPECT_EQ(response, "\"v2only\"") << "Response: " << response;
}

// -------------------------------------------------------------------------
// Unknown version rejection — ERROR_INVALID_RANGE in R4
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, UnknownVersion_99_Rejected)
{
    string response;
    EXPECT_EQ(Core::ERROR_INVALID_RANGE, Call("99.echo", "{}", response));
}

TEST_F(TestVersioningMain, UnknownVersion_5_Rejected)
{
    string response;
    EXPECT_EQ(Core::ERROR_INVALID_RANGE, Call("5.echo", "{}", response));
}

TEST_F(TestVersioningMain, UnknownVersion_0_Rejected)
{
    string response;
    EXPECT_EQ(Core::ERROR_INVALID_RANGE, Call("0.echo", "{}", response));
}

// -------------------------------------------------------------------------
// exists built-in — returns "0" (found) or "22" (not found) in R4
// -------------------------------------------------------------------------

TEST_F(TestVersioningMain, Exists_V2_Echo_ReturnsFound)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.exists", "echo", response));
    EXPECT_EQ(response, std::to_string(Core::ERROR_NONE)) << "Response: " << response;
}

TEST_F(TestVersioningMain, Exists_V1_Echo_ReturnsFound)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.exists", "echo", response));
    EXPECT_EQ(response, std::to_string(Core::ERROR_NONE)) << "Response: " << response;
}

TEST_F(TestVersioningMain, Exists_V1_V2Only_ReturnsNotFound)
{
    // v2only was registered after the v1 clone — absent on v1.
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.exists", "v2only", response));
    EXPECT_NE(response, std::to_string(Core::ERROR_NONE))
        << "v2only must not be visible on v1 handler. Response: " << response;
}

TEST_F(TestVersioningMain, Exists_V2_V2Only_ReturnsFound)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.exists", "v2only", response));
    EXPECT_EQ(response, std::to_string(Core::ERROR_NONE)) << "Response: " << response;
}

// =========================================================================
// Scenario A: Single base handler {1}
// Unversioned and .1. succeed; .2. rejected with ERROR_INVALID_RANGE
// =========================================================================

class ScenarioAPlugin : public PluginHost::JSONRPCSupportsEventStatus {
public:
    // Default constructor: JSONRPC() initializes with {1}.
    // JSONRPCSupportsEventStatus() default-constructs identically.
    ScenarioAPlugin()
        : PluginHost::JSONRPCSupportsEventStatus()
    {
        Register<Core::JSON::String, Core::JSON::String>(
            _T("echo"), &ScenarioAPlugin::Echo, this);
    }

    ~ScenarioAPlugin() override { Unregister(_T("echo")); }

    uint32_t Echo(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("v1only");
        return Core::ERROR_NONE;
    }

    BEGIN_INTERFACE_MAP(ScenarioAPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    uint32_t AddRef() const override { return Core::ERROR_COMPOSIT_OBJECT; }
    uint32_t Release() const override { return Core::ERROR_COMPOSIT_OBJECT; }
};

class TestVersioningScenarioA : public ::testing::Test {
protected:
    void SetUp() override    { _plugin = new ScenarioAPlugin(); }
    void TearDown() override { delete _plugin; _plugin = nullptr; }

    uint32_t Call(const string& method, const string& params, string& response)
    {
        return PluginCall(*_plugin, method, params, response);
    }

    ScenarioAPlugin* _plugin{ nullptr };
};

TEST_F(TestVersioningScenarioA, Unversioned_RoutesToV1)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("echo", "{}", response));
    EXPECT_EQ(response, "\"v1only\"") << "Response: " << response;
}

TEST_F(TestVersioningScenarioA, ExplicitV1_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.echo", "{}", response));
    EXPECT_EQ(response, "\"v1only\"") << "Response: " << response;
}

TEST_F(TestVersioningScenarioA, ExplicitV2_Rejected)
{
    string response;
    EXPECT_EQ(Core::ERROR_INVALID_RANGE, Call("2.echo", "{}", response));
}

// =========================================================================
// Scenario B: Single handler supporting {1, 2}
// Both .1. and .2. succeed with identical results; .3. rejected
// =========================================================================

class ScenarioBPlugin : public PluginHost::JSONRPCSupportsEventStatus {
public:
    ScenarioBPlugin()
        : PluginHost::JSONRPC({ 1, 2 })  // Explicitly init virtual base
        , PluginHost::JSONRPCSupportsEventStatus({ 1, 2 })
    {
        Register<Core::JSON::String, Core::JSON::String>(
            _T("echo"), &ScenarioBPlugin::Echo, this);
    }

    ~ScenarioBPlugin() override { Unregister(_T("echo")); }

    uint32_t Echo(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("same");
        return Core::ERROR_NONE;
    }

    BEGIN_INTERFACE_MAP(ScenarioBPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    uint32_t AddRef() const override { return Core::ERROR_COMPOSIT_OBJECT; }
    uint32_t Release() const override { return Core::ERROR_COMPOSIT_OBJECT; }
};

class TestVersioningScenarioB : public ::testing::Test {
protected:
    void SetUp() override    { _plugin = new ScenarioBPlugin(); }
    void TearDown() override { delete _plugin; _plugin = nullptr; }

    uint32_t Call(const string& method, const string& params, string& response)
    {
        return PluginCall(*_plugin, method, params, response);
    }

    ScenarioBPlugin* _plugin{ nullptr };
};

TEST_F(TestVersioningScenarioB, ExplicitV1_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.echo", "{}", response));
    EXPECT_EQ(response, "\"same\"") << "Response: " << response;
}

TEST_F(TestVersioningScenarioB, ExplicitV2_Works)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("2.echo", "{}", response));
    EXPECT_EQ(response, "\"same\"") << "Response: " << response;
}

TEST_F(TestVersioningScenarioB, V1AndV2_ReturnIdenticalResult)
{
    string v1Response, v2Response;
    EXPECT_EQ(Core::ERROR_NONE, Call("1.echo", "{}", v1Response));
    EXPECT_EQ(Core::ERROR_NONE, Call("2.echo", "{}", v2Response));
    EXPECT_EQ(v1Response, v2Response)
        << "Both versions must return identical results from the same handler.";
}

TEST_F(TestVersioningScenarioB, Unversioned_UsesFirstHandler)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("echo", "{}", response));
    EXPECT_EQ(response, "\"same\"") << "Response: " << response;
}

TEST_F(TestVersioningScenarioB, ExplicitV3_Rejected)
{
    string response;
    EXPECT_EQ(Core::ERROR_INVALID_RANGE, Call("3.echo", "{}", response));
}

// =========================================================================
// VersionedValidationPlugin — base={2,3,4} + TokenCheckFunction + v1 clone
//
// Models the ThunderNanoServices JSONRPCPlugin example:
//   token == "deferred"    -> classification::DEFERRED
//   method == "restricted" -> classification::INVALID
//   otherwise              -> classification::VALID
// =========================================================================

class VersionedValidationPlugin : public PluginHost::JSONRPCSupportsEventStatus {
public:
    VersionedValidationPlugin()
        : PluginHost::JSONRPC(  // Explicitly init virtual base with validation fn
            { 2, 3, 4 },
            [](const string& token, const string& method, const string& /*params*/)
                -> PluginHost::JSONRPC::classification {
                if (token == _T("deferred")) {
                    return PluginHost::JSONRPC::classification::DEFERRED;
                }
                if (method == _T("restricted")) {
                    return PluginHost::JSONRPC::classification::INVALID;
                }
                return PluginHost::JSONRPC::classification::VALID;
            })
        , PluginHost::JSONRPCSupportsEventStatus()
    {
        Register<Core::JSON::String, Core::JSON::String>(
            _T("open"), &VersionedValidationPlugin::Open, this);
        Register<Core::JSON::String, Core::JSON::String>(
            _T("restricted"), &VersionedValidationPlugin::Restricted, this);

        // Clone to v1; TokenCheckFunction is on the JSONRPC instance so it
        // fires for v1 calls as well.
        Core::JSONRPC::Handler& v1 = JSONRPC::CreateHandler({ 1 }, *this);

        // Override "open" on v1 to distinguish which handler was hit.
        v1.Register<Core::JSON::String, Core::JSON::String>(
            _T("open"), &VersionedValidationPlugin::OpenV1, this);
    }

    ~VersionedValidationPlugin() override
    {
        Core::JSONRPC::Handler* v1 = JSONRPC::GetHandler(1);
        if (v1 != nullptr) {
            v1->Unregister(_T("open"));
        }
        Unregister(_T("restricted"));
        Unregister(_T("open"));
    }

    uint32_t Open(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("open_v2");
        return Core::ERROR_NONE;
    }
    uint32_t OpenV1(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("open_v1");
        return Core::ERROR_NONE;
    }
    uint32_t Restricted(const Core::JSON::String& /*in*/, Core::JSON::String& out)
    {
        out = _T("should_never_reach_here");
        return Core::ERROR_NONE;
    }

    BEGIN_INTERFACE_MAP(VersionedValidationPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    uint32_t AddRef() const override { return Core::ERROR_COMPOSIT_OBJECT; }
    uint32_t Release() const override { return Core::ERROR_COMPOSIT_OBJECT; }
};

class TestVersioningWithValidation : public ::testing::Test {
protected:
    void SetUp() override    { _plugin = new VersionedValidationPlugin(); }
    void TearDown() override { delete _plugin; _plugin = nullptr; }

    uint32_t Call(const string& token, const string& method,
                  const string& params, string& response)
    {
        return PluginCall(*_plugin, token, method, params, response);
    }

    VersionedValidationPlugin* _plugin{ nullptr };
};

// -------------------------------------------------------------------------
// classification::VALID — permitted calls execute normally
// -------------------------------------------------------------------------

TEST_F(TestVersioningWithValidation, ValidToken_Open_Succeeds)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("valid_token", "open", "{}", response));
    EXPECT_EQ(response, "\"open_v2\"") << "Response: " << response;
}

TEST_F(TestVersioningWithValidation, ValidToken_VersionedOpen_Succeeds)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("valid_token", "2.open", "{}", response));
    EXPECT_EQ(response, "\"open_v2\"") << "Response: " << response;
}

TEST_F(TestVersioningWithValidation, ValidToken_V1Open_Succeeds)
{
    // Validation passes; v1 handler serves the overridden response.
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("valid_token", "1.open", "{}", response));
    EXPECT_EQ(response, "\"open_v1\"") << "Response: " << response;
}

TEST_F(TestVersioningWithValidation, EmptyToken_Open_Succeeds)
{
    string response;
    EXPECT_EQ(Core::ERROR_NONE, Call("", "open", "{}", response));
    EXPECT_EQ(response, "\"open_v2\"") << "Response: " << response;
}

// -------------------------------------------------------------------------
// classification::INVALID — returns ERROR_PRIVILIGED_REQUEST
// -------------------------------------------------------------------------

TEST_F(TestVersioningWithValidation, ValidToken_RestrictedMethod_Blocked)
{
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_REQUEST, Call("valid_token", "restricted", "{}", response));
}

TEST_F(TestVersioningWithValidation, ValidToken_VersionedRestrictedMethod_Blocked)
{
    // Version prefix does not bypass validation; Validate receives "restricted".
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_REQUEST, Call("valid_token", "2.restricted", "{}", response));
}

TEST_F(TestVersioningWithValidation, ValidToken_V1RestrictedMethod_Blocked)
{
    // TokenCheckFunction is on the JSONRPC instance, fires for v1 calls too.
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_REQUEST, Call("valid_token", "1.restricted", "{}", response));
}

TEST_F(TestVersioningWithValidation, Validation_FiresBeforeHandlerDispatch)
{
    // "restricted" on version 99 (no handler) still returns PRIVILIGED_REQUEST,
    // not INVALID_RANGE, confirming Validate fires before Handler lookup.
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_REQUEST, Call("valid_token", "99.restricted", "{}", response));
}

// -------------------------------------------------------------------------
// classification::DEFERRED — returns ERROR_PRIVILIGED_DEFERRED
// -------------------------------------------------------------------------

TEST_F(TestVersioningWithValidation, DeferredToken_OpenMethod_Deferred)
{
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_DEFERRED, Call("deferred", "open", "{}", response));
}

TEST_F(TestVersioningWithValidation, DeferredToken_VersionedCall_Deferred)
{
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_DEFERRED, Call("deferred", "2.open", "{}", response));
}

TEST_F(TestVersioningWithValidation, DeferredToken_V1Call_Deferred)
{
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_DEFERRED, Call("deferred", "1.open", "{}", response));
}

TEST_F(TestVersioningWithValidation, DeferredToken_FiresBeforeVersionCheck)
{
    // DEFERRED fires in Validate before Invoke is called.
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_DEFERRED, Call("deferred", "99.open", "{}", response));
}

TEST_F(TestVersioningWithValidation, DeferredToken_TakesPriorityOverMethodBlock)
{
    // "restricted" would be INVALID but DEFERRED is checked first in our lambda.
    string response;
    EXPECT_EQ(Core::ERROR_PRIVILIGED_DEFERRED, Call("deferred", "restricted", "{}", response));
}

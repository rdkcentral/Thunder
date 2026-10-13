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

namespace Thunder {
namespace Tests {
namespace Core {

    class RichErrorData : public ::Thunder::Core::JSON::Container {
    public:
        RichErrorData()
            : ::Thunder::Core::JSON::Container()
            , Subcode(0)
            , Reason()
        {
            Add(_T("subcode"), &Subcode);
            Add(_T("reason"), &Reason);
        }

        RichErrorData(const RichErrorData& copy)
            : ::Thunder::Core::JSON::Container()
            , Subcode(copy.Subcode)
            , Reason(copy.Reason)
        {
            Add(_T("subcode"), &Subcode);
            Add(_T("reason"), &Reason);
        }

        ~RichErrorData() override = default;

        ::Thunder::Core::JSON::DecUInt16 Subcode;
        ::Thunder::Core::JSON::String Reason;
    };

    class RichError : public ::Thunder::Core::JSON::Container {
    public:
        RichError()
            : ::Thunder::Core::JSON::Container()
            , Code(0)
            , Message()
            , Data()
        {
            Add(_T("code"), &Code);
            Add(_T("message"), &Message);
            Add(_T("data"), &Data);
        }

        RichError(const RichError& copy)
            : ::Thunder::Core::JSON::Container()
            , Code(copy.Code)
            , Message(copy.Message)
            , Data(copy.Data)
        {
            Add(_T("code"), &Code);
            Add(_T("message"), &Message);
            Add(_T("data"), &Data);
        }

        ~RichError() override = default;

        ::Thunder::Core::JSON::DecSInt32 Code;
        ::Thunder::Core::JSON::String Message;
        RichErrorData Data;
    };

    class RichInput : public ::Thunder::Core::JSON::Container {
    public:
        RichInput()
            : ::Thunder::Core::JSON::Container()
            , Input()
        {
            Add(_T("input"), &Input);
        }

        RichInput(const RichInput& copy)
            : ::Thunder::Core::JSON::Container()
            , Input(copy.Input)
        {
            Add(_T("input"), &Input);
        }

        ~RichInput() override = default;

        ::Thunder::Core::JSON::String Input;
    };

    static ::Thunder::Core::JSONRPC::Context RichErrorContext()
    {
        return (::Thunder::Core::JSONRPC::Context(1, 1, string()));
    }

    static ::Thunder::Core::JSONRPC::Message::Info ParseRichError(const string& response)
    {
        ::Thunder::Core::JSONRPC::Message::Info error;
        ::Thunder::Core::OptionalType<::Thunder::Core::JSON::Error> report;
        error.FromString(response, report);
        EXPECT_FALSE(report.IsSet());
        return (error);
    }

    TEST(Core_JSONRPC_RichError, InboundOnly_DataOnlyPreservesThunderCodeAndMessage)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        bool invoked = false;

        handler.RegisterWithError<RichInput, void, RichError>("inbound", [&invoked](const RichInput& parameters, RichError& details) -> uint32_t {
            invoked = true;
            EXPECT_EQ(parameters.Input.Value(), string("request"));
            details.Data.Subcode = 42;
            details.Data.Reason = _T("additional context");
            return (::Thunder::Core::ERROR_BAD_REQUEST);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("inbound"), _T("{\"input\":\"request\"}"), response);

        EXPECT_TRUE(invoked);
        EXPECT_EQ(result, ::Thunder::Core::ERROR_BAD_REQUEST);

        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);
        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());

        RichErrorData data;
        data.FromString(error.Data.Value());
        EXPECT_EQ(data.Subcode.Value(), 42);
        EXPECT_EQ(data.Reason.Value(), string("additional context"));
    }

    TEST(Core_JSONRPC_RichError, OutboundOnly_CodeOnlyPreservesThunderMessage)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        const int32_t customCode = -32042;

        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>("outbound", [customCode](::Thunder::Core::JSON::String& output, RichError& details) -> uint32_t {
            output = _T("must not be returned");
            details.Code = customCode;
            return (::Thunder::Core::ERROR_GENERAL);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("outbound"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_GENERAL);
        EXPECT_EQ(error.Code.Value(), customCode);
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());
        EXPECT_EQ(response.find("must not be returned"), string::npos);
    }

    TEST(Core_JSONRPC_RichError, InboundOutbound_SuccessReturnsOutput)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });

        handler.RegisterWithError<RichInput, ::Thunder::Core::JSON::String, RichError>("transform", [](const RichInput& parameters, ::Thunder::Core::JSON::String& output, RichError&) -> uint32_t {
            output = parameters.Input.Value() + _T(" transformed");
            return (::Thunder::Core::ERROR_NONE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("transform"), _T("{\"input\":\"request\"}"), response);
        ::Thunder::Core::JSON::String output;
        output.FromString(response);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(output.Value(), string("request transformed"));
    }

    TEST(Core_JSONRPC_RichError, InboundOutbound_MessageOnlyPreservesThunderCode)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });

        handler.RegisterWithError<RichInput, ::Thunder::Core::JSON::String, RichError>("transform", [](const RichInput&, ::Thunder::Core::JSON::String&, RichError& details) -> uint32_t {
            details.Message = _T("custom message");
            return (::Thunder::Core::ERROR_UNAVAILABLE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("transform"), _T("{\"input\":\"request\"}"), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), string("custom message"));
    }

    TEST(Core_JSONRPC_RichError, MalformedInboundDoesNotInvokeImplementation)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        bool invoked = false;

        handler.RegisterWithError<RichInput, void, RichError>("inbound", [&invoked](const RichInput&, RichError&) -> uint32_t {
            invoked = true;
            return (::Thunder::Core::ERROR_NONE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("inbound"), _T("{\"input\":"), response);

        EXPECT_FALSE(invoked);
        EXPECT_EQ(result, ::Thunder::Core::ERROR_PARSE_FAILURE);
        EXPECT_FALSE(response.empty());
    }

    TEST(Core_JSONRPC_RichError, Property_DataOnlyPreservesThunderDefaults)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });

        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>("value", [](::Thunder::Core::JSON::String&, RichError& details) -> uint32_t {
            details.Data.Reason = _T("property unavailable");
            return (::Thunder::Core::ERROR_UNAVAILABLE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("value"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());
        EXPECT_TRUE(error.Data.IsSet());
    }

    TEST(Core_JSONRPC_RichError, IndexedProperty_PassesIndexAndPreservesThunderDefaults)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        string receivedIndex;

        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>("entry", [&receivedIndex](const string& index, ::Thunder::Core::JSON::String&, RichError& details) -> uint32_t {
            receivedIndex = index;
            details.Data.Reason = _T("indexed property unavailable");
            return (::Thunder::Core::ERROR_UNAVAILABLE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("entry@requested-index"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(receivedIndex, string("requested-index"));
        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());
        EXPECT_TRUE(error.Data.IsSet());
    }

} // namespace Core
} // namespace Tests
} // namespace Thunder

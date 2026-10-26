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

    class RichErrorWithoutData : public ::Thunder::Core::JSON::Container {
    public:
        RichErrorWithoutData()
            : ::Thunder::Core::JSON::Container()
            , Code(0)
            , Message()
        {
            Add(_T("code"), &Code);
            Add(_T("message"), &Message);
        }

        RichErrorWithoutData(const RichErrorWithoutData& copy)
            : ::Thunder::Core::JSON::Container()
            , Code(copy.Code)
            , Message(copy.Message)
        {
            Add(_T("code"), &Code);
            Add(_T("message"), &Message);
        }

        ~RichErrorWithoutData() override = default;

        ::Thunder::Core::JSON::DecSInt32 Code;
        ::Thunder::Core::JSON::String Message;
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

    class RichErrorService {
    public:
        RichErrorService()
            : _invocations(0)
            , _input()
            , _index()
        {
        }

        uint32_t NoPayload(RichError&)
        {
            _invocations |= 0x01;
            return (::Thunder::Core::ERROR_NONE);
        }

        uint32_t Inbound(const RichInput& parameters, RichError&)
        {
            _invocations |= 0x02;
            _input = parameters.Input.Value();
            return (::Thunder::Core::ERROR_NONE);
        }

        uint32_t Outbound(::Thunder::Core::JSON::String& output, RichError&)
        {
            _invocations |= 0x04;
            output = _T("member output");
            return (::Thunder::Core::ERROR_NONE);
        }

        uint32_t Transform(const RichInput& parameters, ::Thunder::Core::JSON::String& output, RichError&)
        {
            _invocations |= 0x08;
            output = parameters.Input.Value() + _T(" member transformed");
            return (::Thunder::Core::ERROR_NONE);
        }

        uint32_t IndexedFailure(const string& index, ::Thunder::Core::JSON::String&, RichError& details)
        {
            _index = index;
            details.Data.Subcode = 73;
            details.Data.Reason = _T("member indexed failure");
            return (::Thunder::Core::ERROR_UNAVAILABLE);
        }

        uint8_t Invocations() const
        {
            return (_invocations);
        }

        const string& Input() const
        {
            return (_input);
        }

        const string& Index() const
        {
            return (_index);
        }

    private:
        uint8_t _invocations;
        string _input;
        string _index;
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

    static uint32_t FreeAction()
    {
        return (::Thunder::Core::ERROR_NONE);
    }

    static uint32_t FreeRichAction(RichError& details)
    {
        details.Message = _T("free function error");
        return (::Thunder::Core::ERROR_UNAVAILABLE);
    }

    static uint32_t FreeContextRichAction(const ::Thunder::Core::JSONRPC::Context& context, RichError& details)
    {
        details.Message = (context.ChannelId() == 47 ? _T("free context error") : _T("unexpected context"));
        return (::Thunder::Core::ERROR_UNAVAILABLE);
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

    TEST(Core_JSONRPC_RichError, ErrorWithoutData_MessageOnlyPreservesThunderCodeAndOmitsData)
    {
        ::Thunder::Core::ErrorDetails<> nativeDetails;
        nativeDetails.message = _T("native message");
        EXPECT_TRUE(nativeDetails.message.IsSet());

        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.RegisterWithError<void, void, RichErrorWithoutData>(_T("withoutData"), [](RichErrorWithoutData& details) -> uint32_t {
            details.Message = _T("message without data");
            return (::Thunder::Core::ERROR_UNAVAILABLE);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("withoutData"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), string("message without data"));
        EXPECT_FALSE(error.Data.IsSet());
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

    TEST(Core_JSONRPC_RichError, IndexedHandler_PassesIndexAndPreservesThunderDefaults)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        string receivedIndex;

        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>("entry", [&receivedIndex](const string& index, ::Thunder::Core::JSON::String&, RichError& details) -> uint32_t {
            receivedIndex = index;
            details.Data.Reason = _T("indexed handler unavailable");
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

    TEST(Core_JSONRPC_RichError, RealObject_AllInputOutputFormsInvokeTarget)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        RichErrorService service;
        string response;

        handler.RegisterWithError<void, void, RichError>(_T("memberNone"), &RichErrorService::NoPayload, &service);
        handler.RegisterWithError<RichInput, void, RichError>(_T("memberInbound"), &RichErrorService::Inbound, &service);
        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>(_T("memberOutbound"), &RichErrorService::Outbound, &service);
        handler.RegisterWithError<RichInput, ::Thunder::Core::JSON::String, RichError>(_T("memberTransform"), &RichErrorService::Transform, &service);

        EXPECT_EQ(handler.Invoke(RichErrorContext(), _T("memberNone"), string(), response), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(response.empty());

        EXPECT_EQ(handler.Invoke(RichErrorContext(), _T("memberInbound"), _T("{\"input\":\"member input\"}"), response), ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(response.empty());
        EXPECT_EQ(service.Input(), string("member input"));

        EXPECT_EQ(handler.Invoke(RichErrorContext(), _T("memberOutbound"), string(), response), ::Thunder::Core::ERROR_NONE);
        ::Thunder::Core::JSON::String output;
        output.FromString(response);
        EXPECT_EQ(output.Value(), string("member output"));

        EXPECT_EQ(handler.Invoke(RichErrorContext(), _T("memberTransform"), _T("{\"input\":\"request\"}"), response), ::Thunder::Core::ERROR_NONE);
        output.Clear();
        output.FromString(response);
        EXPECT_EQ(output.Value(), string("request member transformed"));
        EXPECT_EQ(service.Invocations(), 0x0F);
    }

    TEST(Core_JSONRPC_RichError, RealObject_IndexedFailureSerializesDetailsAndPreservesDefaults)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        RichErrorService service;

        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>(_T("memberEntry"), &RichErrorService::IndexedFailure, &service);

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("memberEntry@member-index"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNAVAILABLE);
        EXPECT_EQ(service.Index(), string("member-index"));
        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());

        RichErrorData data;
        data.FromString(error.Data.Value());
        EXPECT_EQ(data.Subcode.Value(), 73);
        EXPECT_EQ(data.Reason.Value(), string("member indexed failure"));
    }

    TEST(Core_JSONRPC_RichError, DirectLambda_ContextSelectsContextDispatch)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        uint32_t receivedChannelId = 0;

        handler.RegisterWithError<void, void, RichError>(_T("withContext"), [&receivedChannelId](const ::Thunder::Core::JSONRPC::Context& context, RichError&) -> uint32_t {
            receivedChannelId = context.ChannelId();
            return (::Thunder::Core::ERROR_NONE);
        });

        string response;
        const ::Thunder::Core::JSONRPC::Context context(47, 1, string());
        const uint32_t result = handler.Invoke(context, _T("withContext"), string(), response);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(receivedChannelId, 47u);
    }

    TEST(Core_JSONRPC_RichError, Context_AllInputOutputFormsSelectContextDispatch)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        uint8_t invocations = 0;

        handler.RegisterWithError<RichInput, void, RichError>(_T("contextInbound"), [&invocations](const ::Thunder::Core::JSONRPC::Context& context, const RichInput& input, RichError&) -> uint32_t {
            EXPECT_EQ(context.ChannelId(), 47u);
            EXPECT_EQ(input.Input.Value(), string("inbound"));
            invocations |= 0x01;
            return (::Thunder::Core::ERROR_NONE);
        });
        handler.RegisterWithError<void, ::Thunder::Core::JSON::String, RichError>(_T("contextOutbound"), [&invocations](const ::Thunder::Core::JSONRPC::Context& context, ::Thunder::Core::JSON::String& output, RichError&) -> uint32_t {
            EXPECT_EQ(context.ChannelId(), 47u);
            output = _T("outbound");
            invocations |= 0x02;
            return (::Thunder::Core::ERROR_NONE);
        });
        handler.RegisterWithError<RichInput, ::Thunder::Core::JSON::String, RichError>(_T("contextTransform"), [&invocations](const ::Thunder::Core::JSONRPC::Context& context, const RichInput& input, ::Thunder::Core::JSON::String& output, RichError&) -> uint32_t {
            EXPECT_EQ(context.ChannelId(), 47u);
            output = input.Input.Value() + _T(" transformed");
            invocations |= 0x04;
            return (::Thunder::Core::ERROR_NONE);
        });

        const ::Thunder::Core::JSONRPC::Context context(47, 1, string());
        string response;
        EXPECT_EQ(handler.Invoke(context, _T("contextInbound"), _T("{\"input\":\"inbound\"}"), response), ::Thunder::Core::ERROR_NONE);
        EXPECT_EQ(handler.Invoke(context, _T("contextOutbound"), string(), response), ::Thunder::Core::ERROR_NONE);
        ::Thunder::Core::JSON::String output;
        output.FromString(response);
        EXPECT_EQ(output.Value(), string("outbound"));
        EXPECT_EQ(handler.Invoke(context, _T("contextTransform"), _T("{\"input\":\"input\"}"), response), ::Thunder::Core::ERROR_NONE);
        output.Clear();
        output.FromString(response);
        EXPECT_EQ(output.Value(), string("input transformed"));
        EXPECT_EQ(invocations, 0x07);
    }

    TEST(Core_JSONRPC_RichError, IndexedInboundHandler_PassesIndexAndPreservesThunderMessage)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        string receivedIndex;
        handler.RegisterWithError<::Thunder::Core::JSON::String, void, RichError>(_T("indexedInbound"), [&receivedIndex](const string& index, const ::Thunder::Core::JSON::String& value, RichError& details) -> uint32_t {
            receivedIndex = index;
            EXPECT_EQ(value.Value(), string("setter value"));
            details.Code = -32168;
            return (::Thunder::Core::ERROR_BAD_REQUEST);
        });

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("indexedInbound@requested-index"), _T("\"setter value\""), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(receivedIndex, string("requested-index"));
        EXPECT_EQ(error.Code.Value(), -32168);
        EXPECT_EQ(error.Text.Value(), expected.Text.Value());
    }

    TEST(Core_JSONRPC_RichError, FreeFunctionByName_RegularRegistrationUsesFunctionPointerTraits)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.Register<void, void>(_T("freeAction"), FreeAction);

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("freeAction"), string(), response);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(response.empty());
    }

    TEST(Core_JSONRPC_RichError, FreeFunctionByName_RichErrorRegistrationUsesFunctionPointerTraits)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.RegisterWithError<void, void, RichError>(_T("freeRichAction"), FreeRichAction);

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("freeRichAction"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNAVAILABLE);
        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), string("free function error"));
    }

    TEST(Core_JSONRPC_RichError, FreeFunctionByName_ContextSelectsContextDispatch)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.RegisterWithError<void, void, RichError>(_T("freeContextAction"), FreeContextRichAction);

        string response;
        const ::Thunder::Core::JSONRPC::Context context(47, 1, string());
        const uint32_t result = handler.Invoke(context, _T("freeContextAction"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));

        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNAVAILABLE);
        EXPECT_EQ(error.Text.Value(), string("free context error"));
    }

    TEST(Core_JSONRPC_RichError, StdBind_RegularRegistrationRetainsNoPrefixDispatch)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.Register<void, void>(_T("boundAction"), std::bind(FreeAction));

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("boundAction"), string(), response);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_NONE);
        EXPECT_TRUE(response.empty());
    }

    TEST(Core_JSONRPC_RichError, StdBind_RichErrorRegistrationRetainsNoPrefixDispatch)
    {
        ::Thunder::Core::JSONRPC::Handler handler(std::vector<uint8_t>{ 1 });
        handler.RegisterWithError<void, void, RichError>(_T("boundRichAction"), std::bind(FreeRichAction, std::placeholders::_1));

        string response;
        const uint32_t result = handler.Invoke(RichErrorContext(), _T("boundRichAction"), string(), response);
        const ::Thunder::Core::JSONRPC::Message::Info error(ParseRichError(response));
        ::Thunder::Core::JSONRPC::Message::Info expected;
        expected.SetError(result);

        EXPECT_EQ(result, ::Thunder::Core::ERROR_UNAVAILABLE);
        EXPECT_EQ(error.Code.Value(), expected.Code.Value());
        EXPECT_EQ(error.Text.Value(), string("free function error"));
    }

} // namespace Core
} // namespace Tests
} // namespace Thunder

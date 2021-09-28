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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

enum class CommandType {
    ENUM_1,
    ENUM_2,
    ENUM_3,
    ENUM_4
};

namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(CommandType)
        { CommandType::ENUM_1, _TXT("enum_1") },
        { CommandType::ENUM_2, _TXT("enum_2") },
        { CommandType::ENUM_3, _TXT("enum_3") },
        { CommandType::ENUM_4, _TXT("enum_4") },
    ENUM_CONVERSION_END(CommandType)

}

class CommandParameters : public WPEFramework::Core::JSON::Container {

public:
    CommandParameters(const CommandParameters&) = delete;
    CommandParameters& operator=(const CommandParameters&) = delete;

    CommandParameters()
        : Core::JSON::Container()
        , G(00)
        , H(0)
        , I()
        , J()
        , K(1)
        , L(0.0)
    {
        Add(_T("g"), &G);
        Add(_T("h"), &H);
        Add(_T("i"), &I);
        Add(_T("j"), &J);
        Add(_T("k"), &K);
        Add(_T("l"), &L);
    }

    ~CommandParameters()
    {
    }

public:
    WPEFramework::Core::JSON::OctSInt16 G;
    WPEFramework::Core::JSON::DecSInt16 H;
    WPEFramework::Core::JSON::EnumType<CommandType> I;
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::DecUInt16> J;
    WPEFramework::Core::JSON::Float K;
    WPEFramework::Core::JSON::Double L;
};

class CommandRequest : public WPEFramework::Core::JSON::Container {
public:
    CommandRequest(const CommandRequest&) = delete;
    CommandRequest& operator=(const CommandRequest&) = delete;

public:
    CommandRequest()
        : Core::JSON::Container()
        , A(0x0)
        , B()
        , C(0x0)
        , D(false)
        , E(00)
        , F()
        , M()
        , N(0.0)
        , O(0.0)
    {
        Add(_T("a"), &A);
        Add(_T("b"), &B);
        Add(_T("c"), &C);
        Add(_T("d"), &D);
        Add(_T("e"), &E);
        Add(_T("f"), &F);
        Add(_T("m"), &M);
        Add(_T("n"), &N);
        Add(_T("o"), &O);
    }

    ~CommandRequest()
    {
    }

    void Clear()
    {
        WPEFramework::Core::JSON::Container::Clear();
    }

public:
    WPEFramework::Core::JSON::HexSInt32 A;
    WPEFramework::Core::JSON::String B;
    WPEFramework::Core::JSON::HexUInt32 C;
    WPEFramework::Core::JSON::Boolean D;
    WPEFramework::Core::JSON::OctUInt16 E;
    CommandParameters F;
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::String> M;
    WPEFramework::Core::JSON::Float N;
    WPEFramework::Core::JSON::Double O;
};

TEST(Core_JSON, simpleSet)
{
    {
        //Tester
        string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"],"k":"1.1","l":"2.11"},"m":["Test"],"n":"3.2","o":"-65.22"})";
        string inputRequired = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22],"k":1.1,"l":2.11},"m":["Test"],"n":3.2,"o":-65.22})";
        string output;
        WPEFramework::Core::ProxyType<CommandRequest> command = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        command->A = -90;
        command->B = _T("TestIdentifier");
        command->C = 90;
        command->D = true;
        command->E = 12;
        command->F.G = -12;
        command->F.H = -44;
        command->F.I = CommandType::ENUM_4;
        command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(6, true));
        command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(14, true));
        command->F.J.Add(WPEFramework::Core::JSON::DecUInt16(22, true));
        command->F.K = 1.1;
        command->F.L = 2.11;
        command->N = 3.2;
        command->O = -65.22;

        WPEFramework::Core::JSON::String str;
        str = string("Test");
        command->M.Add(str);
        WPEFramework::Core::JSON::Tester<1, CommandRequest> parser;
        //ToString
        parser.ToString(command, output);
        EXPECT_STREQ(inputRequired.c_str(), output.c_str());
        //FromString
        WPEFramework::Core::ProxyType<CommandRequest> received = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
        EXPECT_STREQ(inputRequired.c_str(), output.c_str());

        parser.FromString(inputRequired, received);
        output.clear();
        parser.ToString(received, output);
        EXPECT_STREQ(inputRequired.c_str(), output.c_str());

        //ArrayType Iterator
        WPEFramework::Core::JSON::ArrayType<Core::JSON::DecUInt16>::Iterator settings(command->F.J.Elements());
        for(int i = 0; settings.Next(); i++)
            EXPECT_EQ(settings.Current().Value(), command->F.J[i]);
        //null test
        input = R"({"a":null,"b":null,"c":"0x5A","d":null,"f":{"g":"-014","h":-44}})";
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
        //EXPECT_STREQ(input.c_str(), output.c_str());
    }
    //JsonObject and JsonValue
    {
        string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22]}})";
        JsonObject command;
        command.FromString(input);
        string output;
        command.ToString(output);
        EXPECT_STREQ(input.c_str(), output.c_str());

        JsonObject object;
        object["g"] = "-014";
        object["h"] = -44;
        object["i"] = "enum_4";
        JsonArray arrayValue;
        arrayValue.Add(6);
        arrayValue.Add(14);
        arrayValue.Add(22);
        object["j"] = arrayValue;
        JsonObject demoObject;
        demoObject["a"] = "-0x5A";
        demoObject["b"] = "TestIdentifier";
        demoObject["c"] = "0x5A";
        demoObject["d"] = true;
        demoObject["e"] = "014";
        demoObject["f"] = object;
        string serialized;
        demoObject.ToString(serialized);
        EXPECT_STREQ(input.c_str(), serialized.c_str());

        JsonObject::Iterator index = demoObject.Variants();
        while (index.Next()) {
            JsonValue value(demoObject.Get(index.Label()));
            EXPECT_EQ(value.Content(), index.Current().Content());
            EXPECT_STREQ(value.Value().c_str(), index.Current().Value().c_str());
        }
    }
    //JsonObject Serialization and Deserialization with escape sequences
    {
        class StringContainer : public Core::JSON::Container {
        public:
            StringContainer(const StringContainer&) = delete;
            StringContainer& operator=(const StringContainer&) = delete;

            StringContainer()
                : Name(_T(""))
            {
                Add(_T("name"), &Name);
            }
            virtual ~StringContainer()
            {
            }

        public:
            Core::JSON::String Name;
        } strInput, strOutput;

        JsonObject command;
        string output, input;

        input = R"({"method":"WifiControl.1.config@Test\\","params":{"ssid":"Test\\"}})";
        command.FromString(input);
        command.ToString(output);
        printf("\n\n Case 1: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"jsonrpc":"2.0","id":1234567890,"method1":"Te\\st","params":{"ssid":"Te\\st","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
        command.FromString(input);
        command.ToString(output);
        printf("\n\n Case 2: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.config@Test","params":{"ssid":"Test\\","type":"WPA2","hidden":false,"accesspoint":false,"psk":"12345678"}})";
        command.FromString(input);
        command.ToString(output);
        printf("\n\n Case 3: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"jsonrpc":"2.0","id":1234567890,"method":"hPho\\ne","params":{"ssid":"iPh\one"}})";
        command.FromString(input);
        command.ToString(output);
        printf("\n\n Case 4: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n\n\n", output.length(), output.c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"("hPh\\one")";
        Core::JSON::String str;
        str.FromString(input);
        str.ToString(output);
        printf("\n\n Case 5: \n");
        printf("string --- = %s \n", str.Value().c_str());
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"Iphone\\"})";
        strInput.Name = R"(Iphone\)";
        strOutput.FromString(input);
        printf("\n\n Case 6: \n");
        printf("strInput --- = %s \n", strInput.Name.Value().c_str());
        printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

        strInput.Name = R"(Ipho\ne)";
        strOutput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 7: \n");
        printf("strInput --- = %s \n", strInput.Name.Value().c_str());
        printf("strOutput --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

        strInput.Name = R"(Iphone\)";
        printf("\n\n Case 8: \n");
        printf("name --- = %s \n", strInput.Name.Value().c_str());
        strInput.ToString(output);
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        strOutput.FromString(output);
        printf("name --- = %s \n", strInput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

        input = R"({"name":"IPh\\one"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 9: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"IPhone\\\\"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 10: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"IPho\\\\ne\\\\"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 11: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"\\IPh\\one\\\\"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 12: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"\\\\\\IPhone\\\\"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 13: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        input = R"({"name":"IPho\ne"})";
        strInput.FromString(input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 14: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        printf("strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());
        EXPECT_STREQ(input.c_str(), output.c_str());

        char name[] = {73, 80, 104, 111, 13, 101 };
        input = name;
        strInput.Name = (input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 15: \n");
        printf("     input  %zd --- = %s \n", input.length(), input.c_str());
        printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("     output %zd --- = %s \n", output.length(), output.c_str());
        printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

        char escapeSequence = 13;
        input = escapeSequence;
        strInput.Name = (input);
        strInput.ToString(output);
        strOutput.FromString(output);
        printf("\n\n Case 16: \n");
        printf("     input  %zd --- = %s \n", input.length(), input.c_str());
        printf("     strInput.Name --- = %s \n", strInput.Name.Value().c_str());
        printf("     output %zd --- = %s \n", output.length(), output.c_str());
        printf("     strOutput.Name --- = %s \n", strOutput.Name.Value().c_str());
        EXPECT_STREQ(strInput.Name.Value().c_str(), strOutput.Name.Value().c_str());

        escapeSequence = 10;
        input = escapeSequence;
        printf("\n\n Case 17 \n");
        WPEFramework::Core::ProxyType<CommandRequest> commandInput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        WPEFramework::Core::JSON::Tester<1, CommandRequest> parserInput;
        commandInput->B = input;
        output.clear();
        parserInput.ToString(commandInput, output);
        WPEFramework::Core::ProxyType<CommandRequest> commandOutput = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        WPEFramework::Core::JSON::Tester<1, CommandRequest> parserOutput;
        parserOutput.FromString(output, commandOutput);
        EXPECT_STREQ(commandInput->B.Value().c_str(), commandOutput->B.Value().c_str());

        class ParamsInfo : public Core::JSON::Container {
        public:
            ParamsInfo()
                : Core::JSON::Container()
            {
                Add(_T("ssid"), &Ssid);
            }

            ParamsInfo(const ParamsInfo&) = delete;
            ParamsInfo& operator=(const ParamsInfo&) = delete;

        public:
            Core::JSON::String Ssid; // Identifier of a network
        };

        Core::JSONRPC::Message message;
        input = R"({"jsonrpc":"2.0","id":1234567890,"method":"WifiControl.1.connect","params":{"ssid":"iPhone\\"}})";
        message.FromString(input);
        message.ToString(output);
        EXPECT_STREQ(input.c_str(), output.c_str());
        printf("\n\n Case 18: \n");
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());
        ParamsInfo paramsInfo;
        message.Parameters.ToString(input);
        paramsInfo.FromString(message.Parameters.Value());
        paramsInfo.ToString(output);
        printf("message.params = %s\n", message.Parameters.Value().c_str());
        printf("message.params.ssid = %s\n", paramsInfo.Ssid.Value().c_str());
        printf("input  %zd --- = %s \n", input.length(), input.c_str());
        printf("output %zd --- = %s \n", output.length(), output.c_str());

        EXPECT_STREQ(input.c_str(), output.c_str());
    }
}

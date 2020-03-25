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
    {
        Add(_T("g"), &G);
        Add(_T("h"), &H);
        Add(_T("i"), &I);
        Add(_T("j"), &J);
    }

    ~CommandParameters()
    {
    }

public:
    WPEFramework::Core::JSON::OctSInt16 G;
    WPEFramework::Core::JSON::DecSInt16 H;
    WPEFramework::Core::JSON::EnumType<CommandType> I;
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::DecUInt16> J;
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
    {
        Add(_T("a"), &A);
        Add(_T("b"), &B);
        Add(_T("c"), &C);
        Add(_T("d"), &D);
        Add(_T("e"), &E);
        Add(_T("f"), &F);
    }

    ~CommandRequest()
    {
    }

public:
    WPEFramework::Core::JSON::HexSInt32 A;
    WPEFramework::Core::JSON::String B;
    WPEFramework::Core::JSON::HexUInt32 C;
    WPEFramework::Core::JSON::Boolean D;
    WPEFramework::Core::JSON::OctUInt16 E;
    CommandParameters F;
};

TEST(Core_JSON, simpleSet)
{
    {
        //Tester
        string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":"-44","i":"enum_4","j":["6","14","22"]}})";
        string inputRequired = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22]}})";
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
        WPEFramework::Core::JSON::Tester<1, CommandRequest> parser;
        //ToString
        parser.ToString(command, output);
        ASSERT_STREQ(inputRequired.c_str(), output.c_str());
        //FromString
        WPEFramework::Core::ProxyType<CommandRequest> received = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
        ASSERT_STREQ(inputRequired.c_str(), output.c_str());

        parser.FromString(inputRequired, received);
        output.clear();
        parser.ToString(received, output);
        ASSERT_STREQ(inputRequired.c_str(), output.c_str());

        //ArrayType Iterator
        WPEFramework::Core::JSON::ArrayType<Core::JSON::DecUInt16>::Iterator settings(command->F.J.Elements());
        for(int i = 0; settings.Next(); i++)
            ASSERT_EQ(settings.Current().Value(), command->F.J[i]);
        //null test
        input = R"({"a":null,"b":null,"c":"0x5A","d":null,"f":{"g":"-014","h":-44}})";
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
        //ASSERT_STREQ(input.c_str(), output.c_str());
    }
    //JsonObject and JsonValue
    {
        string input = R"({"a":"-0x5A","b":"TestIdentifier","c":"0x5A","d":true,"e":"014","f":{"g":"-014","h":-44,"i":"enum_4","j":[6,14,22]}})";
        JsonObject command;
        command.FromString(input);
        string output;
        command.ToString(output);
        ASSERT_STREQ(input.c_str(), output.c_str());

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
        ASSERT_STREQ(input.c_str(), serialized.c_str());

        JsonObject::Iterator index = demoObject.Variants();
        while (index.Next()) {
            JsonValue value(demoObject.Get(index.Label()));
            ASSERT_EQ(value.Content(), index.Current().Content());
            ASSERT_STREQ(value.Value().c_str(), index.Current().Value().c_str());
        }
    }
}

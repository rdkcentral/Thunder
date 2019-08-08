#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;

typedef enum {
    ENUM_1,
    ENUM_2,
    ENUM_3,
    ENUM_4
} CommandType;

namespace WPEFramework {
ENUM_CONVERSION_BEGIN(CommandType)

    { ENUM_1, _TXT("enum_1") },
    { ENUM_2, _TXT("enum_2") },
    { ENUM_3, _TXT("enum_3") },
    { ENUM_4, _TXT("enum_4") },

ENUM_CONVERSION_END(CommandType)
}

class CommandParameters : public WPEFramework::Core::JSON::Container {

public:
    CommandParameters()
    {
        Add(_T("f"), &F);
        Add(_T("g"), &G);
        Add(_T("h"), &H);
        Add(_T("i"), &I);
    }
    ~CommandParameters()
    {
    }

private:
    CommandParameters(const CommandParameters&) = delete;
    CommandParameters& operator=(const CommandParameters&) = delete;

public:
    WPEFramework::Core::JSON::OctSInt16 F;
    WPEFramework::Core::JSON::DecUInt16 G;
    WPEFramework::Core::JSON::EnumType<CommandType> H;
    WPEFramework::Core::JSON::ArrayType<WPEFramework::Core::JSON::DecUInt16> I;
};

class CommandRequest : public WPEFramework::Core::JSON::Container {
private:
    CommandRequest(const CommandRequest&) = delete;
    CommandRequest& operator=(const CommandRequest&) = delete;

public:
    CommandRequest()
    {
        Add(_T("a"), &A);
        Add(_T("b"), &B);
        Add(_T("c"), &C);
        Add(_T("d"), &D);
        Add(_T("e"), &E);
    }
    ~CommandRequest()
    {
    }

public:
    WPEFramework::Core::JSON::DecUInt32 A;
    WPEFramework::Core::JSON::String B;
    WPEFramework::Core::JSON::HexUInt32 C;
    WPEFramework::Core::JSON::Boolean D;
    CommandParameters E;
};

TEST(Core_JSON, simpleSet)
{
    {
        //Tester
        string input = R"({"a":35,"b":"TestIdentifier","c":"0x567","d":true,"e":{"f":"-014","g":44,"h":"enum_4","i":[6,14,22]}})";
        string output;
        WPEFramework::Core::ProxyType<CommandRequest> command = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        command->A = 35;
        command->B = _T("TestIdentifier");
        command->C = 0x567;
        command->D = true;
        command->E.F = -12;
        command->E.G = 44;
        command->E.H = ENUM_4;
        command->E.I.Add(WPEFramework::Core::JSON::DecUInt16(6, true));
        command->E.I.Add(WPEFramework::Core::JSON::DecUInt16(14, true));
        command->E.I.Add(WPEFramework::Core::JSON::DecUInt16(22, true));
        WPEFramework::Core::JSON::Tester<1, CommandRequest> parser;
        //ToString
        parser.ToString(command, output);
        ASSERT_STREQ(input.c_str(), output.c_str());
        //FromString
        WPEFramework::Core::ProxyType<CommandRequest> received = WPEFramework::Core::ProxyType<CommandRequest>::Create();
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
        ASSERT_STREQ(input.c_str(), output.c_str());
        //ArrayType Iterator
        WPEFramework::Core::JSON::ArrayType<Core::JSON::DecUInt16>::Iterator settings(command->E.I.Elements());
        for(int i = 0; settings.Next(); i++)
            ASSERT_EQ(settings.Current().Value(), command->E.I[i]);
        //null test
        input = R"({"a":null,"b":null,"c":"0x567","d":null,"e":{"f":"-014","g":44,"h":null}})";
        parser.FromString(input, received);
        output.clear();
        parser.ToString(received, output);
    }
    //JsonObject and JsonValue
    {
        string input = R"({"a":10,"b":20,"c":false,"d":"demo"})";
        JsonObject demoObject;
        demoObject["a"] = 10;
        demoObject["b"] = 20;
        demoObject["c"] = false;
        demoObject["d"] = "demo";
        string serialized;
        demoObject.ToString(serialized);
        ASSERT_STREQ(input.c_str(), serialized.c_str());
        JsonObject::Iterator index = demoObject.Variants();
        while (index.Next() == true) {
            JsonValue value(demoObject.Get(index.Label()));
            ASSERT_EQ(value.Content(), index.Current().Content());
            ASSERT_STREQ(value.Value().c_str(), index.Current().Value().c_str());
        }

        demoObject["e"] = -1;
        input = R"({"a":10,"b":20,"c":false,"d":"demo","e":-1})";
        serialized.clear();
        demoObject.ToString(serialized);
        ASSERT_STREQ(input.c_str(), serialized.c_str());
    }
}

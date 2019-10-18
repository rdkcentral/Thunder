#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <tracing/tracing.h>
#include <tracing/TraceUnit.h>
#include <core/Time.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

string g_tracingPathName = "/tmp/tracebuffer01";
time_t g_startTime =time(NULL)+1;

constexpr uint32_t CyclicBufferSize = ((8 * 1024) - (sizeof(struct Core::CyclicBuffer::control))); /* 8Kb */

unsigned int g_maxBufferValue = 256;

#pragma pack(push)
#pragma pack(1)
struct TraceHeader
{
    uint16_t _Length;
    uint64_t _ClockTicks;
    uint32_t _LineNumber;
};
#pragma pack(pop)

struct TraceData
{
    TraceHeader _Header;
    string _File;
    string _Module;
    string _Category;
    string _Class;
    string _Text;

    string ToString()
    {
        std::stringstream output;
        output << _File << "(" << _Header._LineNumber << "): " << _Text;
        return output.str();
    }
};

class ServerCyclicBuffer01 : public Core::CyclicBuffer
{
public:
    ServerCyclicBuffer01(const string& fileName)
        : CyclicBuffer(fileName, 0, true)
    {
    }

    virtual uint32_t GetReadSize(Cursor& cursor) override
    {
        uint16_t entrySize = 0;
        cursor.Peek(entrySize);
        return entrySize;
    }
};

bool ReadTraceString(const uint8_t buffer[], uint32_t length, uint32_t& offset, string& output)
{
    output = "";

    const char * charBuffer = reinterpret_cast<const char *>(buffer);

    while(true) {
        char c = charBuffer[offset];

        if (c == '\0') {
            // Found the end
            offset++;
            return true;
        }

        output += string(1, c);
        offset++;

        if (offset >= length) {
            // Buffer overrun
            return false;
        }
    }

    return true;
}


bool ParseTraceData(const uint8_t buffer[], uint32_t length, uint32_t& offset, TraceData& traceData)
{
    uint32_t startOffset = offset;

    const TraceHeader * header = reinterpret_cast<const TraceHeader *>(buffer + offset);
    offset += sizeof(TraceHeader);

    if (offset > length) {
        std::cerr << "Offset " << offset << " is larger than length " << length << std::endl;
        return false;
    }

    traceData._Header = *header;
    uint16_t entrySize = traceData._Header._Length;
    EXPECT_TRUE(entrySize <= CyclicBufferSize);

    if (!ReadTraceString(buffer, length, offset, traceData._File)) {
        std::cerr << "Failed to read file name" << std::endl;
        return false;
    }

    if (!ReadTraceString(buffer, length, offset, traceData._Module)) {
        std::cerr << "Failed to read module name" << std::endl;
        return false;
    }

    if (!ReadTraceString(buffer, length, offset, traceData._Category)) {
        std::cerr << "Failed to read category" << std::endl;
        return false;
    }
    if (!ReadTraceString(buffer, length, offset, traceData._Class)) {
        std::cerr << "Failed to read class name" << std::endl;
        return false;
    }

    uint16_t totalHeaderLength = offset - startOffset;
    uint16_t textLength = entrySize - totalHeaderLength;
    uint16_t textBufferLength = textLength + 1;
    char textBuffer[textBufferLength];

    memcpy(textBuffer, buffer + offset, textLength);
    textBuffer[textLength] = '\0';
    traceData._Text = string(textBuffer);

    offset += textLength;

    EXPECT_TRUE(offset == (startOffset + entrySize));

    return true;
}

void DebugCheckIfConsistent(const uint8_t * buffer, int length, Core::CyclicBuffer& cycBuffer)
{
    uint entryCount = 0;

    int index = 0;
    while (index < length) {
        uint16_t entrySize = 0;
        entrySize += static_cast<uint16_t>(buffer[index]);
        index++;
        entrySize += static_cast<uint16_t>(buffer[index]) << 8;

        EXPECT_TRUE(entrySize < CyclicBufferSize);
        index += entrySize - 1;

        entryCount++;
    }

    EXPECT_TRUE(index == length);
}

void CreateTraceBuffer()
{
   char systemCmd[1024];
   sprintf(systemCmd, "mkdir -p %s", g_tracingPathName.c_str());
   system(systemCmd);
}

TEST(Core_tracing, simpleTracing)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {

       testAdmin.Sync("client start");
       std::string db = (g_tracingPathName + "/tracebuffer.doorbell");
       string cycBufferName = (g_tracingPathName + "/tracebuffer.0");
       Core::DoorBell doorBell(db.c_str());
       ServerCyclicBuffer01 cycBuffer(cycBufferName);

       // TODO: maximum running time?
       if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
           doorBell.Acknowledge();
           uint32_t bufferLength = CyclicBufferSize;
           uint8_t buffer[bufferLength];
           uint32_t actuallyRead = cycBuffer.Read(buffer, sizeof(buffer));
           testAdmin.Sync("server done");

           EXPECT_TRUE(actuallyRead < cycBuffer.Size());

           DebugCheckIfConsistent(buffer, actuallyRead, cycBuffer);

           uint32_t offset = 0;
           int traceCount = 0;
           while(offset < actuallyRead) {
               TraceData traceData;
               EXPECT_TRUE(ParseTraceData(buffer, actuallyRead, offset, traceData));
               string time(Core::Time::Now().ToRFC1123(true));

               EXPECT_STREQ(traceData._File.c_str(),"test_tracing.cpp");
               EXPECT_STREQ(traceData._Class.c_str(),"<<Global>>");
               EXPECT_STREQ(traceData._Category.c_str(),"Information");
               EXPECT_STREQ(traceData._Text.c_str(),"Trace Log");

               traceCount++;
           }
       }
       doorBell.Relinquish();
    };

   // This side (tested) acts as client.
   IPTestAdministrator testAdmin(otherSide);
   {
        CreateTraceBuffer();
        Trace::TraceUnit::Instance().Open(g_tracingPathName,0);
        testAdmin.Sync("client start");
        sleep(2);
        Trace::TraceType<Trace::Information, &Core::System::MODULE_NAME>::Enable(true);

        TRACE_GLOBAL(Trace::Information, (_T("Trace Log")));
        testAdmin.Sync("server done");

        Trace::TraceUnit::Instance().SetCategories(true,"Tracing", reinterpret_cast<const char*>("Information"));
        Trace::TraceUnit::Iterator index = Trace::TraceUnit::Instance().GetCategories();

        while (index.Next() == true)
            if((*index)->Enabled() == true) {
                EXPECT_STREQ((*index)->Module(),"Tracing");
                EXPECT_STREQ((*index)->Category(),"Information");
            }

        bool enabled = false;
        Trace::TraceUnit::Instance().IsDefaultCategory("Tracing",reinterpret_cast<const char*>("Information"),enabled);

#if 0
        string jsonDefaultCategories("Information");
        if (jsonDefaultCategories.empty() == false) {
            Trace::TraceUnit::Instance().SetDefaultCategoriesJson(jsonDefaultCategories);
        }
        Trace::TraceUnit::Instance().GetDefaultCategoriesJson(jsonDefaultCategories);
#endif

        TRACE(Trace::Information,(Trace::Format(_T("Checking the Format() with 1 parameter"))));
        std::string text = "Hello";
        TRACE(Trace::Information,(Trace::Format(text.c_str(),_T("Checking the Format() with 2 parameter"))));
        Trace::TraceUnit::Instance().Close();
        Trace::TraceUnit::Instance().Open(1);
   }
   Core::Singleton::Dispose();
}

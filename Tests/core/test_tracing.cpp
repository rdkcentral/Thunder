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

void waitUntilAfterStart(time_t secs)
{
   time_t target = g_startTime + secs;

   // Check every 0.1 second if we can continue.
   while (time(nullptr) < target) {
      const useconds_t miliSec = 1000;
      usleep(100 * miliSec);
   }
}

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
        : CyclicBuffer(fileName, 255, true)
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
    ASSERT(entrySize <= CyclicBufferSize);

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

    ASSERT(offset == (startOffset + entrySize));

    return true;
}

string GetCyclicBufferName()
{
    Core::Directory dir(g_tracingPathName.c_str());

    uint32_t bufferCount = 0;
    string bufferPath;

    while(dir.Next()) {
        string triedPath = dir.Current();
        // Skip "." and ".."
        if(triedPath[triedPath.size() - 1] != '.') {
            bufferCount++;
            bufferPath = triedPath;
        }
    }

    ASSERT(bufferCount == 1);
    
    return bufferPath;
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

        ASSERT(entrySize < CyclicBufferSize);
        index += entrySize - 1;

        entryCount++;
    }

    ASSERT(index == length);
}


TEST(Core_tracing, simpleTracing)
{
    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
       testAdmin.Sync("server start");

       string cycBufferName = GetCyclicBufferName();
       Core::DoorBell doorBell("tracebuffer");
       ServerCyclicBuffer01 cycBuffer(cycBufferName+".0");
       testAdmin.Sync("server setup");
       testAdmin.Sync("client done");

       // TODO: maximum running time?
       while (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
           uint32_t bufferLength = CyclicBufferSize;
           uint8_t buffer[bufferLength];
           uint32_t actuallyRead = cycBuffer.Read(buffer, sizeof(buffer));

           ASSERT(actuallyRead < cycBuffer.Size());

           DebugCheckIfConsistent(buffer, actuallyRead, cycBuffer);

           uint32_t offset = 0;
           int traceCount = 0;
           while(offset < actuallyRead) {
               TraceData traceData;
               ASSERT(!ParseTraceData(buffer, actuallyRead, offset, traceData));
               string time(Core::Time::Now().ToRFC1123(true));

               fprintf(stderr, "[%s]:[%s:%d]:[%s] %s: %s\n", time.c_str(), traceData._File.c_str(), traceData._Header._LineNumber, traceData._Class.c_str(), traceData._Category.c_str(), traceData._Text.c_str());

               traceCount++;
           }
       }
       testAdmin.Sync("server done");

    };

   // This side (tested) acts as client (consumer).
   IPTestAdministrator testAdmin(otherSide);
   {
        system("mkdir -p /tmp/tracebuffer01");
        system("rm -f /tmp/tracebuffer01/*");
        Trace::TraceUnit::Instance().Open(g_tracingPathName,0);
        testAdmin.Sync("server start");
        Trace::TraceType<Trace::Information, &Core::System::MODULE_NAME>::Enable(true);
        testAdmin.Sync("server setup");

        // Build too long trace statement.
        const int longBufferSize = CyclicBufferSize * 4 / 3;
        char longBuffer[longBufferSize];
        for (int j = 0; j < (longBufferSize - 1); j++)
            longBuffer[j] = 'a';
        longBuffer[longBufferSize - 1] = '\0';
        // One long trace.
        TRACE_GLOBAL(Trace::Information, (longBuffer));

        int traceCount = rand() % 100 + 50;
        for (int i = 0; i < traceCount; i++) {
            int traceLength = 50 + rand() % 200;
            char buffer[traceLength + 1];
            for (int j = 0; j < traceLength; j++)
                buffer[j] = static_cast<char>(static_cast<int>('a') + rand() % 26);

            buffer[traceLength] = '\0';

            TRACE_GLOBAL(Trace::Information, (buffer));
        }
        testAdmin.Sync("client done");
        testAdmin.Sync("server done");

        Trace::TraceUnit::Instance().Close();
   }
   Core::Singleton::Dispose();
}

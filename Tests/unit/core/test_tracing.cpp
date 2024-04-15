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
#include <tracing/tracing.h>
#include <tracing/TraceUnit.h>
#include <core/Time.h>

using namespace Thunder;
using namespace Thunder::Core;

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

class ServerCyclicBuffer01 : public CyclicBuffer
{
public:
    ServerCyclicBuffer01(const string& fileName, uint32_t size)
        : CyclicBuffer(fileName, File::USER_WRITE|File::USER_READ|File::SHAREABLE, size, true)
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

    while (true) {
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

bool ParseTraceData(const uint8_t buffer[], uint32_t length, uint32_t& offset, TraceData& traceData, uint32_t bufferSize)
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
    EXPECT_TRUE(entrySize <= bufferSize);

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

void DebugCheckIfConsistent(const uint8_t * buffer, int length, CyclicBuffer& cycBuffer, uint32_t bufferSize)
{
    uint entryCount = 0;

    int index = 0;
    while (index < length) {
        uint16_t entrySize = 0;
        entrySize += static_cast<uint16_t>(buffer[index]);
        index++;
        entrySize += static_cast<uint16_t>(buffer[index]) << 8;

        EXPECT_TRUE(entrySize < bufferSize);
        index += entrySize - 1;

        entryCount++;
    }

    EXPECT_TRUE(index == length);
}

void CreateTraceBuffer(string tracePath)
{
    char systemCmd[1024];
    string command = "mkdir -p ";
    snprintf(systemCmd, command.size()+tracePath.size()+1, "%s%s", command.c_str(),tracePath.c_str());
    system(systemCmd);
}

TEST(Core_tracing, simpleTracing)
{
    // Call dispose to ensure there is no any resource handler registered to
    // avoid hang on the poll
    Core::Singleton::Dispose();

    std::string tracePath = "/tmp/tracebuffer01";
    auto lambdaFunc = [tracePath](IPTestAdministrator & testAdmin) {
        std::string db = (tracePath + "/tracebuffer.doorbell");
        string cycBufferName = (tracePath + "/tracebuffer");

        testAdmin.Sync("client start");
        DoorBell doorBell(db.c_str());
        constexpr uint32_t bufferSize = ((8 * 1024) - (sizeof(struct CyclicBuffer::control))); /* 8Kb */
        ServerCyclicBuffer01 cycBuffer(cycBufferName, bufferSize);

        // Note: this test case is forking a child process with parent process space.
        // In such case, signalfd:poll of parent will not get signal from pthread_kill(SIGUSR2) from child
        // process. Hence please ensure parent process cleared all resource registration
        // using Singleton::Dispose() call from parent context.
        // https://lore.kernel.org/linux-man/20190923222413.5c79b179@kappa.digital-domain.net/T/

        // TODO: maximum running time?
        if (doorBell.Wait(infinite) == ERROR_NONE) {
            doorBell.Acknowledge();
            uint32_t bufferLength = bufferSize;
            uint8_t buffer[bufferLength];
            uint32_t actuallyRead = cycBuffer.Read(buffer, sizeof(buffer));
            testAdmin.Sync("server done");

            EXPECT_TRUE(actuallyRead < cycBuffer.Size());

            DebugCheckIfConsistent(buffer, actuallyRead, cycBuffer, bufferSize);

            uint32_t offset = 0;
            int traceCount = 0;
            while (offset < actuallyRead) {
                TraceData traceData;
                EXPECT_TRUE(ParseTraceData(buffer, actuallyRead, offset, traceData, bufferSize));
                string time(Time::Now().ToRFC1123(true));

                EXPECT_STREQ(traceData._File.c_str(), "test_tracing.cpp");
                EXPECT_STREQ(traceData._Class.c_str(), "TestBody");
                EXPECT_STREQ(traceData._Category.c_str(), "Information");
                EXPECT_STREQ(traceData._Text.c_str(), "Trace Log");

                traceCount++;
            }
        }
        doorBell.Relinquish();
    };

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as client.
    IPTestAdministrator testAdmin(otherSide);
    {
        CreateTraceBuffer(tracePath);
        Trace::TraceUnit::Instance().Open(tracePath);
        testAdmin.Sync("client start");
        sleep(2);
        Trace::TraceType<Trace::Information, &System::MODULE_NAME>::Enable(true);

        TRACE_GLOBAL(Trace::Information, (_T("Trace Log")));
        testAdmin.Sync("server done");

        Trace::TraceUnit::Instance().SetCategories(true,"Tracing", reinterpret_cast<const char*>("Information"));
        Trace::TraceUnit::Iterator index = Trace::TraceUnit::Instance().GetCategories();

        while (index.Next() == true)
            if ((*index)->Enabled() == true) {
                if ((strcmp((*index)->Module(), "Tracing")) == 0 ) {
                    EXPECT_STREQ((*index)->Category(), "Information");
                }
            }

        bool enabled = false;
        Trace::TraceUnit::Instance().IsDefaultCategory("Tracing", reinterpret_cast<const char*>("Information"), enabled);
        TRACE(Trace::Information,(Trace::Format(_T("Checking the Format() with 1 parameter"))));
        std::string text = "Hello";
        TRACE(Trace::Information,(Trace::Format(text.c_str(), _T("Checking the Format() with 2 parameter"))));
        Trace::TraceUnit::Instance().Close();
        Trace::TraceUnit::Instance().Open(1);
    }

    testAdmin. WaitForChildCompletion();
    Singleton::Dispose();
}

TEST(Core_tracing, simpleTracingReversed)
{
    std::string tracePath = "/tmp/tracebuffer02";
    auto lambdaFunc = [&](IPTestAdministrator & testAdmin) {
        CreateTraceBuffer(tracePath);
        Trace::TraceUnit::Instance().Open(tracePath);
        testAdmin.Sync("client start");
        sleep(2);
        Trace::TraceType<Trace::Information, &System::MODULE_NAME>::Enable(true);

        TRACE_GLOBAL(Trace::Information, (_T("Trace Log")));
        testAdmin.Sync("server done");

        Trace::TraceUnit::Instance().SetCategories(true,"Tracing", reinterpret_cast<const char*>("Information"));
        Trace::TraceUnit::Iterator index = Trace::TraceUnit::Instance().GetCategories();

        while (index.Next() == true)
            if ((*index)->Enabled() == true) {
                if ((strcmp((*index)->Module(), "Tracing")) == 0 ) {
                    EXPECT_STREQ((*index)->Category(), "Information");
                }
            }

        bool enabled = false;
        Trace::TraceUnit::Instance().IsDefaultCategory("Tracing", reinterpret_cast<const char*>("Information"), enabled);
        TRACE(Trace::Information,(Trace::Format(_T("Checking the Format() with 1 parameter"))));
        std::string text = "Hello";
        TRACE(Trace::Information,(Trace::Format(text.c_str(), _T("Checking the Format() with 2 parameter"))));
        Trace::TraceUnit::Instance().Close();
        Trace::TraceUnit::Instance().Open(1);

        Singleton::Dispose();
    };

    static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

    IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

    // This side (tested) acts as client.
    IPTestAdministrator testAdmin(otherSide);
    {
        std::string db = (tracePath + "/tracebuffer.doorbell");
        string cycBufferName = (tracePath + "/tracebuffer");

        testAdmin.Sync("server start");
        DoorBell doorBell(db.c_str());
        constexpr uint32_t bufferSize = ((8 * 1024) - (sizeof(struct CyclicBuffer::control))); /* 8Kb */
        ServerCyclicBuffer01 cycBuffer(cycBufferName, bufferSize);

        // TODO: maximum running time?
        if (doorBell.Wait(infinite) == ERROR_NONE) {
            doorBell.Acknowledge();
            uint32_t bufferLength = bufferSize;
            uint8_t buffer[bufferLength];
            uint32_t actuallyRead = cycBuffer.Read(buffer, sizeof(buffer));
            testAdmin.Sync("client done");

            EXPECT_TRUE(actuallyRead < cycBuffer.Size());

            DebugCheckIfConsistent(buffer, actuallyRead, cycBuffer, bufferSize);

            uint32_t offset = 0;
            int traceCount = 0;
            while (offset < actuallyRead) {
                TraceData traceData;
                EXPECT_TRUE(ParseTraceData(buffer, actuallyRead, offset, traceData, bufferSize));
                string time(Time::Now().ToRFC1123(true));

                EXPECT_STREQ(traceData._File.c_str(), "test_tracing.cpp");
                EXPECT_STREQ(traceData._Class.c_str(), "operator()");
                EXPECT_STREQ(traceData._Category.c_str(), "Information");
                EXPECT_STREQ(traceData._Text.c_str(), "Trace Log");

                traceCount++;
            }
        }
        doorBell.Relinquish();
    }

    testAdmin.WaitForChildCompletion();
    Singleton::Dispose();
}
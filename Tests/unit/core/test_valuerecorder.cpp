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

#include "gtest/gtest.h"
#include "core/core.h"

using namespace Thunder;
using namespace Thunder::Core;

const unsigned int BLOCKSIZE = 20;

class WriterClass : public RecorderType<uint32_t, BLOCKSIZE>::Writer
{
    public:
        WriterClass() = delete;

        WriterClass(string filename)
            : Writer(filename)
            , _file(filename)
        {
        }

        ~WriterClass()
        {
        }

    public:
        void WriterJob()
        {
            uint8_t arr[] = {1,2,3};
            SetBuffer(arr);
            auto object = Create(_file);
            Record(10);
            uint64_t TimeValue = Time();
            std::string storageName = Source();
            uint32_t  value = Value();
            object.Release();
        }

    private:
        string _file;
};

class ReaderClass : public RecorderType<uint32_t, BLOCKSIZE>::Reader
{
    public:
        ReaderClass() = delete;

        ReaderClass(string filename)
            : Reader(filename)
            , _file(filename)
        {
        }

        ReaderClass(const ProxyType<WriterClass>& recorder, const uint32_t id = static_cast<uint32_t>(~0))
                : Reader(recorder->Source())
                , _file(recorder->Source())
        {
        }

        ~ReaderClass()
        {
        }

    public:
        void ReaderJob()
        {
            Next();
            EXPECT_TRUE(IsValid());

            uint32_t time = 20;
            Core::Time curTime = Core::Time::Now();
            curTime.Add(time);
            uint32_t index = Store(curTime.Ticks(), 1);
            
            StepForward();
            StepBack();
            ClearData();

            Reader obj1(_file, 1u);
            EXPECT_FALSE(obj1.Previous());
            EXPECT_TRUE(obj1.Next());

            EXPECT_EQ(StartId(),1u);

            if (EndId() == StartId())
                EXPECT_EQ(EndId(),1u);
            else
                EXPECT_EQ(EndId(),2u);

            uint32_t id = EndId();
            std::string storageName = Source();
        }

    private:
        string _file;
};

TEST(test_valuerecorder, test_writer)
{
    string filename = "baseRecorder.txt";

    auto obj1 = RecorderType<uint32_t, BLOCKSIZE>::Writer::Create(filename);

    obj1->Copy(*(obj1),1);
    obj1->Copy(*(obj1),100);

    static_cast<WriterClass&>(*obj1).WriterJob();

    ReaderClass obj2(filename);
    obj2.ReaderJob();

    ReaderClass obj4(ProxyType<WriterClass>(obj3));

    obj1.Release();
}


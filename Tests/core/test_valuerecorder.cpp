#include "../IPTestAdministrator.h"

#include "gtest/gtest.h"
#include "core/core.h"

using namespace WPEFramework;
using namespace WPEFramework::Core;

const unsigned int BLOCKSIZE = 20;

class WriterClass : public RecorderType<uint32_t, BLOCKSIZE>::Writer
{
    public:
        WriterClass(string filename)
            :Writer(filename)
            ,file(filename)
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
            Create(file);
            Record(10);
            Time();
            Source();
            Value();
        }

    private:
        string file;
};

class ReaderClass : public RecorderType<uint32_t, BLOCKSIZE>::Reader
{
    public:
        ReaderClass(string filename)
            :Reader(filename)
            ,file(filename)
        {
        }
        ReaderClass(const ProxyType<WriterClass>& recorder, const uint32_t id = static_cast<uint32_t>(~0))
                : Reader(recorder->Source())
                , file(recorder->Source())
        {
        }
        ~ReaderClass()
        {
        }

    public:
        void ReaderJob()
        {
            uint32_t time = 20;
            Core::Time curTime = Core::Time::Now();
            curTime.Add(time);
            Store(curTime.Ticks(), 1);
            
            StepForward();
            StepBack();
            ClearData();
            Reader obj1(file, 1u);
            EXPECT_FALSE(obj1.Previous());
            EXPECT_TRUE(obj1.Next());

            EXPECT_EQ(StartId(),1u);;
            EXPECT_EQ(EndId(),2u);
            Source();
        }
    private:
        string file;
};

TEST(test_valuerecorder, test_writer)
{
    string filename = "baseRecorder.txt";
    WriterClass obj1(filename);
    obj1.Copy(obj1,1);
    obj1.Copy(obj1,100);
    obj1.WriterJob();
    ReaderClass obj2(filename);
    obj2.ReaderJob();
    ReaderClass obj3(ProxyType<WriterClass>(obj1));
}


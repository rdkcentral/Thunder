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
            uint32_t value = Value();
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
        ~ReaderClass()
        {
        }

    public:
        void ReaderJob()
        {
            ClearData();
            Reader obj1(file, unsigned(1));
            EXPECT_FALSE(obj1.Previous());
            EXPECT_TRUE(obj1.Next());
            StepForward();
            StepBack();
        }
    private:
        string file;
};

TEST(test_valuerecorder, test_writer)
{
    string filename = "baseRecorder.txt";
    WriterClass obj1(filename);
    obj1.WriterJob();
    ReaderClass obj2(filename);
    obj2.ReaderJob();
}


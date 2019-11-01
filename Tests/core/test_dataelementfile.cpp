#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

#define FILENAME "dataFile.txt"

class DataFile: public Core::DataElementFile
{
    public:
        DataFile(File& file)
            : DataElementFile(file)
        {
        }
        DataFile(string fileName, uint32_t type, uint32_t size)
            :DataElementFile(fileName, type, size)
        {
        }
        void MemoryMap()
        {
            Reallocation(54);
            ReopenMemoryMappedFile();
        }
};
TEST(test_datafile, simple_test)
{
    char buffer[100];
    sprintf(buffer,"echo 'DataElement file checking......' >%s",FILENAME);
    const string fileName = FILENAME;
    File file(fileName);
    File fileSample(file);
    DataFile obj1(fileSample);
    DataFile object(fileName, 1, 10);
    EXPECT_EQ(obj1.ErrorCode(),unsigned(2));
    DataFile obj2(fileName, 1, 50);
    obj1.Sync();
    obj2.MemoryMap();

    const string& Name = obj1.Name();
    EXPECT_EQ(Name.c_str(), fileName);
    EXPECT_FALSE(obj2.IsValid());
    obj1.Storage();
    obj1.ReloadFileInfo();
    obj1.MemoryMap();
}

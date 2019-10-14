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
        DataFile(string fileName)
            :DataElementFile(fileName)
        {
        }
        void memoryMap()
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
    uint32_t errorCode = 2;
    EXPECT_EQ(obj1.ErrorCode(),errorCode);

    DataFile obj2(fileName);
    obj1.Sync();

    const string& Name = obj1.Name();
    EXPECT_EQ(Name.c_str(), fileName);
    EXPECT_FALSE(obj2.IsValid());
    obj1.Storage();
    obj1.ReloadFileInfo();
    obj2.memoryMap();
}

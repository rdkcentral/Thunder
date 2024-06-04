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
#include <core/core.h>

using namespace Thunder;
using namespace Thunder::Core;

class DataFile: public Core::DataElementFile
{
public:
    DataFile() = delete;

    DataFile(File& file)
        : DataElementFile(file, File::USER_READ)
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
    const string fileName = "dataFile.txt";
    string message = ">echo 'DataElement file checking......'";
    snprintf(buffer,(message.size() + fileName.size()+1), "%s%s",message.c_str(),fileName.c_str());
    File file(fileName);

    // Clear error no since it will be already set internally from
    // the LoadInfo sequence with non existing file
    EXPECT_EQ(file.ErrorCode(), unsigned(2));
    errno = 0;

    file.Create(true);
    EXPECT_EQ(file.IsOpen(), true);
    EXPECT_EQ(file.Name(), fileName);
    DataFile obj1(file);

    EXPECT_EQ(obj1.ErrorCode(), unsigned(0));
    DataFile object(fileName, 1, 10);
    DataFile obj2(fileName, 1, 50);
    obj1.Sync();
    obj2.MemoryMap();

    const string& name = obj1.Name();
    EXPECT_EQ(name.c_str(), fileName);
    EXPECT_EQ(obj2.IsValid(), true);
    obj1.Storage();
    obj1.ReloadFileInfo();
    obj1.MemoryMap();
    file.Destroy();
}

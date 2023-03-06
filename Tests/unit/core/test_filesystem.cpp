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

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST (test_file, file)
{
    File file;
    File fileObj1("Sample.txt");
    fileObj1.Create(true);
    File fileObj2(fileObj1);
    fileObj2.SetSize(150);

    EXPECT_TRUE(fileObj1.Open());
    EXPECT_TRUE(fileObj1.Append());
    EXPECT_TRUE(fileObj1.Create());
    EXPECT_TRUE(fileObj1.Create(Core::File::USER_WRITE));
    EXPECT_TRUE(fileObj1.Open(true));

    char buffer[] = "New  Line is added to the File.";
    if (fileObj1.IsOpen()) {
      fileObj1.Write(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
    }
    if (fileObj1.IsOpen()) {
        fileObj1.Read(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
    }

    static string fileName = file.FileName("/home/file/datafile.txt");
    EXPECT_EQ(fileName, "datafile");
    static string filenameExt = file.FileNameExtended("/home/file/datafile.txt");
    EXPECT_EQ(filenameExt, "datafile.txt");
    static string pathName = file.PathName("/home/file/datafile.txt");
    EXPECT_EQ(pathName, "/home/file/");
    static string extension = file.Extension("/home/file/datafile.txt");
    EXPECT_EQ(extension, "txt");
    fileObj1.Destroy();
}

TEST (test_file, file_functions)
{
    File fileObj1("Sample2.txt");
    fileObj1.Create(true);
    char buffer[] = "Sample2.txt is moved to newFile.txt";
    fileObj1.Write(reinterpret_cast<uint8_t*>(buffer), sizeof(buffer));
    EXPECT_TRUE(fileObj1.IsOpen());
    EXPECT_TRUE(fileObj1.Exists());
    EXPECT_FALSE(fileObj1.IsReadOnly());
    EXPECT_FALSE(fileObj1.IsHidden());
    EXPECT_FALSE(fileObj1.IsSystem());
    EXPECT_FALSE(fileObj1.IsArchive());
    EXPECT_FALSE(fileObj1.IsDirectory());
    EXPECT_FALSE(fileObj1.IsLink());
    EXPECT_FALSE(fileObj1.IsCompressed());
    EXPECT_FALSE(fileObj1.IsEncrypted());
    uint64_t size = 0;
    EXPECT_EQ(fileObj1.Size(), size);
    //EXPECT_EQ(fileObj1.DuplicateHandle(), 11); TODO
    EXPECT_TRUE(fileObj1.Move("newFile.txt"));
    fileObj1.Destroy();
}

TEST (test_file, directory)
{
    string path = "home/file";
    Directory dirOne(path.c_str());
    Directory dirTwo(path.c_str(), _T("*"));
    Directory dirThree = dirOne ;

    EXPECT_TRUE(dirOne.CreatePath());
    EXPECT_FALSE(dirOne.Create());
    EXPECT_FALSE(dirThree.IsValid());
    EXPECT_TRUE(dirOne.Next());

    char buffer[15];
    string currenPath = "..";
    snprintf(buffer,(path.size() + currenPath.size() + 2), "%s/%s",path.c_str(), currenPath.c_str());
#ifdef BUILD_ARM
    if ((dirOne.Current(), buffer) == 0) {
#else
    if (strcmp(dirOne.Current().c_str(), buffer) == 0) {
#endif
        EXPECT_EQ(dirOne.Current(), buffer);
        EXPECT_EQ(dirOne.Name(), currenPath.c_str());
    } else {
        currenPath = ".";
        snprintf(buffer,(path.size() + currenPath.size() + 2), "%s/%s",path.c_str(), currenPath.c_str());
        EXPECT_EQ(dirOne.Current(), buffer);
        EXPECT_EQ(dirOne.Name(), currenPath.c_str());
    }

    EXPECT_TRUE(dirOne.IsDirectory());
    dirOne.Reset();
    EXPECT_TRUE(dirOne.Next());
    system("rm -rf home");
}

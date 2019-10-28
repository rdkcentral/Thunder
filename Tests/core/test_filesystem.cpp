#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST (test_file, file)
{
    File file;
    File fileObj1("Sample.txt", true);
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
    File fileObj1("Sample2.txt", true);
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
    EXPECT_EQ(fileObj1.DuplicateHandle(), 10);
    EXPECT_TRUE(fileObj1.Move("newFile.txt"));
}

TEST (test_file, directory)
{
    Directory dir;
    string path = "home/file";
    Directory dirOne(path.c_str());
    Directory dirTwo(path.c_str(), _T("*"));
    Directory dirThree = dirOne ;

    EXPECT_TRUE(dirOne.CreatePath());
    EXPECT_FALSE(dirOne.Create());
    EXPECT_FALSE(dirThree.IsValid());
    EXPECT_TRUE(dirOne.Next());
    EXPECT_EQ(dirOne.Current(), "home/file/.");
    EXPECT_EQ(dirOne.Name(), ".");
    EXPECT_TRUE(dirOne.IsDirectory());
    dirOne.Reset();
    EXPECT_TRUE(dirOne.Next());
}

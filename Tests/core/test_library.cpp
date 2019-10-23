#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(Core_Library, simpleSet)
{
    Library libObj;
    const string file = string(BUILD_DIR) + _T("/../../Source/core/libWPEFrameworkCore.so");
    const TCHAR* function = _T("Core::Library::AddRef()");
    const string file1 = _T("libWPEFramework.so");
    Library LibObj1(file.c_str());
    LibObj1.LoadFunction(function);
    Library LibObjTest(file1.c_str());
    Library LibObj2(LibObj1);
    Library LibObj3;
    LibObj3 = LibObj2;

    EXPECT_TRUE(LibObj1.IsLoaded());
    EXPECT_EQ(LibObj3.Error(), "");
    EXPECT_EQ(LibObj3.Name(), file);
}

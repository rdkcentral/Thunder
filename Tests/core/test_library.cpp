#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

TEST(Core_Library, simpleSet)
{
    Library libObj;
    const TCHAR* file = "build/wpeframework/build/Source/core/libWPEFrameworkCore.so";
    const TCHAR* function = "Core::Library::AddRef()";
    Library LibObj1(file);
    LibObj1.LoadFunction(function);
    Library LibObj2(LibObj1);
    Library LibObj3;
    LibObj3 = LibObj2;

    EXPECT_TRUE(LibObj1.IsLoaded());
    EXPECT_EQ(LibObj3.Error(), "");
    EXPECT_EQ(LibObj3.Name(), file);
}

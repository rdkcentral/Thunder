#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;


TEST(test_ReadWritelock, simpleSet)
{
    ReadWriteLock readObj;
    EXPECT_TRUE(readObj.ReadLock());

    readObj.ReadUnlock();
    ReadWriteLock writeObj;
    EXPECT_TRUE(writeObj.WriteLock());

    writeObj.WriteUnlock();
}


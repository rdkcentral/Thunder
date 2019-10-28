#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

class Container
{
    private:
        int length;
        int* m_data;
    public:
        Container()
            :length(0)
            ,m_data(nullptr)
        {
        }
        ~Container()
        {
        }
};
TEST(test_lockableContainer, lockContainer_test)
{
    LockableContainerType<Container> containerObj1;
    LockableContainerType<Container> containerObj2(containerObj1);
    LockableContainerType<Container> containerObj3;
    containerObj3 = containerObj2;

    EXPECT_TRUE(containerObj1.ReadLock());
    containerObj1.ReadUnlock();
    EXPECT_TRUE(containerObj2.WriteLock());
    containerObj2.WriteUnlock();
}

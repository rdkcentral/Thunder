#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

using namespace WPEFramework;
using namespace WPEFramework::Core;

class SynchronizeClass {
    private:
        string msg;
    public:
        SynchronizeClass()
        {
        }
        ~SynchronizeClass()
        {
        }
    public:
        bool Copy(const string message)
        {
            bool result = false;
            msg = message;
            if (!msg.empty())
                result = true;
            return result;
        }
};

TEST(test_synchronize, synchronize_test)
{
    string MESSAGE = "SynchronizeType request";
    SynchronizeClass syncObject1;
    SynchronizeType<string> syncObject2;
    SynchronizeType<SynchronizeClass> syncObject3;

    syncObject2.Load();
    syncObject2.Evaluate();

    syncObject3.Load(syncObject1);
    EXPECT_EQ(syncObject3.Aquire(unsigned(5)), unsigned(11));
    syncObject3.Load(syncObject1);
    EXPECT_TRUE(syncObject3.Evaluate<string>(MESSAGE));

    syncObject2.Lock();
    syncObject2.Flush();
    syncObject2.Unlock();

    syncObject3.Lock();
    syncObject3.Flush();
    syncObject3.Unlock();
}

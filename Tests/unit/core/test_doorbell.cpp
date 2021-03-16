#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    TEST(Core_DoorBell, simpleSet)
    {
        std::string fileName {"/tmp/doorbell01"};
        auto lambdaFunc = [fileName] (IPTestAdministrator & testAdmin) {
            Core::DoorBell doorBell(fileName.c_str());

            EXPECT_EQ(doorBell.Wait(Core::infinite), Core::ERROR_NONE);
            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("First ring");
            }

            EXPECT_EQ(doorBell.Wait(Core::infinite), Core::ERROR_NONE);
            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("Second ring");
            }
            doorBell.Relinquish();
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::DoorBell doorBell(fileName.c_str());
            ::SleepMs(10);
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        }
    }

    TEST(Core_DoorBell, simpleSetReversed)
    {
        std::string fileName {"/tmp/doorbell02"};
        auto lambdaFunc = [fileName] (IPTestAdministrator & testAdmin) {

            Core::DoorBell doorBell(fileName.c_str());
            ::SleepMs(10);
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        };

        static std::function<void (IPTestAdministrator&)> lambdaVar = lambdaFunc;

        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator& testAdmin ) { lambdaVar(testAdmin); };

        IPTestAdministrator testAdmin(otherSide);
        {
            Core::DoorBell doorBell(fileName.c_str());
            EXPECT_EQ(doorBell.Wait(Core::infinite), Core::ERROR_NONE);
            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("First ring");
            }

            EXPECT_EQ(doorBell.Wait(Core::infinite), Core::ERROR_NONE);
            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("Second ring");
            }
            doorBell.Relinquish();
        }
        Core::Singleton::Dispose();
    }

} // Tests
} // WPEFramework

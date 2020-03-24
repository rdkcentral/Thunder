#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    TEST(Core_DoorBell, simpleSet)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            string fileName = "/tmp/doorbell01";
            Core::DoorBell doorBell(fileName.c_str());
            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("First ring");
            }

            if (doorBell.Wait(Core::infinite) == Core::ERROR_NONE) {
                doorBell.Acknowledge();
                testAdmin.Sync("Second ring");
            }
            doorBell.Relinquish();
        };

        IPTestAdministrator testAdmin(otherSide);
        {
            string fileName = "/tmp/doorbell01";
            Core::DoorBell doorBell(fileName.c_str());
            ::SleepMs(1);
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        }
    }
} // Tests
} // WPEFramework

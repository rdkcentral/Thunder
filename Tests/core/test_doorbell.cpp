#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace WPEFramework {
namespace Tests {

    string g_fileName = "/tmp/doorbell01";

    TEST(Core_DoorBell, simpleSet)
    {
        IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
            Core::DoorBell doorBell(g_fileName.c_str());
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
            usleep(1000);
            Core::DoorBell doorBell(g_fileName.c_str());
            doorBell.Ring();
            testAdmin.Sync("First ring");
            doorBell.Ring();
            testAdmin.Sync("Second ring");
        }
    }
}
}

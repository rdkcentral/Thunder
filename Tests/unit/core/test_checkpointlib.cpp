#include <gtest/gtest.h>

#include "../../../Source/addons/hibernate/hibernate.h"

namespace Thunder {
namespace Tests {

TEST(Core_CheckpointLib, UnsupportedOperationsPreserveStorage)
{
    // Stack storage must never be freed or changed by an unimplemented backend.
    int token = 42;
    void* storage = &token;
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, HibernateProcess(0, 1, nullptr, nullptr, &storage));
    EXPECT_EQ(&token, storage);
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, WakeupProcess(0, 1, nullptr, nullptr, &storage));
    EXPECT_EQ(&token, storage);
    EXPECT_EQ(42, token);
}

TEST(Core_CheckpointLib, UnsupportedOperationsAcceptNullStorage)
{
    void* storage = nullptr;
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, HibernateProcess(0, 1, "", "", &storage));
    EXPECT_EQ(nullptr, storage);
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, WakeupProcess(0, 1, "", "", &storage));
    EXPECT_EQ(nullptr, storage);

    // No assertion-only or dereference-based handling of an absent storage slot.
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, HibernateProcess(0, 1, nullptr, nullptr, nullptr));
    EXPECT_EQ(HIBERNATE_ERROR_GENERAL, WakeupProcess(0, 1, nullptr, nullptr, nullptr));
}

} // namespace Tests
} // namespace Thunder

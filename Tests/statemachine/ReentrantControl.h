#pragma once

#include <cstdint>

namespace Thunder {
namespace TestSupport {

    enum class ReentrantTrigger : uint8_t {
        Activate,
        Deactivate,
        Unavailable,
        Hibernate
    };

    extern ReentrantTrigger g_reentrantTrigger; // set by the test before activation
    extern uint32_t g_reentrantResult; // captured result of the re-entrant call

} // namespace TestSupport
} // namespace Thunder

#pragma once

#include "DiagnosticsTypes.h"

#include <mutex>
#include <vector>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

/**
 * @brief Fixed-capacity circular store with monotonically increasing cursors.
 */
class EventStore {
public:
    explicit EventStore(const uint32_t capacity);

    void Add(EventSnapshot event);
    EventPage Snapshot(const uint64_t afterSequence, const uint32_t limit) const;
    void Reset();

private:
    const uint32_t _capacity;
    mutable std::mutex _lock;
    std::vector<EventSnapshot> _events;
    uint64_t _nextSequence;
    uint64_t _overwritten;
};

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder

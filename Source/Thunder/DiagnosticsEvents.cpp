#include "DiagnosticsEvents.h"

#include <algorithm>

namespace Thunder {
namespace PluginHost {
namespace Diagnostics {

EventStore::EventStore(const uint32_t capacity)
    : _capacity(std::max<uint32_t>(capacity, 1))
    , _lock()
    , _events()
    , _nextSequence(1)
    , _overwritten(0)
{
    _events.reserve(_capacity);
}

void EventStore::Add(EventSnapshot event)
{
    std::lock_guard<std::mutex> guard(_lock);

    event.sequence = _nextSequence++;
    if (_events.size() == _capacity) {
        _events.erase(_events.begin());
        ++_overwritten;
    }
    _events.emplace_back(std::move(event));
}

EventPage EventStore::Snapshot(const uint64_t afterSequence, const uint32_t limit) const
{
    std::lock_guard<std::mutex> guard(_lock);

    EventPage page;
    page.overwritten = _overwritten;
    const uint32_t boundedLimit = std::max<uint32_t>(1, std::min<uint32_t>(limit, _capacity));

    for (const EventSnapshot& event : _events) {
        if (event.sequence <= afterSequence) {
            continue;
        }

        if (page.events.size() == boundedLimit) {
            page.truncated = true;
            break;
        }
        page.events.emplace_back(event);
    }

    page.nextSequence = (page.events.empty() == true ? afterSequence : page.events.back().sequence);
    return page;
}

void EventStore::Reset()
{
    std::lock_guard<std::mutex> guard(_lock);
    _events.clear();
    _overwritten = 0;
}

} // namespace Diagnostics
} // namespace PluginHost
} // namespace Thunder

#pragma once
#include "TraceMessage.h"

namespace WPEFramework {
namespace Trace {

    class EXTERNAL Factory : public Core::Messaging::IEventFactory {
    public:
        Factory()
            : _tracePool(2)
        {
        }
        ~Factory() override = default;
        Factory(const Factory&) = delete;
        Factory& operator=(const Factory&) = delete;

        inline Core::ProxyType<Core::Messaging::IEvent> Create() override
        {
            Core::ProxyType<Message> proxy = _tracePool.Element();
            return Core::ProxyType<Core::Messaging::IEvent>(proxy);
        }

    private:
        Core::ProxyPoolType<Message> _tracePool;
    };
}
}
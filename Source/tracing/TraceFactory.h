#pragma once
#include "TextMessage.h"

namespace WPEFramework {
namespace Messaging {

    class EXTERNAL TraceFactory : public Core::Messaging::IEventFactory {
    public:
        TraceFactory()
            : _tracePool(2)
        {
        }
        ~TraceFactory() override = default;
        TraceFactory(const TraceFactory&) = delete;
        TraceFactory& operator=(const TraceFactory&) = delete;

        inline Core::ProxyType<Core::Messaging::IEvent> Create() override
        {
            Core::ProxyType<TextMessage> proxy = _tracePool.Element();
            return Core::ProxyType<Core::Messaging::IEvent>(proxy);
        }

    private:
        Core::ProxyPoolType<Messaging::TextMessage> _tracePool;
    };
}
}
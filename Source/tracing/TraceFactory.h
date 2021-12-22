#pragma once
#include "TraceMessage.h"

namespace WPEFramework {
namespace Trace {

    class EXTERNAL Factory : public Core::IMessageEventFactory {
    public:
        Factory()
            : _tracePool(2)
        {
        }
        ~Factory() override = default;
        Factory(const Factory&) = delete;
        Factory& operator=(const Factory&) = delete;

        inline Core::ProxyType<Core::IMessageEvent> Create() override
        {
            Core::ProxyType<TraceMessage> proxy = _tracePool.Element();
            return Core::ProxyType<Core::IMessageEvent>(proxy);
        }

    private:
        Core::ProxyPoolType<TraceMessage> _tracePool;
    };
}
}
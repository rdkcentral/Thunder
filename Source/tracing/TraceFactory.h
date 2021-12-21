#pragma once
#include "TraceMessage.h"

namespace WPEFramework {
namespace Trace {

    class Factory : public Core::IMessageEventFactory {
    private:
        class DefaultTrace : public TraceMessage<DefaultTrace, nullptr> {
        public:
            using Base = TraceMessage<DefaultTrace, nullptr>;

            DefaultTrace()
            {
            }
        };

    public:
        Factory()
            : _tracePool(2)
        {
        }

        Core::ProxyType<Core::IMessageEvent> Create() override
        {
            Core::ProxyType<DefaultTrace> proxy = _tracePool.Element();
            return Core::ProxyType<Core::IMessageEvent>(proxy);
        }

    private:
        Core::ProxyPoolType<DefaultTrace> _tracePool;
    };
}
}
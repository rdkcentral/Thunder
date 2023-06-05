/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include "TextMessage.h"

namespace WPEFramework {

namespace Messaging {

    struct EXTERNAL IEventFactory {
        virtual ~IEventFactory() = default;
        virtual Core::ProxyType<Core::Messaging::IEvent> Create() = 0;
    };

    class EXTERNAL TraceFactory : public IEventFactory {
    public:
        TraceFactory(const TraceFactory&) = delete;
        TraceFactory& operator=(const TraceFactory&) = delete;

        TraceFactory() : _tracePool(2)
        {
        }
        ~TraceFactory() override = default;

    public:
        Core::ProxyType<Core::Messaging::IEvent> Create() override
        {
            Core::ProxyType<TextMessage> proxy = _tracePool.Element();
            
            return (Core::ProxyType<Core::Messaging::IEvent>(proxy));
        }

    private:
        Core::ProxyPoolType<Messaging::TextMessage> _tracePool;
    };

} // namespace Messaging
}

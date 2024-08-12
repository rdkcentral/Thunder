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

namespace Thunder {

namespace Messaging {

    struct EXTERNAL IEventFactory {
        virtual ~IEventFactory() = default;
        virtual Core::ProxyType<Core::Messaging::MessageInfo> GetMetadata() = 0;
        virtual Core::ProxyType<Core::Messaging::IEvent> GetMessage() = 0;
    };

    template<typename METADATA, typename EVENT>
    class TraceFactoryType : public IEventFactory {
    public:
        TraceFactoryType(const TraceFactoryType<METADATA, EVENT>&) = delete;
        TraceFactoryType<METADATA, EVENT>& operator=(const TraceFactoryType<METADATA, EVENT>&) = delete;

        TraceFactoryType() : _eventPool(2), _metadataPool(2)
        {
        }
        ~TraceFactoryType() override = default;

    public:
        Core::ProxyType<Core::Messaging::MessageInfo> GetMetadata() override
        {
            Core::ProxyType<METADATA> proxy = _metadataPool.Element();

            return (Core::ProxyType<Core::Messaging::MessageInfo>(proxy));
        }
        Core::ProxyType<Core::Messaging::IEvent> GetMessage() override
        {
            Core::ProxyType<EVENT> proxy = _eventPool.Element();

            return (Core::ProxyType<Core::Messaging::IEvent>(proxy));
        }

    private:
        Core::ProxyPoolType<EVENT> _eventPool;
        Core::ProxyPoolType<METADATA> _metadataPool;
    };

} // namespace Messaging
}

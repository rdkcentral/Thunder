/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#include "AssertionUnit.h"

namespace Thunder {
namespace Assertion {

    AssertionUnit::AssertionUnit()
    {
        AssertionUnitProxy::Instance().Handle(this);
    }

    AssertionUnit& AssertionUnit::Instance()
    {
        return (Core::SingletonType<AssertionUnit>::Instance());
    }

    AssertionUnit::~AssertionUnit()
    {
        AssertionUnitProxy::Instance().Handle(nullptr);
    }

    Core::Messaging::OutputTargets AssertionUnit::Targets(const Core::Messaging::Metadata& metadata, const Core::Messaging::OutputMode outputMode, const bool localEnabled) const
    {
        return (Thunder::Messaging::MessageUnit::Instance().Targets(metadata, outputMode, localEnabled));
    }

    void AssertionUnit::AssertionEvent(Core::Messaging::IStore::Assert& metadata, const Core::Messaging::TextMessage& message, const Core::Messaging::OutputTargets& targets)
    {
        Thunder::Messaging::MessageUnit::Instance().Push(metadata, &message, targets);
    }
}
}

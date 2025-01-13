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

#include "AssertionControl.h"
#include "Sync.h"

namespace Thunder {
namespace Assertion {

    // Announce upfront the general Assert category..
    ANNOUNCE_ASSERT_CONTROL

    AssertionUnitProxy::AssertionUnitProxy()
        : _handler(nullptr)
        , _adminLock(new Core::CriticalSection())
    {
    }

    AssertionUnitProxy::~AssertionUnitProxy()
    {
        delete _adminLock;
    }

    // Can't use Core::SingletonType since it uses ASSERTs
    AssertionUnitProxy& AssertionUnitProxy::Instance()
    {
        static AssertionUnitProxy AssertionUnitProxy;
        return (AssertionUnitProxy);
    }

    void AssertionUnitProxy::AssertionEvent(Core::Messaging::IStore::Assert& metadata, const Core::Messaging::TextMessage& message)
    {
        _adminLock->Lock();
        if (_handler != nullptr) {
            _handler->AssertionEvent(metadata, message);
        }
        _adminLock->Unlock();
    }

    void AssertionUnitProxy::Handle(IAssertionUnit* handler)
    {
        _adminLock->Lock();
        _handler = handler;
        _adminLock->Unlock();
    }
}
}
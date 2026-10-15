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

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        static class Instantiation {
        public:
            Instantiation()
            {
                ANNOUNCE_ASSERT_CONTROL
            }
        } ControlRegistration;

    }

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

    Core::Messaging::OutputTargets AssertionUnitProxy::Targets(const Core::Messaging::Metadata& metadata, const Core::Messaging::OutputMode outputMode, const bool localEnabled) const
    {
        Core::Messaging::OutputTargets targets = { localEnabled, false, false };

        _adminLock->LockWithoutWarning();
        if (_handler != nullptr) {
            targets = _handler->Targets(metadata, outputMode, localEnabled);
        }
        _adminLock->Unlock();

        return (targets);
    }

    void AssertionUnitProxy::AssertionEvent(Core::Messaging::IStore::Assert& metadata, const Core::Messaging::TextMessage& message, const Core::Messaging::OutputTargets& targets)
    {
        _adminLock->LockWithoutWarning();

        metadata.TimeStamp(Thunder::Core::Time::Now().Ticks());

        // print the ASSERT to stderr if handler is not set, possibly due to the messaging engine not being fully initialized yet
        if (_handler != nullptr) {
            _handler->AssertionEvent(metadata, message, targets);
            _adminLock->Unlock();
        }
        else {
            _adminLock->Unlock();
            ::fprintf(stderr, "%s%s\n", metadata.ToString(Core::Messaging::MessageInfo::abbreviate::FULL).c_str(), message.Data().c_str());
            ::fflush(stderr);
        }
    }

    void AssertionUnitProxy::Handle(IAssertionUnit* handler)
    {
        _adminLock->LockWithoutWarning();
        _handler = handler;
        _adminLock->Unlock();
    }
}
}

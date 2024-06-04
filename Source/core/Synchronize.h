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
 
#ifndef __SYNCHRONIZE_H
#define __SYNCHRONIZE_H

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    template <typename MESSAGE>
    class SynchronizeType {

    public:
        SynchronizeType(const SynchronizeType<MESSAGE>&) = delete;
        SynchronizeType<MESSAGE> operator=(const SynchronizeType<MESSAGE>&) = delete;
        SynchronizeType()
            : _response(nullptr)
            , _signal(false, true)
        {
        }
        virtual ~SynchronizeType() = default;

    public:
        template <typename INBOUND>
        bool Evaluate(const INBOUND& newMessage)
        {
            bool result = false;

            _adminLock.Lock();

            if ((_response != nullptr) && (_response->Copy(newMessage) == true)) {
                // It is the message we where waiting on. Signal succesfull retrieval.
                result = true;
                _response = nullptr;
                _signal.SetEvent();
            }

            _adminLock.Unlock();

            return (result);
        }
        inline void Flush()
        {
            _signal.SetEvent();
            _response = nullptr;
        }
        void Load(MESSAGE& response)
        {
            _adminLock.Lock();

            ASSERT(_response == nullptr);

            _signal.ResetEvent();

            _response = &response;

            _adminLock.Unlock();
        }
        void Load()
        {
            _adminLock.Lock();

            ASSERT(_response == nullptr);

            _signal.ResetEvent();

            _response = reinterpret_cast<MESSAGE*>(~0);

            _adminLock.Unlock();
        }
        void Evaluate()
        {
            _adminLock.Lock();

            if (_response == reinterpret_cast<MESSAGE*>(~0)) {
                _response = nullptr;
                _signal.SetEvent();
            }

            _adminLock.Unlock();
        }
        uint32_t Acquire(const uint32_t duration)
        {
            uint32_t result = _signal.Lock(duration);

            _adminLock.Lock();

            if (_response != nullptr) {
                // Seems it failed, question why?
                result = (_signal.IsSet() ? Core::ERROR_ASYNC_ABORTED : Core::ERROR_TIMEDOUT);
                _response = nullptr;
            }

            _adminLock.Unlock();

            return (result);
        }
        inline void Lock()
        {
            _adminLock.Lock();
        }
        inline void Unlock()
        {
            _adminLock.Unlock();
        }

    private:
        MESSAGE* _response;
        Core::Event _signal;
        Core::CriticalSection _adminLock;
    };
}
} // namespace Core

#endif // __SYNCHRONIZE_H

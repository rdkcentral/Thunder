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

#pragma once

#include "Ids.h"
#include "IIteratorType.h"

// @stubgen:include "IIteratorType.h"

namespace WPEFramework {
    namespace RPC {

        struct EXTERNAL IRemoteConnection : virtual public Core::IUnknown {
            enum { ID = ID_COMCONNECTION };

            virtual ~IRemoteConnection() = default;

            struct EXTERNAL INotification : virtual public Core::IUnknown {
                enum { ID = ID_COMCONNECTION_NOTIFICATION };

                virtual ~INotification() = default;
                
                virtual void Activated(IRemoteConnection*) = 0;
                virtual void Deactivated(IRemoteConnection*) = 0;
                virtual void Terminated(IRemoteConnection*) { };
            };

            virtual uint32_t Id() const = 0;
            virtual uint32_t RemoteId() const = 0;
            virtual void* /* @interface:interfaceId */ Acquire(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version) = 0;
            virtual void Terminate() = 0;
            virtual uint32_t Launch() = 0;
            virtual void PostMortem() = 0;

            template <typename REQUESTEDINTERFACE>
            REQUESTEDINTERFACE* Acquire(const uint32_t waitTime, const string& className, const uint32_t version)
            {
                void* baseInterface(Acquire(waitTime, className, REQUESTEDINTERFACE::ID, version));

                if (baseInterface != nullptr) {

                    return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
                }

                return (nullptr);
            }
        };

        typedef IIteratorType<string, ID_STRINGITERATOR> IStringIterator;
        typedef IIteratorType<uint32_t, ID_VALUEITERATOR> IValueIterator;
    }
}

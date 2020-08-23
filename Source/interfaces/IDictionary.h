/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Browser to change
    // Browser specific properties like displayed URL.
    struct EXTERNAL IDictionary : virtual public Core::IUnknown {
        enum { ID = ID_DICTIONARY };

        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_DICTIONARY_NOTIFICATION };

            // Signal changes on the subscribed namespace..
            virtual void Modified(const string& nameSpace, const string& key, const string& value) = 0;
        };

        struct EXTERNAL IIterator : virtual public Core::IUnknown {
            enum { ID = ID_DICTIONARY_ITERATOR };

            virtual void Reset() = 0;
            virtual bool IsValid() const = 0;
            virtual bool Next() = 0;

            // Signal changes on the subscribed namespace..
            virtual const string Key() const = 0;
            virtual const string Value() const = 0;
        };

        // Allow to observe values in the dictionary. If they are changed, the sink gets notified.
        virtual void Register(const string& nameSpace, struct IDictionary::INotification* sink) = 0;
        virtual void Unregister(const string& nameSpace, struct IDictionary::INotification* sink) = 0;

        // Getters and Setters for the dictionary.
        virtual bool Get(const string& nameSpace, const string& key, string& value /* @out */) const = 0;
        virtual bool Set(const string& nameSpace, const string& key, const string& value) = 0;
        virtual IIterator* Get(const string& nameSpace) const = 0;
    };
}
}


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
namespace ProcessContainers {

    using IStringIterator = Core::IteratorType<std::vector<string>, const string>;

    struct IContainerAdministrator {

        struct IContainer {
            IContainer() = default;
            virtual ~IContainer() = default;

            virtual const string& Id() const = 0;
            virtual pid_t Pid() const = 0;
            virtual bool IsRunning() const = 0;
            virtual bool Start(const string& command, IStringIterator& parameters) = 0; // returns true when started
            virtual bool Stop(const uint32_t timeout /*ms*/) = 0; // returns true when stopped, note if timeout == 0 asynchronous

            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;
        };

        static IContainerAdministrator& Instance();

        IContainerAdministrator() = default;
        virtual ~IContainerAdministrator() = default;

        // Lifetime management
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;

        // Methods
        virtual IContainer* Container(const string& id, 
                                      IStringIterator& searchpaths, 
                                      const string& logpath,
                                      const string& configuration) = 0; //searchpaths will be searched in order in which they are iterated

        virtual void Logging(const string& logpath, const string& logid, const string& loggingoptions) = 0;
    };
} // ProcessContainers
} // WPEFramework


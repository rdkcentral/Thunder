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

#include "Module.h"

namespace WPEFramework {

    namespace Messaging {

        /**
         * @brief Create a "direct" flush output, if needed..
         */
        class EXTERNAL DirectOutput {
        public:
            DirectOutput(const DirectOutput&) = delete;
            DirectOutput& operator=(const DirectOutput&) = delete;

            DirectOutput()
                : _baseTime(Core::Time::Now().Ticks())
                , _isSyslog(false)
                , _abbreviate(false)
            {
            }
            ~DirectOutput() = default;

        public:
            void Mode(const bool syslog, const bool abbreviated)
            {
                _isSyslog = syslog;
                _abbreviate = abbreviated;
            }
            
            void Output(const Core::Messaging::IStore::Logging& log, const Core::Messaging::IEvent* message) const;
            void Output(const Core::Messaging::IStore::Tracing& trace, const Core::Messaging::IEvent* message) const;

        private:
            uint64_t _baseTime;
            bool _isSyslog;
            bool _abbreviate;
        };

    } // namespace Messaging
}

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

        struct EXTERNAL IOutput {
            virtual ~IOutput() = default;
            virtual void Output(const Core::Messaging::IStore::Information& info, const Core::Messaging::IEvent* message) = 0;
        };

        /**
         * @brief Create a "direct" flush output, if needed..
         */
        class EXTERNAL LoggingOutput : public IOutput {
        public:
            LoggingOutput()
                : _baseTime(Core::Time::Now().Ticks())
                , _isSyslog(true)
                , _abbreviate(true)
            {
            }
            ~LoggingOutput() override = default;
            LoggingOutput(const LoggingOutput&) = delete;
            LoggingOutput& operator=(const LoggingOutput&) = delete;

        public:
            void IsBackground(bool background)
            {
                _isSyslog.store(background);
            }
            void IsAbbreviated(bool abbreviate)
            {
                _abbreviate.store(abbreviate);
            }

            void Output(const Core::Messaging::IStore::Information& info, const Core::Messaging::IEvent* message) override;

        private:
            string Prepare(const bool abbreviate, const Core::Messaging::IStore::Information& info, const Core::Messaging::IEvent* message) const;

        private:
            uint64_t _baseTime;
            std::atomic_bool _isSyslog;
            std::atomic_bool _abbreviate;
        };

    } // namespace Messaging
} // namespace WPEFramework

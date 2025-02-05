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
#include "OperationalCategories.h"
#include "MessageUnit.h"

namespace Thunder {
    namespace Messaging {
        class EXTERNAL StandardOut {
        public:
            StandardOut(StandardOut&&);
            StandardOut(const StandardOut&);
            StandardOut& operator=(StandardOut&&);
            StandardOut& operator=(const StandardOut&);

            StandardOut() = default;
            ~StandardOut() = default;

        public:
            void Output(const uint16_t length, const TCHAR buffer[])
            {
                if (OperationalStream::StandardOut::IsEnabled() == true) {
                    Core::Messaging::MessageInfo messageInfo(OperationalStream::StandardOut::Metadata(), Core::Time::Now().Ticks());
                    Core::Messaging::IStore::OperationalStream operationalStream(messageInfo);
                    Core::Messaging::TextMessage data(length, buffer);
                    MessageUnit::Instance().Push(operationalStream, &data);
                }
            }

        };

        class EXTERNAL StandardError {
        public:
            StandardError(StandardError&&);
            StandardError(const StandardError&);
            StandardError& operator=(StandardError&&);
            StandardError& operator=(const StandardError&);

            StandardError() = default;
            ~StandardError() = default;

        public:
            void Output(const uint16_t length, const TCHAR buffer[])
            {
                if (OperationalStream::StandardError::IsEnabled() == true) {
                    Core::Messaging::MessageInfo messageInfo(OperationalStream::StandardError::Metadata(), Core::Time::Now().Ticks());
                    Core::Messaging::IStore::OperationalStream operationalStream(messageInfo);
                    Core::Messaging::TextMessage data(length, buffer);
                    MessageUnit::Instance().Push(operationalStream, &data);
                }
            }
        };

        class EXTERNAL ConsoleStandardOut : public Core::TextStreamRedirectType<StandardOut> {
        public:
            ConsoleStandardOut(ConsoleStandardOut&&) = delete;
            ConsoleStandardOut(const ConsoleStandardOut&) = delete;
            ConsoleStandardOut& operator=(ConsoleStandardOut&&) = delete;
            ConsoleStandardOut& operator=(const ConsoleStandardOut&) = delete;

        private:
            ConsoleStandardOut();

        public:
            ~ConsoleStandardOut() = default;

            static ConsoleStandardOut& Instance()
            {
                static ConsoleStandardOut singleton;

                return (singleton);
            }
        };

        class EXTERNAL ConsoleStandardError : public Core::TextStreamRedirectType<StandardError> {
        public:
            ConsoleStandardError(ConsoleStandardError&&) = delete;
            ConsoleStandardError(const ConsoleStandardError&) = delete;
            ConsoleStandardError& operator=(ConsoleStandardError&&) = delete;
            ConsoleStandardError& operator=(const ConsoleStandardError&) = delete;

        private:
            ConsoleStandardError();

        public:
            ~ConsoleStandardError() = default;

            static ConsoleStandardError& Instance()
            {
                static ConsoleStandardError singleton;

                return (singleton);
            }
        };
    }
}

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

#include "DirectOutput.h"

namespace WPEFramework {

    namespace Messaging {

        /**
        * @brief Simply printing a message of any type to either syslog or console
        */
        void DirectOutput::Output(const Core::Messaging::MessageInfo& messageInfo, const Core::Messaging::IEvent* message) const
        {
            ASSERT(message != nullptr);
            ASSERT(messageInfo.Type() != Core::Messaging::Metadata::type::INVALID);

            string result = messageInfo.ToString(_abbreviate).c_str() +
                            Core::Format("%s\n", message->Data().c_str());

#ifndef __WINDOWS__
            if (_isSyslog == true) {
                //use longer messages for syslog
                syslog(LOG_NOTICE, "%s\n", result.c_str());
            }
            else
#endif
            {
                std::cout << result << std::endl;
            }
        }
    } // namespace Messaging
}

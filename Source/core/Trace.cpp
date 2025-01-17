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
 
#include <sstream>

#include "Sync.h"
#include "TextFragment.h"
#include "Trace.h"
#if defined(__APPLE__)
#include <pthread.h>
#endif

namespace Thunder {
namespace Core {

    const char* FileNameOnly(const char fileName[])
    {
        uint16_t index = static_cast<uint16_t>(strlen(fileName));
        bool found = false;

        if (index > 0) {
            do {
                index--;

                found = ((fileName[index] == '\\') || (fileName[index] == '/'));

            } while ((found == false) && (index != 0));
        }

        return (found == false ? fileName : &fileName[index + 1]);
    }

    string LogMessage(const TCHAR fileName[], const uint32_t lineNumber, const TCHAR* message)
    {
#ifdef _UNICODE
        std::wstringstream ss;
#else
        std::stringstream ss;
#endif

        ss << _T("[") << fileName << _T(":") << lineNumber << _T("]  == ") << message;

        return (ss.str());
    }
#if defined(__CORE_MESSAGING__) && defined(__WINDOWS__)
    const std::string& GetProgramName()
    {
        static std::string programName;

        if (programName.empty()) {

            char buffer[MAX_PATH];

            if (::GetModuleFileName(NULL, buffer, MAX_PATH)) {
                programName = FileNameOnly(buffer);
            } else {
                programName = "Unknown";
            }
        }
        return (programName);
    }
#endif
}
} // namespace Core

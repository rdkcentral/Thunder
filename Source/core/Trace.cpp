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

namespace Thunder {
namespace Core {
    class Demangling {
    public:
        Demangling(const Demangling&) = delete;
        Demangling& operator=(const Demangling&) = delete;

        Demangling() = default;
        ~Demangling() = default;

        inline TextFragment Demangled(const char name[])
        {
            char allocationName[512];
            size_t allocationSize = sizeof(allocationName) - 1;

#ifdef __LINUX__
            int status;
            char* demangledName = abi::__cxa_demangle(name, allocationName, &allocationSize, &status);
            std::string newName;

            // Check for, and deal with, error.
            if (demangledName == nullptr) {
                newName = allocationName;
            } else {
                newName = demangledName;
            }
#endif

#ifdef __WINDOWS__
            uint16_t index = 0;
            uint16_t moveTo = 0;
            while ((name[index] != '\0') && (moveTo < (allocationSize - 1))) {
                if ((name[index] == 'c') && (name[index + 1] == 'l') && (name[index + 2] == 'a') && (name[index + 3] == 's') && (name[index + 4] == 's') && (name[index + 5] == ' ')) {
                    // we need to skip class :-)
                    index += 6;
                } else if ((name[index] == 's') && (name[index + 1] == 't') && (name[index + 2] == 'r') && (name[index + 3] == 'u') && (name[index + 4] == 'c') && (name[index + 5] == 't') && (name[index + 6] == ' ')) {
                    // We need to skip struct
                    index += 7;
                } else if ((name[index] == 'e') && (name[index + 1] == 'n') && (name[index + 2] == 'u') && (name[index + 3] == 'm') && (name[index + 4] == ' ')) {
                    // We need to skip enum
                    index += 5;
                } else {
                    allocationName[moveTo++] = name[index++];
                }
            }
            allocationName[moveTo] = '\0';
            std::string newName(allocationName, moveTo);
#endif

            return (TextFragment(newName));
        }

        inline TextFragment ClassName(const char name[])
        {
            return(Demangled(name));
        }

        inline TextFragment ClassNameOnly(const char name[])
        {
            TextFragment result(Demangled(name));
            uint16_t index = 0;
            uint16_t lastIndex = static_cast<uint16_t>(~0);

            while ((index < result.Length()) && (result[index] != '<')) {
                if (result[index] == ':') {
                    lastIndex = index;
                }
                index++;
            }

            return (lastIndex < (index - 1) ? TextFragment(result, lastIndex + 1, result.Length() - (lastIndex + 1)) : result);
        }
  
    };

    static Demangling demangleClassNames;

    TextFragment ClassName(const char className[])
    {
        return (demangleClassNames.ClassName(className));
    }

    TextFragment ClassNameOnly(const char className[])
    {
        return (demangleClassNames.ClassNameOnly(className));
    }

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
}
} // namespace Core

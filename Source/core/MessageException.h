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
 
#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception> // For exception class

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"

namespace Thunder {
namespace Core {

    class MessageException : public std::exception {
    private:
        MessageException();

    public:
        MessageException(const string& message, bool inclSysMsg = false) throw();
        ~MessageException() throw();

        const TCHAR* Message() const throw();

    private:
        string m_Message; // Exception message
    };
}
} // namespace Exceptions

#endif // EXCEPTION_H

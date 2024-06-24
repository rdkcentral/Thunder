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
 
#ifndef XGETOPT_H
#define XGETOPT_H

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    class EXTERNAL Options {
    public:
        Options() = delete;
        Options(const Options&) = delete;
        Options& operator=(const Options&) = delete;

        Options(int argumentCount, TCHAR* arguments[], const TCHAR options[])
            : _argumentCount(argumentCount)
            , _arguments(arguments)
            , _options(options)
            , _command(nullptr)
            , _valid(false)
            , _requestUsage(false)
        {
        }
        virtual ~Options() = default;

    public:
        inline bool HasErrors() const
        {
            return (!_valid);
        }
        inline const TCHAR* Command() const
        {
            return (_command);
        }
        inline bool RequestUsage() const
        {
            return (_valid || _requestUsage);
        }

        virtual void Option(const TCHAR option, const TCHAR* argument) = 0;
        void Parse();

    protected:
        inline void RequestUsage(const bool value)
        {
            _requestUsage = value;
        }

    private:
        int _argumentCount;
        TCHAR** _arguments;
        const TCHAR* _options;
        TCHAR* _command;
        bool _valid;
        bool _requestUsage;
    };
}
} // namespace Core

#endif //XGETOPT_H

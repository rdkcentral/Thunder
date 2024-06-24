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
 
#pragma once

#include <cstring>

#include "Module.h"
#include "core.h"

namespace Thunder {
namespace Core {
    class EXTERNAL Options {
    public:
        Options() = delete;
        Options(const Options&) = delete;
        Options& operator=(const Options&) = delete;
        Options(Options&&) = delete;
        Options& operator=(Options&&) = delete;

        Options(int argumentCount, const TCHAR* const arguments[], const TCHAR options[])
            : _argumentCount{argumentCount}
            , _arguments{nullptr}
            , _options{nullptr}
            , _command(nullptr)
            , _valid{false}
            , _requestUsage{false}
        {
            ASSERT(argumentCount > 0);
            ASSERT(options != nullptr);

            ::size_t count = ::strlen(arguments[0]) + 1;
            ASSERT(count > 1);

            _arguments = new TCHAR*[argumentCount];
            ASSERT(_arguments != nullptr);

            for(int i = 0; i < argumentCount; i++) {
                _arguments[i] = new TCHAR[count];
                ASSERT(_arguments[i] != nullptr);
                ::strncpy(_arguments[i], arguments[i], count);
            }

            count = ::strlen(options) + 1;
            ASSERT(count > 1);

            _options =  new TCHAR[count];
            ASSERT(_options != nullptr);
            ::strncpy(_options, options, count); 

        }
        virtual ~Options()
        {
            for(int i = 0; i < _argumentCount; i++) {
                delete [] _arguments[i];
            }
            delete [] _arguments;

            delete [] _options;
        }

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
        TCHAR* _options;
        TCHAR* _command;
        bool _valid;
        bool _requestUsage;
    };
}
} // namespace Core


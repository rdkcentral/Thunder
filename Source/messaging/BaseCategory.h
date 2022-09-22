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

#define _INTERNAL_DEFINE_CATEGORY(BASE, CATEGORY) \
    class EXTERNAL CATEGORY : public BASE {                 \
    public:                                                 \
        using BASE::BASE;                                   \
        ~CATEGORY() = default;                              \
        CATEGORY(const CATEGORY&) = delete;                 \
        CATEGORY& operator=(const CATEGORY&) = delete;      \
    }

#define DEFINE_TRACING_CATEGORY(CATEGORY) _INTERNAL_DEFINE_CATEGORY(BaseCategory, CATEGORY)

namespace WPEFramework {
namespace Trace {

    class BaseCategory {
    public:
        ~BaseCategory() = default;
        BaseCategory& operator=(const BaseCategory&) = delete;

        template<typename... Args>
        BaseCategory(const string& formatter, Args... args)
            : _text()
        {
            Core::Format(_text, formatter.c_str(), args...);
        }

        BaseCategory(const string& text)
            : _text(text)
        {
        }

        BaseCategory()
            : _text()
        {
        }

    public:
        const char* Data() const
        {
            return (_text.c_str());
        }

        uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    protected:
        void Set(const string& text)
        {
            _text = text;
        }

    private:
        std::string _text;
    };

} // namespace Trace
}
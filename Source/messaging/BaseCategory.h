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

namespace Thunder {

namespace Messaging {

    template <const Core::Messaging::Metadata::type TYPE>
    class BaseCategoryType {
    public:
        ~BaseCategoryType() = default;
        BaseCategoryType& operator=(const BaseCategoryType&) = delete;

        template<typename... Args>
        BaseCategoryType(const string& formatter, Args... args)
            : _text()
        {
            Core::Format(_text, formatter.c_str(), args...);
        }

        BaseCategoryType(const string& text)
            : _text(text)
        {
        }

        BaseCategoryType()
            : _text()
        {
        }

    public:
        using BaseCategory = BaseCategoryType<TYPE>;

        static constexpr Core::Messaging::Metadata::type Type = TYPE;

        const char* Data() const {
            return (_text.c_str());
        }

        uint16_t Length() const {
            return (static_cast<uint16_t>(_text.length()));
        }

    protected:
        void Set(const string& text) {
            _text = text;
        }

    private:
        string _text;
    };

} // namespace Messaging
}

#define DEFINE_MESSAGING_CATEGORY(BASECATEGORY, CATEGORY)   \
    class EXTERNAL CATEGORY : public BASECATEGORY {         \
    private:                                                \
        using BaseClass = BASECATEGORY;                     \
    public:                                                 \
        using BaseClass::BaseClass;                         \
        CATEGORY() = default;                               \
        ~CATEGORY() = default;                              \
        CATEGORY(const CATEGORY&) = delete;                 \
        CATEGORY& operator=(const CATEGORY&) = delete;      \
    };

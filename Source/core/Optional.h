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
 
#ifndef __OPTIONAL_H
#define __OPTIONAL_H

#include "Portability.h"

namespace Thunder {
namespace Core {
    template <typename TYPE>
    class OptionalType {
    public:
        OptionalType()
            : m_Value()
            , m_Set(false)
        {
        }

        OptionalType(const TYPE& value)
            : m_Value(value)
            , m_Set(true)
        {
        }

        OptionalType(const OptionalType<TYPE>& value)
            : m_Value(value.m_Value)
            , m_Set(value.m_Set)
        {
        }

        OptionalType(OptionalType<TYPE>&& value)
            : m_Value(std::move(value.m_Value))
            , m_Set(value.m_Set)
        {
            value.m_Set = false;
        }


        ~OptionalType()
        {
        }

        inline OptionalType<TYPE>& operator=(const OptionalType<TYPE>& value)
        {
            m_Value = value.m_Value;
            m_Set = value.m_Set;

            return (*this);
        }

        inline OptionalType<TYPE>& operator=(OptionalType<TYPE>&& move)
        {
            if (this != &move) {
                m_Value = std::move(move.m_Value);
                m_Set = move.m_Set;

                move.m_Set = false;
            }
            return (*this);
        }

        inline OptionalType<TYPE>& operator=(const TYPE& value)
        {
            m_Value = value;
            m_Set = true;

            return (*this);
        }

        inline bool operator==(const TYPE& value) const
        {
            return (value == m_Value);
        }

        inline bool operator==(const OptionalType<TYPE>& value) const
        {
            return ((value.m_Set == m_Set) && operator==(value.m_Value));
        }

        inline bool operator!=(const TYPE& value) const
        {
            return (!operator==(value));
        }

        inline bool operator!=(const OptionalType<TYPE>& value) const
        {
            return (!operator==(value));
        }

    public:
        bool IsSet() const
        {
            return (m_Set);
        }
        inline void Clear()
        {
            m_Set = false;
            m_Value = TYPE{};
        }

        operator TYPE&()
        {
            return (m_Value);
        }

        operator const TYPE&() const
        {
            return (m_Value);
        }

        TYPE& Value()
        {
            return (m_Value);
        }

        const TYPE& Value() const
        {
            return (m_Value);
        }

    private:
        TYPE m_Value;
        bool m_Set;
    };
}
} // namespace Core

#endif // __OPTIONAL_H

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

#ifndef __ISO639_H
#define __ISO639_H

#include "Module.h"
#include "Portability.h"
#include "Trace.h"

namespace Thunder {
namespace Core {

    struct ISO639Entry {
        const TCHAR* CharacterCode3;
        const TCHAR* CharacterCode2;
        const TCHAR* Description;
    };

    class EXTERNAL Language {
    public:
        Language()
            : m_SelectedLanguage(nullptr)
            , m_Index(NUMBER_MAX_UNSIGNED(uint16_t))
        {
        }
        explicit Language(const TCHAR* language, const uint32_t length = NUMBER_MAX_UNSIGNED(uint16_t))
            : m_SelectedLanguage(nullptr)
            , m_Index(NUMBER_MAX_UNSIGNED(uint16_t))
        {
            FindLanguage(language, length);
        }
        explicit Language(const uint16_t ID)
            : m_SelectedLanguage(nullptr)
            , m_Index(NUMBER_MAX_UNSIGNED(uint16_t))
        {
            FindLanguage(ID);
        }
        Language(const Language& copy)
            : m_SelectedLanguage(copy.m_SelectedLanguage)
            , m_Index(copy.m_Index)
        {
        }
        Language(Language&& move)
            : m_SelectedLanguage(std::move(move.m_SelectedLanguage))
            , m_Index(std::move(move.m_Index))
        {
            move.m_SelectedLanguage = nullptr;
        }
        ~Language()
        {
        }

        Language& operator=(const Language& RHS)
        {
            m_SelectedLanguage = RHS.m_SelectedLanguage;
            m_Index = RHS.m_Index;

            return (*this);
        }
        Language& operator=(Language&& move)
        {
            if (this != &move) {
                m_SelectedLanguage = std::move(move.m_SelectedLanguage);
                m_Index = std::move(move.m_Index);

                move.m_SelectedLanguage = nullptr;
            }
            return (*this);
        }
    public:
        inline bool IsValid() const
        {
            return (m_SelectedLanguage != nullptr);
        }
        inline const TCHAR* LetterCode2() const
        {
            ASSERT(IsValid() == true);

            return (m_SelectedLanguage->CharacterCode2);
        }
        inline const TCHAR* LetterCode3() const
        {
            ASSERT(IsValid() == true);

            return (m_SelectedLanguage->CharacterCode3);
        }
        inline const TCHAR* Description() const
        {
            return (IsValid() == true ? m_SelectedLanguage->Description : _T("Unknown"));
        }
        inline uint16_t Id() const
        {
            return (m_Index);
        }
        inline bool operator==(const Language& RHS) const
        {
            return (m_Index == RHS.m_Index);
        }
        inline bool operator!=(const Language& RHS) const
        {
            return !(operator==(RHS));
        }

    private:
        void FindLanguage(const TCHAR* language, const uint32_t length);
        void FindLanguage(const uint16_t ID);

    private:
        ISO639Entry* m_SelectedLanguage;
        uint16_t m_Index;
    };
}
} // namespace Core

#endif // __ISO639_H

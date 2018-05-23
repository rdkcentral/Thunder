#ifndef __ISO639_H
#define __ISO639_H

#include "Module.h"
#include "Portability.h"
#include "Trace.h"

namespace WPEFramework {
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
        ~Language()
        {
        }

        Language& operator=(const Language& RHS)
        {
            m_SelectedLanguage = RHS.m_SelectedLanguage;
            m_Index = RHS.m_Index;

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

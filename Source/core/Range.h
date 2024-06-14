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
 
#ifndef __RANGETYPE_H
#define __RANGETYPE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace Thunder {
namespace Core {
    template <typename TYPE, const bool BEGININCLUSIVE, const bool ENDINCLUSIVE>
    class RangeType {
    public:
        RangeType()
            : m_Minumum()
            , m_Maximum()
        {
        }

        RangeType(const TYPE& min, const TYPE& max)
            : m_Minumum(min)
            , m_Maximum(max)
        {
        }

        RangeType(const RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& copy)
            : m_Minumum(copy.m_Minumum)
            , m_Maximum(copy.m_Maximum)
        {
        }

        RangeType(RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>&& move)
            : m_Minumum(std::move(move.m_Minumum))
            , m_Maximum(std::move(move.m_Maximum))
        {
        }

        ~RangeType()
        {
        }

        inline RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& operator=(const RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& RHS)
        {
            m_Minumum = RHS.m_Minumum;
            m_Maximum = RHS.m_Maximum;

            return (*this);
        }

        inline RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& operator=(RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>&& move)
        {
            if (this != &move) {
                m_Minumum = std::move(move.m_Minumum);
                m_Maximum = std::move(move.m_Maximum);
            }
            return (*this);
        }

        inline bool operator==(const RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& RHS) const
        {
            return (m_Minumum == RHS.m_Minumum) && (m_Maximum == RHS.m_Maximum);
        }

        inline bool operator!=(const RangeType<TYPE, BEGININCLUSIVE, ENDINCLUSIVE>& value) const
        {
            return (!operator==(value));
        }

    public:
        inline bool IsValid() const
        {
            return (m_Minumum <= m_Maximum);
        }
        //TYPE Random()
        //{
        //    TYPE range = m_Maximum - m_Minumum - (BEGININCLUSIVE == false ? 1 : 0) - (ENDINCLUSIVE == false ? 1 : 0);

        //    TYPE base = static_cast<TYPE>(::rand());
        //    base = base % range;

        //    return (m_Minumum + (BEGININCLUSIVE == false ? 1 : 0) + base);
        //}

        inline TYPE Range() const
        {
            if ((m_Maximum == m_Minumum) && ((BEGININCLUSIVE == false) || (ENDINCLUSIVE == false))) {
                return (0);
            }

            return (1 + m_Maximum - m_Minumum - (BEGININCLUSIVE == false ? 1 : 0) - (ENDINCLUSIVE == false ? 1 : 0));
        }

        inline TYPE Minimum() const
        {
            return (m_Minumum);
        }

        inline TYPE Maximum() const
        {
            return (m_Maximum);
        }

        inline bool InRange(const TYPE& RHS) const
        {
            return (((BEGININCLUSIVE == true) && (RHS >= m_Minumum)) || ((BEGININCLUSIVE == false) && (RHS > m_Minumum))) && (((ENDINCLUSIVE == true) && (RHS <= m_Maximum)) || ((ENDINCLUSIVE == false) && (RHS < m_Maximum)));
        }

    private:
        TYPE m_Minumum;
        TYPE m_Maximum;
    };
}
} // namespace Core

#endif // __RANGETYPE_H

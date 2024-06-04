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
 
#include <math.h>

#include "Number.h"

namespace Thunder {
namespace Core {
    extern "C" {
    unsigned char FromDigits(const TCHAR element)
    {
        return (isdigit(element) ? (element - '0') : 0);
    }
    unsigned char FromHexDigits(const TCHAR element)
    {
        if (isdigit(element)) {
            return (element - '0');
        } else if ((element >= 'a') && (element <= 'f')) {
            return (element - 'a' + 10);
        } else if ((element >= 'A') && (element <= 'F')) {
            return (element - 'A' + 10);
        }
        return (0);
    }
    unsigned char FromBase64(const TCHAR element)
    {
        if (isdigit(element)) {
            return (element - '0' + 52);
        } else if ((element >= 'a') && (element <= 'z')) {
            return (element - 'a' + 26);
        } else if ((element >= 'A') && (element <= 'Z')) {
            return (element - 'A');
        } else if (element == '+') {
            return (62);
        } else if (element == '/') {
            return (63);
        }
        return (0);
    }
    unsigned char FromDirect(const TCHAR element)
    {
        return static_cast<unsigned int>(element & 0xFF);
    }
    TCHAR ToDigits(const unsigned char element)
    {
        return (element < 10 ? static_cast<TCHAR>(element + '0') : '0');
    }
    TCHAR ToHexDigits(const unsigned char element)
    {
        if (element < 10) {
            return (static_cast<TCHAR>(element + '0'));
        } else if (element < 16) {
            return static_cast<TCHAR>((element - 10) + 'A');
        }
        return ('0');
    }
    TCHAR ToBase64(const unsigned char element)
    {
        static const TCHAR conversion[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        return (element < 64 ? conversion[element] : '0');
    }
    TCHAR ToDirect(const unsigned char element)
    {
        return static_cast<TCHAR>(element);
    }
    }

    Fractional::Fractional()
        : m_Integer(0)
        , m_Remainder(0)
    {
    }

    Fractional::Fractional(const int32_t& integer, const uint32_t& remainder)
        : m_Integer(integer)
        , m_Remainder(remainder)
    {
    }

    Fractional::Fractional(const Fractional& copy)
        : m_Integer(copy.m_Integer)
        , m_Remainder(copy.m_Remainder)
    {
    }

    Fractional::Fractional(Fractional&& move)
        : m_Integer(move.m_Integer)
        , m_Remainder(move.m_Remainder)
    {
        move.m_Integer = 0;
        move.m_Remainder = 0;
    }

    /* virtual */ Fractional::~Fractional()
    {
    }

    Fractional& Fractional::operator=(const Fractional& RHS)
    {
        m_Integer = RHS.m_Integer;
        m_Remainder = RHS.m_Remainder;

        return (*this);
    }

    Fractional& Fractional::operator=(Fractional&& move)
    {
        if (this != &move) {
            m_Integer = move.m_Integer;
            m_Remainder = move.m_Remainder;

            move.m_Integer = 0;
            move.m_Remainder = 0;
        }
        return (*this);
    }
}
} // namespace Solution::Core

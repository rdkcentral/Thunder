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
 
#include "TextReader.h"

namespace Thunder {
namespace Core {

    TextFragment TextReader::ReadLine() const
    {
        const uint32_t location = m_Index;
        m_Index = static_cast<uint32_t>(m_DataBlock.SearchNumber<TCHAR, ENDIAN_PLATFORM>(location, '\n'));
        uint32_t length = m_Index - static_cast<uint32_t>(location);

        if (m_Index < m_DataBlock.Size()) {
            // See if we have a LINE_FEED just in front of us..
            if ((length > sizeof(TCHAR)) && (m_DataBlock.GetNumber<TCHAR, ENDIAN_PLATFORM>(m_Index - sizeof(TCHAR)) == '\r')) {
                length -= sizeof(TCHAR);
            }
            else if (((m_Index + sizeof(TCHAR)) < m_DataBlock.Size()) && (m_DataBlock.GetNumber<TCHAR, ENDIAN_PLATFORM>(m_Index + sizeof(TCHAR)) == '\r')) {
                // Seems we did not end up at the end so we found the EOLN marker. Jump over it.
                m_Index += (2 * sizeof(TCHAR));
            }
            else {
                // Seems we did not end up at the end so we found the EOLN marker and there is no LF. Jump over it.
                m_Index += sizeof(TCHAR);
            }
        }

        return (length == 0 ? TextFragment() : TextFragment(reinterpret_cast<const TCHAR*>(&(m_DataBlock[static_cast<uint32_t>(location)])), length));
    }
}
} //namespace Solution::Core

#include "TextReader.h"

namespace WPEFramework {
namespace Core {
    TextFragment TextReader::ReadLine() const
    {
        const uint64_t location = m_Index;
        m_Index = static_cast<uint32_t>(m_DataBlock.SearchNumber<TCHAR, ENDIAN_PLATFORM>(location, static_cast<TCHAR>('\n')));
        uint32_t length = m_Index - static_cast<const uint32_t>(location);

        if (m_Index < m_DataBlock.Size()) {
            // Seems we did not end up at the end so we found the EOLN marker. Jump over it.
            m_Index += sizeof(TCHAR);
        }

        return (length == 0 ? TextFragment() : TextFragment(reinterpret_cast<const TCHAR*>(&(m_DataBlock[static_cast<uint32_t>(location)])), length));
    }
}
} //namespace Solution::Core

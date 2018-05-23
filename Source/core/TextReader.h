#ifndef __TEXTREADER_H
#define __TEXTREADER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "TextFragment.h"
#include "DataElement.h"

namespace WPEFramework {
namespace Core {

    // ---- Referenced classes and types ----

    // ---- Helper types and constants ----

    // ---- Helper functions ----
    class EXTERNAL TextReader {
    public:
        TextReader()
            : m_Index(0)
            , m_DataBlock()
        {
        }
        TextReader(const DataElement& element, const uint32_t offset = 0)
            : m_Index(offset)
            , m_DataBlock(element)
        {
        }
        TextReader(const TextReader& copy)
            : m_Index(copy.m_Index)
            , m_DataBlock(copy.m_DataBlock)
        {
        }
        ~TextReader()
        {
        }

        TextReader& operator=(const TextReader& rhs)
        {
            m_Index = rhs.m_Index;
            m_DataBlock = rhs.m_DataBlock;

            return (*this);
        }

    public:
        inline void Reset()
        {
            m_Index = 0;
        }

        inline bool EndOfText() const
        {
            return (m_Index >= static_cast<uint32_t>(m_DataBlock.Size()));
        }

        TextFragment ReadLine() const;

    private:
        mutable uint32_t m_Index;
        DataElement m_DataBlock;
    };
}
} // namespace Core

#endif // __TEXTREADER_H

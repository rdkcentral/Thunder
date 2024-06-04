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
 
#pragma once

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "DataElement.h"
#include "TextFragment.h"

namespace Thunder {
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
        TextReader(DataElement& element, const uint32_t offset = 0)
            : m_Index(offset)
            , m_DataBlock(element)
        {
        }
        TextReader(const TextReader& copy)
            : m_Index(copy.m_Index)
            , m_DataBlock(copy.m_DataBlock)
        {
        }
        TextReader(TextReader&& move)
            : m_Index(move.m_Index)
            , m_DataBlock(std::move(move.m_DataBlock))
        {
            move.m_Index = 0;
        }
        ~TextReader() = default;

        TextReader& operator=(const TextReader& rhs)
        {
            m_Index = rhs.m_Index;
            m_DataBlock = rhs.m_DataBlock;

            return (*this);
        }
        TextReader& operator=(TextReader&& move)
        {
            if (this != &move) {
                m_Index = move.m_Index;
                m_DataBlock = std::move(move.m_DataBlock);

                move.m_Index = 0;
            }
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

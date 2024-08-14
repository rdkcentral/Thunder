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
 
#include "Parser.h"

namespace Thunder {
namespace Core {

    uint32_t TextParser::ReadText(const TCHAR delimiters[], const uint32_t offset) const
    {
        uint32_t endPoint = 0;

        if (*Data() == '\"') {
            endPoint = ForwardFind(_T("\""), offset + 1);
        } else {
            endPoint = ForwardFind(delimiters, offset);
        }

        return (endPoint);
    }

    void TextParser::ReadText(OptionalType<TextFragment>& result, const TCHAR delimiters[])
    {
        uint32_t marker = ForwardSkip(_T("\t "));

        uint32_t endPoint = ReadText(delimiters, marker);

        if ((endPoint != marker) && (endPoint <= Length())) {
            result = TextFragment(*this, marker, endPoint - marker);
            Forward(endPoint);
        }
    }

    void PathParser::Parse(const TextFragment& input)
    {
        TextParser parser(input);
        OptionalType<TextFragment> info;

        parser.ReadText(info, _T(":"));

        if ((info.IsSet()) && (info.Value().Length() == 1)) {
            m_Drive = *(info.Value().Data());
            parser.Skip(1);
        }

        // Find the last '/ or \ from the back, after the drive
        uint32_t index = parser.ReverseFind(_T("\\/"));

        if (index != NUMBER_MAX_UNSIGNED(uint32_t)) {
            m_Path = TextFragment(parser, 0, index);
            parser.Skip(index + 1);
        }

        // Now we are ate the complete filename
        m_FileName = TextFragment(parser, 0, NUMBER_MAX_UNSIGNED(uint32_t));

        // Find the extension from the current parser...
        index = parser.ReverseFind(_T("."));

        if (index == NUMBER_MAX_UNSIGNED(uint32_t)) {
            // oops there is no extension, BaseFileName == Filename
            m_BaseFileName = m_FileName;
        } else {
            m_BaseFileName = TextFragment(parser, 0, index);
            m_Extension = TextFragment(parser, index + 1, NUMBER_MAX_UNSIGNED(uint32_t));
        }
    }
}
} // namespace Core

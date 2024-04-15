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
 
#ifndef __TEXTFRAGMENT_H
#define __TEXTFRAGMENT_H

#include "Module.h"
#include "Portability.h"
#include "Trace.h"

namespace Thunder {
namespace Core {
    class Fractional;

    class EXTERNAL TextFragment {
    protected:
        class EXTERNAL Index {
        public:
            inline Index(uint32_t begin, uint32_t length)
                : m_Begin(begin)
                , m_Length(length)
            {
            }
            inline Index(const Index& copy, uint32_t offset, uint32_t length)
                : m_Begin(copy.m_Begin + offset)
            {
                if (length == NUMBER_MAX_UNSIGNED(uint32_t)) {
                    ASSERT(offset <= copy.m_Length);

                    m_Length = copy.m_Length - offset;
                } else {
                    ASSERT((length + offset) <= copy.m_Length);
                    m_Length = length;
                }
            }
            inline Index(const Index& copy)
                : m_Begin(copy.m_Begin)
                , m_Length(copy.m_Length)
            {
            }
            inline Index(Index&& move)
                : m_Begin(move.m_Begin)
                , m_Length(move.m_Length)
            {
                move.m_Begin = 0;
                move.m_Length = 0;
            }
            inline ~Index()
            {
            }

            inline Index& operator=(const Index& RHS)
            {
                m_Begin = RHS.m_Begin;
                m_Length = RHS.m_Length;

                return (*this);
            }

            inline Index& operator=(Index&& move)
            {
                if (this != &move) {
                    m_Begin = move.m_Begin;
                    m_Length = move.m_Length;

                    move.m_Begin = 0;
                    move.m_Length = 0;
                }
                return (*this);
            }

            inline uint32_t Begin() const
            {
                return (m_Begin);
            }

            inline uint32_t End() const
            {
                return (m_Begin + m_Length);
            }

            inline uint32_t Length() const
            {
                return (m_Length);
            }

            inline void Increment(const uint32_t offset)
            {
                if (offset > m_Length) {
                    m_Begin += m_Length;
                    m_Length = 0;
                } else {
                    m_Begin += offset;
                    m_Length -= offset;
                }
            }

            inline void Decrement(const uint32_t offset)
            {
                if (offset > m_Begin) {
                    m_Length += m_Begin;
                    m_Begin = 0;
                } else {
                    m_Begin -= offset;
                    m_Length += offset;
                }
            }
            inline void SetIndexInfo(const uint32_t offset, const uint32_t length)
            {
                m_Begin = offset;
                m_Length = length;
            }

        private:
            uint32_t m_Begin;
            uint32_t m_Length;
        };

    private:
        inline void SetIndexInfo(const uint32_t offset, const uint32_t length)
        {
            m_Index.SetIndexInfo(offset, length);
        }

    public:
        TextFragment()
            : m_Index(0, 0)
            , m_Start(nullptr)
            , m_Buffer()
        {
        }
        explicit TextFragment(const TCHAR text[])
            : m_Index(0, static_cast<uint32_t>(_tcslen(text)))
            , m_Start(text)
            , m_Buffer()
        {
        }
        explicit TextFragment(const TCHAR text[], const uint32_t length)
            : m_Index(0, length)
            , m_Start(text)
            , m_Buffer()
        {
        }
        TextFragment(const TCHAR text[], const uint32_t offset, const uint32_t length)
            : m_Index(offset, length)
            , m_Start(text)
            , m_Buffer()
        {
            ASSERT(m_Index.End() <= _tcslen(text));
        }
        explicit TextFragment(const string& text)
            : m_Index(0, static_cast<uint32_t>(text.length()))
            , m_Start(nullptr)
            , m_Buffer(text)
        {
        }
        TextFragment(const string& text, const uint32_t offset, const uint32_t length)
            : m_Index(offset, length)
            , m_Start(nullptr)
            , m_Buffer(text)
        {
            ASSERT(m_Index.End() <= text.length());
        }
        TextFragment(const TextFragment& base, const uint32_t offset, const uint32_t length)
            : m_Index(base.m_Index, offset, length)
            , m_Start(base.m_Start)
            , m_Buffer(base.m_Buffer)
        {
        }
        TextFragment(TextFragment&& move, const uint32_t offset, const uint32_t length)
            : m_Index(std::move(move.m_Index), offset, length)
            , m_Start(move.m_Start)
            , m_Buffer(std::move(move.m_Buffer))
        {
            move.m_Start = nullptr;
        }
        TextFragment(const TextFragment& copy)
            : m_Index(copy.m_Index)
            , m_Start(copy.m_Start)
            , m_Buffer(copy.m_Buffer)
        {
        }
        TextFragment(TextFragment&& move)
            : m_Index(std::move(move.m_Index))
            , m_Start(move.m_Start)
            , m_Buffer(std::move(move.m_Buffer))
        {
            move.m_Start = nullptr;
        }
        ~TextFragment()
        {
        }

        TextFragment& operator=(const TextFragment& RHS)
        {
            m_Index = RHS.m_Index;
            m_Start = RHS.m_Start;
            m_Buffer = RHS.m_Buffer;

            return (*this);
        }
        TextFragment& operator=(TextFragment&& move)
        {
            if (this != &move) {
                m_Index = std::move(move.m_Index);
                m_Start = move.m_Start;
                m_Buffer = std::move(move.m_Buffer);

                move.m_Start = nullptr;
            }
            return (*this);
        }
        inline void Clear()
        {
            m_Index.SetIndexInfo(0, 0);
            m_Start = nullptr,
            m_Buffer.clear();
        }
        inline const TCHAR& operator[](const uint32_t index) const
        {
            ASSERT(index < m_Index.Length());

            return (m_Start == nullptr ? m_Buffer[m_Index.Begin() + index] : m_Start[m_Index.Begin() + index]);
        }

        inline bool operator==(const TextFragment& RHS) const
        {
            return (equal_case_sensitive(RHS));
        }

        inline bool operator==(const TCHAR RHS[]) const
        {
            return (equal_case_sensitive(TextFragment(RHS, 0, static_cast<uint32_t>(_tcslen(RHS)))));
        }

        inline bool operator==(const string& RHS) const
        {
            return (equal_case_sensitive(TextFragment(RHS)));
        }

        inline bool operator!=(const TextFragment& RHS) const
        {
            return (!operator==(RHS));
        }

        inline bool operator!=(const string& RHS) const
        {
            return (!operator==(RHS));
        }

        inline bool operator!=(const TCHAR RHS[]) const
        {
            return (!operator==(RHS));
        }

        inline bool OnMarker(const TCHAR characters[]) const
        {
            return (on_given_character(0, characters));
        }

        inline bool IsEmpty() const
        {
            return (m_Index.Length() == 0);
        }

        inline uint32_t Length() const
        {
            return (m_Index.Length());
        }

        inline const TCHAR* Data() const
        {
            return (m_Start == nullptr ? &(m_Buffer[m_Index.Begin()]) : &m_Start[m_Index.Begin()]);
        }

        inline const string Text() const
        {
            return (m_Start == nullptr ? (((m_Index.Begin() == 0) && (m_Index.Length() == m_Buffer.length())) ? m_Buffer : m_Buffer.substr(m_Index.Begin(), m_Index.Length())) : string(&(m_Start[m_Index.Begin()]), m_Index.Length()));
        }

        inline bool EqualText(const TextFragment& RHS, const bool caseSensitive = false) const
        {
            return (caseSensitive == true ? equal_case_sensitive(RHS) : equal_case_insensitive(RHS));
        }

        inline bool EqualText(const TCHAR* compare, const uint32_t offset VARIABLE_IS_NOT_USED = 0, const uint32_t length = 0, const bool caseSensitive = true) const
        {
            uint32_t size = (length != 0 ? length : static_cast<uint32_t>(_tcslen(compare)));

            return (caseSensitive == true ? equal_case_sensitive(TextFragment(compare, size)) : equal_case_insensitive(TextFragment(compare, size)));
        }

        inline uint32_t ForwardFind(const TCHAR delimiter, const uint32_t offset = 0) const
        {
            return (find_first_of(offset, delimiter));
        }

        inline uint32_t ForwardFind(const TCHAR delimiter[], const uint32_t offset = 0) const
        {
            return (find_first_of(offset, delimiter));
        }

        inline uint32_t ForwardSkip(const TCHAR delimiter[], const uint32_t offset = 0) const
        {
            return (find_first_not_of(offset, delimiter));
        }

        inline void TrimBegin(const TCHAR delimiter[])
        {
            uint32_t index = 0;

            if (m_Start == nullptr) {
                while ((index < m_Index.Length()) && (_tcschr(delimiter, m_Buffer[m_Index.Begin() + index]) != nullptr))
                    index++;
            } else {
                while ((index < m_Index.Length()) && (_tcschr(delimiter, m_Start[m_Index.Begin() + index]) != nullptr))
                    index++;
            }
            m_Index.SetIndexInfo(m_Index.Begin() + index, m_Index.Length() - index);
        }
        inline void TrimEnd(const TCHAR delimiter[])
        {
            uint32_t index = Length() - 1;

            if (index > 0) {
                if (m_Start == nullptr) {
                    while ((index < Length()) && (_tcschr(delimiter, m_Buffer[m_Index.Begin() + index]) != nullptr))
                        --index;
                } else {
                    while ((index < Length()) && (_tcschr(delimiter, m_Start[m_Index.Begin() + index]) != nullptr))
                        --index;
                }
                m_Index.SetIndexInfo(m_Index.Begin(), (index < Length() ? (index + 1) : 0));
            }
        }
        inline uint32_t ReverseFind(const TCHAR delimiter[], const uint32_t offset = ~0)
        {
            uint32_t index = (offset == static_cast<uint32_t>(~0) ? Length() - 1 : offset);

            if (index > 0) {
                if (m_Start == nullptr) {
                    while ((index < Length()) && (_tcschr(delimiter, m_Buffer[m_Index.Begin() + index]) == nullptr))
                        --index;
                } else {
                    while ((index < Length()) && (_tcschr(delimiter, m_Start[m_Index.Begin() + index]) == nullptr))
                        --index;
                }
            }
            return (index);
        }
        inline uint32_t ReverseSkip(const TCHAR delimiter[], const uint32_t offset = ~0)
        {
            uint32_t index = (offset == static_cast<uint32_t>(~0) ? Length() - 1 : offset);

            if (index > 0) {
                if (m_Start == nullptr) {
                    while ((index < Length()) && (_tcschr(delimiter, m_Buffer[m_Index.Begin() + index]) != nullptr))
                        --index;
                } else {
                    while ((index < Length()) && (_tcschr(delimiter, m_Start[m_Index.Begin() + index]) != nullptr))
                        --index;
                }
            }
            return (index);
        }

    protected:
        inline void Forward(const uint32_t forward)
        {
            m_Index.Increment(forward);
        }

        inline void Reverse(const uint32_t reverse)
        {
            m_Index.Decrement(reverse);
        }

        inline const Index& PartIndex() const
        {
            return (m_Index);
        }

    private:
        bool on_given_character(const uint32_t offset, const TCHAR characters[]) const
        {
            bool equal = false;

            if (offset < m_Index.Length()) {
                if (m_Start == nullptr) {
                    equal = (_tcschr(characters, m_Buffer[m_Index.Begin() + offset]) != nullptr);
                } else {
                    equal = (_tcschr(characters, m_Start[m_Index.Begin() + offset]) != nullptr);
                }
            }

            return (equal);
        }

        bool equal_case_sensitive(const TextFragment& RHS) const
        {
            bool equal = false;

            if (RHS.m_Index.Length() == m_Index.Length()) {
                if (m_Start != nullptr) {
                    if (RHS.m_Start == nullptr) {
                        equal = (RHS.m_Buffer.compare(RHS.m_Index.Begin(), RHS.m_Index.Length(), &(m_Start[m_Index.Begin()]), m_Index.Length()) == 0);
                    } else {
                        equal = (_tcsncmp(&(RHS.m_Start[RHS.m_Index.Begin()]), &(m_Start[m_Index.Begin()]), RHS.m_Index.Length()) == 0);
                    }
                } else {
                    if (RHS.m_Start == nullptr) {
                        equal = (m_Buffer.compare(m_Index.Begin(), m_Index.Length(), RHS.m_Buffer, RHS.m_Index.Begin(), RHS.m_Index.Length()) == 0);
                    } else {
                        equal = (m_Buffer.compare(m_Index.Begin(), m_Index.Length(), &(RHS.m_Start[RHS.m_Index.Begin()]), RHS.m_Index.Length()) == 0);
                    }
                }
            }

            return (equal);
        }

        bool equal_case_insensitive(const TextFragment& RHS) const
        {
            bool equal = false;

            if (RHS.m_Index.Length() == m_Index.Length()) {
                if (m_Start != nullptr) {
                    if (RHS.m_Start == nullptr) {
                        equal = (_tcsnicmp(&(RHS.m_Buffer[RHS.m_Index.Begin()]), &(m_Start[m_Index.Begin()]), m_Index.Length()) == 0);
                    } else {
                        equal = (_tcsnicmp(&(RHS.m_Start[RHS.m_Index.Begin()]), &(m_Start[m_Index.Begin()]), m_Index.Length()) == 0);
                    }
                } else {
                    if (RHS.m_Start == nullptr) {
                        equal = (_tcsnicmp(&(RHS.m_Buffer[RHS.m_Index.Begin()]), &(m_Buffer[m_Index.Begin()]), m_Index.Length()) == 0);
                    } else {
                        equal = (_tcsnicmp(&(RHS.m_Start[RHS.m_Index.Begin()]), &(m_Buffer[m_Index.Begin()]), m_Index.Length()) == 0);
                    }
                }
            }

            return (equal);
        }

        uint32_t find_first_of(const uint32_t offset, const TCHAR delimiter) const
        {
            // If we do not find it, we end up at the end.
            uint32_t index = m_Index.Length();

            if (offset < m_Index.Length()) {
                if (m_Start != nullptr) {
                    const TCHAR* start = &m_Start[m_Index.Begin()];
                    const TCHAR* entry = _tcschr(&start[offset], delimiter);

                    if (entry != nullptr) {
                        index = static_cast<uint32_t>(entry - start);

                        if (index > m_Index.Length()) {
                            index = m_Index.Length();
                        }
                    }
                } else {
                    uint32_t found = static_cast<uint32_t>(m_Buffer.find_first_of(delimiter, m_Index.Begin() + offset));

                    if (found <= m_Index.End()) {
                        index = found - m_Index.Begin();
                    }
                }
            }

            return (index);
        }

        uint32_t find_first_of(const uint32_t offset, const TCHAR delimiter[]) const
        {
            // If we do not find it, we end up at the end.
            uint32_t index = m_Index.Length();

            if (offset < m_Index.Length()) {
                if (m_Start != nullptr) {
                    const TCHAR* pointer = &(Data()[offset]);
                    uint32_t length = m_Index.Length() - offset;

                    while ((length != 0) && ((_tcschr(delimiter, *pointer) == nullptr))) {
                        pointer++;
                        length--;
                    }

                    index = m_Index.Length() - length;
                } else {
                    uint32_t found = static_cast<uint32_t>(m_Buffer.find_first_of(delimiter, m_Index.Begin() + offset));

                    if (found <= m_Index.End()) {
                        index = found - m_Index.Begin();
                    }
                }
            }

            return (index);
        }

        uint32_t find_first_not_of(const uint32_t offset, const TCHAR delimiter[]) const
        {
            // If we do not find it, we end up at the end.
            uint32_t index = m_Index.Length();

            if (offset < m_Index.Length()) {
                if (m_Start != nullptr) {
                    const TCHAR* pointer = &(Data()[offset]);
                    uint32_t length = m_Index.Length() - offset;

                    while ((length != 0) && ((_tcschr(delimiter, *pointer) != nullptr))) {
                        pointer++;
                        length--;
                    }

                    index = m_Index.Length() - length;
                } else {
                    uint32_t found = static_cast<uint32_t>(m_Buffer.find_first_not_of(delimiter, m_Index.Begin() + offset));

                    if (found <= m_Index.End()) {
                        index = found - m_Index.Begin();
                    }
                }
            }

            return (index);
        }

        uint32_t find_last_not_of(const uint32_t offset, const TCHAR delimiter[]) const
        {
            // If we do not find it, we end up at the end.
            uint32_t index = NUMBER_MAX_UNSIGNED(uint32_t);

            if (offset < m_Index.Length()) {
                if (m_Start != nullptr) {
                    const TCHAR* pointer = &(Data()[m_Index.Length()]);
                    uint32_t count = m_Index.Length() - offset;

                    while ((count != 0) && ((_tcschr(delimiter, *(--pointer)) != nullptr))) {
                        count--;
                    }
                    if (count != 0) {
                        index = count - 1;
                    }
                } else {
                    uint32_t found = static_cast<uint32_t>(m_Buffer.find_last_not_of(delimiter, m_Index.End()));

                    if ((found >= (m_Index.Begin() + offset)) && (found != m_Index.End())) {
                        index = found - m_Index.Begin();
                    }
                }
            }

            return (index);
        }

    private:
        Index m_Index;
        const TCHAR* m_Start;
        string m_Buffer;
    };

    class EXTERNAL TextSegmentIterator {
    public:
        TextSegmentIterator()
            : _delimiter(0)
            , _delimiters()
            , _index(~0)
            , _current()
            , _source()
            , _suppressEmpty(false)
        {
        }
        TextSegmentIterator(const TextFragment& text, const bool suppressEmpty, const TCHAR splitter)
            : _delimiter(splitter)
            , _delimiters()
            , _index(~0)
            , _current()
            , _source(text)
            , _suppressEmpty(suppressEmpty)
        {
        }
        TextSegmentIterator(const TextFragment& text, const bool suppressEmpty, const TCHAR splitters[])
            : _delimiter(0)
            , _delimiters(splitters)
            , _index(~0)
            , _current()
            , _source(text)
            , _suppressEmpty(suppressEmpty)
        {
        }
        TextSegmentIterator(const TextSegmentIterator& copy, const bool fixatFromCurrentPosition = false)
            : _delimiter(copy._delimiter)
            , _delimiters(copy._delimiters)
            , _index(fixatFromCurrentPosition ? (copy._index < copy._source.Length() ? 0 : 1) : copy._index)
            , _current(copy._current)
            , _source(fixatFromCurrentPosition ? (copy._index < copy._source.Length() ? TextFragment(copy._source, copy._index, copy._source.Length() - copy._index) : TextFragment()) : copy._source)
            , _suppressEmpty(copy._suppressEmpty)
        {
        }
        TextSegmentIterator(TextSegmentIterator&& move, const bool fixatFromCurrentPosition = false)
            : _delimiter(move._delimiter)
            , _delimiters(std::move(move._delimiters))
            , _index(fixatFromCurrentPosition ? (move._index < move._source.Length() ? 0 : 1) : move._index)
            , _current(std::move(move._current))
            , _suppressEmpty(move._suppressEmpty)
        {
            uint32_t moveLength = move._source.Length();
            _source = fixatFromCurrentPosition ? (move._index < moveLength ? TextFragment(std::move(move._source), move._index, moveLength - move._index) : TextFragment()) : std::move(move._source);

            move._delimiter = 0;
            move._index = ~0;
            move._suppressEmpty = false;
        }
        ~TextSegmentIterator()
        {
        }

        TextSegmentIterator& operator=(const TextSegmentIterator& RHS)
        {
            _delimiter = RHS._delimiter;
            _delimiters = RHS._delimiters;
            _index = RHS._index;
            _current = RHS._current;
            _source = RHS._source;
            _suppressEmpty = RHS._suppressEmpty;

            return (*this);
        }

        TextSegmentIterator& operator=(TextSegmentIterator&& move)
        {
            if (this != &move) {
                _delimiter = move._delimiter;
                _delimiters = std::move(move._delimiters);
                _index = move._index;
                _current = std::move(move._current);
                _source = std::move(move._source);
                _suppressEmpty = move._suppressEmpty;

                move._delimiter = 0;
                move._index = ~0;
                move._suppressEmpty = false;
            }
            return (*this);
        }

    public:
        inline bool IsValid() const
        {
            return (_index <= _source.Length());
        }
        inline void Reset()
        {
            _index = ~0;
        }
        bool Next()
        {
            bool valid = false;
            uint32_t start = (_index == static_cast<uint32_t>(~0) ? 0 : _index + 1);

            while ((start < _source.Length()) && (valid == false)) {
                _index = (_delimiter == 0 ? _source.ForwardFind(_delimiters.c_str(), start) : _source.ForwardFind(_delimiter, start));

                if ((_suppressEmpty == false) || (start <= _index)) {
                    valid = true;

                    _current = TextFragment(_source, start, _index - start);
                } else {
                    start = _index + 1;
                }
            }
            if (valid == false) {
                _index = _source.Length() + 1;
            }

            return (valid);
        }
        inline TextFragment Current()
        {
            ASSERT(IsValid());

            return (_current);
        }
        inline const TextFragment& Current() const
        {
            ASSERT(IsValid());

            return (_current);
        }
        inline const TextFragment Remainder() const
        {
            ASSERT(IsValid());

            uint32_t currentStart = _index - _current.Length();

            return (TextFragment(_source, currentStart, (_source.Length() - currentStart)));
        }

    private:
        TCHAR _delimiter;
        string _delimiters;
        uint32_t _index;
        TextFragment _current;
        TextFragment _source;
        bool _suppressEmpty;
    };
}
} // namespace Core

#endif // __TEXTFRAGMENT_H

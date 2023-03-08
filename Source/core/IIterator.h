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

#ifndef IITERATOR_H_INCLUDED
#define IITERATOR_H_INCLUDED

#include "Module.h"
#include "Portability.h"
#include "Trace.h"

#include <map>

namespace WPEFramework {
namespace Core {
    struct IIterator {
        virtual ~IIterator() = default;
        virtual bool Next() = 0;
        virtual bool Previous() = 0;
        virtual void Reset(const uint32_t position) = 0;
        virtual bool IsValid() const = 0;
        virtual uint32_t Index() const = 0;
        virtual uint32_t Count() const = 0;
    };

    template <typename CONTAINER, typename ELEMENT, typename ITERATOR = typename CONTAINER::iterator>
    class IteratorType : public IIterator {
    public:
        IteratorType()
            : m_Container(nullptr)
            , m_Iterator()
            , m_Index(0)
        {
        }
        IteratorType(CONTAINER& container)
            : m_Container(&container)
            , m_Iterator(container.begin())
            , m_Index(0)
        {
        }
        IteratorType(CONTAINER& container, ITERATOR startPoint)
            : m_Container(&container)
            , m_Iterator(startPoint)
            , m_Index(1)
        {
            /* Calculate the index.... */
            ITERATOR index = m_Container->begin();

            // Move forward the requested number of steps..
            while (index != m_Iterator) {
                m_Index++;
                index++;
            }
        }
        IteratorType(IteratorType<CONTAINER, ELEMENT, ITERATOR>&& move)
            : m_Container(std::move(move.m_Container))
            , m_Iterator(std::move(move.m_Iterator))
            , m_Index(move.m_Index)
        {
        }
        IteratorType(const IteratorType<CONTAINER, ELEMENT, ITERATOR>& copy)
            : m_Container(copy.m_Container)
            , m_Iterator(copy.m_Iterator)
            , m_Index(copy.m_Index)
        {
        }
        ~IteratorType() = default;

        IteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(const IteratorType<CONTAINER, ELEMENT, ITERATOR>& RHS)
        {
            m_Container = RHS.m_Container;
            m_Iterator = RHS.m_Iterator;
            m_Index = RHS.m_Index;

            return (*this);
        }

    public:
        virtual bool IsValid() const
        {
            return ((m_Index > 0) && (m_Index <= Count()));
        }

        virtual void Reset(const uint32_t position)
        {
            if (m_Container != nullptr) {
                if (position == 0) {
                    m_Iterator = m_Container->begin();
                    m_Index = 0;
                } else if (position > Count()) {
                    m_Iterator = m_Container->end();
                    m_Index = Count() + 1;
                } else if ((position < m_Index) && ((m_Index - position) < position)) {
                    // Better that we walk back from where we are ;-)
                    while (m_Index != position) {
                        m_Index--;
                        m_Iterator--;
                    }
                } else {
                    m_Iterator = m_Container->begin();
                    m_Index = position;

                    // Move forward the requested number of steps..
                    for (uint32_t teller = 1; teller < position; teller++) {
                        m_Iterator++;
                    }

                    ASSERT(m_Iterator != m_Container->end());
                }
            }
        }

        virtual bool Previous()
        {
            if ((m_Container != nullptr) && (m_Index != 0)) {
                if (m_Index > 1) {
                    m_Iterator--;
                }
                m_Index--;

                ASSERT((m_Index != 0) || (m_Iterator == m_Container->begin()));
            }
            return (IsValid());
        }

        virtual bool Next()
        {
            if ((m_Container != nullptr) && (m_Index != Count() + 1)) {
                m_Index++;

                if (m_Index != 1) {
                    m_Iterator++;

                    ASSERT((m_Index != (Count() + 1)) || (m_Iterator == m_Container->end()));
                }
            }
            return (IsValid());
        }

        virtual uint32_t Index() const
        {
            return (m_Index);
        }

        virtual uint32_t Count() const
        {
            return (m_Container == nullptr ? 0 : static_cast<uint32_t>(m_Container->size()));
        }

        inline ELEMENT& Current()
        {
            ASSERT(IsValid());

                return (*m_Iterator);
        }

        inline const ELEMENT& Current() const
        {
            ASSERT(IsValid());

            return (*m_Iterator);
        }

        inline ELEMENT operator->()
        {
            ASSERT(IsValid());

            return (*m_Iterator);
        }

        inline ELEMENT operator->() const
        {
            ASSERT(IsValid());

            return (*m_Iterator);
        }

        inline ELEMENT operator*()
        {
            ASSERT(IsValid());

            return (*m_Iterator);
        }

        inline const ELEMENT operator*() const
        {
            ASSERT(IsValid());

            return (m_Iterator.operator*());
        }

    protected:
        inline CONTAINER* Container()
        {
            return (m_Container);
        }
        inline const CONTAINER* Container() const
        {
            return (m_Container);
        }
        inline void Container(CONTAINER& container) {
            m_Container = &container;
            m_Iterator = m_Container->begin();
            m_Index = 0;
        }

    private:
        CONTAINER* m_Container;
        mutable ITERATOR m_Iterator;
        mutable uint32_t m_Index;
    };

    template <typename CONTAINER, typename ELEMENT, typename KEY, typename ITERATOR = typename CONTAINER::iterator>
    class IteratorMapType : public IIterator {
    public:
        IteratorMapType()
            : m_Container(nullptr)
            , m_Iterator()
            , m_Index(0)
        {
        }
        IteratorMapType(CONTAINER& container)
            : m_Container(&container)
            , m_Iterator(container.begin())
            , m_Index(0)
        {
        }
        IteratorMapType(CONTAINER& container, ITERATOR& startPoint)
            : m_Container(&container)
            , m_Iterator(startPoint)
            , m_Index(1)
        {
            /* Calculate the index.... */
            ITERATOR index = m_Container->begin();

            // Move forward the requested number of steps..
            while (index != m_Iterator) {
                m_Index++;
                index++;
            }
        }
        IteratorMapType(const IteratorMapType<CONTAINER, KEY, ELEMENT, ITERATOR>& copy)
            : m_Container(copy.m_Container)
            , m_Iterator(copy.m_Iterator)
            , m_Index(copy.m_Index)
        {
        }
        ~IteratorMapType()
        {
        }

        IteratorMapType<CONTAINER, KEY, ELEMENT, ITERATOR>& operator=(const IteratorMapType<CONTAINER, KEY, ELEMENT, ITERATOR>& RHS)
        {
            m_Container = RHS.m_Container;
            m_Iterator = RHS.m_Iterator;
            m_Index = RHS.m_Index;

            return (*this);
        }

    public:
        virtual bool IsValid() const
        {
            return ((m_Index > 0) && (m_Index <= Count()));
        }

        virtual void Reset(const uint32_t position)
        {
            if (m_Container != nullptr) {
                if (position == 0) {
                    m_Iterator = m_Container->begin();
                    m_Index = 0;
                } else if (position > Count()) {
                    m_Iterator = m_Container->end();
                    m_Index = Count() + 1;
                } else if ((position < m_Index) && ((m_Index - position) < position)) {
                    // Better that we walk back from where we are ;-)
                    while (m_Index != position) {
                        m_Index--;
                        m_Iterator--;
                    }
                } else {
                    m_Iterator = m_Container->begin();
                    m_Index = position;

                    // Move forward the requested number of steps..
                    for (uint32_t teller = 1; teller < position; teller++) {
                        m_Iterator++;
                    }

                    ASSERT(m_Iterator != m_Container->end());
                }
            }
        }

        virtual bool Previous()
        {
            if ((m_Container != nullptr) && (m_Index != 0)) {
                if (m_Index > 1) {
                    m_Iterator--;
                }
                m_Index--;

                ASSERT((m_Index != 0) || (m_Iterator == m_Container->begin()));
            }
            return (IsValid());
        }

        virtual bool Next()
        {
            if ((m_Container != nullptr) && (m_Index != Count() + 1)) {
                m_Index++;

                if (m_Index != 1) {
                    m_Iterator++;

                    ASSERT((m_Index != (Count() + 1)) || (m_Iterator == m_Container->end()));
                }
            }
            return (IsValid());
        }

        virtual uint32_t Index() const
        {
            return (m_Index);
        }

        virtual uint32_t Count() const
        {
            return (m_Container == nullptr ? 0 : static_cast<uint32_t>(m_Container->size()));
        }

        inline KEY Key() const
        {
            ASSERT(IsValid());

            return (*m_Iterator).first;
        }

        inline ELEMENT& Current()
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

        inline const ELEMENT& Current() const
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

        inline ELEMENT operator->()
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

        inline const ELEMENT operator->() const
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

        inline ELEMENT operator*()
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

        inline const ELEMENT operator*() const
        {
            ASSERT(IsValid());

            return (*m_Iterator).second;
        }

    protected:
        inline std::map<KEY, ELEMENT>* Container()
        {
            return (m_Container);
        }
        inline const std::map<KEY, ELEMENT>* Container() const
        {
            return (m_Container);
        }

    private:
        CONTAINER* m_Container;
        mutable ITERATOR m_Iterator;
        mutable uint32_t m_Index;
    };

    template <typename CONTAINER, typename ELEMENT, typename ITERATOR = typename CONTAINER::iterator>
    class LockableIteratorType : public IteratorType<CONTAINER, ELEMENT, ITERATOR> {
    public:
        LockableIteratorType()
            : IteratorType<CONTAINER, ELEMENT, ITERATOR>()
        {
        }
        LockableIteratorType(CONTAINER& container)
            : IteratorType<CONTAINER, ELEMENT, ITERATOR>(container)
        {
            container.ReadLock();
        }
        LockableIteratorType(CONTAINER& container, ITERATOR& startPoint)
            : IteratorType<CONTAINER, ELEMENT, ITERATOR>(container, startPoint)
        {
            container.ReadLock();
        }
        LockableIteratorType(const LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>& copy)
            : IteratorType<CONTAINER, ELEMENT, ITERATOR>(copy)
        {
            if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != nullptr) {
                IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container()->ReadLock();
            }
        }
        ~LockableIteratorType()
        {
            if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != nullptr) {
                IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container()->ReadUnlock();
            }
        }

        LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(const LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>& RHS)
        {
            if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != RHS.Container()) {
                if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != nullptr) {
                    IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container()->ReadUnlock();
                }

                IteratorType<CONTAINER, ELEMENT, ITERATOR>::operator=(RHS);

                if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != nullptr) {
                    IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container()->ReadLock();
                }
            } else {
                IteratorType<CONTAINER, ELEMENT, ITERATOR>::operator=(RHS);
            }

            return (*this);
        }
    };
}
} // namespace Core

#endif

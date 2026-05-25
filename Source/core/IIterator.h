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

namespace Thunder {
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
            : m_Owned(nullptr)
            , m_Container(nullptr)
            , m_Iterator()
            , m_Index(0)
        {
        }
        // Reference mode: iterator does NOT own the container.
        IteratorType(CONTAINER& container)
            : m_Owned(nullptr)
            , m_Container(&container)
            , m_Iterator(container.begin())
            , m_Index(0)
        {
        }
        IteratorType(CONTAINER& container, ITERATOR startPoint)
            : m_Owned(nullptr)
            , m_Container(&container)
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
        // Snapshot mode: iterator takes ownership of a copy/move of the container.
        // Use when the iterator must outlive the source or when a snapshot is needed.
        // Note: non-const lvalue still binds to CONTAINER& (reference mode) above;
        //       pass std::as_const(c) or std::move(c) to choose snapshot mode.
        // Disabled when CONTAINER is already const to avoid duplicate constructor signature.
        template<typename C = CONTAINER, typename std::enable_if<!std::is_const<C>::value, int>::type = 0>
        IteratorType(const CONTAINER& container)
            : m_Owned(new CONTAINER(container))
            , m_Container(m_Owned)
            , m_Iterator(m_Owned->begin())
            , m_Index(0)
        {
        }
        IteratorType(CONTAINER&& container)
            : m_Owned(new CONTAINER(std::move(container)))
            , m_Container(m_Owned)
            , m_Iterator(m_Owned->begin())
            , m_Index(0)
        {
        }
        IteratorType(IteratorType<CONTAINER, ELEMENT, ITERATOR>&& move) noexcept
            : m_Owned(move.m_Owned)
            , m_Container(move.m_Container)
            , m_Iterator(std::move(move.m_Iterator))
            , m_Index(move.m_Index)
        {
            move.m_Owned = nullptr;
            move.m_Container = nullptr;
            move.m_Index = 0;
        }
        // Copy: in snapshot mode the owned container is deep-copied and the
        // position is reset; in reference mode the external pointer is shared.
        IteratorType(const IteratorType<CONTAINER, ELEMENT, ITERATOR>& copy)
            : m_Owned(copy.m_Owned != nullptr ? new CONTAINER(*copy.m_Owned) : nullptr)
            , m_Container(copy.m_Owned != nullptr ? m_Owned : copy.m_Container)
            , m_Iterator(copy.m_Owned != nullptr ? m_Owned->begin() : copy.m_Iterator)
            , m_Index(copy.m_Owned != nullptr ? 0 : copy.m_Index)
        {
        }
        ~IteratorType()
        {
            delete m_Owned;
        }

        IteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(const IteratorType<CONTAINER, ELEMENT, ITERATOR>& RHS)
        {
            if (this != &RHS) {
                delete m_Owned;
                m_Owned     = (RHS.m_Owned != nullptr ? new CONTAINER(*RHS.m_Owned) : nullptr);
                m_Container = (RHS.m_Owned != nullptr ? m_Owned : RHS.m_Container);
                m_Iterator  = (RHS.m_Owned != nullptr ? m_Owned->begin() : RHS.m_Iterator);
                m_Index     = (RHS.m_Owned != nullptr ? 0 : RHS.m_Index);
            }
            return (*this);
        }
        IteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(IteratorType<CONTAINER, ELEMENT, ITERATOR>&& move)
        {
            if (this != &move) {
                delete m_Owned;
                m_Owned     = move.m_Owned;
                m_Container = move.m_Container;
                m_Iterator  = std::move(move.m_Iterator);
                m_Index     = move.m_Index;
                move.m_Owned     = nullptr;
                move.m_Container = nullptr;
                move.m_Index     = 0;
            }
            return (*this);
        }
        // Snapshot-from-container assignment: replaces the held container with a
        // moved-in snapshot, resetting the iteration position to the beginning.
        IteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(CONTAINER&& container)
        {
            delete m_Owned;
            m_Owned     = new CONTAINER(std::move(container));
            m_Container = m_Owned;
            m_Iterator  = m_Owned->begin();
            m_Index     = 0;
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

        // Convenience overload: reset to the beginning (equivalent to Reset(0)).
        inline void Reset()
        {
            Reset(0);
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

        inline std::remove_reference_t<ELEMENT>* operator->()
        {
            ASSERT(IsValid());

            return &(*m_Iterator);
        }

        inline const std::remove_reference_t<ELEMENT>* operator->() const
        {
            ASSERT(IsValid());

            return &(*m_Iterator);
        }

        inline std::remove_reference_t<ELEMENT>& operator*()
        {
            ASSERT(IsValid());

            return (*m_Iterator);
        }

        inline const std::remove_reference_t<ELEMENT>& operator*() const
        {
            ASSERT(IsValid());

            return (*m_Iterator);
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
            delete m_Owned;
            m_Owned = nullptr;
            m_Container = &container;
            m_Iterator = m_Container->begin();
            m_Index = 0;
        }

    private:
        CONTAINER*       m_Owned;     // non-null only in snapshot (owned) mode
        CONTAINER*       m_Container; // points to m_Owned or to an external container
        mutable ITERATOR m_Iterator;
        mutable uint32_t m_Index;
    };

    template <typename CONTAINER, typename ELEMENT, typename KEY, typename ITERATOR = typename CONTAINER::iterator>
    class IteratorMapType : public IIterator {
    public:
        IteratorMapType()
            : m_Owned(nullptr)
            , m_Container(nullptr)
            , m_Iterator()
            , m_Index(0)
        {
        }
        // Reference mode: iterator does NOT own the container.
        IteratorMapType(CONTAINER& container)
            : m_Owned(nullptr)
            , m_Container(&container)
            , m_Iterator(container.begin())
            , m_Index(0)
        {
        }
        IteratorMapType(CONTAINER& container, ITERATOR& startPoint)
            : m_Owned(nullptr)
            , m_Container(&container)
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
        // Snapshot mode: iterator takes ownership of a copy/move of the container.
        // Disabled when CONTAINER is already const to avoid duplicate constructor signature.
        template<typename C = CONTAINER, typename std::enable_if<!std::is_const<C>::value, int>::type = 0>
        IteratorMapType(const CONTAINER& container)
            : m_Owned(new CONTAINER(container))
            , m_Container(m_Owned)
            , m_Iterator(m_Owned->begin())
            , m_Index(0)
        {
        }
        IteratorMapType(CONTAINER&& container)
            : m_Owned(new CONTAINER(std::move(container)))
            , m_Container(m_Owned)
            , m_Iterator(m_Owned->begin())
            , m_Index(0)
        {
        }
        IteratorMapType(const IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>& copy)
            : m_Owned(copy.m_Owned != nullptr ? new CONTAINER(*copy.m_Owned) : nullptr)
            , m_Container(copy.m_Owned != nullptr ? m_Owned : copy.m_Container)
            , m_Iterator(copy.m_Owned != nullptr ? m_Owned->begin() : copy.m_Iterator)
            , m_Index(copy.m_Owned != nullptr ? 0 : copy.m_Index)
        {
        }
        IteratorMapType(IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>&& move) noexcept
            : m_Owned(move.m_Owned)
            , m_Container(move.m_Container)
            , m_Iterator(std::move(move.m_Iterator))
            , m_Index(move.m_Index)
        {
            move.m_Owned = nullptr;
            move.m_Container = nullptr;
            move.m_Index = 0;
        }
        ~IteratorMapType()
        {
            delete m_Owned;
        }

        IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>& operator=(const IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>& RHS)
        {
            if (this != &RHS) {
                delete m_Owned;
                m_Owned     = (RHS.m_Owned != nullptr ? new CONTAINER(*RHS.m_Owned) : nullptr);
                m_Container = (RHS.m_Owned != nullptr ? m_Owned : RHS.m_Container);
                m_Iterator  = (RHS.m_Owned != nullptr ? m_Owned->begin() : RHS.m_Iterator);
                m_Index     = (RHS.m_Owned != nullptr ? 0 : RHS.m_Index);
            }
            return (*this);
        }
        IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>& operator=(IteratorMapType<CONTAINER, ELEMENT, KEY, ITERATOR>&& move)
        {
            if (this != &move) {
                delete m_Owned;
                m_Owned     = move.m_Owned;
                m_Container = move.m_Container;
                m_Iterator  = std::move(move.m_Iterator);
                m_Index     = move.m_Index;
                move.m_Owned     = nullptr;
                move.m_Container = nullptr;
                move.m_Index     = 0;
            }
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
                    if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag,
                                      typename std::iterator_traits<ITERATOR>::iterator_category>) {
                        // Better that we walk back from where we are ;-)
                        while (m_Index != position) {
                            m_Index--;
                            m_Iterator--;
                        }
                    } else {
                        m_Iterator = m_Container->begin();
                        m_Index = position;
                        for (uint32_t teller = 1; teller < position; teller++) {
                            m_Iterator++;
                        }
                        ASSERT(m_Iterator != m_Container->end());
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
                    if constexpr (std::is_base_of_v<std::bidirectional_iterator_tag,
                                      typename std::iterator_traits<ITERATOR>::iterator_category>) {
                        m_Iterator--;
                    } else {
                        m_Iterator = m_Container->begin();
                        for (uint32_t teller = 1; teller < m_Index - 1; teller++) {
                            m_Iterator++;
                        }
                    }
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

        // Convenience overload: reset to the beginning (equivalent to Reset(0)).
        inline void Reset()
        {
            Reset(0);
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
        inline CONTAINER* Container()
        {
            return (m_Container);
        }
        inline const CONTAINER* Container() const
        {
            return (m_Container);
        }

    private:
        CONTAINER*       m_Owned;     // non-null only in snapshot (owned) mode
        CONTAINER*       m_Container; // points to m_Owned or to an external container
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
        LockableIteratorType(LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>&& move)
            : IteratorType<CONTAINER, ELEMENT, ITERATOR>(move)
        {
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
	LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>& operator=(LockableIteratorType<CONTAINER, ELEMENT, ITERATOR>&& move)
        {
            if (this != &move) {
                if (IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container() != nullptr) {
                    IteratorType<CONTAINER, ELEMENT, ITERATOR>::Container()->ReadUnlock();
                }

                IteratorType<CONTAINER, ELEMENT, ITERATOR>::operator=(move);
            }

            return (*this);
        }
    };

}
} // namespace Core

#endif

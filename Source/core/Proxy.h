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

#ifndef __PROXY_H
#define __PROXY_H

// ---- Include system wide include files ----
#include <memory>

// ---- Include local include files ----
#include "Portability.h"
#include "StateTrigger.h"
#include "Sync.h"
#include "TypeTraits.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Core {

    template <typename CONTEXT>
    class ProxyService : public CONTEXT {
    private:
        // ----------------------------------------------------------------
        // Never, ever allow reference counted objects to be assigned.
        // Create a new object and modify it. If the assignment operator
        // is used, give a compile error.
        // ----------------------------------------------------------------
        ProxyService<CONTEXT>& operator=(const ProxyService<CONTEXT>& rhs) = delete;

    protected:
        ProxyService(CONTEXT& copy)
            : CONTEXT(copy)
            , m_RefCount(0)
        {
            __Initialize<CONTEXT>();
        }

    public:
        // Here we claim the buffer, plus some size for the buffer.
        void*
        operator new(
            size_t stAllocateBlock,
            unsigned int AdditionalSize)
        {
            uint8_t* Space = nullptr;

            // memory alignment
            size_t alignedSize = ((stAllocateBlock + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));

            if (AdditionalSize != 0) {
                Space = reinterpret_cast<uint8_t*>(::malloc(alignedSize + sizeof(void*) + AdditionalSize));

                if (Space != nullptr) {
                    *(reinterpret_cast<uint32_t*>(&Space[alignedSize])) = AdditionalSize;
                }
            } else {
                Space = reinterpret_cast<uint8_t*>(::malloc(alignedSize));
            }

            return Space;
        }
        // Somehow Purify gets lost if we do not delete it, overide the delete operator
        void
        operator delete(
            void* stAllocateBlock)
        {
            ::free(stAllocateBlock);
        }

    public:
        template <typename... Args>
        inline ProxyService(Args&&... args)
            : CONTEXT(std::forward<Args>(args)...)
            , m_RefCount(0)
        {
            __Initialize<CONTEXT>();
        }
        virtual ~ProxyService()
        {
            __Deinitialize<CONTEXT>();

            /* Hotfix for gcc linker issue preventing debug builds.
             *
             * Please see WPE-546 for details.
             */
            // ASSERT(m_RefCount == 0);

            TRACE_L5("Destructor ProxyObject <%p>", (this));
        }

    public:
        virtual void AddRef() const
        {
            Core::InterlockedIncrement(m_RefCount);
        }

        virtual uint32_t Release() const
        {
            uint32_t Result;

            if ((Result = Core::InterlockedDecrement(m_RefCount)) == 0) {
                delete this;

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }

            return (Core::ERROR_NONE);
        }

        inline operator CONTEXT&()
        {
            return (*this);
        }

        inline operator const CONTEXT&() const
        {
            return (*this);
        }

        inline uint32_t Size() const
        {
            size_t alignedSize = ((sizeof(ProxyService<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));

            return (*(reinterpret_cast<const uint32_t*>(&(reinterpret_cast<const uint8_t*>(this)[alignedSize]))));
        }

        template <typename TYPE>
        TYPE* Store()
        {
            size_t alignedSize = ((sizeof(ProxyService<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));
            void* data = reinterpret_cast<void*>(&(reinterpret_cast<uint8_t*>(this)[alignedSize + sizeof(void*)]));
            void* result = Alignment(alignof(TYPE), data);
            return (reinterpret_cast<TYPE*>(result));
        }

        template <typename TYPE>
        const TYPE* Store() const
        {
            size_t alignedSize = ((sizeof(ProxyService<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));
            void* data = const_cast<void*>(reinterpret_cast<const void*>(&(reinterpret_cast<const uint8_t*>(this)[alignedSize + sizeof(void*)])));
            const void* result = Alignment(alignof(TYPE), data);
            return (reinterpret_cast<const TYPE*>(result));
        }

        inline bool LastRef() const
        {
            return (m_RefCount == 1);
        }

        inline void Clear()
        {
            __Clear<CONTEXT>();
        }

        inline void CompositRelease()
        {
            // This release is intended for objects that Composit ProxyObject<> objects as a composition part.
            // At the moment these go out of scope, this CompositRelease has to be called. It is assuming the
            // last release but will not delete this object as that the the responsibility of the object that
            // has this object as a composit.
            Core::InterlockedDecrement(m_RefCount);

            ASSERT(m_RefCount == 0);
        }

    private:
        // -----------------------------------------------------
        // Check for Clear method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Clear, hasClear);

        typedef hasClear<CONTEXT, void (CONTEXT::*)()> TraitClear;

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<ProxyService<TYPE>::TraitClear::value, void>::type
            __Clear()
        {
            CONTEXT::Clear();
        }

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<!ProxyService<TYPE>::TraitClear::value, void>::type
            __Clear()
        {
        }

        // -----------------------------------------------------
        // Check for Initialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Initialize, hasInitialize);

        typedef hasInitialize<CONTEXT, uint32_t (CONTEXT::*)()> TraitInitialize;

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<ProxyService<TYPE>::TraitInitialize::value, uint32_t>::type
        __Initialize()
        {
            return (CONTEXT::Initialize());
        }

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<!ProxyService<TYPE>::TraitInitialize::value, uint32_t>::type
        __Initialize()
        {
            return (Core::ERROR_NONE);
        }

        // -----------------------------------------------------
        // Check for Deinitialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Deinitialize, hasDeinitialize);

        typedef hasDeinitialize<CONTEXT, void (CONTEXT::*)()> TraitDeinitialize;

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<ProxyService<TYPE>::TraitDeinitialize::value, void>::type
        __Deinitialize()
        {
            CONTEXT::Deinitialize();
        }

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<!ProxyService<TYPE>::TraitDeinitialize::value, void>::type
        __Deinitialize()
        {
        }

    protected:
        mutable uint32_t m_RefCount;
    };

    template <typename CONTEXT>
    class ProxyObject : public ProxyService<CONTEXT>, public IReferenceCounted {
    private:
        // ----------------------------------------------------------------
        // Never, ever allow reference counted objects to be assigned.
        // Create a new object and modify it. If the assignment operator
        // is used, give a compile error.
        // ----------------------------------------------------------------
        ProxyObject<CONTEXT>& operator=(const ProxyObject<CONTEXT>& rhs) = delete;

    protected:
        ProxyObject(CONTEXT& copy)
            : ProxyService<CONTEXT>(copy)
        {
        }

    public:
        template <typename... Args>
        inline ProxyObject(Args&&... args)
            : ProxyService<CONTEXT>(std::forward<Args>(args)...)
        {
        }
        virtual ~ProxyObject()
        {
        }

    public:
        void AddRef() const override
        {
            ProxyService<CONTEXT>::AddRef();
        }

        uint32_t Release() const override
        {
            return (ProxyService<CONTEXT>::Release());
        }
    };

    // ------------------------------------------------------------------------------
    // Reference counted object can only exist on heap (if reference count reaches 0)
    // delete is done. To avoid creation on the stack, this object can only be created
    // via the static create methods on this object.
    template <typename CONTEXT>
    class ProxyType {
    public:
        ProxyType()
            : m_RefCount(nullptr)
            , _realObject(nullptr)
        {
        }
        explicit ProxyType(ProxyObject<CONTEXT>& theObject)
            : m_RefCount(&theObject)
            , _realObject(&theObject)
        {
            m_RefCount->AddRef();
        }
        template <typename DERIVEDTYPE>
        explicit ProxyType(const ProxyType<DERIVEDTYPE>& theObject)
            : m_RefCount(theObject)
            , _realObject(nullptr)
        {
            if (m_RefCount != nullptr) {
                Construct<DERIVEDTYPE>(theObject, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
            }
        }
        explicit ProxyType(CONTEXT& realObject)
            : m_RefCount(const_cast<IReferenceCounted*>(dynamic_cast<const IReferenceCounted*>(&realObject)))
            , _realObject(&realObject)
        {
            if (m_RefCount != nullptr) {
                m_RefCount->AddRef();
            }
        }
        explicit ProxyType(IReferenceCounted* refObject, CONTEXT* realObject)
            : m_RefCount(refObject)
            , _realObject(realObject)
        {
            ASSERT((refObject == nullptr) || (realObject != nullptr));

            if (m_RefCount != nullptr) {
                m_RefCount->AddRef();
            }
        }
        ProxyType(const ProxyType<CONTEXT>& copy)
            : m_RefCount(copy.m_RefCount)
            , _realObject(copy._realObject)
        {
            if (m_RefCount != nullptr) {
                m_RefCount->AddRef();
            }
        }
        ~ProxyType()
        {
            if (m_RefCount != nullptr) {
                m_RefCount->Release();
            }
        }

    public:
        template <typename... Args>
        inline static ProxyType<CONTEXT> Create(Args&&... args)
        {
            return (CreateObject(TemplateIntToType<std::is_base_of<IReferenceCounted, CONTEXT>::value>(), 0, std::forward<Args>(args)...));
        }

        template <typename... Args>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, Args&&... args)
        {
            return (CreateObject(TemplateIntToType<std::is_base_of<IReferenceCounted, CONTEXT>::value>(), size, std::forward<Args>(args)...));
        }

        ProxyType<CONTEXT>& operator=(const ProxyType<CONTEXT>& rhs)
        {
            if (m_RefCount != rhs.m_RefCount) {
                // Release the current holding object
                if (m_RefCount != nullptr) {
                    m_RefCount->Release();
                }

                // Get the new object
                m_RefCount = rhs.m_RefCount;
                _realObject = rhs._realObject;

                if (m_RefCount != nullptr) {
                    m_RefCount->AddRef();
                }
            }

            return (*this);
        }

    public:
        inline bool IsValid() const
        {
            return (m_RefCount != nullptr);
        }
        inline uint32_t Release() const
        {
            // Only allowed on valid objects.
            ASSERT(m_RefCount != nullptr);

            uint32_t result = m_RefCount->Release();

            m_RefCount = nullptr;

            return (result);
        }
        inline void AddRef() const
        {
            // Only allowed on valid objects.
            ASSERT(m_RefCount != nullptr);

            m_RefCount->AddRef();
        }
        inline bool operator==(const ProxyType<CONTEXT>& a_RHS) const
        {
            return (m_RefCount == a_RHS.m_RefCount);
        }

        inline bool operator!=(const ProxyType<CONTEXT>& a_RHS) const
        {
            return !(operator==(a_RHS));
        }
        inline bool operator==(const CONTEXT& a_RHS) const
        {
            return ((m_RefCount != nullptr) && (_realObject == &a_RHS));
        }

        inline bool operator!=(const CONTEXT& a_RHS) const
        {
            return (!operator==(a_RHS));
        }

        inline CONTEXT* operator->() const
        {
            ASSERT(m_RefCount != nullptr);

            return (_realObject);
        }

        inline CONTEXT& operator*() const
        {
            ASSERT(m_RefCount != nullptr);

            return (*_realObject);
        }

        inline operator IReferenceCounted*() const
        {
            return (m_RefCount);
        }

        void Destroy()
        {
            delete m_RefCount;
            m_RefCount = nullptr;
        }

    private:
        template <typename DERIVED>
        inline void Construct(const ProxyType<DERIVED>& source, const TemplateIntToType<true>&)
        {
            _realObject = source.operator->();
            m_RefCount->AddRef();
        }
        template <typename DERIVED>
        inline void Construct(const ProxyType<DERIVED>& source, const TemplateIntToType<false>&)
        {
            CONTEXT* result(dynamic_cast<CONTEXT*>(source.operator->()));

            if (result == nullptr) {
                m_RefCount = nullptr;
            } else {
                _realObject = result;
                m_RefCount->AddRef();
            }
        }

        template <typename... Args>
        inline static ProxyType<CONTEXT> CreateObject(const ::TemplateIntToType<false>&, const uint32_t size, Args&&... args)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(std::forward<Args>(args)...));
        }
        template <typename... Args>
        inline static ProxyType<CONTEXT> CreateObject(const ::TemplateIntToType<true>&, const uint32_t size, Args&&... args)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyService<CONTEXT>(std::forward<Args>(args)...));
        }

    private:
        mutable IReferenceCounted* m_RefCount;
        CONTEXT* _realObject;
    };

    template <typename CASTOBJECT, typename SOURCE>
    ProxyType<CASTOBJECT> proxy_cast(const ProxyType<SOURCE>& castObject)
    {
        return (ProxyType<CASTOBJECT>(castObject));
    }

    const unsigned int PROXY_DEFAULT_ALLOC_SIZE = 20;
    const unsigned int PROXY_LIST_ERROR = static_cast<unsigned int>(-1);

    template <typename CONTEXT>
    class ProxyList {
        // http://www.ictp.trieste.it/~manuals/programming/sun/c-plusplus/c++_ug/Templates.new.doc.html

        // The design goal of this template class is speed. To speed up the
        // processing of the list leaves out the concurrency aspects. This template
        // is NOT thread safe, but fast. So if it is used by more threads, make it
        // thread safe. If not use it.
        // Further more the assumption is made that the template is used by smart
        // software engineers. Do not request an object from an empty list or from
        // an index that is out of scope. The code will assert in TRACE_LEVEL >= 1.

    public:
        explicit ProxyList(unsigned int a_Size)
            : m_List(nullptr)
            , m_Max(a_Size)
            , m_Current(0)
        {
            m_List = new IReferenceCounted*[m_Max];

#ifdef __DEBUG__
            memset(&m_List[0], 0, (m_Max * sizeof(IReferenceCounted*)));
#endif
        }

        ~ProxyList()
        {
            if (m_List != nullptr) {
#ifdef __DEBUG__
                for (unsigned int l_Teller = 0; l_Teller != m_Current; l_Teller++) {
                    if (m_List[l_Teller] != nullptr) {
                        TRACE_L1(_T("Possible memory leak detected on object queue [%d]"), l_Teller);
                    }
                }
#endif

                delete[] m_List;
            }
        }

        unsigned int Find(const ProxyType<CONTEXT>& a_Entry)
        {
            // Remember the item on the location.
            unsigned int l_Index = 0;

            ASSERT(a_Entry.IsValid() == true);

            // Find the given pointer
            while ((l_Index < m_Current) && (m_List[l_Index] != a_Entry.GetContext())) {
                l_Index++;
            }

            // If we found it, return the index.
            return (l_Index != m_Current ? l_Index : PROXY_LIST_ERROR);
        }

        unsigned int Add(ProxyType<CONTEXT>& a_Entry)
        {
            ASSERT(a_Entry.IsValid() == true);

            // That's easy, if there is space left..
            if (m_Current >= m_Max) {
                // Time to expand. Double the capacity. Allocate the capacity.
                IReferenceCounted** l_NewList = new IReferenceCounted*[static_cast<uint32_t>(m_Max << 1)];

                // Copy the old list in (Dirty but quick !!!!)
                memcpy(l_NewList, &m_List[0], (m_Max * sizeof(IReferenceCounted*)));

                // Update the capacity counter.
                m_Max = m_Max << 1;

                // Delete the old buffer.
                delete[] m_List;

                // Get the new copy installed.
                m_List = l_NewList;
            }

            // Now there is space enough, insert the new pointer.
            SetProxy(m_Current, a_Entry);
            a_Entry.AddRef();

            // Make sure the next one is placed on the next spot.
            Core::InterlockedIncrement(m_Current);

            // return the index of the newly added element.
            return (m_Current - 1);
        }

        void Remove(unsigned int a_Index, ProxyType<CONTEXT>& a_Entry)
        {
            ASSERT(a_Index < m_Current);
            ASSERT(m_List != nullptr);

            // Remember the item on the location, It should be a relaes and an add for
            // the new one, To optimize for speed, just copy the count.
            a_Entry = GetProxy(a_Index);

            // If it is taken out, release the reference that we took during the add
            m_List[a_Index]->Release();

            // Delete one element.
            Core::InterlockedDecrement(m_Current);

            // If it is not the last one, we have to move...
            if (a_Index < m_Current) {
                // Kill the entry, (again dirty but quick).
                memmove(&(m_List[a_Index]), &(m_List[a_Index + 1]), (m_Current - a_Index) * sizeof(IReferenceCounted*));
            }

#ifdef __DEBUG__
            m_List[m_Current] = nullptr;
#endif
        }

        void Remove(unsigned int a_Index)
        {
            ASSERT(a_Index < m_Current);
            ASSERT(m_List != nullptr);

            // If it is taken out, release the reference that we took during the add
            m_List[a_Index]->Release();

            // Delete one element.
            Core::InterlockedDecrement(m_Current);

            // If it is not the last one, we have to move...
            if (a_Index < m_Current) {
                // Kill the entry, (again dirty but quick).
                memmove(&(m_List[a_Index]), &(m_List[a_Index + 1]), (m_Current - a_Index) * sizeof(IReferenceCounted*));
            }

#ifdef __DEBUG__
            m_List[m_Current] = nullptr;
#endif
        }

        bool Remove(const ProxyType<CONTEXT>& a_Entry)
        {
            ASSERT(a_Entry.IsValid() != false);
            ASSERT(m_List != nullptr);

            // Remember the item on the location.
            unsigned int l_Index = Find(a_Entry);

            // If it is found, remove it.
            if (l_Index != PROXY_LIST_ERROR) {
                ProxyType<CONTEXT> Entry;

                // Remove this index.
                Remove(l_Index, Entry);
            }

            // And did we succeed ?
            return (l_Index != PROXY_LIST_ERROR);
        }

        void Clear(const unsigned int a_Start, const unsigned int a_Count)
        {
            ASSERT((a_Start + a_Count) <= m_Current);

            if (m_List != nullptr) {
                // Lower the ref count of all elements to be destructed.
                for (unsigned int l_Teller = a_Start; l_Teller != a_Start + a_Count; l_Teller++) {
                    ASSERT(m_List[l_Teller] != nullptr);

                    // Relinquish our reference to this element.
                    m_List[l_Teller]->Release();
                }

                // If it is not the last one, we have to move...
                if ((a_Start + a_Count) < m_Current) {
                    // Kill the entry, (again dirty but quick).
                    memmove(&(m_List[a_Start]), &(m_List[a_Start + a_Count + 1]), (a_Count * sizeof(IReferenceCounted*)));

#ifdef __DEBUG__
                    // Set all no longer used element to nullptr
                    memset(&(m_List[m_Current - a_Count]), 0, (a_Count * sizeof(IReferenceCounted*)));
                } else {
                    // Set all no longer used element to nullptr
                    memset(&(m_List[a_Start]), 0, (a_Count * sizeof(IReferenceCounted*)));
#endif
                }

                // Delete the given elements.
                m_Current = m_Current - a_Count;
            }
        }

        inline unsigned int
        Count() const
        {
            return (m_Current);
        }

        inline ProxyType<CONTEXT>
        operator[](unsigned int a_Index) const
        {
            ASSERT(a_Index < m_Current);
            ASSERT(m_List[a_Index] != nullptr);

            return (GetProxy(a_Index));
        }
        inline uint32_t CurrentQueueSize() const
        {
            return (m_Max);
        }

    private:
        inline ProxyType<CONTEXT>
        GetProxy(
            unsigned int Index) const
        {
            CONTEXT* baseClass = dynamic_cast<CONTEXT*>(m_List[Index]);
            return (ProxyType<CONTEXT>(m_List[Index], baseClass));
        }

        inline void
        SetProxy(
            unsigned int Index,
            ProxyType<CONTEXT>& proxy)
        {
            m_List[Index] = static_cast<IReferenceCounted*>(proxy);
        }

        //------------------------------------------------------------------------
        // Protected Attributes
        //------------------------------------------------------------------------
    private:
        IReferenceCounted** m_List;
        unsigned int m_Max;
        unsigned int m_Current;
    };

    template <typename CONTEXT>
    class ProxyQueue {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statements.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        ProxyQueue();
        ProxyQueue(const ProxyQueue<CONTEXT>&);
        ProxyQueue& operator=(const ProxyQueue<CONTEXT>&);

    public:
        explicit ProxyQueue(
            const unsigned int a_HighWaterMark)
            : m_Queue(a_HighWaterMark)
            , m_State(EMPTY)
            , m_MaxEntries(a_HighWaterMark)
        {
            // A highwatermark of 0 is bullshit.
            ASSERT(a_HighWaterMark != 0);

            TRACE_L5("Constructor ProxyQueue <%p>", (this));
        }

        ~ProxyQueue()
        {
            TRACE_L5("Destructor ProxyQueue <%p>", (this));

            // Disable the queue and flush all entries.
            Disable();
        }

        typedef enum {
            EMPTY = 0x0001,
            ENTRIES = 0x0002,
            LIMITED = 0x0004,
            DISABLED = 0x0008

        } enumQueueState;

        // -------------------------------------------------------------------
        // This queue enforces a Producer-Consumer pattern. It takes a
        // pointer on the heap. This pointer is created by the caller
        // (producer) of the Post method. It should be destructed by the
        // receiver (consumer). The consumer is the one that calls the
        // Receive method.
        // -------------------------------------------------------------------
        bool
        Remove(
            const ProxyType<CONTEXT>& a_Entry);

        bool
        Post(
            ProxyType<CONTEXT>& a_Entry);

        bool
        Insert(
            ProxyType<CONTEXT>& a_Entry,
            uint32_t a_WaitTime = Core::infinite);

        bool
        Extract(
            ProxyType<CONTEXT>& a_Entry,
            uint32_t a_WaitTime = Core::infinite);

        void Enable()
        {
            // This needs to be atomic. Make sure it is.
            m_State.Lock();

            if (m_State == DISABLED) {
                m_State.SetState(EMPTY);
            }

            // Done with the administration. Release the lock.
            m_State.Unlock();
        }

        void Disable()
        {
            // This needs to be atomic. Make sure it is.
            m_State.Lock();

            if (m_State != DISABLED) {
                // Change the state
                //lint -e{534}
                m_State.SetState(DISABLED);

                // Clear all entries !!
                m_Queue.Clear(0, m_Queue.Count());
            }

            // Done with the administration. Release the lock.
            m_State.Unlock();
        }

        inline void Lock()
        {
            // Lock the Queue.
            m_State.Lock();
        }

        inline void Unlock()
        {
            // Unlock the queue
            m_State.Unlock();
        }

        inline void FreeSlot() const
        {
            m_State.WaitState(false, DISABLED | ENTRIES | EMPTY, Core::infinite);
        }

        inline bool IsEmpty() const
        {
            return (m_Queue.Count() == 0);
        }

        inline bool IsFull() const
        {
            return (m_Queue.Count() >= m_MaxEntries);
        }

        inline uint32_t Count() const
        {
            return (m_Queue.Count());
        }

    private:
        ProxyList<CONTEXT> m_Queue;
        StateTrigger<enumQueueState> m_State;
        unsigned int m_MaxEntries;
    };

    // ---------------------------------------------------------------------------
    // PROXY QUEUE
    // ---------------------------------------------------------------------------

    template <class CONTEXT>
    bool
    ProxyQueue<CONTEXT>::Remove(
        const ProxyType<CONTEXT>& a_Entry)
    {
        bool l_Removed = false;

        // This needs to be atomic. Make sure it is.
        m_State.Lock();

        if (m_State != DISABLED) {
            // Yep, let's fill it
            l_Removed = m_Queue.Remove(a_Entry);

            // Determine the new state.
            m_State.SetState(IsEmpty() ? EMPTY : ENTRIES);
        }

        // Done with the administration. Release the lock.
        m_State.Unlock();

        return (l_Removed);
    }

    template <class CONTEXT>
    bool
    ProxyQueue<CONTEXT>::Post(
        ProxyType<CONTEXT>& a_Entry)
    {
        bool Result = false;

        // This needs to be atomic. Make sure it is.
        m_State.Lock();

        if (m_State != DISABLED) {
            // Yep, let's fill it
            //lint -e{534}
            m_Queue.Add(a_Entry);

            // Determine the new state.
            m_State.SetState(IsFull() ? LIMITED : ENTRIES);

            Result = true;
        }

        // Done with the administration. Release the lock.
        m_State.Unlock();

        return (Result);
    }

    template <class CONTEXT>
    bool
    ProxyQueue<CONTEXT>::Insert(
        ProxyType<CONTEXT>& a_Entry,
        uint32_t a_WaitTime)
    {
        bool l_Posted = false;
        bool l_Triggered = true;

        // This needs to be atomic. Make sure it is.
        m_State.Lock();

        if (m_State != DISABLED) {
            do {
                // And is there a slot available to us ?
                if (m_State != LIMITED) {
                    // We have posted it.
                    l_Posted = true;

                    // Yep, let's fill it
                    //lint -e{534}
                    m_Queue.Add(a_Entry);

                    // Determine the new state.
                    m_State.SetState(IsFull() ? LIMITED : ENTRIES);
                } else {
                    // We are moving into a wait, release the lock.
                    m_State.Unlock();

                    // Wait till the status of the queue changes.
                    l_Triggered = m_State.WaitState(false, DISABLED | ENTRIES | EMPTY, a_WaitTime);

                    // Seems something happend, lock the administration.
                    m_State.Lock();

                    // If we were reset, that is assumed to be also a timeout
                    l_Triggered = l_Triggered && (m_State != DISABLED);
                }

            } while ((l_Posted == false) && (l_Triggered != false));
        }

        // Done with the administration. Release the lock.
        m_State.Unlock();

        return (l_Posted);
    }

    template <class CONTEXT>
    bool
    ProxyQueue<CONTEXT>::Extract(
        ProxyType<CONTEXT>& a_Result,
        uint32_t a_WaitTime)
    {
        bool l_Received = false;
        bool l_Triggered = true;

        // This needs to be atomic. Make sure it is.
        m_State.Lock();

        if (m_State != DISABLED) {
            do {
                // And is there a slot to read ?
                if (m_State != EMPTY) {
                    l_Received = true;

                    // Get the first entry from the first spot..
                    m_Queue.Remove(0, a_Result);

                    // Determine the new state.
                    //lint -e{534}
                    m_State.SetState(IsEmpty() ? EMPTY : ENTRIES);
                } else {
                    // We are moving into a wait, release the lock.
                    m_State.Unlock();

                    // Wait till the status of the queue changes.
                    l_Triggered = m_State.WaitState(DISABLED | ENTRIES | LIMITED, a_WaitTime);

                    // Seems something happend, lock the administration.
                    m_State.Lock();

                    // If we were reset, that is assumed to be also a timeout
                    l_Triggered = l_Triggered && (m_State != DISABLED);
                }

            } while ((l_Received == false) && (l_Triggered != false));
        }

        // Done with the administration. Release the lock.
        m_State.Unlock();

        return (l_Received);
    }

    struct IProxyContainerElement : public virtual IReferenceCounted {
        ~IProxyContainerElement() override = default;

        virtual void Unlink() = 0;
    };
    template <typename ELEMENT>
    using ProxyContainerElement = typename std::conditional<std::is_base_of<IReferenceCounted, ELEMENT>::value != 0, ProxyService<ELEMENT>, ProxyObject<ELEMENT>>::type;

    template <typename CONTAINER, typename ELEMENT, typename EXPOSED = ELEMENT>
    class ProxyContainerType : public ProxyContainerElement<ELEMENT>, public IProxyContainerElement {
    private:
        using ThisClass = ProxyContainerType<CONTAINER, ELEMENT, EXPOSED>;

    public:
        ProxyContainerType() = delete;
        ProxyContainerType(const ProxyContainerType<CONTAINER, ELEMENT, EXPOSED>&) = delete;
        ProxyContainerType<CONTAINER, ELEMENT, EXPOSED>& operator=(const ProxyContainerType<CONTAINER, ELEMENT, EXPOSED>&) = delete;

    protected:
        template <typename... Args>
        ProxyContainerType(CONTAINER& parent, Args&&... args)
            : ProxyContainerElement<ELEMENT>(std::forward<Args>(args)...)
            , _parent(&parent)
        {
        }

    public:
        ~ProxyContainerType() override = default;

        template <typename ACTUALOBJECT, typename... Args>
        static typename Core::TypeTraits::enable_if<!std::is_same<ACTUALOBJECT, ELEMENT>::value, EXPOSED*>::type
            Create(IProxyContainerElement*& refInterface, CONTAINER& parent, Args&&... args)
        {
            return (ProxyContainerType<CONTAINER, ACTUALOBJECT, EXPOSED>::template Create<ACTUALOBJECT>(refInterface, parent, std::forward<Args>(args)...));
        }
        template <typename ACTUALOBJECT, typename... Args>
        static typename Core::TypeTraits::enable_if<std::is_same<ACTUALOBJECT, ELEMENT>::value, EXPOSED*>::type
            Create(IProxyContainerElement*& refInterface, CONTAINER& parent, Args&&... args) 
        {
            ThisClass* element = new (0) ThisClass(parent, std::forward<Args>(args)...);

            ASSERT(element != nullptr);

            if (element->__IsInitialized<CONTAINER, ELEMENT, EXPOSED>() == true) {
                refInterface = static_cast<IProxyContainerElement*>(element);
            }
            else {
                delete element;
            }

            return (element);
        }

    public:
        void AddRef() const override
        {
            if (ProxyService<ELEMENT>::LastRef() == true) {
                const_cast<ThisClass*>(this)->__Acquire<CONTAINER, ELEMENT, EXPOSED>();
            }
            ProxyService<ELEMENT>::AddRef();
        }
        uint32_t Release() const override
        {
            if (ProxyService<ELEMENT>::LastRef() == true) {
                // This can only happen of the parent has unlinked us, other wise 
                // the last release is always in the Unlink..
                ASSERT(_parent == nullptr);
                const_cast<ThisClass*>(this)->__Relinquish<CONTAINER, ELEMENT, EXPOSED>();
            }
            uint32_t result = ProxyService<ELEMENT>::Release();

            if ((result != Core::ERROR_DESTRUCTION_SUCCEEDED) && (ProxyService<ELEMENT>::LastRef() == true)) {
                const_cast<ThisClass*>(this)->Notify();

                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }

            return (result);
        }
        void Unlink() override
        {
            ASSERT(_parent != nullptr);

            if (ProxyService<ELEMENT>::LastRef() == false) {
                // This can only happen if the parent has unlinked us, while we are still being used somewhere..
                const_cast<ThisClass*>(this)->__Unlink<CONTAINER, ELEMENT, EXPOSED>();
            }

            // By incrementing this refcount the last reference count is definitily not reached, so safe to remove the parent as we are sure
            // that it will not be used while we clear it... 
            ProxyService<ELEMENT>::AddRef();
            _parent = nullptr;
            ProxyService<ELEMENT>::Release();
            ProxyService<ELEMENT>::Release();
        }
 
    private:
        void Notify() {
            __Relinquish<CONTAINER, ELEMENT, EXPOSED>();
            ProxyContainerElement<ELEMENT>::Clear();

            if (_parent != nullptr) {
                // The parent is the only one still holding this proxy. Let him now...
                _parent->Notify(static_cast<IProxyContainerElement*>(this), static_cast<EXPOSED*>(this));
            }
        }

        // -----------------------------------------------------
        // Check for IsInitialized method on Object
        // -----------------------------------------------------
        HAS_MEMBER(IsInitialized, hasIsInitialized);

        typedef hasIsInitialized<ELEMENT, bool (ELEMENT::*)() const> TraitIsInitialized;

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<ProxyContainerType<A, B, C>::TraitIsInitialized::value, bool>::type
            __IsInitialized() const
        {
            reurn (ELEMENT::IsInitialized());
        }

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<!ProxyContainerType<A, B, C>::TraitIsInitialized::value, bool>::type
            __IsInitialized() const
        {
            return (true);
        }

        // -----------------------------------------------------
        // Check for Aquire method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Acquire, hasAcquire);

        typedef hasAcquire<ELEMENT, void (ELEMENT::*)()> TraitAcquire;

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<ProxyContainerType<A, B, C>::TraitAcquire::value, void>::type
            __Acquire()
        {
            ELEMENT::Acquire();
        }

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<!ProxyContainerType<A, B, C>::TraitAcquire::value, void>::type
            __Acquire()
        {
        }

        // -----------------------------------------------------
        // Check for Relinquish method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Relinquish, hasRelinquish);

        typedef hasRelinquish<ELEMENT, void (ELEMENT::*)()> TraitRelinquish;

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<ProxyContainerType<A, B, C>::TraitRelinquish::value, void>::type
            __Relinquish()
        {
            ELEMENT::Relinquish();
        }

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<!ProxyContainerType<A, B, C>::TraitRelinquish::value, void>::type
            __Relinquish()
        {
        }

        // -----------------------------------------------------
        // Check for Unlink method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Unlink, hasUnlink);

        typedef hasUnlink<ELEMENT, void (ELEMENT::*)()> TraitUnlink;

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<ProxyContainerType<A, B, C>::TraitUnlink::value, void>::type
            __Unlink()
        {
            ELEMENT::Unlink();
        }

        template <typename A, typename B, typename C>
        inline typename Core::TypeTraits::enable_if<!ProxyContainerType<A, B, C>::TraitUnlink::value, void>::type
            __Unlink()
        {
        }

    private:
        CONTAINER* _parent;
    };

    template <typename PROXYELEMENT>
    class ProxyPoolType {
    private:
        using ContainerElement = ProxyContainerType< ProxyPoolType<PROXYELEMENT>, PROXYELEMENT>;
        using ContainerList = std::list< std::pair<IProxyContainerElement*, PROXYELEMENT*> >;

    public:
        ProxyPoolType(const ProxyPoolType<PROXYELEMENT>&) = delete;
        ProxyPoolType<PROXYELEMENT>& operator=(const ProxyPoolType<PROXYELEMENT>&) = delete;

        template <typename... Args>
        ProxyPoolType(const uint32_t initialQueueSize, Args&&... args)
            : _createdElements(initialQueueSize)
            , _lock()
        {
            for (uint32_t index = 0; index < initialQueueSize; index++) {
                IProxyContainerElement* refInterface;
                PROXYELEMENT* element = ContainerElement::template Create<PROXYELEMENT>(refInterface, *this, std::forward<Args>(args)...);

                ASSERT(element != nullptr);
                ASSERT(refInterface != nullptr);

                refInterface->AddRef();

                _queue.emplace_back(refInterface, element);
            }
        }
        ~ProxyPoolType()
        {
            // Clear the created objects..
            uint16_t attempt = 500;
            do {
                while (_queue.size() != 0) {

                    _lock.Lock();

                    _queue.front().first->Unlink();
                    _queue.pop_front();
                    _createdElements--;

                    _lock.Unlock();
                }

                if (_createdElements != 0) {
                    // Give up the slice, we are waiting for ProxyPool objects to return.
                    TRACE_L1("Pending ProxyPool objects. Waiting for %d objects.", _createdElements);
                    ::SleepMs(1);

                    attempt--;
                }

            } while ((attempt != 0) && (_createdElements != 0));

            if (_createdElements != 0) {
                TRACE_L1("Missing Pool Elements. PLease find leaking objects in: %s", typeid(PROXYELEMENT).name());
            }
        }

    public:
        template <typename... Args>
        Core::ProxyType<PROXYELEMENT> Element(Args&&... args)
        {
            Core::ProxyType<PROXYELEMENT> result;
            IProxyContainerElement* refInterface;
            PROXYELEMENT* element = nullptr;

            _lock.Lock();

            if (_queue.size() == 0) {

                _createdElements++;

                _lock.Unlock();


                element = ContainerElement::template Create<PROXYELEMENT>(refInterface, *this, std::forward<Args>(args)...);                
                refInterface->AddRef();
            }
            else {
                element = _queue.front().second;;
                refInterface = _queue.front().first;;
                _queue.pop_front();

                _lock.Unlock();
            }

            ASSERT(element != nullptr);

            // TRACE_L1("Reused an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));
            result = Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(refInterface), element);

            return (result);
        }
        inline uint32_t CreatedElements() const
        {
            return (_createdElements);
        }
        inline uint32_t QueuedElements() const
        {
            return (static_cast<uint32_t>(_queue.size()));
        }
        void Notify(IProxyContainerElement* refCount, PROXYELEMENT* element)
        {
            _lock.Lock();

            // TRACE_L1("Returned an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*element));
            _queue.emplace_back(refCount, element);

            _lock.Unlock();
        }

    private:
        uint32_t _createdElements;
        ContainerList _queue;
        mutable Core::CriticalSection _lock;
    };

    template <typename PROXYKEY, typename PROXYELEMENT>
    class ProxyMapType {
    private:
        using ContainerElement = ProxyContainerType< ProxyMapType< PROXYKEY, PROXYELEMENT>, PROXYELEMENT>;
        using ContainerMap = std::map<PROXYKEY, std::pair<IProxyContainerElement*, PROXYELEMENT*> >;

    public:
        ProxyMapType(const ProxyMapType<PROXYKEY, PROXYELEMENT>&) = delete;
        ProxyMapType<PROXYKEY, PROXYELEMENT>& operator=(const ProxyMapType<PROXYKEY, PROXYELEMENT>&) = delete;

        ProxyMapType()
            : _map()
            , _lock()
        {
        }
        ~ProxyMapType() {
            Clear();
        }

    public:
        template <typename ACTUALOBJECT, typename... Args>
        Core::ProxyType<PROXYELEMENT> Instance(const PROXYKEY& key, Args&&... args)
        {
            Core::ProxyType<PROXYELEMENT> result;
            IProxyContainerElement* refInterface;

            _lock.Lock();

            typename ContainerMap::iterator index(_map.find(key));

            if (index == _map.end()) {
                // Oops we do not have such an element, create it...
                PROXYELEMENT* newItem = ContainerElement::template Create<ACTUALOBJECT>(refInterface, *this, std::forward<Args>(args)...);

                if (newItem != nullptr) {
                    _map.emplace(std::piecewise_construct,
                        std::forward_as_tuple(key),
                        std::forward_as_tuple(refInterface, newItem));

                    // On account of our map we maintain a reference as well, untill we are the last :-)
                    refInterface->AddRef();

                    // Make sure the return value is already "accounted" for otherwise the copy of the
                    // element into the map will trigger the "last" on map reference.
                    result = Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(refInterface), newItem);
                }
            } else {
                result = Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(index->second.first), index->second.second);
            }

            _lock.Unlock();

            return (result);
        }
        Core::ProxyType<PROXYELEMENT> Find(const PROXYKEY& key)
        {
            Core::ProxyType<PROXYELEMENT> result;

            _lock.Lock();

            typename ContainerMap::iterator index(_map.find(key));

            if (index != _map.end()) {
                result = Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(index->second.first), index->second.second);
            }

            _lock.Unlock();

            return (result);
        }
        // void action<const PROXYKEY& key, const Core::ProxyType<PROXYELEMENT>& element>
        template<typename ACTION>
        void Visit(ACTION&& action ) const {
            _lock.Lock();
            for (const std::pair< PROXYKEY, std::pair<IProxyContainerElement*, PROXYELEMENT*> >& entry : _map) {
                action(entry.first, Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(entry.second.first), entry.second.second));
            }
            _lock.Unlock();
        }
        void Clear()
        {
            _lock.Lock();
            for (const std::pair< PROXYKEY, std::pair<IProxyContainerElement*, PROXYELEMENT*> >& entry : _map) {
                entry.second.first->Unlink();
            }
            _map.clear();
            _lock.Unlock();
        }
        void Notify(IProxyContainerElement* refCount, PROXYELEMENT* element)
        {
            _lock.Lock();

            typename ContainerMap::iterator index(_map.begin());

            // Find the element in the map..
            while ((index != _map.end()) && (index->second.first != refCount)) {
                index++;
            }

            ASSERT(index != _map.end());

            if (index != _map.end()) {
                refCount->Unlink();
                _map.erase(index);
            }

            _lock.Unlock();
        }

    private:
        ContainerMap _map;
        mutable Core::CriticalSection _lock;
    };

    template <typename PROXYELEMENT>
    class ProxyListType {
    private:
        using ContainerElement = ProxyContainerType< ProxyListType<PROXYELEMENT>, PROXYELEMENT>;
        using ContainerList = std::list<std::pair<IProxyContainerElement*, PROXYELEMENT*> >;

    public:
        ProxyListType(const ProxyListType<PROXYELEMENT>&) = delete;
        ProxyListType<PROXYELEMENT>& operator=(const ProxyListType<PROXYELEMENT>&) = delete;

        ProxyListType()
            : _lock()
            , _list()
        {
        }
        ~ProxyListType()
        {
            Clear();
        }

    public:
        template <typename ACTUALOBJECT, typename... Args>
        Core::ProxyType<PROXYELEMENT> Instance(Args&&... args)
        {
            IProxyContainerElement* refInterface;
            Core::ProxyType<PROXYELEMENT> result;

            _lock.Lock();

            PROXYELEMENT* newItem = ContainerElement::template Create<ACTUALOBJECT>(refInterface, *this, std::forward<Args>(args)...);

            if (newItem != nullptr) {
                _list.emplace_back(refInterface, newItem);

                // On account of our map we maintain a reference as well, untill we are the last :-)    
                newItem->AddRef();

                // Make sure the return value is already "accounted" for otherwise the copy of the
                // element into the map will trigger the "last" on map reference.
                result = Core::ProxyType<PROXYELEMENT>(static_cast<IReferenceCounted*>(refInterface), newItem);
            }

            _lock.Unlock();

            return (result);
        }

        void Clear()
        {
            _lock.Lock();
            for (const std::pair< IProxyContainerElement*, PROXYELEMENT*>& entry : _list) {
                entry.first->Unlink();
            }
            _list.clear();
            _lock.Unlock();
        }
        void Notify(IProxyContainerElement* refCount, PROXYELEMENT* element)
        {
            _lock.Lock();

            typename ContainerList::iterator index = _list.begin();

            while ( (index != _list.end()) && (index->first != refCount) ) {
                index++;
            }

            ASSERT(index != _list.end());

            if (index != _list.end()) {
                refCount->Unlink();
                _list.erase(index);
            }

            _lock.Unlock();
        }

    private:
        mutable Core::CriticalSection _lock;
        ContainerList _list;
    };
}
} // namespace Core

#endif // __INFRAPROXY_H
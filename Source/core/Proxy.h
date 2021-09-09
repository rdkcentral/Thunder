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

    template<typename CONTEXT>
    class ProxyType;

    template <typename CONTEXT>
    class ProxyObject final : public CONTEXT, virtual public IReferenceCounted {
    public:
        // ----------------------------------------------------------------
        // Never, ever allow reference counted objects to be assigned.
        // Create a new object and modify it. If the assignment operator
        // is used, give a compile error.
        // ----------------------------------------------------------------
        ProxyObject<CONTEXT>& operator=(const ProxyObject<CONTEXT>& rhs) = delete;

    protected:
        ProxyObject(CONTEXT& copy)
            : CONTEXT(copy)
            , _refCount(0)
        {
            __Initialize();
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
            }
            else {
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
        inline ProxyObject(Args&&... args)
            : CONTEXT(std::forward<Args>(args)...)
            , _refCount(0)
        {
            __Initialize();
        }
        ~ProxyObject() override
        {
            __Deinitialize();

            /* Hotfix for gcc linker issue preventing debug builds.
             *
             * Please see WPE-546 for details.
             */
             // ASSERT(_refCount == 0);

            TRACE_L5("Destructor ProxyObject <%p>", (this));
        }

    public:
        void AddRef() const override
        {
            const_cast<ProxyObject<CONTEXT>*>(this)->__Acquire();
            Core::InterlockedIncrement(_refCount);
        }
        uint32_t Release() const override
        {
            uint32_t result = Core::ERROR_NONE;

            if (Core::InterlockedDecrement(_refCount) == 0) {
                delete this;
                result = Core::ERROR_DESTRUCTION_SUCCEEDED;
            }
            else {
                const_cast<ProxyObject<CONTEXT>*>(this)->__Relinquish();
            }

            return (result);
        }
        inline operator const IReferenceCounted* () const {
            return (this);
        }
        inline operator CONTEXT& ()
        {
            return (*this);
        }
        inline operator const CONTEXT& () const
        {
            return (*this);
        }
        inline uint32_t Size() const
        {
            size_t alignedSize = ((sizeof(ProxyObject<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));

            return (*(reinterpret_cast<const uint32_t*>(&(reinterpret_cast<const uint8_t*>(this)[alignedSize]))));
        }
        template <typename TYPE>
        TYPE* Store()
        {
            size_t alignedSize = ((sizeof(ProxyObject<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));
            void* data = reinterpret_cast<void*>(&(reinterpret_cast<uint8_t*>(this)[alignedSize + sizeof(void*)]));
            void* result = Alignment(alignof(TYPE), data);
            return (reinterpret_cast<TYPE*>(result));
        }
        template <typename TYPE>
        const TYPE* Store() const
        {
            size_t alignedSize = ((sizeof(ProxyObject<CONTEXT>) + (sizeof(void*) - 1)) & (static_cast<size_t>(~(sizeof(void*) - 1))));
            void* data = const_cast<void*>(reinterpret_cast<const void*>(&(reinterpret_cast<const uint8_t*>(this)[alignedSize + sizeof(void*)])));
            const void* result = Alignment(alignof(TYPE), data);
            return (reinterpret_cast<const TYPE*>(result));
        }
        inline bool LastRef() const
        {
            return (_refCount == 1);
        }
        inline void Clear()
        {
            __Clear();
        }
        inline bool IsInitialized() const
        {
            return (__IsInitialized());
        }
        inline void CompositRelease()
        {
            // This release is intended for objects that Composit ProxyObject<> objects as a composition part.
            // At the moment these go out of scope, this CompositRelease has to be called. It is assuming the
            // last release but will not delete this object as that the the responsibility of the object that
            // has this object as a composit.
            Core::InterlockedDecrement(_refCount);

            ASSERT(_refCount == 0);
        }

    private:
        void Myself(Core::ProxyType<CONTEXT>&);

        // -----------------------------------------------------
        // Check for Clear method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Clear, hasClear);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasClear<TYPE, void (TYPE::*)()>::value, void>::type
            __Clear()
        {
            TYPE::Clear();
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasClear<TYPE, void (TYPE::*)()>::value, void>::type
            __Clear()
        {
        }

        // -----------------------------------------------------
        // Check for Initialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Initialize, hasInitialize);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasInitialize<TYPE, uint32_t(TYPE::*)()>::value, uint32_t>::type
            __Initialize()
        {
            return (TYPE::Initialize());
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasInitialize<TYPE, uint32_t(TYPE::*)()>::value, uint32_t>::type
            __Initialize()
        {
            return (Core::ERROR_NONE);
        }

        // -----------------------------------------------------
        // Check for Deinitialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Deinitialize, hasDeinitialize);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasDeinitialize<TYPE, void (TYPE::*)()>::value, void>::type
            __Deinitialize()
        {
            TYPE::Deinitialize();
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasDeinitialize<TYPE, void (TYPE::*)()>::value, void>::type
            __Deinitialize()
        {
        }

        // -----------------------------------------------------
        // Check for IsInitialized method on Object
        // -----------------------------------------------------
        HAS_MEMBER(IsInitialized, hasIsInitialized);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasIsInitialized<TYPE, bool (TYPE::*)() const>::value, bool>::type
            __IsInitialized() const
        {
            return(TYPE::IsInitialized());
        }
        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasIsInitialized<TYPE, bool (TYPE::*)() const>::value, bool>::type
            __IsInitialized() const
        {
            return (true);
        }

        // -----------------------------------------------------
        // Check for Aquire method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Acquire, hasAcquire);

        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasAcquire<TYPE, void (TYPE::*)(Core::ProxyType<CONTEXT>&)>::value, void>::type
            __Acquire()
        {
            if (LastRef() == true) {
                Core::ProxyType<TYPE> source;
                Myself(source);
                TYPE::Acquire(source);
                source.Reset();
            }
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasAcquire<TYPE, void (TYPE::*)(Core::ProxyType<CONTEXT>&)>::value, void>::type
            __Acquire()
        {
        }

        // -----------------------------------------------------
        // Check for Relinquish method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Relinquish, hasRelinquish);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasRelinquish<TYPE, void (TYPE::*)(Core::ProxyType<CONTEXT>&)>::value, void>::type
            __Relinquish()
        {
            if (LastRef() == true) {
                Core::ProxyType<TYPE> source;
                Myself(source);
                TYPE::Relinquish(source);
                source.Reset();
            }
        }
        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasRelinquish<TYPE, void (TYPE::*)(Core::ProxyType<CONTEXT>&)>::value, void>::type
            __Relinquish()
        {
        }

    protected:
        mutable uint32_t _refCount;
    };

    // ------------------------------------------------------------------------------
    // Reference counted object can only exist on heap (if reference count reaches 0)
    // delete is done. To avoid creation on the stack, this object can only be created
    // via the static create methods on this object.
    template <typename CONTEXT>
    class ProxyType {
    private:
        friend class ProxyObject<CONTEXT>;

	template <typename DERIVED>
        friend class ProxyType;

        void Load(IReferenceCounted& refObject, CONTEXT& realObject)
        {
            if (_refCount == nullptr) {
                _refCount->Release();
            }
            _refCount = &refObject;
            _realObject = &realObject;

            ASSERT((_refCount == nullptr) || (_realObject != nullptr));
        }
        void Reset()
        {
            _refCount = nullptr;
            _realObject = nullptr;
        }

    public:
        ProxyType()
            : _refCount(nullptr)
            , _realObject(nullptr)
        {
        }
        explicit ProxyType(ProxyObject<CONTEXT>& theObject)
            : _refCount(&theObject)
            , _realObject(&theObject)
        {
            _refCount->AddRef();
        }
        explicit ProxyType(CONTEXT& realObject)
            : _refCount(const_cast<IReferenceCounted*>(dynamic_cast<const IReferenceCounted*>(&realObject)))
            , _realObject(&realObject)
        {
            if (_refCount != nullptr) {
                _refCount->AddRef();
            }
        }
        explicit ProxyType(IReferenceCounted* refObject, CONTEXT* realObject)
            : _refCount(refObject)
            , _realObject(realObject)
        {
            ASSERT((refObject == nullptr) || (realObject != nullptr));

            if (_refCount != nullptr) {
                _refCount->AddRef();
            }
        }
        template <typename DERIVEDTYPE>
        explicit ProxyType(const ProxyType<DERIVEDTYPE>& copy)
        {
            CopyConstruct<DERIVEDTYPE>(copy, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
        }
        ProxyType(const ProxyType<CONTEXT>& copy)
            : _refCount(copy._refCount)
            , _realObject(copy._realObject)
        {
        }
        template <typename DERIVEDTYPE>
        explicit ProxyType(ProxyType<DERIVEDTYPE>&& move)
        {
            MoveConstruct<DERIVEDTYPE>(move, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
        }
        explicit ProxyType(ProxyType<CONTEXT>&& move)
            : ProxyType<CONTEXT>(move)
        {
        }
        ~ProxyType()
        {
            if (_refCount != nullptr) {
                _refCount->Release();
            }
        }

    public:
        template <typename... Args>
        inline static ProxyType<CONTEXT> Create(Args&&... args)
        {
            return (CreateEx(0, std::forward<Args>(args)...));
        }
        template <typename... Args>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, Args&&... args)
        {
            return (CreateObject(size, std::forward<Args>(args)...));
        }

        ProxyType<CONTEXT>& operator=(const ProxyType<CONTEXT>& rhs)
        {
            if (_refCount != rhs._refCount) {
                // Release the current holding object
                if (_refCount != nullptr) {
                    _refCount->Release();
                }

                // Get the new object
                _refCount = rhs._refCount;
                _realObject = rhs._realObject;

                if (_refCount != nullptr) {
                    _refCount->AddRef();
                }
            }

            return (*this);
        }

    public:
        inline bool IsValid() const
        {
            return (_refCount != nullptr);
        }
        inline uint32_t Release() const
        {
            // Only allowed on valid objects.
            ASSERT(_refCount != nullptr);

            uint32_t result = _refCount->Release();

            _refCount = nullptr;

            return (result);
        }
        inline void AddRef() const
        {
            // Only allowed on valid objects.
            ASSERT(_refCount != nullptr);

            _refCount->AddRef();
        }
        inline bool operator==(const ProxyType<CONTEXT>& a_RHS) const
        {
            return (_refCount == a_RHS._refCount);
        }

        inline bool operator!=(const ProxyType<CONTEXT>& a_RHS) const
        {
            return !(operator==(a_RHS));
        }
        inline bool operator==(const CONTEXT& a_RHS) const
        {
            return ((_refCount != nullptr) && (_realObject == &a_RHS));
        }

        inline bool operator!=(const CONTEXT& a_RHS) const
        {
            return (!operator==(a_RHS));
        }

        inline CONTEXT* operator->() const
        {
            ASSERT(_refCount != nullptr);

            return (_realObject);
        }

        inline CONTEXT& operator*() const
        {
            ASSERT(_refCount != nullptr);

            return (*_realObject);
        }

        inline operator IReferenceCounted* () const
        {
            return (_refCount);
        }

        void Destroy()
        {
            delete _refCount;
            _refCount = nullptr;
        }

    private:
        template <typename DERIVED>
        inline void CopyConstruct(const ProxyType<DERIVED>& source, const TemplateIntToType<true>&)
        {
            _refCount = source;

            if (_refCount == nullptr) {
                _realObject = nullptr;
            }
            else {
                _realObject = source.operator->();
                _refCount->AddRef();
            }
        }
        template <typename DERIVED>
        inline void CopyConstruct(const ProxyType<DERIVED>& source, const TemplateIntToType<false>&)
        {
            _realObject = (dynamic_cast<CONTEXT*>(source.operator->()));

            if (_realObject == nullptr) {
                _refCount = nullptr;
            }
            else {
                _refCount = source._refCount;
                _refCount->AddRef();
            }
        }
        template <typename DERIVED>
        inline void MoveConstruct(ProxyType<DERIVED>& source, const TemplateIntToType<true>&)
        {
            _refCount = source;

            if (_refCount == nullptr) {
                _realObject = nullptr;
            }
            else {
                _realObject = source.operator->();
                source.Reset();
            }
        }
        template <typename DERIVED>
        inline void MoveConstruct(ProxyType<DERIVED>& source, const TemplateIntToType<false>&)
        {
            _realObject = (dynamic_cast<CONTEXT*>(source.operator->()));

            if (_realObject == nullptr) {
                _refCount = nullptr;
            }
            else {
                _refCount = source;
                source.Reset();
            }
        }
        template <typename... Args>
        inline static ProxyType<CONTEXT> CreateObject(const uint32_t size, Args&&... args)
        {
            ProxyType<CONTEXT> result;
            ProxyObject<CONTEXT>* newItem = new (size) ProxyObject<CONTEXT>(std::forward<Args>(args)...);

            ASSERT(newItem != nullptr);

            if (newItem->IsInitialized() == false) {
                delete newItem;
            }
            else {
                result = ProxyType<CONTEXT>(static_cast<IReferenceCounted*>(newItem), static_cast<CONTEXT*>(newItem));
            }

            return (result);
        }

    private:
        mutable IReferenceCounted* _refCount;
        CONTEXT* _realObject;
    };

    template<typename CONTEXT>
    void ProxyObject<CONTEXT>::Myself(Core::ProxyType<CONTEXT>& myself) {
        myself.Load(static_cast<IReferenceCounted&>(*this), static_cast<CONTEXT&>(*this));
    }

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


    template <typename CONTAINER, typename CONTEXT>
    class ProxyContainerType : public CONTEXT {
    private:
        using ThisClass = ProxyContainerType<CONTAINER, CONTEXT>;

    public:
        ProxyContainerType() = delete;
        ProxyContainerType(const ProxyContainerType<CONTAINER, CONTEXT>&) = delete;
        ProxyContainerType<CONTAINER, CONTEXT>& operator=(const ProxyContainerType<CONTAINER, CONTEXT>&) = delete;

        template <typename... Args>
        ProxyContainerType(CONTAINER& parent, Args&&... args)
            : CONTEXT(std::forward<Args>(args)...)
            , _parent(&parent)
        {
        }
        ~ProxyContainerType() {
            if (_parent != nullptr) {
                __Unlink();
            }
        }

    public:
        void Unlink() {
            ASSERT(_parent != nullptr);

            _parent = nullptr;

            // This can only happen if the parent has unlinked us, while we are still being used somewhere..
            __Unlink();
        }

        // Forwarders as SFINEA is not lokking through calls inheritance trees..
        bool IsInitialized() const {
            return (__IsInitialized());
        }
        void Clear() {
            __Clear();
        }
        uint32_t Initialize() {
            return (__Initialize());
        }
        void Deinitialize() {
            __Deinitialize();
        }
        void Acquire(Core::ProxyType<ThisClass>& source) {
            Core::ProxyType<CONTEXT> base(std::move(source));
            __Acquire(base);
        }
        void Relinquish(Core::ProxyType<ThisClass>& source) {
            __Clear();

            if (_parent != nullptr) {
                // The parent is the only one still holding this proxy. Let him now...
                _parent->Notify(source);
            }
            else {
                Core::ProxyType<CONTEXT> base(std::move(source));
                __Relinquish(base);
            }
        }
        void Relinquish(Core::ProxyType<CONTEXT>& base) {
            __Relinquish(base);
        }

    private:        
        // -----------------------------------------------------
        // Check for Clear method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Clear, hasClear);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasClear<TYPE, void (TYPE::*)()>::value, void>::type
            __Clear()
        {
            CONTEXT::Clear();
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasClear<TYPE, void (TYPE::*)()>::value, void>::type
            __Clear()
        {
        }

        // -----------------------------------------------------
        // Check for Initialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Initialize, hasInitialize);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasInitialize<TYPE, uint32_t(TYPE::*)()>::value, uint32_t>::type
            __Initialize()
        {
            return (CONTEXT::Initialize());
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasInitialize<TYPE, uint32_t(TYPE::*)()>::value, uint32_t>::type
            __Initialize()
        {
            return (Core::ERROR_NONE);
        }

        // -----------------------------------------------------
        // Check for Deinitialize method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Deinitialize, hasDeinitialize);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasDeinitialize<TYPE, void (TYPE::*)()>::value, void>::type
            __Deinitialize()
        {
            CONTEXT::Deinitialize();
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasDeinitialize<TYPE, void (TYPE::*)()>::value, void>::type
            __Deinitialize()
        {
        }

        // -----------------------------------------------------
        // Check for IsInitialized method on Object
        // -----------------------------------------------------
        HAS_MEMBER(IsInitialized, hasIsInitialized);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasIsInitialized<TYPE, bool (TYPE::*)() const>::value, bool>::type
            __IsInitialized() const
        {
            reurn(TYPE::IsInitialized());
        }
        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasIsInitialized<TYPE, bool (TYPE::*)() const>::value, bool>::type
            __IsInitialized() const
        {
            return (true);
        }

        // -----------------------------------------------------
        // Check for Aquire method on Object
        // -----------------------------------------------------

        HAS_MEMBER(Acquire, hasAcquire);

        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasAcquire<TYPE, void (TYPE::*)(Core::ProxyType<TYPE>& source)>::value, void>::type
            __Acquire(Core::ProxyType<TYPE>& source)
        {
            TYPE::Acquire(source);
        }
        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasAcquire<TYPE, void (TYPE::*)(Core::ProxyType<TYPE>& source)>::value, void>::type
            __Acquire(Core::ProxyType<TYPE>& source)
        {
        }

        // -----------------------------------------------------
        // Check for Relinquish method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Relinquish, hasRelinquish);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasRelinquish<TYPE, void (TYPE::*)(Core::ProxyType<TYPE>&)>::value, void>::type
            __Relinquish(Core::ProxyType<TYPE>& source)
        {
            TYPE::Relinquish(source);
        }
        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasRelinquish<TYPE, void (TYPE::*)(Core::ProxyType<TYPE>&)>::value, void>::type
            __Relinquish(Core::ProxyType<TYPE>& source )
        {
        }

        // -----------------------------------------------------
        // Check for Unlink method on Object
        // -----------------------------------------------------
        HAS_MEMBER(Unlink, hasUnlink);

        template <typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<hasUnlink<TYPE, void (TYPE::*)()>::value, void>::type
            __Unlink()
        {
            TYPE::Unlink();
        }
        template < typename TYPE = CONTEXT>
        inline typename Core::TypeTraits::enable_if<!hasUnlink<TYPE, void (TYPE::*)()>::value, void>::type
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
        using ContainerList = std::list< Core::ProxyType<ContainerElement> >;

    public:
        ProxyPoolType(const ProxyPoolType<PROXYELEMENT>&) = delete;
        ProxyPoolType<PROXYELEMENT>& operator=(const ProxyPoolType<PROXYELEMENT>&) = delete;

        template <typename... Args>
        ProxyPoolType(const uint32_t initialQueueSize, Args&&... args)
            : _createdElements(initialQueueSize)
            , _lock()
        {
            for (uint32_t index = 0; index < initialQueueSize; index++) {
                _queue.emplace_back(Core::ProxyType<ContainerElement>::template Create(*this, std::forward<Args>(args)...));
                ASSERT(_queue.back().IsValid() == true);
            }
        }
        ~ProxyPoolType()
        {
            // Clear the created objects..
            uint16_t attempt = 500;
            do {
                while (_queue.size() != 0) {

                    _lock.Lock();

                    // Make sure we store it in a ProxyType, so that the refcount is
                    // by definition 2, one in the queue and 1 in the expandable.
                    // This way the Notify (that uses the _parent in the ContainerElment)
                    // will not be used, fired used, as that pointer is not protected there..
                    Core::ProxyType<ContainerElement> expendable(_queue.front());
                    expendable->Unlink();
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
            Core::ProxyType<ContainerElement> element;

            _lock.Lock();

            if (_queue.size() == 0) {

                _createdElements++;

                _lock.Unlock();

                element = Core::ProxyType<ContainerElement>::template Create(*this, std::forward<Args>(args)...);
                result = Core::ProxyType<PROXYELEMENT>(element);
            }
            else {
                element = _queue.front();
                result = Core::ProxyType<PROXYELEMENT>(element);
                _queue.pop_front();

                _lock.Unlock();
            }

            ASSERT(element != nullptr);

            // As it is removed from the queue, wewill keep a "flying reference", this 
            // way if the user of ths object releases it, it will trigger the last 
            // refernce notification (Relinquish) prior to the user dropping the 
            // objects last reference (cuase we hold it here), we will get a notificaion
            // and move this "AddRef" into the queue again (move)
            result.AddRef();

            // TRACE_L1("Reused an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));

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
        void Notify(Core::ProxyType<ContainerElement>& source)
        {
            // We should send the Relinquish(Core::ProxyType<PROXYELEMENT>&) from here be we need to chang the incoming
            // source element without touching the reference count.. oops, since now it is not used,
            // lets skip it for now..
            // TODO: Call source->Relinquish(Core::ProxyType<PROXYELEMENT>&);

            _lock.Lock();

            // TRACE_L1("Returned an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*element));
            _queue.emplace_back(std::move(source));

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
        using ContainerMap = std::map<PROXYKEY, Core::ProxyType<ContainerElement> >;

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
        template <typename... Args>
        Core::ProxyType<PROXYELEMENT> Instance(PROXYKEY& key, Args&&... args)
        {
            Core::ProxyType<PROXYELEMENT> result;

            _lock.Lock();

            typename ContainerMap::iterator index(_map.find(key));

            if (index == _map.end()) {
                // Oops we do not have such an element, create it...
                Core::ProxyType<ContainerElement> newItem = Core::ProxyType<ContainerElement>::template Create(*this, std::forward<Args>(args)...);

                if (newItem.IsValid() == true) {
                    _map.emplace(std::piecewise_construct,
                        std::forward_as_tuple(key),
                        std::forward_as_tuple(newItem));

                    // Make sure the return value is already "accounted" for otherwise the copy of the
                    // element into the map will trigger the "last" on map reference.
                    result = Core::ProxyType<PROXYELEMENT>(newItem);
                }
            } else {
                result = Core::ProxyType<PROXYELEMENT>(index->second);
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
                result = Core::ProxyType<PROXYELEMENT>(index->second);
            }

            _lock.Unlock();

            return (result);
        }
        // void action<const PROXYKEY& key, const Core::ProxyType<PROXYELEMENT>& element>
        template<typename ACTION>
        void Visit(ACTION&& action ) const {
            _lock.Lock();
            for (const std::pair< PROXYKEY, Core::ProxyType<ContainerElement> >& entry : _map) {
                action(entry.first, Core::ProxyType<PROXYELEMENT>(entry.second.second));
            }
            _lock.Unlock();
        }
        void Clear()
        {
            _lock.Lock();
            for (const std::pair< PROXYKEY, Core::ProxyType<ContainerElement> >& entry : _map) {
                entry.second->Unlink();
            }
            _map.clear();
            _lock.Unlock();
        }
        void Notify(Core::ProxyType<ContainerElement>& source)
        {
            _lock.Lock();

            typename ContainerMap::iterator index(_map.begin());

            // Find the element in the map..
            while ((index != _map.end()) && (index->second != source)) {
                index++;
            }

            ASSERT(index != _map.end());

            if (index != _map.end()) {
                source->Unlink();
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
        using ContainerList = std::list< Core::ProxyType<ContainerElement> >;

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
        template <typename... Args>
        Core::ProxyType<PROXYELEMENT> Instance(Args&&... args)
        {
            Core::ProxyType<PROXYELEMENT> result;

            _lock.Lock();

            Core::ProxyType<ContainerElement> newItem = Core::ProxyType<ContainerElement>::template Create(*this, std::forward<Args>(args)...);

            if (newItem != nullptr) {
                _list.emplace_back(newItem);

                // Make sure the return value is already "accounted" for otherwise the copy of the
                // element into the map will trigger the "last" on map reference.
                result = Core::ProxyType<PROXYELEMENT>(newItem);
            }

            _lock.Unlock();

            return (result);
        }

        void Clear()
        {
            _lock.Lock();
            for (const Core::ProxyType<ContainerElement>& entry : _list) {
                entry->Unlink();
            }
            _list.clear();
            _lock.Unlock();
        }
        void Notify(Core::ProxyType<ContainerElement>& source)
        {
            _lock.Lock();

            typename ContainerList::iterator index = _list.begin();

            while ( (index != _list.end()) && (index != source) ) {
                index++;
            }

            ASSERT(index != _list.end());

            if (index != _list.end()) {
                source->Unlink();
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

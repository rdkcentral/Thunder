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
#include <atomic>

// ---- Include local include files ----
#include "Portability.h"
#include "StateTrigger.h"
#include "Sync.h"
#include "TypeTraits.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace Thunder {
    namespace Core {

        template<typename CONTEXT>
        class ProxyType;

        PUSH_WARNING(DISABLE_WARNING_MULTPILE_INHERITENCE_OF_BASE_CLASS)
        template <typename CONTEXT>
        class ProxyObject final : public CONTEXT, public std::conditional<std::is_base_of<IReferenceCounted, CONTEXT>::value, Void, IReferenceCounted>::type {
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
                reinterpret_cast<ProxyObject<CONTEXT>*>(stAllocateBlock)->__Destructed();
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
            uint32_t AddRef() const override
            {
                if (_refCount == 1) {
                    const_cast<ProxyObject<CONTEXT>*>(this)->__Acquire();
                }
                _refCount++;

                return (Core::ERROR_NONE);
            }
            uint32_t Release() const override
            {
                uint32_t result = Core::ERROR_NONE;
                uint32_t lastRef = --_refCount;

                if (lastRef == 0) {
                    result = Core::ERROR_DESTRUCTION_SUCCEEDED;
                    delete this;
                }
                else if (lastRef == 1) {
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
                _refCount--;

                ASSERT(_refCount == 0);
            }

        private:
            void Myself(Core::ProxyType<CONTEXT>&);

            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Clear, hasClear);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasClear<TYPE, void>::value, void>::type
                __Clear()
            {
                TYPE::Clear();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasClear<TYPE, void>::value, void>::type
                __Clear()
            {
            }

            // -----------------------------------------------------
            // Check for Initialize method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Initialize, hasInitialize);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasInitialize<TYPE, uint32_t>::value, uint32_t>::type
                __Initialize()
            {
                return (TYPE::Initialize());
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasInitialize<TYPE, uint32_t>::value, uint32_t>::type
                __Initialize()
            {
                return (Core::ERROR_NONE);
            }

            // -----------------------------------------------------
            // Check for Deinitialize method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Deinitialize, hasDeinitialize);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasDeinitialize<TYPE, void>::value, void>::type
                __Deinitialize()
            {
                TYPE::Deinitialize();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasDeinitialize<TYPE, void>::value, void>::type
                __Deinitialize()
            {
            }

            // -----------------------------------------------------
            // Check for Destructed method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Destructed, hasDestructed);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasDestructed<TYPE, void>::value, void>::type
                __Destructed()
            {
                TYPE::Destructed();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasDestructed<TYPE, void>::value, void>::type
                __Destructed()
            {
            }


            // -----------------------------------------------------
            // Check for IsInitialized method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(IsInitialized, hasIsInitialized);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasIsInitialized<const TYPE, bool>::value, bool>::type
                __IsInitialized() const
            {
                return(TYPE::IsInitialized());
            }
            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasIsInitialized<const TYPE, bool>::value, bool>::type
                __IsInitialized() const
            {
                return (true);
            }

            // -----------------------------------------------------
            // Check for Acquire method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Acquire, hasAcquire);

            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasAcquire<TYPE, void, Core::ProxyType<TYPE>&>::value, void>::type
                __Acquire()
            {
                Core::ProxyType<TYPE> source;
                Myself(source);
                TYPE::Acquire(source);
                source.Reset();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasAcquire<TYPE, void, Core::ProxyType<TYPE>&>::value, void>::type
                __Acquire()
            {
            }

            // -----------------------------------------------------
            // Check for Relinquish method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Relinquish, hasRelinquish);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasRelinquish<TYPE, void, Core::ProxyType<TYPE>&>::value, void>::type
                __Relinquish()
            {
                Core::ProxyType<TYPE> source;
                Myself(source);
                TYPE::Relinquish(source);
                source.Reset();
            }
            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasRelinquish<TYPE, void, Core::ProxyType<TYPE>&>::value, void>::type
                __Relinquish()
            {
            }

        protected:
            mutable std::atomic<uint32_t> _refCount;
        };
POP_WARNING()


        // ------------------------------------------------------------------------------
        // Reference counted object can only exist on heap (if reference count reaches 0)
        // delete is done. To avoid creation on the stack, this object can only be created
        // via the static create methods on this object.
        template <typename CONTEXT>
        class ProxyType {
        private:
            friend class ProxyObject<CONTEXT>;

            template <typename DERIVED>
            friend class ProxyList;

            template <typename DERIVED>
            friend class ProxyType;

            template <typename CONTAINER, typename PROXYCONTEXT, typename STORED>
            friend class ProxyContainerType;

            template<typename OBJECT>
            void Load(Core::ProxyObject<OBJECT>& realObject)
            {
                ASSERT(_refCount == nullptr);

                _refCount = static_cast<IReferenceCounted*>(&realObject);
                _realObject = static_cast<CONTEXT*>(&realObject);

                ASSERT((_refCount != nullptr) && (_realObject != nullptr));
            }
            void Reset()
            {
                _refCount = nullptr;
                _realObject = nullptr;
            }
            explicit ProxyType(IReferenceCounted* refObject)
                : _refCount(refObject)
                , _realObject(dynamic_cast<CONTEXT*>(refObject))
            {
                ASSERT(refObject != nullptr);

                if (_realObject != nullptr) {
                    _refCount->AddRef();
                }
                else {
                    _refCount = nullptr;
                }
            }

        public:
            ProxyType()
                : _refCount(nullptr)
                , _realObject(nullptr)
            {
            }
            ProxyType(IReferenceCounted& lifetime, CONTEXT& contex)
                : _refCount(&lifetime)
                , _realObject(&contex)
            {
                _refCount->AddRef();
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
            template <typename DERIVEDTYPE>
            explicit ProxyType(const ProxyType<DERIVEDTYPE>& copy)
            {
                CopyConstruct<DERIVEDTYPE>(copy, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
            }
            template <typename DERIVEDTYPE>
            explicit ProxyType(ProxyType<DERIVEDTYPE>&& move)
            {
                MoveConstruct<DERIVEDTYPE>(std::move(move), TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
            }
            ProxyType(const ProxyType<CONTEXT>& copy)
                : _refCount(copy._refCount)
                , _realObject(copy._realObject)
            {
                ASSERT((_refCount == nullptr) || (_realObject != nullptr));

                if (_refCount != nullptr) {
                    _refCount->AddRef();
                }
            }
            ProxyType(ProxyType<CONTEXT>&& move) noexcept
                : _refCount(move._refCount)
                , _realObject(move._realObject)
            {
                move._refCount = nullptr;
            }
            ~ProxyType()
            {
                if (_refCount != nullptr) {
                    _refCount->Release();
                }
            }

        public:
            template <typename... Args>
            inline static void CreateMove(ProxyType<CONTEXT>& newObject, const uint32_t size, Args&&... args)
            {
                newObject = std::move(ProxyType<CONTEXT>(*CreateObject(size, std::forward<Args>(args)...)));
            }
            template <typename... Args>
            inline static ProxyType<CONTEXT> Create(Args&&... args)
            {
                return (ProxyType<CONTEXT>(*CreateObject(0, std::forward<Args>(args)...)));
            }
            template <typename... Args>
            inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, Args&&... args)
            {
                return (ProxyType<CONTEXT>(*CreateObject(size, std::forward<Args>(args)...)));
            }

            template <typename DERIVEDTYPE>
            ProxyType<CONTEXT>& operator=(const ProxyType<DERIVEDTYPE>& rhs)
            {
                // If we already have one, lets remove the one we got first
                if (_refCount != nullptr) _refCount->Release();

                CopyConstruct<DERIVEDTYPE>(rhs, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());

                return(*this);
            }

            ProxyType<CONTEXT>& operator=(const ProxyType<CONTEXT>& rhs)
            {
                // If we already have one, lets remove the one we got first
                if (_refCount != nullptr) _refCount->Release();

                CopyConstruct<CONTEXT>(rhs, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, CONTEXT>::value>());

                return(*this);
            }

            template <typename DERIVEDTYPE>
            ProxyType<CONTEXT>& operator=(ProxyType<DERIVEDTYPE>&& rhs)
            {
                // If we already have one, lets remove the one we got first
                if (_refCount != nullptr) _refCount->Release();

                MoveConstruct<DERIVEDTYPE>(std::move(rhs), TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());

                return(*this);
            }

            ProxyType<CONTEXT>& operator=(ProxyType<CONTEXT>&& rhs) noexcept
            {
                // If we already have one, lets remove the one we got first
                if (_refCount != nullptr) _refCount->Release();

                MoveConstruct<CONTEXT>(std::move(rhs), TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, CONTEXT>::value>());

                return(*this);
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

                IReferenceCounted* resource = _refCount;
                _refCount = nullptr;

                uint32_t result = resource->Release();

                return (result);
            }
            inline uint32_t AddRef() const
            {
                // Only allowed on valid objects.
                ASSERT(_refCount != nullptr);

                return (_refCount->AddRef());
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

            inline ProxyObject<CONTEXT>* Origin() const {
                return (static_cast<ProxyObject<CONTEXT>*>(_realObject));
            }

        private:
            template <typename DERIVED>
            inline void CopyConstruct(const ProxyType<DERIVED>& source, const TemplateIntToType<true>&)
            {
                _refCount = source._refCount;

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
                if (source.IsValid() == false) {
                    _refCount = nullptr;
                    _realObject = nullptr;
                }
                else {
                    DERIVED* sourceClass = source.operator->();
                    _realObject = (dynamic_cast<CONTEXT*>(sourceClass));

                    if (_realObject == nullptr) {
                        _refCount = nullptr;
                    }
                    else {
                        _refCount = source._refCount;
                        _refCount->AddRef();
                    }

                }
            }
            template <typename DERIVED>
            inline void MoveConstruct(ProxyType<DERIVED>&& source, const TemplateIntToType<true>&)
            {
                _refCount = source._refCount;

                if (_refCount == nullptr) {
                    _realObject = nullptr;
                }
                else {
                    _realObject = source.operator->();
                    source.Reset();
                }
            }
            template <typename DERIVED>
            inline void MoveConstruct(ProxyType<DERIVED>&& source, const TemplateIntToType<false>&)
            {
                if (source.IsValid() == false) {
                    _refCount = nullptr;
                    _realObject = nullptr;
                }
                else {
                    _realObject = (dynamic_cast<CONTEXT*>(source.operator->()));

                    if (_realObject == nullptr) {
                        _refCount = nullptr;
                    }
                    else {
                        _refCount = source._refCount;
                        source.Reset();
                    }
                }
            }
            template <typename... Args>
            inline static ProxyObject<CONTEXT>* CreateObject(const uint32_t size, Args&&... args)
            {
                ProxyObject<CONTEXT>* newItem = new (size) ProxyObject<CONTEXT>(std::forward<Args>(args)...);

                ASSERT(newItem != nullptr);

                if (newItem->IsInitialized() == false) {
                    delete newItem;
                    newItem = nullptr;
                }

                return (newItem);
            }

        private:
            mutable IReferenceCounted* _refCount;
            CONTEXT* _realObject;
        };

        template<typename CONTEXT>
        void ProxyObject<CONTEXT>::Myself(Core::ProxyType<CONTEXT>& myself) {
            myself.template Load<CONTEXT>(*this);
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
                m_List = new IReferenceCounted * [m_Max];

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
                    IReferenceCounted** l_NewList = new IReferenceCounted * [static_cast<uint32_t>(m_Max << 1)];

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
                ASSERT(m_List[a_Index] != nullptr);

                // Remember the item on the location, It should be a relaes and an add for
                // the new one, To optimize for speed, just copy the count.
                a_Entry = GetProxy(a_Index);

                // If it is taken out, release the reference that we took during the add
                m_List[a_Index]->Release();
                m_List[a_Index] = nullptr;

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
                ASSERT(m_List[a_Index] != nullptr);

                // If it is taken out, release the reference that we took during the add
                m_List[a_Index]->Release();
                m_List[a_Index] = nullptr;

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
            void Clear() {
                for (unsigned int teller = 0; teller != m_Current; teller++) {
                    m_List[teller]->Release();
                }
                m_Current = 0;
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
                    }
                    else {
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
                return (ProxyType<CONTEXT>(m_List[Index]));
            }
            inline void
                SetProxy(
                    unsigned int Index,
                    ProxyType<CONTEXT>& proxy)
            {
                m_List[Index] = proxy._refCount;
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
        public:
            ProxyQueue() = delete;
            ProxyQueue(const ProxyQueue<CONTEXT>&) = delete;
            ProxyQueue& operator=(const ProxyQueue<CONTEXT>&) = delete;

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

        class UnlinkStorage {
        public:
            UnlinkStorage() = delete;
            UnlinkStorage& operator=(const UnlinkStorage&) = delete;

            UnlinkStorage(void (*callback)(void*), void* thisPtr)
                : _callback(callback)
                , _thisptr(thisPtr) {
            }
            UnlinkStorage(const UnlinkStorage& copy)
                : _callback(copy._callback)
                , _thisptr(copy._thisptr) {
            }
            ~UnlinkStorage() = default;

        public:
            void Unlink() {
                ASSERT(_callback != nullptr);
                _callback(_thisptr);
            }

        private:
            void (*_callback)(void*);
            void* _thisptr;
        };

        template <typename CONTAINER, typename CONTEXT, typename STORED>
        class ProxyContainerType : public CONTEXT {
        private:
            using ThisClass = ProxyContainerType<CONTAINER, CONTEXT, STORED>;

        public:
            ProxyContainerType() = delete;
            ProxyContainerType(const ProxyContainerType<CONTAINER, CONTEXT, STORED>&) = delete;
            ProxyContainerType<CONTAINER, CONTEXT, STORED>& operator=(const ProxyContainerType<CONTAINER, CONTEXT, STORED>&) = delete;

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
            UnlinkStorage Handler() {
                return (UnlinkStorage(UnlinkFromParent, static_cast<ThisClass*>(this)));
            }
            void Unlink()
            {
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
                __Acquire(source);
            }
            void Relinquish(Core::ProxyType<ThisClass>& source) {

                __Relinquish(source);

                if (_parent != nullptr) {
                    // The parent is the only one still holding this proxy. Let him now...
                    Notify(source, TemplateIntToType<Core::TypeTraits::is_same<CONTEXT, STORED>::value>());
                }
            }
            void Relinquish(Core::ProxyType<CONTEXT>& base) {
                __Relinquish(base);
            }

        private:
            static void UnlinkFromParent(void* parent)
            {
                reinterpret_cast<ThisClass*>(parent)->Unlink();
            }
            void Notify(ProxyType<ThisClass>& source, const TemplateIntToType<true>&) {
                _parent->Notify(source);
            }
            void Notify(ProxyType<ThisClass>& source, const TemplateIntToType<false>&) {
                Core::ProxyType<STORED> base(std::move(source));

                _parent->Notify(base);

                base.Reset();
            }
            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Clear, hasClear);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasClear<TYPE, void>::value, void>::type
                __Clear()
            {
                CONTEXT::Clear();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasClear<TYPE, void>::value, void>::type
                __Clear()
            {
            }

            // -----------------------------------------------------
            // Check for Initialize method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Initialize, hasInitialize);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasInitialize<TYPE, uint32_t>::value, uint32_t>::type
                __Initialize()
            {
                return (TYPE::Initialize());
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasInitialize<TYPE, uint32_t>::value, uint32_t>::type
                __Initialize()
            {
                return (Core::ERROR_NONE);
            }

            // -----------------------------------------------------
            // Check for Deinitialize method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Deinitialize, hasDeinitialize);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasDeinitialize<TYPE, void>::value, void>::type
                __Deinitialize()
            {
                TYPE::Deinitialize();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasDeinitialize<TYPE, void>::value, void>::type
                __Deinitialize()
            {
            }

            // -----------------------------------------------------
            // Check for IsInitialized method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(IsInitialized, hasIsInitialized);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasIsInitialized<const TYPE, bool>::value, bool>::type
                __IsInitialized() const
            {
                reurn(TYPE::IsInitialized());
            }
            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasIsInitialized<const TYPE, bool>::value, bool>::type
                __IsInitialized() const
            {
                return (true);
            }

            // -----------------------------------------------------
            // Check for Acquire method on Object
            // -----------------------------------------------------

            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Acquire, hasAcquire);

            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasAcquire<TYPE, void, Core::ProxyType<STORED>&>::value, void>::type
                __Acquire(Core::ProxyType<ThisClass>& source)
            {
                Core::ProxyType<STORED> base;

                // TODO: This dynamic_cast can by definition be changed to a static cast... I think :-)
                base.template Load<ThisClass>(*static_cast<Core::ProxyObject<ThisClass>*>(source.operator->()));

                TYPE::Acquire(base);

                base.Reset();
            }
            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasAcquire<TYPE, void, Core::ProxyType<STORED>&>::value, void>::type
                __Acquire(Core::ProxyType<ThisClass>&)
            {
            }

            // -----------------------------------------------------
            // Check for Relinquish method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Relinquish, hasRelinquish);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasRelinquish<TYPE, void, Core::ProxyType<STORED>&>::value, void>::type
                __Relinquish(Core::ProxyType<ThisClass>& source)
            {
                Core::ProxyType<STORED> base;

                base.template Load<ThisClass>(*static_cast<Core::ProxyObject<ThisClass>*>(source.operator->()));

                TYPE::Relinquish(base);

                base.Reset();
            }
            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasRelinquish<TYPE, void, Core::ProxyType<STORED>&>::value, void>::type
                __Relinquish(Core::ProxyType<ThisClass>&)
            {
            }

            // -----------------------------------------------------
            // Check for Unlink method on Object
            // -----------------------------------------------------
            IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Unlink, hasUnlink);

            template <typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<hasUnlink<TYPE, void>::value, void>::type
                __Unlink()
            {
                TYPE::Unlink();
            }
            template < typename TYPE = CONTEXT>
            inline typename Core::TypeTraits::enable_if<!hasUnlink<TYPE, void>::value, void>::type
                __Unlink()
            {
            }

        private:
            CONTAINER* _parent;
        };

        template <typename PROXYELEMENT>
        class ProxyPoolType {
        private:
            using ContainerElement = ProxyContainerType< ProxyPoolType<PROXYELEMENT>, PROXYELEMENT, PROXYELEMENT>;
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
                    Core::ProxyType<ContainerElement> newElement;

                    Core::ProxyType<ContainerElement>::template CreateMove(newElement, 0, *this, std::forward<Args>(args)...);
                    _queue.emplace_back(std::move(newElement));
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

                        Core::ProxyType<ContainerElement> expendable(std::move(_queue.front()));
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

                    Core::ProxyType<ContainerElement>::template CreateMove(element, 0, *this, std::forward<Args>(args)...);

                    result = Core::ProxyType<PROXYELEMENT>(element);
                }
                else {
                    element = _queue.front();
                    result = Core::ProxyType<PROXYELEMENT>(element);
                    _queue.pop_front();

                    _lock.Unlock();
                }

                ASSERT(element.IsValid());

                // As it is removed from the queue, we will keep a "flying reference", this
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

                source->Clear();

                // Lets see if the source is already in there :-)
                ASSERT(std::find(_queue.begin(), _queue.end(), source) == _queue.end());

                // TRACE_L1("Returned an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*element));
                _queue.emplace_back(std::move(source));

                _lock.Unlock();
            }
            uint32_t Count() const {
                return (_createdElements);
            }

        private:
            uint32_t _createdElements;
            ContainerList _queue;
            mutable Core::CriticalSection _lock;
        };

        template <typename PROXYKEY, typename PROXYELEMENT>
        class ProxyMapType {
        private:
            using ContainerElement = ProxyContainerType< ProxyMapType< PROXYKEY, PROXYELEMENT>, PROXYELEMENT, PROXYELEMENT>;
            using ContainerStorage = std::pair < Core::ProxyType<PROXYELEMENT>, UnlinkStorage>;
            using ContainerMap = std::map<PROXYKEY, ContainerStorage>;

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
            struct IFind {
                // Return true if Check is ok
                virtual bool Check(const PROXYKEY& key, const Core::ProxyType<PROXYELEMENT>& element) const = 0;
                virtual ~IFind() = default;
            };

            bool Find(const IFind& callback) const {
                bool found(false);

                _lock.Lock();
                for (const auto& entry : _map) {
                    if(callback.Check(entry.first, entry.second.first) == true){
                        found = true;
                        break; 
                    }
                }
                _lock.Unlock();

                return found;
            }

            template <typename ACTUALOBJECT, typename... Args>
            Core::ProxyType<PROXYELEMENT> Instance(const PROXYKEY& key, Args&&... args)
            {
                using ActualElement = ProxyContainerType< ProxyMapType< PROXYKEY, PROXYELEMENT>, ACTUALOBJECT, PROXYELEMENT>;

                Core::ProxyType<PROXYELEMENT> result;

                _lock.Lock();

                typename ContainerMap::iterator index(_map.find(key));

                if (index == _map.end()) {
                    // Oops we do not have such an element, create it...
                    Core::ProxyType<ActualElement> newItem;
                    Core::ProxyType<ActualElement>::template CreateMove(newItem, 0, *this, std::forward<Args>(args)...);

                    if (newItem.IsValid() == true) {

                        UnlinkStorage linkInfo(newItem->Handler());

                        // Make sure the return value is already "accounted" for otherwise the copy of the
                        // element into the map will trigger the "last" on map reference.
                        result = Core::ProxyType<PROXYELEMENT>(std::move(newItem));

                        _map.emplace(std::piecewise_construct,
                            std::forward_as_tuple(key),
                            std::forward_as_tuple(ContainerStorage(result, linkInfo)));

                    }
                } else {
                    result = Core::ProxyType<PROXYELEMENT>(index->second.first);
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
                    result = Core::ProxyType<PROXYELEMENT>(index->second.first);
                }

                _lock.Unlock();

                return (result);
            }
            Core::ProxyType<const PROXYELEMENT> Find(const PROXYKEY& key) const
            {
                Core::ProxyType<const PROXYELEMENT> result;

                _lock.Lock();

                typename ContainerMap::const_iterator index(_map.find(key));

                if (index != _map.end()) {
                    result = Core::ProxyType<const PROXYELEMENT>(index->second.first);
                }

                _lock.Unlock();

                return (result);
            }
            // void action<const PROXYKEY& key, const Core::ProxyType<PROXYELEMENT>& element>
            template<typename ACTION>
            void Visit(ACTION&& action) {
                _lock.Lock();
                for (std::pair<const PROXYKEY, ContainerStorage>& entry : _map) {
                    action(entry.first, entry.second.first);
                }
                _lock.Unlock();
            }
            // void action<const PROXYKEY& key, const Core::ProxyType<PROXYELEMENT>& element>
            template<typename ACTION>
            void Visit(ACTION&& action) const {
                _lock.Lock();
                for (const std::pair<const PROXYKEY, ContainerStorage>& entry : _map) {
                    action(entry.first, entry.second.first);
                }
                _lock.Unlock();
            }
            void Clear()
            {
                _lock.Lock();
                for (std::pair<const PROXYKEY, ContainerStorage>& entry : _map) {
                    entry.second.second.Unlink();
                }
                _map.clear();
                _lock.Unlock();
            }
            void Notify(Core::ProxyType<ContainerElement>& source)
            {
                _lock.Lock();

                typename ContainerMap::iterator index(_map.begin());

                // Find the element in the map..
                while ((index != _map.end()) && (index->second.first.operator->() != source.operator->())) {
                    index++;
                }

                ASSERT(index != _map.end());

                if (index != _map.end()) {
                    index->second.second.Unlink();
                    _map.erase(index);
                }

                _lock.Unlock();
            }
            void Notify(Core::ProxyType<PROXYELEMENT>& source)
            {
                _lock.Lock();

                typename ContainerMap::iterator index(_map.begin());

                // Find the element in the map..
                while ((index != _map.end()) && (index->second.first != source)) {
                    index++;
                }

                ASSERT(index != _map.end());

                if (index != _map.end()) {
                    index->second.second.Unlink();
                    _map.erase(index);
                }

                _lock.Unlock();
            }
            uint32_t Count() const {
                return (static_cast<uint32_t>(_map.size()));
            }

        private:
            ContainerMap _map;
            mutable Core::CriticalSection _lock;
        };

        template <typename PROXYELEMENT>
        class ProxyListType {
        private:
            using ContainerElement = ProxyContainerType< ProxyListType<PROXYELEMENT>, PROXYELEMENT, PROXYELEMENT>;
            using ContainerStorage = std::pair < Core::ProxyType<PROXYELEMENT>, UnlinkStorage>;
            using ContainerList = std::list< ContainerStorage >;

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
            Core::ProxyType<ACTUALOBJECT> Instance(Args&&... args)
            {
                using ActualElement = ProxyContainerType < ProxyListType<PROXYELEMENT>, ACTUALOBJECT, PROXYELEMENT>;

                Core::ProxyType<ACTUALOBJECT> result;

                Core::ProxyType<ActualElement> newItem;
                Core::ProxyType<ActualElement>::template CreateMove(newItem, 0, *this, std::forward<Args>(args)...);

                if (newItem.IsValid() == true) {

                    UnlinkStorage linkInfo(newItem->Handler());

                    // Make sure the return value is already "accounted" for otherwise the copy of the
                    // element into the map will trigger the "last" on list reference.
                    result = Core::ProxyType<ACTUALOBJECT>(std::move(newItem));

                    _lock.Lock();

                    _list.emplace_back(ContainerStorage(result, linkInfo));

                    _lock.Unlock();
                }

                return (result);
            }

            void Clear()
            {
                _lock.Lock();
                for (ContainerStorage& entry : _list) {
                    entry.second.Unlink();
                }
                _list.clear();
                _lock.Unlock();
            }
            void Notify(Core::ProxyType<ContainerElement>& source)
            {
                _lock.Lock();

                typename ContainerList::iterator index = _list.begin();

                while ((index != _list.end()) && (index->first != source)) {
                    index++;
                }

                ASSERT(index != _list.end());

                if (index != _list.end()) {
                    index->second.Unlink();
                    _list.erase(index);
                }

                _lock.Unlock();
            }
            void Notify(Core::ProxyType<PROXYELEMENT>& source)
            {
                _lock.Lock();

                typename ContainerList::iterator index = _list.begin();

                while ((index != _list.end()) && (index->first != source)) {
                    index++;
                }

                ASSERT(index != _list.end());

                if (index != _list.end()) {
                    index->second.Unlink();
                    _list.erase(index);
                }

                _lock.Unlock();
            }
            uint32_t Count() const {
                return (static_cast<uint32_t>(_list.size()));
            }

        private:
            mutable Core::CriticalSection _lock;
            ContainerList _list;
        };
    }
} // namespace Core

#endif // __INFRAPROXY_H

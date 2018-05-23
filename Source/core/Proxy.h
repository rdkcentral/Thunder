// ===========================================================================
//
// Filename:    Proxy.h
//
// Description: Header file for the Posix thread functions. This class
//              encapsulates all posix thread functionality defined by the
//              system.
//
// History
//
// Author        Reason                                             Date
// ---------------------------------------------------------------------------
// P. Wielders   Initial creation                                   2002/05/24
//
// ===========================================================================

#ifndef __PROXY_H
#define __PROXY_H

// ---- Include system wide include files ----
#include <map>

// ---- Include local include files ----
#include "Sync.h"
#include "StateTrigger.h"
#include "TypeTraits.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Core {

    template <typename CONTEXT>
    class ProxyObject : public CONTEXT, public IReferenceCounted {
    private:
        // ----------------------------------------------------------------
        // Never, ever allow reference counted objects to be assigned.
        // Create a new object and modify it. If the assignement operator
        // is used, give a compile error.
        // ----------------------------------------------------------------
        ProxyObject<CONTEXT>& operator=(const ProxyObject<CONTEXT>& rhs) = delete;

	protected:
		ProxyObject(CONTEXT& copy)
			: CONTEXT(copy)
			, m_RefCount(0) {
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

			// 32 bit align..
			size_t alignedSize = ((stAllocateBlock + 3) & (static_cast<size_t>(~0) ^ 0x3));

			if (AdditionalSize != 0) {
				Space = reinterpret_cast<uint8_t*>(::malloc(alignedSize + sizeof(uint32_t) + AdditionalSize));

				if (Space != nullptr) {
					*(reinterpret_cast<uint32_t*>(&Space[alignedSize])) = AdditionalSize;
				}
			}
			else {
				// If we do not have an addtional buffer, it has to be "empty". The elements m_Size and m_Buffer will not be used !!!
				ASSERT(AdditionalSize == 0);

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
        inline ProxyObject()
            : CONTEXT()
            , m_RefCount(0)
        {
			__Initialize<CONTEXT>();
		}
        template <typename arg1>
        inline ProxyObject(arg1 a_Arg1)
            : CONTEXT(a_Arg1)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}
        template <typename arg1, typename arg2>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2)
            : CONTEXT(a_Arg1, a_Arg2)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}
        template <typename arg1, typename arg2, typename arg3>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}
        template <typename arg1, typename arg2, typename arg3, typename arg4>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}
        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6, arg7 a_Arg7)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6, a_Arg7)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7, typename arg8>
        inline ProxyObject(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6, arg7 a_Arg7, arg8 a_Arg8)
            : CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6, a_Arg7, a_Arg8)
            , m_RefCount(0)
		{
			__Initialize<CONTEXT>();
		}

        virtual ~ProxyObject()
        {
			__Deinitialize<CONTEXT>();

			ASSERT(m_RefCount == 0);

            TRACE_L5("Destructor ProxyObject <0x%X>", TRACE_POINTER(this));
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

        inline bool LastRef() const
        {
            return (m_RefCount == 1);
        }

		inline void CompositRelease() {
			// This release is intended for objects that Composit ProxyObject<> objects as a composition part.
			// At the moment these go out of scope, this CompositRelease has to be called. It is assuming the 
			// last release but will not delete this object as that the the responsibility of the object that 
			// has this object as a composit.
			Core::InterlockedDecrement(m_RefCount);
		}

		// -----------------------------------------------------
		// Check for Initialize method on Object
		// -----------------------------------------------------
		HAS_MEMBER(Initialize, hasInitialize);

		typedef hasInitialize<CONTEXT, void (CONTEXT::*)()> TraitInitialize;

		template <typename TYPE>
		inline typename Core::TypeTraits::enable_if<ProxyObject<TYPE>::TraitInitialize::value, void>::type
			__Initialize()
		{
			CONTEXT::Initialize();
		}

		template <typename TYPE>
		inline typename Core::TypeTraits::enable_if<!ProxyObject<TYPE>::TraitInitialize::value, void>::type
			__Initialize()
		{
		}

		// -----------------------------------------------------
		// Check for Deinitialize method on Object
		// -----------------------------------------------------
		HAS_MEMBER(Deinitialize, hasDeinitialize);

		typedef hasDeinitialize<CONTEXT, void (CONTEXT::*)()> TraitDeinitialize;

		template <typename TYPE>
		inline typename Core::TypeTraits::enable_if<ProxyObject<TYPE>::TraitDeinitialize::value, void>::type
			__Deinitialize()
		{
			CONTEXT::Deinitialize();
		}

		template <typename TYPE>
		inline typename Core::TypeTraits::enable_if<!ProxyObject<TYPE>::TraitDeinitialize::value, void>::type
			__Deinitialize()
		{
		}

    protected:
        mutable uint32_t m_RefCount;
    };

    // ------------------------------------------------------------------------------
    // Reference counted object can only exist on heap (if reference count reaches 0)
    // delete is done. To avoid creation on the stack, this object can only be created
    // via the static create methods on this object.
    template <typename CONTEXT>
    class ProxyType {
	public:
		ProxyType() : m_RefCount(nullptr), _realObject(nullptr)
		{
		}
		explicit ProxyType(ProxyObject<CONTEXT>& theObject) : m_RefCount(&theObject), _realObject(&theObject)
		{
			m_RefCount->AddRef();
		}
		template<typename DERIVEDTYPE>
		ProxyType(const ProxyType<DERIVEDTYPE>& theObject) : m_RefCount(theObject), _realObject(nullptr)
		{
			if (m_RefCount != nullptr) {
				Construct<DERIVEDTYPE>(theObject, TemplateIntToType<Core::TypeTraits::same_or_inherits<CONTEXT, DERIVEDTYPE>::value>());
			}
		}
		explicit ProxyType(CONTEXT& realObject) : m_RefCount(const_cast<IReferenceCounted*>(dynamic_cast<const IReferenceCounted*>(&realObject))), _realObject(&realObject)
		{
			if (m_RefCount != nullptr) {
				m_RefCount->AddRef();
			}
		}
		explicit ProxyType(IReferenceCounted* refObject, CONTEXT* realObject) : m_RefCount(refObject), _realObject(realObject)
		{
			ASSERT((refObject == nullptr) || (realObject != nullptr));

			if (m_RefCount != nullptr) {
				m_RefCount->AddRef();
			}
		}
		ProxyType(const ProxyType<CONTEXT>& copy) : m_RefCount(copy.m_RefCount) , _realObject(copy._realObject)
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

        inline static ProxyType<CONTEXT> Create()
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>());
        }

        template <typename arg1>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1));
        }

        template <typename arg1, typename arg2>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1, arg2 a_Arg2)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1, a_Arg2));
        }

        template <typename arg1, typename arg2, typename arg3>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
        inline static ProxyType<CONTEXT> Create(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6)
        {
            return ProxyType<CONTEXT>(*new (0) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6));
        }

        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>());
        }

        template <typename arg1>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1));
        }

        template <typename arg1, typename arg2>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1, arg2 a_Arg2)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1, a_Arg2));
        }

        template <typename arg1, typename arg2, typename arg3>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5));
        }

        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
        inline static ProxyType<CONTEXT> CreateEx(const uint32_t size, arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6)
        {
            return ProxyType<CONTEXT>(*new (size) ProxyObject<CONTEXT>(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6));
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
		inline bool IsValid() const {
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

		inline operator IReferenceCounted* () const {
			return (m_RefCount);
		}

	private:
		template<typename DERIVED>
		inline void Construct(const ProxyType<DERIVED>& source, const TemplateIntToType<true>&) {
			_realObject = source.operator->();
			m_RefCount->AddRef();
		}
		template<typename DERIVED>
		inline void Construct(const ProxyType<DERIVED>& source, const TemplateIntToType<false>&) {
			CONTEXT* result(dynamic_cast<CONTEXT*>(source.operator->())); 
			
			// Althoug the constructor was called under the assumption that the object could be 
			// casted. It can *NOT* be casted, if we get a nullptr. Please fix your casting request
			// that caused this assert !!!
			ASSERT(result != nullptr);

			if (result == nullptr) {
				m_RefCount = nullptr;
			}
			else {
				_realObject = result;
				m_RefCount->AddRef();
			}
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

	template <typename CONTEXT>
	class ProxyStorage : public CONTEXT {
	private:
		// ----------------------------------------------------------------
		// Never, ever allow reference counted objects to be assigned.
		// Create a new object and modify it. If the assignement operator
		// is used, give a compile error.
		// ----------------------------------------------------------------
		ProxyStorage<CONTEXT>& operator=(const ProxyStorage<CONTEXT>& rhs) = delete;

		typedef ProxyStorage<CONTEXT> ProxyElement;

	public:
		ProxyStorage(const ProxyStorage<CONTEXT>& copy)
			: CONTEXT(copy) {
		}
		ProxyStorage()
			: CONTEXT() {
		}
		template <typename arg1>
		inline ProxyStorage(arg1 a_Arg1)
			: CONTEXT(a_Arg1)
		{
		}
		template <typename arg1, typename arg2>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2)
			: CONTEXT(a_Arg1, a_Arg2)
		{
		}
		template <typename arg1, typename arg2, typename arg3>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3)
		{
		}
		template <typename arg1, typename arg2, typename arg3, typename arg4>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4)
		{
		}
		template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5)
		{
		}
		template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6)
		{
		}
		template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6, arg7 a_Arg7)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6, a_Arg7)
		{
		}
		template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7, typename arg8>
		inline ProxyStorage(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6, arg7 a_Arg7, arg8 a_Arg8)
			: CONTEXT(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6, a_Arg7, a_Arg8)
		{
		}
		virtual ~ProxyStorage() {
		}

	public:
		inline uint8_t* Buffer()
		{
			return (&(AllocatedBuffer()[sizeof(uint32_t)]));
		}
		inline const uint8_t* Buffer() const
		{
			return (&(AllocatedBuffer()[sizeof(uint32_t)]));
		}
		inline uint32_t Size() const
		{
			return (*reinterpret_cast<const uint32_t*>(AllocatedBuffer()));
		}
		inline Core::ProxyType<ProxyElement> Clone(const uint32_t size)
		{
			return (InternalClone(size, TemplateIntToType<std::is_copy_constructible<CONTEXT>::value>()));
		}
		inline uint8_t& operator[](const uint32_t index)
		{
			ASSERT(index < Size());

			return (AllocatedBuffer()[sizeof(uint32_t) + index]);
		}
		inline const uint8_t& operator[](const uint32_t index) const
		{
			ASSERT(index < Size());

			return (AllocatedBuffer()[sizeof(uint32_t) + index]);
		}

	private:
		inline ProxyType<ProxyElement> InternalClone(const uint32_t size, const TemplateIntToType<true>&) {
			ProxyObject<ProxyElement>* newElement(new (size) ProxyObject<ProxyElement>(static_cast<const CONTEXT&>(*this)));
			const uint32_t copySize(size >= Size() ? static_cast<uint32_t>(Size()) : size);
			if (copySize > 0) {
				::memcpy(newElement->Buffer(), AllocatedBuffer(), copySize);
			}
			return (ProxyType<ProxyElement>(newElement, newElement));
		}
		inline Core::ProxyType<ProxyElement> InternalClone(const uint32_t size, const TemplateIntToType<false>&) {
			return (Core::ProxyType<ProxyElement>());
		}
		inline uint8_t* AllocatedBuffer()
		{
			return (&(reinterpret_cast<uint8_t*>(this)[((sizeof(ProxyObject<ProxyElement>) + 3) & ((~0) ^ 0x3))]));
		}
		inline const uint8_t* AllocatedBuffer() const
		{
			return (&(reinterpret_cast<const uint8_t*>(this)[((sizeof(ProxyObject<ProxyElement>) + 3) & ((~0) ^ 0x3))]));
		}
	};

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
                        TRACE_L1 (_T("Possible memory leak detected on object queue [%d]"), l_Teller);
                    }
                }
#endif

                delete[] m_List;
            }
        }

        unsigned int Find(const ProxyType<CONTEXT>& a_Entry)
        {
            // Remeber the item on the location.
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
                IReferenceCounted** l_NewList = new IReferenceCounted*[(m_Max << 1)];

                // Copy the old list in (Dirty but quick !!!!)
                memcpy(l_NewList, &m_List[0], (m_Max * sizeof(IReferenceCounted*)));

                // Update the capcacity counter.
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
                memcpy(&(m_List[a_Index]), &(m_List[a_Index + 1]), (m_Current - a_Index) * sizeof(IReferenceCounted*));
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
                memcpy(&(m_List[a_Index]), &(m_List[a_Index + 1]), (m_Current - a_Index) * sizeof(IReferenceCounted*));
            }

#ifdef __DEBUG__
            m_List[m_Current] = nullptr;
#endif
        }

        bool Remove(const ProxyType<CONTEXT>& a_Entry)
        {
            ASSERT(a_Entry.IsValid() != false);
            ASSERT(m_List != nullptr);

            // Remeber the item on the location.
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

                    // Relinguish our reference to this element.
                    m_List[l_Teller]->Release();
                }

                // If it is not the last one, we have to move...
                if ((a_Start + a_Count) < m_Current) {
                    // Kill the entry, (again dirty but quick).
                    memcpy(&(m_List[a_Start]), &(m_List[a_Start + a_Count + 1]), (a_Count * sizeof(IReferenceCounted*)));

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
			CONTEXT* baseClass = dynamic_cast<CONTEXT*> (m_List[Index]);
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
        // Protected Attributubes
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
        // following statments.
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

            TRACE_L5("Constructor ProxyQueue <0x%X>", TRACE_POINTER(this));
        }

        ~ProxyQueue()
        {
            TRACE_L5("Destructor ProxyQueue <0x%X>", TRACE_POINTER(this));

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
                }
                else {
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
                }
                else {
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

    template <typename PROXYPOOLELEMENT>
    class ProxyPoolType {
    private:
        template <typename ELEMENT>
        class ProxyObjectType : public Core::ProxyObject<ELEMENT> {
        private:
            typedef ProxyObjectType<ELEMENT> ThisClass;

            ProxyObjectType() = delete;
            ProxyObjectType(const ProxyObjectType<ELEMENT>&) = delete;
            ProxyObjectType<ELEMENT>& operator=(const ProxyObjectType<ELEMENT>&) = delete;

            ProxyObjectType(ProxyPoolType<ELEMENT>* queue)
                : Core::ProxyObject<ELEMENT>()
                , _queue(*queue)
            {
                ASSERT(queue != nullptr);
            }
            template <typename Arg1>
            ProxyObjectType(ProxyPoolType<ELEMENT>* queue, Arg1 a_Arg1)
                : Core::ProxyObject<ELEMENT>(a_Arg1)
                , _queue(*queue)
            {
                ASSERT(queue != nullptr);
            }

        public:
            ~ProxyObjectType()
            {
				this->__Deinitialize<PROXYPOOLELEMENT>();
            }
            inline static Core::ProxyType<ELEMENT> Create(ProxyPoolType<ELEMENT>& queue)
            {
				ThisClass* newElement(new (0) ThisClass(&queue));
				newElement->__Initialize<PROXYPOOLELEMENT>();
				return Core::ProxyType<ELEMENT>(static_cast<IReferenceCounted*>(newElement), newElement);
            }
            template <typename Arg1>
            inline static Core::ProxyType<ELEMENT> Create(ProxyPoolType<ELEMENT>& queue, Arg1 argument)
            {
				ThisClass* newElement(new (0) ThisClass(&queue, argument));
				newElement->__Initialize<PROXYPOOLELEMENT>();
				return Core::ProxyType<ELEMENT>(static_cast<IReferenceCounted*>(newElement), newElement);
            }

        public:
            virtual uint32_t Release() const
            {
                uint32_t result;

                if ((result = Core::InterlockedDecrement(Core::ProxyObject<ELEMENT>::m_RefCount)) == 0) {

		ThisClass* baseElement(const_cast<ThisClass*>(this));

			baseElement->__Clear<PROXYPOOLELEMENT>();

                    Core::ProxyType<ThisClass> returnObject (static_cast<IReferenceCounted*>(baseElement), baseElement);


                    _queue.Return(returnObject);

                    return (Core::ERROR_DESTRUCTION_SUCCEEDED);
                }

                return (Core::ERROR_NONE);
            }

        private:
            // -----------------------------------------------------
            // Check for Clear method on Object
            // -----------------------------------------------------
            HAS_MEMBER(Clear, hasClear);

            typedef hasClear<ELEMENT, void (ELEMENT::*)()> TraitClear;

            template <typename TYPE>
            inline typename Core::TypeTraits::enable_if<ProxyObjectType<TYPE>::TraitClear::value, void>::type
            __Clear()
            {
                ELEMENT::Clear();
            }

            template <typename TYPE>
            inline typename Core::TypeTraits::enable_if<!ProxyObjectType<TYPE>::TraitClear::value, void>::type
            __Clear()
            {
            }

			// -----------------------------------------------------
			// Check for Initialize method on Object
			// -----------------------------------------------------
			HAS_MEMBER(Initialize, hasInitialize);

			typedef hasInitialize<ELEMENT, void (ELEMENT::*)()> TraitInitialize;

			template <typename TYPE>
			inline typename Core::TypeTraits::enable_if<ProxyObjectType<TYPE>::TraitInitialize::value, void>::type
				__Initialize()
			{
				ELEMENT::Initialize();
			}

			template <typename TYPE>
			inline typename Core::TypeTraits::enable_if<!ProxyObjectType<TYPE>::TraitInitialize::value, void>::type
				__Initialize()
			{
			}

			// -----------------------------------------------------
			// Check for Deinitialize method on Object
			// -----------------------------------------------------
			HAS_MEMBER(Deinitialize, hasDeinitialize);

			typedef hasDeinitialize<ELEMENT, void (ELEMENT::*)()> TraitDeinitialize;

			template <typename TYPE>
			inline typename Core::TypeTraits::enable_if<ProxyObjectType<TYPE>::TraitDeinitialize::value, void>::type
				__Deinitialize()
			{
				ELEMENT::Deinitialize();
			}

			template <typename TYPE>
			inline typename Core::TypeTraits::enable_if<!ProxyObjectType<TYPE>::TraitDeinitialize::value, void>::type
				__Deinitialize()
			{
			}

		private:
            ProxyPoolType<ELEMENT>& _queue;
        };

    private:
        typedef ProxyObjectType<PROXYPOOLELEMENT> ProxyPoolElement;
        ProxyPoolType(const ProxyPoolType<PROXYPOOLELEMENT>&);
        ProxyPoolType<PROXYPOOLELEMENT>& operator=(const ProxyPoolType<PROXYPOOLELEMENT>&);

    public:
        ProxyPoolType(const uint32_t initialQueueSize)
            : _createdElements(0)
            , _queue(initialQueueSize)
            , _lock()
        {
        }
        ~ProxyPoolType()
        {
        }

    public:
        Core::ProxyType<PROXYPOOLELEMENT> Element()
        {
            Core::ProxyType<PROXYPOOLELEMENT> result;

            _lock.Lock();

            if (_queue.Count() == 0) {

                _createdElements++;

                _lock.Unlock();

                result = ProxyPoolElement::Create(*this);

                // TRACE_L1("Created a new element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));
            }
            else {
                Core::ProxyType<ProxyPoolElement> listLoad;

                _queue.Remove(0, listLoad);

                _lock.Unlock();

                result = Core::proxy_cast<PROXYPOOLELEMENT>(listLoad);

                // TRACE_L1("Reused an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));
            }

            return (result);
        }
        template <typename Arg1>
        Core::ProxyType<PROXYPOOLELEMENT> Element(Arg1 argument1)
        {
            Core::ProxyType<PROXYPOOLELEMENT> result;

            _lock.Lock();

            if (_queue.Count() == 0) {

                _createdElements++;

                _lock.Unlock();

                result = ProxyPoolElement::Create(*this, argument1);

                // TRACE_L1("Created a new element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));
            }
            else {
                Core::ProxyType<ProxyPoolElement> listLoad;

                _queue.Remove(0, listLoad);

                _lock.Unlock();

                result = Core::proxy_cast<PROXYPOOLELEMENT>(listLoad);

                // TRACE_L1("Reused an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*result));
            }

            return (result);
        }
        void Return(Core::ProxyType<ProxyPoolElement>& element) const
        {
            _lock.Lock();
            // TRACE_L1("Returned an element for: %s [%p]\n", typeid(PROXYPOOLELEMENT).name(), &static_cast<PROXYPOOLELEMENT&>(*element));
            _queue.Add(element);
            _lock.Unlock();
        }
        inline uint32_t CreatedElements() const
        {
            return (_createdElements);
        }
        inline uint32_t QueuedElements() const
        {
            return (_queue.Count());
        }
        inline uint32_t CurrentQueueSize() const
        {
            return (_queue.CurrentQueueSize());
        }

    private:
        uint32_t _createdElements;
        mutable Core::ProxyList<ProxyPoolElement> _queue;
        mutable Core::CriticalSection _lock;
    };

    template <typename PROXYKEY, typename PROXYELEMENT>
    class ProxyMapType {
    private:
        template <typename KEY, typename ELEMENT>
        class ProxyObjectType : public Core::ProxyObject<ELEMENT> {
        private:
            typedef ProxyObjectType<KEY, ELEMENT> ThisClass;

            ProxyObjectType() = delete;
            ProxyObjectType(const ProxyObjectType<KEY, ELEMENT>&) = delete;
            ProxyObjectType<KEY, ELEMENT>& operator=(const ProxyObjectType<KEY, ELEMENT>&) = delete;

        public:
            ProxyObjectType(ProxyMapType<KEY, ELEMENT>* parent, const KEY& key)
                : Core::ProxyObject<ELEMENT>(key)
                , _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            template <typename Arg1>
            ProxyObjectType(ProxyMapType<KEY, ELEMENT>* parent, const KEY& key, Arg1 a_Arg1)
                : Core::ProxyObject<ELEMENT>(key, a_Arg1)
                , _parent(*parent)
            {
                ASSERT(parent != nullptr);
            }
            ~ProxyObjectType()
            {
            }

        public:
            virtual uint32_t Release() const
            {
                uint32_t result = Core::InterlockedDecrement(Core::ProxyObject<ELEMENT>::m_RefCount);

                if (result == 1) {
                    // The Map is the only one still holding this proxy. Kill it....
                    _parent.RemoveObject(*this);
                }
                else if (result == 0) {
                    delete this;

                    return (Core::ERROR_DESTRUCTION_SUCCEEDED);
                }

                return (Core::ERROR_NONE);
            }

        private:
            ProxyMapType<KEY, ELEMENT>& _parent;
        };

    private:
        typedef ProxyObjectType<PROXYKEY, PROXYELEMENT> ProxyMapElement;
        ProxyMapType(const ProxyMapType<PROXYKEY, PROXYELEMENT>&);
        ProxyMapType<PROXYKEY, PROXYELEMENT>& operator=(const ProxyMapType<PROXYKEY, PROXYELEMENT>&);

    public:
        ProxyMapType()
            : _map()
            , _lock()
        {
        }
        ~ProxyMapType()
        {
        }

    public:
        Core::ProxyType<PROXYELEMENT> Create(const PROXYKEY& key)
        {
            Core::ProxyType<PROXYELEMENT> result;

            _lock.Lock();

            typename std::map<PROXYKEY, Core::ProxyType<ProxyMapElement> >::iterator index(_map.find(key));

            if (index == _map.end()) {
				// Oops we do not have such an element, create it...
				ProxyObjectType<PROXYKEY, PROXYELEMENT>* newItem(new (0) ProxyObjectType<PROXYKEY, PROXYELEMENT>(this, key));

                Core::ProxyType<ProxyMapElement> newElement(newItem, newItem);

                // Make sure the return value is already "accounted" for otherwise the copy of the
                // element into the map will trigger the "last" on map reference.
                result = proxy_cast<PROXYELEMENT>(newElement);

                _map.insert(std::pair<PROXYKEY, Core::ProxyType<ProxyMapElement> >(key, newElement));
            }
            else {
                result = proxy_cast<PROXYELEMENT>(index->second);
            }

            _lock.Unlock();

            return (result);
        }
    private:
        void RemoveObject(const ProxyMapElement& element) const
        {
            _lock.Lock();

            // See if we did not hand it out before we reached the lock
            if (element.LastRef() == true) {
                typename std::map<PROXYKEY, ProxyType<ProxyMapElement> >::iterator index(_map.begin());

                // Find the element in the map..
                while ((index != _map.end()) && (index->second != element)) {
                    index++;
                }

                ASSERT(index != _map.end());

                if (index != _map.end()) {
                    _map.erase(index);
                }
            }

            _lock.Unlock();
        }

    private:
        mutable std::map<PROXYKEY, ProxyType<ProxyMapElement> > _map;
        mutable Core::CriticalSection _lock;
    };
}
} // namespace Core

#endif // __INFRAPROXY_H

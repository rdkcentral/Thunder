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
 
#ifndef __SINGLETON_H
#define __SINGLETON_H

// ---- Include system wide include files ----
#include <atomic>
#include <list>

// ---- Include local include files ----
#include "Portability.h"
#include "Sync.h"
#include "TextFragment.h"
#include "Proxy.h"

// ---- Referenced classes and types ----

// ---- Helper functions ----
namespace Thunder {
namespace Core {
    class EXTERNAL Singleton {
    private:
        class EXTERNAL SingletonList {
        public:
            SingletonList(SingletonList&&) = delete;
            SingletonList(const SingletonList&) = delete;
            SingletonList& operator=(SingletonList&&) = delete;
            SingletonList& operator=(const SingletonList&) = delete;

        public:
            SingletonList();
            ~SingletonList();

            void Register(Singleton* singleton);
            void Unregister(Singleton* singleton);
            void Dispose();

        private:
            std::list<Singleton*> m_Singletons;
        };

    public:
        Singleton(Singleton&&) = delete;
        Singleton(const Singleton&) = delete;
        Singleton& operator=(Singleton&&) = delete;
        Singleton& operator=(const Singleton&) = delete;

    public:
        inline Singleton(void** realDeal) : _realDeal(realDeal)
        {
        }

        virtual ~Singleton()
        {
            if (_realDeal != nullptr) {
                (*_realDeal) = nullptr;
            }
        }

        inline static void Dispose()
        {
            ListInstance().Dispose();
        }
        virtual string ImplementationName() const = 0;

    protected:
        static SingletonList& ListInstance();
        
    private:
        void** _realDeal;
    };

    template <class SINGLETON>
    class SingletonType : public Singleton, public SINGLETON {
    private:
        using available = std::is_default_constructible<SINGLETON>;

    protected:
        template <typename... Args>
        inline SingletonType(Args&&... args)
            : Singleton(nullptr)  // g_TypedSingleton is now std::atomic; zeroed in ~SingletonType
            , SINGLETON(std::forward<Args>(args)...)
        {
            ListInstance().Register(this);
            TRACE_L1("Singleton constructing %s", ClassNameOnly(typeid(SINGLETON).name()).Text().c_str());
        }

    public:
        SingletonType(const SingletonType<SINGLETON>&) = delete;
        SingletonType<SINGLETON> operator=(const SingletonType<SINGLETON>&) = delete;

        virtual ~SingletonType()
        {
           ListInstance().Unregister(this);
           // ASSERT(g_TypedSingleton.load(std::memory_order_relaxed) != nullptr);
           g_TypedSingleton.store(nullptr, std::memory_order_relaxed);
        }
    public:
        virtual string ImplementationName() const
        {
            return (ClassNameOnly(typeid(SINGLETON).name()).Text());
        }

        template <typename... Args>
        DEPRECATED inline static SINGLETON& Instance(Args&&... args)
        {
            static CriticalSection g_AdminLock;

            SINGLETON* ptr = g_TypedSingleton.load(std::memory_order_acquire);
            if (ptr == nullptr) {
                g_AdminLock.Lock();
                ptr = g_TypedSingleton.load(std::memory_order_relaxed);
                if (ptr == nullptr) {
                    ptr = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(std::forward<Args>(args)...));
                    g_TypedSingleton.store(ptr, std::memory_order_release);
                }
                g_AdminLock.Unlock();
            }

            ASSERT(ptr != nullptr);

            return *ptr;
        }

        inline static SINGLETON& Instance() {
            // As available does not see through friend clas/protected
            // declarations, we can not rely on the output of it.
            // If this Instance method id called, assume it has a
            // default constructor..
            return (GetObject(TemplateIntToType<true>()));
        }

        // The Create() and Dispose() methods should only be used if the lifetime of 
        // the singleton is well defined. Create() marks the begining. The first instance
        // of the singleton will only be retrieved after the Create() is completed. The
        // Dispose() marks the end of the singleton. This call can only be issued if 
        // the singleton is not being used anymore and will not be requested anymore
        // at the start of the call to Dispose()
        template <typename... Args>
        inline static void Create(Args&&... args)
        {
            ASSERT(g_TypedSingleton.load(std::memory_order_relaxed) == nullptr);

            if (g_TypedSingleton.load(std::memory_order_relaxed) == nullptr) {

                // Create a singleton — caller guarantees single-threaded context
                SINGLETON* ptr = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(std::forward<Args>(args)...));
                g_TypedSingleton.store(ptr, std::memory_order_release);
            }

            ASSERT(g_TypedSingleton.load(std::memory_order_relaxed) != nullptr);


        }

        inline static bool Dispose()
        {
            bool disposed = false;
            // Unprotected. Make sure the dispose is *ONLY* called
            // after all usage of the singleton is completed!!!
            SINGLETON* ptr = g_TypedSingleton.exchange(nullptr, std::memory_order_acq_rel);
            
            if (ptr != nullptr) {
                // Bypass the destructor's store — we already zeroed via exchange.
                delete ptr;
                disposed = true;
            }
            return disposed;
        }

    private:
        static SINGLETON& GetObject(const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            static CriticalSection g_AdminLock;

            SINGLETON* ptr = g_TypedSingleton.load(std::memory_order_acquire);
            if (ptr == nullptr) {
                g_AdminLock.Lock();
                ptr = g_TypedSingleton.load(std::memory_order_relaxed);
                if (ptr == nullptr) {
                    ptr = static_cast<SINGLETON*>(new SingletonType<SINGLETON>());
                    g_TypedSingleton.store(ptr, std::memory_order_release);
                }
                g_AdminLock.Unlock();
            }

            ASSERT(ptr != nullptr);

            return *ptr;
        }
        static SINGLETON& GetObject(const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            // If the Singleton needs to be constructed with a parameter, the Create() method
            // should have been called prior to the instance..
            SINGLETON* ptr = g_TypedSingleton.load(std::memory_order_acquire);
            ASSERT(ptr != nullptr);

            return *ptr;
        }

      private:
        static std::atomic<SINGLETON*> g_TypedSingleton;
    };

    template <typename SINGLETONTYPE>
    EXTERNAL_HIDDEN std::atomic<SINGLETONTYPE*> SingletonType<SINGLETONTYPE>::g_TypedSingleton{ nullptr };

    template <typename PROXYTYPE>
    class SingletonProxyType {
    private:
        friend class SingletonType<SingletonProxyType<PROXYTYPE>>;
        template <typename... Args>
        SingletonProxyType(Args&&... args)
            : _wrapped(ProxyType<PROXYTYPE>::Create(std::forward<Args>(args)...))
        {
        }
        ~SingletonProxyType()
        {
            ASSERT(ProxyType<PROXYTYPE>::LastRefAccessor::LastRef(_wrapped) == true);
        }

    public:
        SingletonProxyType(const SingletonProxyType<PROXYTYPE>&) = delete;
        SingletonProxyType& operator=(const SingletonProxyType<PROXYTYPE>&) = delete;

        static ProxyType<PROXYTYPE>& Instance()
        {
            return (SingletonType<SingletonProxyType<PROXYTYPE>>::Instance()._wrapped);
        }
        template <typename... Args>
        DEPRECATED static ProxyType<PROXYTYPE>& Instance(Args&&... args)
        {
            return (SingletonType<SingletonProxyType<PROXYTYPE>>::Instance(std::forward<Args>(args)...)._wrapped);
        }

    private:
        ProxyType<PROXYTYPE> _wrapped;
    };

    template<typename SINGLETON>
    class UniqueType {
    private:
        using available = std::is_default_constructible<SINGLETON>;

        class Info {
        public:
            Info(Info&&) = delete;
            Info(const Info&) = delete;
            Info& operator=(Info&&) = delete;
            Info& operator=(const Info&) = delete;

            Info()
                : _adminLock()
                , _refCount(1)
                , _entry(nullptr) {
            }
            ~Info () = default;

        public:
            SINGLETON& Instance() {
                _adminLock.Lock();
                if (_entry == nullptr) {
                    _entry = new SINGLETON();
                }
                else {
                    ++_refCount;
                }
                _adminLock.Unlock();

                return (*_entry);
            }
            void Drop() {
                _adminLock.Lock();
                if (_refCount == 1) {
                    delete _entry;
                    _entry = nullptr;
                }
                else {
                    --_refCount;
                }
                _adminLock.Unlock();
            }

        public:
            CriticalSection _adminLock;
            uint32_t        _refCount;
            SINGLETON*      _entry;
        };

    public:
        UniqueType(UniqueType<SINGLETON>&&) = delete;
        UniqueType(const UniqueType<SINGLETON>&) = delete;
        UniqueType<SINGLETON> operator=(SingletonType<SINGLETON>&&) = delete;
        UniqueType<SINGLETON> operator=(const SingletonType<SINGLETON>&) = delete;

        UniqueType() = default;
        ~UniqueType() = default;

        SINGLETON& Instance() {
            return (_singleton.Instance());
        }
        void Drop() {
            return (_singleton.Drop());
        }

    private:
        static Info _singleton;
    };

    template <typename SINGLETONTYPE>
    EXTERNAL_HIDDEN typename UniqueType<SINGLETONTYPE>::Info  UniqueType<SINGLETONTYPE>::_singleton;

}
} // namespace Core

#endif // __SINGLETON_H

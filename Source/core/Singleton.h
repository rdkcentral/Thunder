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
            (*_realDeal) = nullptr;
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
            : Singleton(reinterpret_cast<void**>(&g_TypedSingleton))
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
           ASSERT(g_TypedSingleton != nullptr);
           g_TypedSingleton = nullptr;
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

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(std::forward<Args>(args)...));
                }

                g_AdminLock.Unlock();
            }

            ASSERT(g_TypedSingleton != nullptr);

            return *(g_TypedSingleton);
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
            ASSERT(g_TypedSingleton == nullptr);

            if (g_TypedSingleton == nullptr) {

                // Create a singleton
                g_TypedSingleton = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(std::forward<Args>(args)...));
            }

            ASSERT(g_TypedSingleton != nullptr);
        }
        inline static bool Dispose()
        {
            // Unprotected. Make sure the dispose is *ONLY* called
            // after all usage of the singlton is completed!!!
            bool disposed = false;
            if (g_TypedSingleton != nullptr) {

                delete g_TypedSingleton;
                // note destructor will set g_TypedSingleton to nullptr;
                disposed = true;
            }
            return disposed;
        }

    private:
        static SINGLETON& GetObject(const TemplateIntToType<true>& /* For compile time diffrentiation */)
        {
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = static_cast<SINGLETON*>(new SingletonType<SINGLETON>());
                }

                g_AdminLock.Unlock();
            }

            ASSERT(g_TypedSingleton != nullptr);

            return *(g_TypedSingleton);
        }
        static SINGLETON& GetObject(const TemplateIntToType<false>& /* For compile time diffrentiation */)
        {
            // If the Singleton needs to be constructed with a parameter, the Create() method
            // should have been called prior to the instance..
            ASSERT(g_TypedSingleton != nullptr);

            return *(g_TypedSingleton);
        }

      private:
        static SINGLETON* g_TypedSingleton;
    };

    template <typename SINGLETONTYPE>
    EXTERNAL_HIDDEN SINGLETONTYPE*  SingletonType<SINGLETONTYPE>::g_TypedSingleton = nullptr;

    template <typename PROXYTYPE>
    class SingletonProxyType {
    private:
        friend class SingletonType<SingletonProxyType<PROXYTYPE>>;
        template <typename... Args>
        SingletonProxyType(Args&&... args)
            : _wrapped(ProxyType<PROXYTYPE>::Create(std::forward<Args>(args)...))
        {
        }

    public:
        SingletonProxyType(const SingletonProxyType<PROXYTYPE>&) = delete;
        SingletonProxyType& operator=(const SingletonProxyType<PROXYTYPE>&) = delete;

        static ProxyType<PROXYTYPE> Instance()
        {
            return (SingletonType<SingletonProxyType<PROXYTYPE>>::Instance()._wrapped);
        }
        template <typename... Args>
        DEPRECATED static ProxyType<PROXYTYPE> Instance(Args&&... args)
        {
            return (SingletonType<SingletonProxyType<PROXYTYPE>>::Instance(std::forward<Args>(args)...)._wrapped);
        }

    private:
        ProxyType<PROXYTYPE> _wrapped;
    };
}
} // namespace Core

#endif // __SINGLETON_H

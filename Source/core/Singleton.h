 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

// ---- Referenced classes and types ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Core {
    class EXTERNAL Singleton {
    private:
        class EXTERNAL SingletonList {
        private:
            SingletonList(const SingletonList&);
            SingletonList& operator=(const SingletonList&);

        public:
            SingletonList();
            ~SingletonList();

            void Register(Singleton* singleton);
            void Unregister(Singleton* singleton);
            void Dispose();

        private:
            std::list<Singleton*> m_Singletons;
        };

    private:
        Singleton(const Singleton&);
        Singleton& operator=(const Singleton&);

    public:
        inline Singleton(void** realDeal) : _realDeal(realDeal)
        {
            ListInstance().Register(this);
        }

        virtual ~Singleton()
        {
            ListInstance().Unregister(this);
            (*_realDeal) = nullptr;
        }

        inline static void Dispose()
        {
            ListInstance().Dispose();
        }
        virtual string ImplementationName() const = 0;

    private:
        void** _realDeal;
        static SingletonList& ListInstance();
    };

    template <class SINGLETON>
    class SingletonType : public Singleton, public SINGLETON {
    private:
        SingletonType(const SingletonType<SINGLETON>&) = delete;
        SingletonType<SINGLETON> operator=(const SingletonType<SINGLETON>&) = delete;

    protected:
        SingletonType()
            : Singleton(reinterpret_cast<void**>(&g_TypedSingleton))
            , SINGLETON()
        {
        }
        template <typename ARG1>
        SingletonType(ARG1 arg1)
            : Singleton(reinterpret_cast<void**>(&g_TypedSingleton))
            , SINGLETON(arg1)
        {
        }
        template <typename ARG1, typename ARG2>
        SingletonType(ARG1 arg1, ARG2 arg2)
            : Singleton(reinterpret_cast<void**>(&g_TypedSingleton))
            , SINGLETON(arg1, arg2)
        {
        }

    public:
        virtual ~SingletonType()
        {
           ASSERT(g_TypedSingleton != nullptr);
        }

    public:
        virtual string ImplementationName() const
        {
            return (ClassNameOnly(typeid(SINGLETON).name()).Text());
        }

        static SINGLETON& Instance()
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

            return *(g_TypedSingleton);
        }
        template <typename ARG1>
        static SINGLETON& Instance(ARG1 arg1)
        {
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(arg1));
                }

                g_AdminLock.Unlock();
            } else {
                // You can not do a retrieval of a singleton with arguments !!!!
                ASSERT(false);
            }

            return *(g_TypedSingleton);
        }
        template <typename ARG1, typename ARG2>
        static SINGLETON& Instance(ARG1 arg1, ARG2 arg2)
        {
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = static_cast<SINGLETON*>(new SingletonType<SINGLETON>(arg1, arg2));
                }

                g_AdminLock.Unlock();
            } else {
                // You can not do a retrieval of a singleton with arguments !!!!
                ASSERT(false);
            }

            return *(g_TypedSingleton);
        }

      private:
        static SINGLETON* g_TypedSingleton;
    };

    template <typename SINGLETONTYPE>
    EXTERNAL_HIDDEN SINGLETONTYPE*  SingletonType<SINGLETONTYPE>::g_TypedSingleton = nullptr;
}
} // namespace Core

#endif // __SINGLETON_H

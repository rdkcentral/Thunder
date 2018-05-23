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
        inline Singleton()
        {
            ListInstance().Register(this);
        }

        virtual ~Singleton()
        {
            ListInstance().Unregister(this);
        }

        inline static void Dispose()
        {
            ListInstance().Dispose();
        }
        virtual string Name() const = 0;

    private:
        static SingletonList& ListInstance();
    };

    template <class SINGLETON>
    class SingletonType : public Singleton, public SINGLETON {
    private:
        SingletonType(const SingletonType<SINGLETON>&) = delete;
        SingletonType<SINGLETON> operator=(const SingletonType<SINGLETON>&) = delete;

    protected:
        SingletonType()
            : Singleton()
        {
        }
        template <typename ARG1>
        SingletonType(ARG1 arg1)
            : Singleton()
            , SINGLETON(arg1)
        {
        }
        template <typename ARG1, typename ARG2>
        SingletonType(ARG1 arg1, ARG2 arg2)
            : Singleton()
            , SINGLETON(arg1, arg2)
        {
        }

    public:
        virtual ~SingletonType()
        {
        }

    public:
        virtual string Name() const
        {
            return (ClassNameOnly(typeid(SINGLETON).name()).Text());
        }

        static SINGLETON& Instance()
        {
            static SingletonType<SINGLETON>* g_TypedSingleton = nullptr;
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = new SingletonType<SINGLETON>();
                }

                g_AdminLock.Unlock();
            }

            return *(static_cast<SINGLETON*>(g_TypedSingleton));
        }
        template <typename ARG1>
        static SINGLETON& Instance(ARG1 arg1)
        {
            static SingletonType<SINGLETON>* g_TypedSingleton = nullptr;
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = new SingletonType<SINGLETON>(arg1);
                }

                g_AdminLock.Unlock();
            }
            else {
                // You can not do a retrieval of a singleton with arguments !!!!
                ASSERT(false);
            }

            return *(static_cast<SINGLETON*>(g_TypedSingleton));
        }
        template <typename ARG1, typename ARG2>
        static SINGLETON& Instance(ARG1 arg1, ARG2 arg2)
        {
            static SingletonType<SINGLETON>* g_TypedSingleton = nullptr;
            static CriticalSection g_AdminLock;

            // Hmm Double Lock syndrom :-)
            if (g_TypedSingleton == nullptr) {
                g_AdminLock.Lock();

                if (g_TypedSingleton == nullptr) {
                    // Create a singleton
                    g_TypedSingleton = new SingletonType<SINGLETON>(arg1, arg2);
                }

                g_AdminLock.Unlock();
            }
            else {
                // You can not do a retrieval of a singleton with arguments !!!!
                ASSERT(false);
            }

            return *(static_cast<SINGLETON*>(g_TypedSingleton));
        }
    };
}
} // namespace Core

#endif // __SINGLETON_H

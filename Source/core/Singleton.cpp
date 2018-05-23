#include "Portability.h"
#include "Singleton.h"

namespace WPEFramework {
namespace Core {

    /* static */ Singleton::SingletonList& Singleton::ListInstance()
    {
        static SingletonList g_Singletons;

        return (g_Singletons);
    }

    Singleton::SingletonList::SingletonList()
    {
    }

    /* virtual */ Singleton::SingletonList::~SingletonList()
    {
        // Dispose was not called before main application was ended
        ASSERT(m_Singletons.empty() == true);
        Dispose();
    }

    void Singleton::SingletonList::Dispose()
    {
        // Iterate over all singletons and delete...
        while (m_Singletons.empty() == false) {
            Singleton* element(*(m_Singletons.begin()));
            TRACE_L1("Destructing %s", element->Name().c_str());
            delete element;
        }
        //// Iterate over all singletons and delete...
        //for(std::list<Singleton*>::const_iterator it = m_Singletons.begin(); it != m_Singletons.end(); ++it)
        //{
        //  delete *it;
        //}
        //m_Singletons.clear();
    }

    void Singleton::SingletonList::Register(Singleton* singleton)
    {
        m_Singletons.push_front(singleton);
    }

    void Singleton::SingletonList::Unregister(Singleton* singleton)
    {
        m_Singletons.remove(singleton);
    }
}
} // namespace Solution::Core

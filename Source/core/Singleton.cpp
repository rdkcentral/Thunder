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
 
#include "Singleton.h"
#include "Portability.h"

namespace Thunder {
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
        if (!m_Singletons.empty()) {
            TRACE_L1("%s !!! singleton list is not empty !!!", __FUNCTION__);
        }
        Dispose();
    }

    void Singleton::SingletonList::Dispose()
    {
        // Iterate over all singletons and delete...
        while (m_Singletons.empty() == false) {
            Singleton* element(*(m_Singletons.begin()));
            TRACE_L1("Singleton destructing %s", element->ImplementationName().c_str());
            delete element;
        }
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

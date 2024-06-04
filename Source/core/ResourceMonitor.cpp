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
 
#include "ResourceMonitor.h"
#include "Singleton.h"

namespace Thunder {

namespace Core {

    /* static */ ResourceMonitor& ResourceMonitor::Instance()
    {
        // Tests build/destroy the ResourceMonitor for each test. In production the
        // ResourceMonitor only gets built/destroyed once. The static variable "_instance"
        // makes this method about 5% faster. So here test code and production code are
        // different, but its impact has been thoroughly researched.
#ifdef BUILD_TESTS
        return SingletonType<ResourceMonitor>::Instance();
#else
        static ResourceMonitor& _instance = SingletonType<ResourceMonitor>::Instance();
        return (_instance);
#endif
    }
}
} // namespace Thunder::Core

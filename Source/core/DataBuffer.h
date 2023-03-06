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

#ifndef __DATABUFFER_H
#define __DATABUFFER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"
#include "Proxy.h"

namespace WPEFramework {
    namespace Core {
        // ---- Referenced classes and types ----

        // ---- Helper types and constants ----

        // ---- Helper functions ----
        template <const unsigned int BLOCKSIZE>
        class ScopedStorage {
        public:
            ScopedStorage(ScopedStorage<BLOCKSIZE>&&) = delete;
            ScopedStorage(const ScopedStorage<BLOCKSIZE>&) = delete;
            ScopedStorage<BLOCKSIZE>& operator=(const ScopedStorage<BLOCKSIZE>&) = delete;

            ScopedStorage() = default;
            ~ScopedStorage() = default;

        public:
            inline uint8_t* Buffer()
            {
                return &(m_Buffer[0]);
            }
            inline uint32_t Size() const
            {
                return (BLOCKSIZE);
            }

        private:
            uint8_t m_Buffer[BLOCKSIZE];
        };
    } // namespace Core
} //namespace WPEFramework

#endif // __DATABUFFER_H

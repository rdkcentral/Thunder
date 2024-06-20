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
 
#ifndef __RANDOM_H
#define __RANDOM_H

// ---- Include system wide include files ----
#include <time.h>

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace Thunder {
namespace Crypto {
    extern EXTERNAL void Reseed();
    extern EXTERNAL void Random(uint8_t& value);
    extern EXTERNAL void Random(uint16_t& value);
    extern EXTERNAL void Random(uint32_t& value);
    extern EXTERNAL void Random(uint64_t& value);

    inline void Random(int8_t& value)
    {
        Random(reinterpret_cast<uint8_t&>(value));
    }
    inline void Random(int16_t& value)
    {
        Random(reinterpret_cast<uint16_t&>(value));
    }
    inline void Random(int32_t& value)
    {
        Random(reinterpret_cast<uint32_t&>(value));
    }
    inline void Random(int64_t& value)
    {
        Random(reinterpret_cast<uint64_t&>(value));
    }

    template <typename DESTTYPE>
    void Random(DESTTYPE& value, const DESTTYPE minimum, const DESTTYPE maximum)
    {
        Crypto::Random(value);

        value = ((value % (maximum - minimum)) + minimum);
    }
}
}

#endif // __RANDOM_H

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
 
#include "SharedBuffer.h"

// TODO: remove if no longer needed for simple tracing.
#include <iostream>

namespace Thunder {

namespace Core {

    SharedBuffer::SharedBuffer(const TCHAR name[])
        : DataElementFile(name, File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administrationBuffer((string(name) + ".admin"), File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
        , _producer(false)
        , _consumer(false)
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)])))
    {
        Align<uint64_t>();
    }
    SharedBuffer::SharedBuffer(const TCHAR name[], const uint32_t mode, const uint32_t bufferSize, const uint16_t administratorSize)
        : DataElementFile(name, mode | File::SHAREABLE | File::CREATE, bufferSize)
        , _administrationBuffer((string(name) + ".admin"), mode | File::SHAREABLE | File::CREATE, administratorSize + sizeof(Administration) + (2 * sizeof(void*)) + 8 /* Align buffer on 64 bits boundary */)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
        , _producer(false)
        , _consumer(false)
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration)])))
    {

#ifndef __WINDOWS__
        memset(_administration, 0, sizeof(Administration));

        sem_init(&(_administration->_producer), 1, 1); /* Initial value is 1. */
        sem_init(&(_administration->_consumer), 1, 0); /* Initial value is 0. */
#endif
        Align<uint64_t>();
    }

    SharedBuffer::~SharedBuffer()
    {
    }
}
}

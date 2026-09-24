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

#if defined(__APPLE__)
// std::hash<string> is not guaranteed stable across platforms or compiler versions,
// but since producer and consumer are always built from the same binary this is safe.
static string HashedSemName(const string& name, const char* suffix) {
    size_t h = std::hash<string>{}(name);
    char buf[32];
    snprintf(buf, sizeof(buf), "/%013zx%s", h, suffix);
    return buf;
}
#endif

    SharedBuffer::SharedBuffer(const TCHAR name[])
        : DataElementFile(name, File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administrationBuffer((string(name) + ".admin"), File::USER_READ | File::USER_WRITE | File::SHAREABLE, 0)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
    #ifdef __WINDOWS__
        , _producer((string(name) + ".producer").c_str())
        , _consumer((string(name) + ".consumer").c_str())
    #elif defined(__APPLE__)
        /* 
           macOS limits POSIX named semaphore names to 31 characters. Hash the buffer name
           to produce a compact, collision-resistant identifier that always fits. 
        */
        , _producer(HashedSemName(name, ".producer").c_str())
        , _consumer(HashedSemName(name, ".consumer").c_str())
    #else
        , _producer(reinterpret_cast<uint8_t*>(_administration) + sizeof(Administration))
        , _consumer(reinterpret_cast<uint8_t*>(_administration) + sizeof(Administration) + SharedSemaphore::Size())
    #endif
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration) + (SharedSemaphore::Size() * 2)])))
    {
        Align<uint64_t>();
    }
    SharedBuffer::SharedBuffer(const TCHAR name[], const uint32_t mode, const uint32_t bufferSize, const uint16_t administratorSize)
        : DataElementFile(name, mode | File::SHAREABLE | File::CREATE, bufferSize)
        , _administrationBuffer((string(name) + ".admin"), mode | File::SHAREABLE | File::CREATE, sizeof(Administration) + (SharedSemaphore::Size() * 2) + administratorSize 
            + (sizeof(void*) * 2) + ((sizeof(Administration) + (SharedSemaphore::Size() * 2) + administratorSize) % sizeof(void*) == 0 ?
            0 : (sizeof(void*) - ((sizeof(Administration) + (SharedSemaphore::Size() * 2) + administratorSize) % sizeof(void*)))) /* Align buffer on 32/64 bits boundary */)
        , _administration(reinterpret_cast<Administration*>(PointerAlign(_administrationBuffer.Buffer())))
    #ifdef __WINDOWS__
        , _producer((string(name) + ".producer").c_str(), 1, 1)
        , _consumer((string(name) + ".consumer").c_str(), 0, 1)
    #elif defined(__APPLE__)
        /*
            macOS limits POSIX named semaphore names to 31 characters. Hash the buffer name
            to produce a compact, collision-resistant identifier that always fits. 
        */
        , _producer(HashedSemName(name, ".producer").c_str(), 1, 1)
        , _consumer(HashedSemName(name, ".consumer").c_str(), 0, 1)
    #else
        , _producer(reinterpret_cast<uint8_t*>(_administration) + sizeof(Administration), 1, 1)
        , _consumer(reinterpret_cast<uint8_t*>(_administration) + sizeof(Administration) + SharedSemaphore::Size(), 0, 1)
    #endif
        , _customerAdministration(PointerAlign(&(reinterpret_cast<uint8_t*>(_administration)[sizeof(Administration) + (SharedSemaphore::Size() * 2)])))
    {
        _administration->_bytesWritten = 0;
        Align<uint64_t>();
    }
}
}
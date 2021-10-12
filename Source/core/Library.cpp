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

#include "Library.h"
#include "Sync.h"
#include "Trace.h"

#ifdef __LINUX__
#include <fstream>
#endif


namespace Functions {
void* LoadFunction(void* handle, string& outError, const TCHAR functionName[])
{
    void* function = nullptr;

#ifdef __LINUX__
    ASSERT(handle != nullptr);

    dlerror(); /* clear error code */
    function = dlsym(handle, functionName);
    char* error = dlerror();
    if (error != nullptr) {
        outError = error;
        /* handle error, the symbol wasn't found */
        function = nullptr;
    }
#endif

#ifdef __WINDOWS__
    function = ::GetProcAddress(handle, functionName);

    if (function == nullptr) {
        outError = "Could not load funtion.";
    }
#endif

    return (function);
}
}

namespace WPEFramework {
namespace Core {

    Library::Library()
        : _refCountedHandle(nullptr)
        , _error()
    {
    }
    Library::Library(const TCHAR fileName[])
        : _refCountedHandle(nullptr)
        , _error()
    {
#ifdef __LINUX__
        void* handle = dlopen(fileName, RTLD_LAZY);
#endif
#ifdef __WINDOWS__
        HMODULE handle = ::LoadLibrary(fileName);
#endif

        if (handle != nullptr) {

            auto deleter = [](RefCountedHandle* ptr, ModuleUnload function) {
                ptr->Release(function);
                delete ptr;
            };
            ModuleUnload cleanupFunction = reinterpret_cast<ModuleUnload>(Functions::LoadFunction(handle, _error, _T("ModuleUnload")));


            // Seems we have an dynamic library opened..
            // bind cleanup function to a custom deleter
            _refCountedHandle.reset(new RefCountedHandle, std::bind(deleter, std::placeholders::_1, cleanupFunction));
            
            _refCountedHandle->_handle = handle;
            _refCountedHandle->_name = fileName;
            TRACE_L1("Loaded library: %s", fileName);
        } else {
#ifdef __LINUX__
            _error = dlerror();
            TRACE_L1("Failed to load library: %s, error %s", fileName, _error.c_str());
#endif
        }
    }
    Library::Library(const Library& copy)
        : _refCountedHandle(copy._refCountedHandle)
    {
    }
    Library::~Library()
    {
        Release();
    }

    Library& Library::operator=(const Library& RHS)
    {
        // Only do this if we have different libraries and not self assigning
        if(this == &RHS) { 
            return *this; 
        }
        if (RHS._refCountedHandle != _refCountedHandle) {
            Release();

            // Assigne the new handler
            _refCountedHandle = RHS._refCountedHandle;
            _error = RHS._error;

        }

        return (*this);
    }

    void* Library::LoadFunction(const TCHAR functionName[])
    {
        ASSERT(_refCountedHandle != nullptr);
        ASSERT(_refCountedHandle->_handle != nullptr);
        return Functions::LoadFunction(_refCountedHandle->_handle, _error, functionName);
    }

    uint32_t Library::Release()
    {
        _refCountedHandle.reset();
        return Core::ERROR_NONE;
    }

    void Library::RefCountedHandle::Release(Library::ModuleUnload cleanupFunction)
    {
        if (cleanupFunction != nullptr) {
            // Cleanup class
            cleanupFunction();
        }

#ifdef __LINUX__
        dlclose(_handle);
#endif
#ifdef __WINDOWS__
        ::FreeLibrary(_handle);
#endif
        TRACE_L1("Unloaded library: %s", _name.c_str());
    }
}
} // namespace Core

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
 
#include "Library.h"
#include "Sync.h"
#include "Trace.h"

#ifdef __LINUX__
#include <fstream>
#endif

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
            // Seems we have an dynamic library opened..
            _refCountedHandle = new RefCountedHandle;
            _refCountedHandle->_referenceCount = 1;
            _refCountedHandle->_handle = handle;
            _refCountedHandle->_name = fileName;
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
        AddRef();
    }
    Library::~Library()
    {
        Release();
    }

    Library& Library::operator=(const Library& RHS)
    {
        // Only do this if we have different libraries..
        if (RHS._refCountedHandle != _refCountedHandle) {
            Release();

            // Assigne the new handler
            _refCountedHandle = RHS._refCountedHandle;
            _error = RHS._error;

            AddRef();
        }

        return (*this);
    }

    void* Library::LoadFunction(const TCHAR functionName[])
    {
        void* function = nullptr;

        ASSERT(_refCountedHandle != nullptr);

#ifdef __LINUX__
        ASSERT(_refCountedHandle->_handle != nullptr);

        dlerror(); /* clear error code */
        function = dlsym(_refCountedHandle->_handle, functionName);
        char* error = dlerror();
        if (error != nullptr) {
            _error = error;
            /* handle error, the symbol wasn't found */
            function = nullptr;
        }
#endif

#ifdef __WINDOWS__
        function = ::GetProcAddress(_refCountedHandle->_handle, functionName);

        if (function == nullptr) {
            _error = "Could not load funtion.";
        }
#endif

        return (function);
    }

    void Library::AddRef()
    {
        // Reference count the new, if it exists..
        if (_refCountedHandle != nullptr) {
            Core::InterlockedIncrement(_refCountedHandle->_referenceCount);
        }
    }

    uint32_t Library::Release()
    {
        if (_refCountedHandle != nullptr) {
            if (_refCountedHandle->_referenceCount == 1) {

                ModuleUnload function = reinterpret_cast<ModuleUnload>(LoadFunction(_T("ModuleUnload")));

                if (function != nullptr) {
                    // Cleanup class
                    function();
                }

#ifdef __LINUX__
                dlclose(_refCountedHandle->_handle);
#endif
#ifdef __WINDOWS__
                ::FreeLibrary(_refCountedHandle->_handle);
#endif
                TRACE_L1("Unloaded library: %s", _refCountedHandle->_name.c_str());
            } else {
                Core::InterlockedDecrement(_refCountedHandle->_referenceCount);
            }
        }
        return (Core::ERROR_NONE);
    }
}
} // namespace Core

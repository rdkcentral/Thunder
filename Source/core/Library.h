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
 
#ifndef __LIBRARY_H
#define __LIBRARY_H

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    class EXTERNAL Library {
    private:
        typedef struct {
            uint32_t _referenceCount;
#ifdef __LINUX__
            void* _handle;
#endif
#ifdef __WINDOWS__
            HMODULE _handle;
#endif
            string _name;

        } RefCountedHandle;

        typedef void (*ModuleUnload)();

    public:
        Library();
        Library(const void* functionInLibrary);
        Library(const TCHAR fileName[]);
        Library(Library&& move);
        Library(const Library& copy);
        ~Library();

        Library& operator=(const Library& RHS);
        Library& operator=(Library&& move);

    public:
        inline bool IsLoaded() const
        {
            return (_refCountedHandle != nullptr);
        }
        inline const string& Error() const
        {
            return (_error);
        }
        inline const string& Name() const
        {
            return (_refCountedHandle != nullptr ? _refCountedHandle->_name : emptyString);
        }
        void* LoadFunction(const TCHAR functionName[]);

    private:
        void AddRef();

        friend class ServiceAdministrator;
        uint32_t Release();

    private:
        RefCountedHandle* _refCountedHandle;
        string _error;
    };
}
} // namespace Core

#endif

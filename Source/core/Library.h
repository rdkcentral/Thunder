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
#include "Trace.h"


#ifdef __WINDOWS__
#include <psapi.h>
#else
#include <link.h>
#endif

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
        class EXTERNAL Iterator {
        public:
            Iterator(Iterator&&) = delete;
            Iterator(const Iterator&) = delete;
            Iterator& operator=(Iterator&&) = delete;
            Iterator& operator=(const Iterator&) = delete;

            ~Iterator() = default;

#ifdef __WINDOWS__
            Iterator() : _current(0) {
            }

        public:
            void Reset() {
                _current = 0;
            }
            bool IsValid() const {
                return ((_current != 0) && (_current != static_cast<uint32_t>(~0))) ;
            }
            bool Next() {
                _current++;
                // See if we can find a name..
                if ((_current != static_cast<uint32_t>(~0)) && (LoadIndex(_current) == true)) {
                    return (true);
                }
                _current = static_cast<uint32_t>(~0);
                return (false);
            }
            Library Current() {
                ASSERT(IsValid() == true);
                return (Library(_filename.c_str()));
            }

        private:
            // Index is counted up from 1 up.. (0 is an illegal index as it means no handle)
            bool LoadIndex(const uint32_t index) {
                bool loaded (false);
                ASSERT (index > 0);
                HMODULE* handles = reinterpret_cast<HMODULE*>(ALLOCA(sizeof(HMODULE) * index));
                DWORD    needed;
                if (::EnumProcessModules(GetCurrentProcess(), handles, sizeof(HMODULE) * index, &needed)) {
                    if (index < (needed / sizeof(HANDLE))) { 
                        TCHAR moduleName[MAX_PATH];

                        if (::GetModuleFileNameEx(GetCurrentProcess(), handles[current], moduleName, sizeof(moduleName)/sizeof(TCHAR))) {
                            _current = moduleName;
                            loaded = true;
                        }
                    }
                }
                return (loaded);
            }

        private:
            uint32_t _current;
            string _filename;
#else
            Iterator() : _current(nullptr) {
            }

        public:
            void Reset() {
                _current = nullptr;
            }
            bool IsValid() const {
                return ((_current != nullptr) && (_current != reinterpret_cast<struct link_map*>(~0))) ;
            }
            bool Next() {
                if (_current == nullptr) {
                    _current = reinterpret_cast<const struct link_map*>(::dlopen(nullptr, RTLD_NOW));
                }
                else {
                    _current = _current->l_next;
                }
                if (_current == nullptr) {
                    _current = reinterpret_cast<struct link_map*>(~0);
                    return (false);
                }
                return (true);
            }
            Library Current() {
                ASSERT(IsValid() == true);
                return (Library(static_cast<const TCHAR*>(_current->l_name)));
            }

        private:
            const struct link_map* _current;
#endif
        };

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

        uint32_t WaitUnloaded(const uint32_t timeout = Core::infinite);

        inline const string& Error() const
        {
            return (_error);
        }
        inline const string& Name() const
        {
            return (_refCountedHandle != nullptr ? _refCountedHandle->_name : emptyString);
        }
        void* LoadFunction(const TCHAR functionName[]) const;

    private:
        void AddRef();

        friend class ServiceAdministrator;
        uint32_t Release();

    private:
        RefCountedHandle* _refCountedHandle;
        mutable string _error;
    };
}
} // namespace Core

#endif

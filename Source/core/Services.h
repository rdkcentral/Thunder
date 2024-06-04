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
 
#ifndef __SERVICES_H
#define __SERVICES_H

#include <list>

#include "Library.h"
#include "Module.h"
#include "Portability.h"
#include "Sync.h"
#include "TextFragment.h"
#include "Trace.h"
#include "Proxy.h"
#include "SystemInfo.h"

#include "WarningReportingControl.h"
#include "WarningReportingCategories.h"

namespace Thunder {
namespace Core {

    class EXTERNAL ServiceAdministrator {
    private:
        ServiceAdministrator();

        using Services = std::vector<IService*>;

    public:
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;

            virtual void Destructed() = 0;
        };

    public:
        ServiceAdministrator(ServiceAdministrator&&) = delete;
        ServiceAdministrator(const ServiceAdministrator&) = delete;
        ServiceAdministrator& operator=(ServiceAdministrator&&) = delete;
        ServiceAdministrator& operator=(const ServiceAdministrator&) = delete;
        virtual ~ServiceAdministrator();

        static ServiceAdministrator& Instance();

    public:
        Library LoadLibrary(const TCHAR libraryName[]) {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (Library(libraryName));
        }
        // There is *NO* locking around the _callback pointer. SO this callback 
        // must be set, before any Service Object is created or released!!!
        void Callback(ICallback* callback)
        {
            ASSERT((callback == nullptr) ^ (_callback == nullptr));
            _callback = callback;
        }
        uint32_t AddRef() const
        {
            Core::InterlockedIncrement(_instanceCount);
            return (Core::ERROR_COMPOSIT_OBJECT);
        }
        uint32_t Release() const
        {
            ASSERT(_instanceCount > 0);

            Core::InterlockedDecrement(_instanceCount);

            if (_callback != nullptr) {
                _callback->Destructed();
            }

            return (Core::ERROR_COMPOSIT_OBJECT);
        }
        inline uint32_t Instances() const
        {
            return (_instanceCount);
        }
        void FlushLibraries();
        void ReleaseLibrary(Library&& reference);
        void Announce(IService* service);
        void Revoke(IService* service);
        void* Instantiate(const Library& library, const char name[], const uint32_t version, const uint32_t interfaceNumber);

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Instantiate(const Library& library, const char name[], const uint32_t version)
        {
            void* baseInterface(Instantiate(library, name, version, REQUESTEDINTERFACE::ID));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }

    private:
        Core::CriticalSection _adminLock;
        Services _services;
        mutable uint32_t _instanceCount;
        ICallback* _callback;
        std::list<Library> _unreferencedLibraries;
        static ServiceAdministrator _systemServiceAdministrator;
    };

    template <typename ACTUALSINK>
    class SinkType : public ACTUALSINK {
    private:
        SinkType(SinkType<ACTUALSINK>&&) = delete;
        SinkType(const SinkType<ACTUALSINK>&) = delete;
        SinkType<ACTUALSINK> operator=(const SinkType<ACTUALSINK>&) = delete;

    public:
        template <typename... Args>
        SinkType(Args&&... args)
            : ACTUALSINK(std::forward<Args>(args)...)
            , _referenceCount(0)
        {
        }
        ~SinkType()
        {
            REPORT_OUTOFBOUNDS_WARNING(WarningReporting::SinkStillHasReference, _referenceCount);

            if (_referenceCount != 0) {
                // Since this is a Composit of a larger object, it could be that the reference count has
                // not reached 0. This can happen if a process that has a reference to this SinkType (e.g. 
                // a registered notification) crashed before it could unregister this notification. Due to
                // the failure of this unregistering and the composit not being recovered through the COMRPC
                // framework, the _referenceCount might be larger than 0.
                // No need to worry if it is caused by a crashing process as this sink gets destroyed by the 
                // owning object anyway (hence why you see this printf :-) ) but under normal conditions, 
                // this TRACE should *not* be visible!!! If you also see it in happy-day scenarios there 
                // is an unbalanced AddRef/Release in your code and you should take action !!!
                TRACE_L1("Oops this might be scary, destructing a (%s) sink that still is being refered by something", typeid(ACTUALSINK).name());
            }
        }

    public:
        virtual uint32_t AddRef() const
        {
            Core::InterlockedIncrement(_referenceCount);
            return (Core::ERROR_COMPOSIT_OBJECT);
        }
        virtual uint32_t Release() const
        {
            ASSERT (_referenceCount > 0);
            Core::InterlockedDecrement(_referenceCount);
            return (Core::ERROR_COMPOSIT_OBJECT);
        }

    private:
        mutable uint32_t _referenceCount;
    };

    template <typename ACTUALSERVICE>
    class ServiceType : public ACTUALSERVICE {
    protected:
        template<typename... Args>
        ServiceType(Args&&... args)
            : ACTUALSERVICE(std::forward<Args>(args)...)
            , _referenceLib()
        {
        }

    public:
        ServiceType(ServiceType<ACTUALSERVICE>&&) = delete;
        ServiceType(const ServiceType<ACTUALSERVICE>&) = delete;
        ServiceType<ACTUALSERVICE>& operator=(ServiceType<ACTUALSERVICE>&&) = delete;
        ServiceType<ACTUALSERVICE>& operator=(const ServiceType<ACTUALSERVICE>&) = delete;

        template <typename INTERFACE, typename... Args>
        static INTERFACE* Create(Args&&... args)
        {
            Core::ProxyType< ServiceType<ACTUALSERVICE> > object = Core::ProxyType< ServiceType<ACTUALSERVICE> >::Create(std::forward<Args>(args)...);

            ASSERT (object.IsValid() == true);

            TRACE_L1("Loading the reference library for: [%s] using [%s]", typeid(ACTUALSERVICE).name(), System::MODULE_NAME);
            object->_referenceLib = Library(&System::MODULE_NAME);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        ~ServiceType() override
        {
            // We can not use the default destructor here a that 
            // requires a destructor on the union member, but we 
            // do not want a union destructor, so lets create our
            // own!!!!
        }

        void LockLibrary(const Library& library) {
            ASSERT(_referenceLib.IsLoaded() == false);
            _referenceLib = library;
        }

    protected:
        // Destructed is a method called through SFINAE just before the memory 
        // associated with the object is freed from a Core::ProxyObject. If this
        // method is called, be aware that the destructor of the object has run
        // to completion!!!
        void Destructed() {
            ServiceAdministrator::Instance().ReleaseLibrary(std::move(_referenceLib));
        }
    private:
        template <typename INTERFACE>
        inline static INTERFACE* Extract(const Core::ProxyType< ServiceType<ACTUALSERVICE > >& object, const TemplateIntToType<false>&)
        {
            INTERFACE* result = reinterpret_cast<INTERFACE*>(object->QueryInterface(INTERFACE::ID));

            return (result);
        }
        template <typename INTERFACE>
        inline static INTERFACE* Extract(const Core::ProxyType< ServiceType<ACTUALSERVICE> >& object, const TemplateIntToType<true>&)
        {
            object->AddRef();
            return (object.operator->());
        }

        // The union here is used to avoid the destruction of the _referenceLib during
        // the destructor call. That is required to make sure that the whole object,
        // the actual service, is first fully destructed before we offer it to the
        // service destructor (done in the Destructed call). This avoids the unloading
        // of the referenced library before the object (part of this library) is fully 
        // destructed...
        union { Library _referenceLib; };
    };

    template <typename TYPE>
    using Sink = SinkType<TYPE>;

    template <typename TYPE>
    using Service = ServiceType<TYPE>;

    typedef DEPRECATED IService::IMetadata IServiceMetadata;

    template <typename ACTUALSERVICE>
    class PublishedServiceType : public IService {
    private:
        using Implementation = ServiceType<ACTUALSERVICE>;
 
        template <typename SERVICE>
        class Info : public IService::IMetadata {
        public:
            Info() = delete;
            Info(Info<SERVICE>&&) = delete;
            Info(const Info<SERVICE>&) = delete;
            Info<SERVICE>& operator=(Info<SERVICE>&&) = delete;
            Info<SERVICE>& operator=(const Info<SERVICE>&) = delete;

            Info(const TCHAR* moduleName, const uint8_t major, const uint8_t minor, const uint8_t patch)
                : _version((major << 16) | (minor << 8) | patch)
                , _Id(Core::ClassNameOnly(typeid(SERVICE).name()).Text())
                , _moduleName(moduleName) {
            }
            ~Info() override = default;

        public:
            uint8_t Major() const override {
                return (static_cast<uint8_t>((_version >> 16) & 0xFF));
            }
            uint8_t Minor() const override {
                return (static_cast<uint8_t>((_version >> 8) & 0xFF));
            }
            uint8_t Patch() const override {
                return (static_cast<uint8_t>(_version & 0xFF));
            }
            const TCHAR* ServiceName() const override
            {
                return (_Id.c_str());
            }
            const TCHAR* Module() const override
            {
                return (_moduleName);
            }

        private:
            uint32_t _version;
            string _Id;
            const TCHAR* _moduleName;
        };

    public:
        PublishedServiceType() = delete;
        PublishedServiceType(PublishedServiceType&&) = delete;
        PublishedServiceType(const PublishedServiceType&) = delete;
        PublishedServiceType& operator=(PublishedServiceType&&) = delete;
        PublishedServiceType& operator=(const PublishedServiceType&) = delete;

        PublishedServiceType(const TCHAR* moduleName, const uint8_t major, const uint8_t minor, const uint8_t patch)
            : _info(moduleName, major, minor, patch) {
            Core::ServiceAdministrator::Instance().Announce(this);
        }
        ~PublishedServiceType() {
            Core::ServiceAdministrator::Instance().Revoke(this);
        }

    public:
        void* Create(const Library& library, const uint32_t interfaceNumber) override {
            void* result = nullptr;
            Core::ProxyType< Implementation > object = Core::ProxyType< Implementation >::Create();

            if (object.IsValid() == true) {
                object->LockLibrary(library);
                // This query interface will increment the refcount of the Service at least to 1.
                result = object->QueryInterface(interfaceNumber);
            }
            return (result);
        }
        const IMetadata* Metadata() const override {
            return (&_info);
        }

    private:
        Info<ACTUALSERVICE> _info;
    };


#define SERVICE_REGISTRATION_NAME(N, ...) CONCAT_STRINGS(SERVICE_REGISTRATION_, N)(__VA_ARGS__)
#define SERVICE_REGISTRATION(...) SERVICE_REGISTRATION_NAME(PUSH_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)

#define SERVICE_REGISTRATION_3(ACTUALCLASS, MAJOR, MINOR) \
static Thunder::Core::PublishedServiceType<ACTUALCLASS> ServiceMetadata_##ACTUALCLASS(Thunder::Core::System::MODULE_NAME, MAJOR, MINOR, 0);

#define SERVICE_REGISTRATION_4(ACTUALCLASS, MAJOR, MINOR, PATCH) \
static Thunder::Core::PublishedServiceType<ACTUALCLASS> ServiceMetadata_##ACTUALCLASS(Thunder::Core::System::MODULE_NAME, MAJOR, MINOR, PATCH);


#ifdef BEGIN_INTERFACE_MAP
#undef BEGIN_INTERFACE_MAP
#endif
#ifdef INTERFACE_ENTRY
#undef INTERFACE_ENTRY
#endif
#ifdef INTERFACE_AGGREGATE
#undef INTERFACE_AGGREGATE
#endif
#ifdef INTERFACE_RELAY
#undef INTERFACE_RELAY
#endif
#ifdef END_INTERFACE_MAP
#undef END_INTERFACE_MAP
#endif

#define BEGIN_INTERFACE_MAP(ACTUALCLASS)                                     \
    void* QueryInterface(const uint32_t interfaceNumber) override            \
    {                                                                        \
        if (interfaceNumber == Thunder::Core::IUnknown::ID) {                         \
            AddRef();                                                        \
            return (static_cast<void*>(static_cast<Thunder::Core::IUnknown*>(this))); \
        }

#define INTERFACE_ENTRY(TYPE)                                  \
    else if (interfaceNumber == TYPE::ID)                      \
    {                                                          \
        AddRef();                                              \
        return (static_cast<void*>(static_cast<TYPE*>(this))); \
    }

#define INTERFACE_AGGREGATE(TYPE, AGGREGATE)              \
    else if (interfaceNumber == TYPE::ID)                 \
    {                                                     \
        if (AGGREGATE != nullptr) {                       \
            return (AGGREGATE->QueryInterface(TYPE::ID)); \
        }                                                 \
        return (nullptr);                                 \
    }


#define INTERFACE_RELAY(TYPE, RELAY)                               \
    else if (interfaceNumber == TYPE::ID) {                        \
        if (RELAY != nullptr) {                                    \
           AddRef();                                               \
           return (static_cast<void*>(static_cast<TYPE*>(this)));  \
        }                                                          \
        return (nullptr);                                          \
    }

#define NEXT_INTERFACE_MAP(BASECLASS)                             \
        return (BASECLASS::QueryInterface(interfaceNumber));      \
    }

#define END_INTERFACE_MAP                                         \
        return (nullptr);                                         \
    }

}
} // namespace Core

#endif // __SERVICES_H

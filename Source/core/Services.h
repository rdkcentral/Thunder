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

namespace WPEFramework {
namespace Core {

    struct EXTERNAL IServiceMetadata {
        virtual ~IServiceMetadata() = default;

        virtual const std::string& ServiceName() const = 0;
        virtual const TCHAR* Module() const = 0;
        virtual uint8_t Major() const = 0;
        virtual uint8_t Minor() const = 0;
        virtual uint8_t Patch() const = 0;
    };

    class EXTERNAL ServiceAdministrator {
    private:
        ServiceAdministrator();

    public:
        struct EXTERNAL IServiceFactory {
            virtual ~IServiceFactory() = default;

            virtual void* Create(const IServiceMetadata* info, const Library& library, const uint32_t interfaceNumber) = 0;
        };
        template<typename SERVICE>
        class ServiceFactoryType : public IServiceFactory {
        public:
            ServiceFactoryType(const ServiceFactoryType<SERVICE>&) = delete;
            ServiceFactoryType<SERVICE>& operator= (const ServiceFactoryType<SERVICE>&) = delete;

            ServiceFactoryType() = default;
            virtual ~ServiceFactoryType() = default;

        public:
            void* Create(const IServiceMetadata* info, const Library& library, const uint32_t interfaceNumber) override {
                void* result = nullptr;
                Core::ProxyType< SERVICE > object = Core::ProxyType< SERVICE >::Create(library, info);

                if (object.IsValid() == true) {
                    // This query interface will increment the refcount of the Service at least to 1.
                    result = object->QueryInterface(interfaceNumber);
                }
                return (result);
            }
        };
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;

            virtual void Destructed() = 0;
        };

    private:
        using ServiceBlock = std::pair<IServiceMetadata*, IServiceFactory*>;
        using ServiceList = std::list<ServiceBlock>;

    public:
        ServiceAdministrator(const ServiceAdministrator&) = delete;
        ServiceAdministrator& operator=(const ServiceAdministrator&) = delete;
        virtual ~ServiceAdministrator();

        static ServiceAdministrator& Instance();

    public:
        Library LoadLibrary(const TCHAR libraryName[]) {
            Core::SafeSyncType<Core::CriticalSection> lock(_adminLock);
            return (Library(libraryName));
        }
        // There is *NO* locking around the _callback pointer. SO this callback 
        // must be set, before any Service Object is created ore released!!!
        void Callback(ICallback* callback)
        {
            ASSERT((callback == nullptr) ^ (_callback == nullptr));
            _callback = callback;
        }
        void AddRef() const
        {
            Core::InterlockedIncrement(_instanceCount);
        }
        uint32_t Release() const
        {
            ASSERT(_instanceCount > 0);

            Core::InterlockedDecrement(_instanceCount);

            if (_callback != nullptr) {
                _callback->Destructed();
            }

            return (Core::ERROR_NONE);
        }
        inline uint32_t Instances() const
        {
            return (_instanceCount);
        }
        void FlushLibraries();
        void ReleaseLibrary(Library&& reference);
        void Register(IServiceMetadata* metadata, IServiceFactory* factory);
        void Unregister(IServiceMetadata* metadata, IServiceFactory* factory);
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
        ServiceList _services;
        mutable uint32_t _instanceCount;
        ICallback* _callback;
        std::list<Library> _unreferencedLibraries;
        static ServiceAdministrator _systemServiceAdministrator;
    };

    template <typename ACTUALSERVICE>
    class Service : public ACTUALSERVICE {
    protected:
        template<typename... Args>
        Service(Args&&... args)
            : ACTUALSERVICE(std::forward<Args>(args)...)
        {
            ServiceAdministrator::Instance().AddRef();
        }

    public:
        Service(const Service<ACTUALSERVICE>&) = delete;
        Service<ACTUALSERVICE>& operator=(const Service<ACTUALSERVICE>&) = delete;

        template <typename INTERFACE, typename... Args>
        static INTERFACE* Create(Args&&... args)
        {
            Core::ProxyType< Service<ACTUALSERVICE> > object = Core::ProxyType< Service<ACTUALSERVICE> >::Create(std::forward<Args>(args)...);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        ~Service() override
        {
            ServiceAdministrator::Instance().Release();
        }

    private:
        template <typename INTERFACE>
        inline static INTERFACE* Extract(const Core::ProxyType< Service<ACTUALSERVICE > >& object, const TemplateIntToType<false>&)
        {
            INTERFACE* result = reinterpret_cast<INTERFACE*>(object->QueryInterface(INTERFACE::ID));

            return (result);
        }
        template <typename INTERFACE>
        inline static INTERFACE* Extract(const Core::ProxyType< Service<ACTUALSERVICE> >& object, const TemplateIntToType<true>&)
        {
            object->AddRef();
            return (object.operator->());
        }
    };

    template <typename ACTUALSINK>
    class Sink : public ACTUALSINK {
    private:
        Sink(Sink<ACTUALSINK>&&) = delete;
        Sink(const Sink<ACTUALSINK>&) = delete;
        Sink<ACTUALSINK> operator=(const Sink<ACTUALSINK>&) = delete;

    public:
        template <typename... Args>
        Sink(Args&&... args)
            : ACTUALSINK(std::forward<Args>(args)...)
            , _referenceCount(0)
        {
        }
        ~Sink()
        {
            REPORT_OUTOFBOUNDS_WARNING(WarningReporting::SinkStillHasReference, _referenceCount);

            if (_referenceCount != 0) {
                // This is probably due to the fact that the "other" side killed the connection, we need to
                // Remove our selves at the COM Administrator map.. no need to signal Releases on behalf of the dropped connection anymore..
                TRACE_L1("Oops this is scary, destructing a (%s) sink that still is being refered by something", typeid(ACTUALSINK).name());
            }
        }

    public:
        virtual void AddRef() const
        {
            Core::InterlockedIncrement(_referenceCount);
        }
        virtual uint32_t Release() const
        {
            Core::InterlockedDecrement(_referenceCount);
            return (Core::ERROR_NONE);
        }

    private:
        mutable uint32_t _referenceCount;
    };

    // Baseclass to turn objects into services
    template <typename ACTUALSERVICE>
    class ServiceMetadata : public IServiceMetadata {
    private:
        ServiceMetadata() = delete;
        ServiceMetadata(const ServiceMetadata&) = delete;
        ServiceMetadata& operator=(const ServiceMetadata&) = delete;

        template <typename SERVICE>
        class ServiceImplementation : public Service<SERVICE>, public IServiceMetadata {
        public:
            ServiceImplementation() = delete;
            ServiceImplementation(const ServiceImplementation<SERVICE>&) = delete;
            ServiceImplementation<SERVICE>& operator=(const ServiceImplementation<SERVICE>&) = delete;

            ServiceImplementation(const Library& library, const IServiceMetadata* info)
                : Service<SERVICE>()
                , _referenceLib(library)
                , _info(info) {
            }
            ~ServiceImplementation() override {
            }

        public:
            uint8_t Major() const override {
                return (_info->Major());
            }
            uint8_t Minor() const override {
                return (_info->Minor());
            }
            uint8_t Patch() const override {
                return (_info->Patch());
            }
            const std::string& ServiceName() const override {
                return (_info->ServiceName());
            }
            const TCHAR* Module() const override {
                return (_info->Module());
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
            // The union here is used to avoid the destruction of the _referenceLib during
            // the destructor call. That is required to make sure that the whole object,
            // the actual service, is first fully destructed before we offer it to the
            // service destructor (done in the Destructed call). This avoids the unloading
            // of the refernced library before the object (part of this lirary) is fully 
            // destructed...
            union { Library _referenceLib; };
            const IServiceMetadata* _info;
        };

    public:
        ServiceMetadata(const uint8_t major, const uint8_t minor, const uint8_t patch)
            : _version((major << 16) | (minor << 8) | patch)
            , _Id(Core::ClassNameOnly(typeid(ACTUALSERVICE).name()).Text())
        {
            Core::ServiceAdministrator::Instance().Register(this, &_factory);
        }
        ~ServiceMetadata()
        {
            Core::ServiceAdministrator::Instance().Unregister(this, &_factory);
        }

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
        const std::string& ServiceName() const override
        {
            return (_Id);
        }
        const TCHAR* Module() const override
        {
            return (Core::System::MODULE_NAME);
        }

    private:
        uint32_t _version;
        string _Id;
        ServiceAdministrator::ServiceFactoryType< ServiceImplementation <ACTUALSERVICE> > _factory;
    };

#define SERVICE_REGISTRATION_NAME(N, ...) CONCAT_STRINGS(SERVICE_REGISTRATION_, N)(__VA_ARGS__)
#define SERVICE_REGISTRATION(...) SERVICE_REGISTRATION_NAME(PUSH_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)

#define SERVICE_REGISTRATION_3(ACTUALCLASS, MAJOR, MINOR) \
static WPEFramework::Core::ServiceMetadata<ACTUALCLASS> ServiceMetadata_##ACTUALCLASS(MAJOR, MINOR, 0);

#define SERVICE_REGISTRATION_4(ACTUALCLASS, MAJOR, MINOR, PATCH) \
static WPEFramework::Core::ServiceMetadata<ACTUALCLASS> ServiceMetadata_##ACTUALCLASS(MAJOR, MINOR, PATCH);


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
        if (interfaceNumber == WPEFramework::Core::IUnknown::ID) {                         \
            AddRef();                                                        \
            return (static_cast<void*>(static_cast<WPEFramework::Core::IUnknown*>(this))); \
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

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
#include "Thread.h"

#include "WarningReportingControl.h"
#include "WarningReportingCategories.h"

namespace Thunder {
namespace Core {

    class EXTERNAL ServiceAdministrator {
    private:
        ServiceAdministrator()
            : _adminLock()
            , _instanceCount(0)
            , _callback(nullptr)
            , _unreferencedLibraries() {
        }

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
        virtual ~ServiceAdministrator() = default;

        static ServiceAdministrator& Instance();
        static const IService* LibraryToService (const Library& library) {
            ASSERT(library.IsLoaded() == true);
            System::GetModuleServicesImpl service = reinterpret_cast<System::GetModuleServicesImpl>(library.LoadFunction(_T("GetModuleServices")));
            return (service != nullptr ? service() : nullptr);
        }

    public:
        inline void Created() {
            Core::InterlockedIncrement(_instanceCount);
        }
        inline void Dropped() {
            ASSERT(_instanceCount > 0);

            Core::InterlockedDecrement(_instanceCount);

            if (_callback != nullptr) {
                _callback->Destructed();
            }
        }
        inline uint32_t Instances() {
            return (_instanceCount);
        }
        // There is *NO* locking around the _callback pointer. SO this callback 
        // must be set, before any Service Object is created or released!!!
        void Callback(ICallback* callback)
        {
            ASSERT((callback == nullptr) ^ (_callback == nullptr));
            _callback = callback;
        }
        void FlushLibraries();
        void ReleaseLibrary(Library&& reference);
        void* Instantiate(const IService* service, const char name[], const uint32_t version, const uint32_t interfaceNumber);

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* Instantiate(const IService* service, const char name[], const uint32_t version)
        {
            void* baseInterface(Instantiate(service, name, version, REQUESTEDINTERFACE::ID));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }

    private:
        Core::CriticalSection _adminLock;
        uint32_t _instanceCount;
        ICallback* _callback;
        std::vector<Library> _unreferencedLibraries;
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

        uint32_t WaitReleased(const uint32_t timeout = Core::infinite)
        {
            uint32_t result = Core::ERROR_NONE;
            uint64_t now = Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond;
            uint8_t count = 0;
            while (_referenceCount > 0) {
                    Core::Thread::Yield(count, 100);
                    if (( timeout != Core::infinite ) && ( (Core::Time::Now().Ticks() / Core::Time::TicksPerMillisecond) - now > timeout )) {
                        result = Core::ERROR_TIMEDOUT;
                        break;
                    }
            };

            return result;
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
            , _referenceLib(&System::MODULE_NAME)
        {
            // In the past, we allowed that the _referenceLib was not loaded, for whatever reason
            // with the current setup, I think, it should always be loaded. During manual testing
            // I did not observe an issue (ASSERT never fires) in the automation tests it fires.
            // Lets first get this in, Raise a JIRAticket to look at the test and enable it, if
            // the test is fixed again...
            // ASSERT(_referenceLib.IsLoaded() == true);
            // TRACE_L1("Creating: [%s] in the Module: [%s] using Library: [%s]", typeid(ACTUALSERVICE).name(), System::MODULE_NAME, _referenceLib.Name().c_str());
            ServiceAdministrator::Instance().Created();
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

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        ~ServiceType() override
        {
            // We can not use the default destructor here a that 
            // requires a destructor on the union member, but we 
            // do not want a union destructor, so lets create our
            // own!!!!
            ServiceAdministrator::Instance().Dropped();
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

    template<typename SERVICE>
    class PublishedMetadataType : public IService::IMetadata {
    public:
        PublishedMetadataType() = delete;
        PublishedMetadataType(PublishedMetadataType<SERVICE>&&) = delete;
        PublishedMetadataType(const PublishedMetadataType<SERVICE>&) = delete;
        PublishedMetadataType<SERVICE>& operator=(PublishedMetadataType<SERVICE>&&) = delete;
        PublishedMetadataType<SERVICE>& operator=(const PublishedMetadataType<SERVICE>&) = delete;

        PublishedMetadataType(const TCHAR* moduleName, const uint8_t major, const uint8_t minor, const uint8_t patch)
            : _version((major << 16) | (minor << 8) | patch)
            , _Id(Core::ClassNameOnly(typeid(SERVICE).name()).Text())
            , _moduleName(moduleName) {
        }
        ~PublishedMetadataType() override = default;

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
        const TCHAR* Name() const override
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

    template <typename ACTUALSERVICE, typename METADATA = PublishedMetadataType<ACTUALSERVICE>>
    class PublishedServiceType : public IService {
    private:
        template<typename IMPLEMENTEDSERVICE>
        using Implementation = Core::ServiceType<IMPLEMENTEDSERVICE>;

    public:
        PublishedServiceType(PublishedServiceType&&) = delete;
        PublishedServiceType(const PublishedServiceType&) = delete;
        PublishedServiceType& operator=(PublishedServiceType&&) = delete;
        PublishedServiceType& operator=(const PublishedServiceType&) = delete;

        // This is constructed during the "loading" of an SO and the interface we are going to get is
        // only available in this module (local static variable in this so) so it is safe to assume it
        // is execised, single threaded!!!
        // from the sigle threaded load.
        template<typename... Args>
        PublishedServiceType(Args&&... args)
            : _info(std::forward<Args>(args)...)
            , _next(Core::System::SetModuleServices(this)) {
        }
        ~PublishedServiceType() override = default;

    public:
        void* Create(const uint32_t interfaceNumber) const override {
            void* interfaceInstance = nullptr;
            ACTUALSERVICE* result = Implementation<ACTUALSERVICE>::template Create<ACTUALSERVICE>();
            ASSERT(result != nullptr);
            if (result != nullptr) {
                interfaceInstance = result->QueryInterface(interfaceNumber);
                result->Release();
            }
            return (interfaceInstance);
        }
        const IMetadata* Info() const override {
            return (&_info);
        }
        const IService* Next() const override {
            return (_next);
        }

    private:
        METADATA _info;
        const IService* _next;
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

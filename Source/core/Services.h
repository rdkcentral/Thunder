#ifndef __SERVICES_H
#define __SERVICES_H

#include <list>

#include "Library.h"
#include "Module.h"
#include "Portability.h"
#include "Sync.h"
#include "TextFragment.h"
#include "Trace.h"

namespace WPEFramework {
namespace Core {

    struct EXTERNAL IServiceMetadata {
        virtual ~IServiceMetadata(){};

        virtual const std::string& Name() const = 0;
        virtual const TCHAR* Module() const = 0;
        virtual uint32_t Version() const = 0;
        virtual void* Create(const Library& library, const uint32_t interfaceNumber) = 0;
    };

    class EXTERNAL ServiceAdministrator {
    private:
        ServiceAdministrator();
        ServiceAdministrator(const ServiceAdministrator&) = delete;
        ServiceAdministrator& operator=(const ServiceAdministrator&) = delete;

    public:
        virtual ~ServiceAdministrator();

        static ServiceAdministrator& Instance();

    public:
        void AddRef() const
        {
            Core::InterlockedIncrement(_instanceCount);
        }
        uint32_t Release() const
        {
            ASSERT(_instanceCount > 0);

            Core::InterlockedDecrement(_instanceCount);

            return (Core::ERROR_NONE);
        }
        inline uint32_t Instances() const
        {
            return (_instanceCount);
        }
        void FlushLibraries();
        void ReleaseLibrary(const Library& reference);
        void Register(IServiceMetadata* service);
        void Unregister(IServiceMetadata* service);
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
        std::list<IServiceMetadata*> _services;
        mutable uint32_t _instanceCount;
        static ServiceAdministrator _systemServiceAdministrator;
    };

    template <typename ACTUALSERVICE>
    class Service : public ACTUALSERVICE {
    private:
        Service(const Service<ACTUALSERVICE>&) = delete;
        Service<ACTUALSERVICE> operator=(const Service<ACTUALSERVICE>&) = delete;

    protected:
        Service()
            : ACTUALSERVICE()
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1>
        Service(ARG1 arg1)
            : ACTUALSERVICE(arg1)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2>
        Service(ARG1 arg1, ARG2 arg2)
            : ACTUALSERVICE(arg1, arg2)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3)
            : ACTUALSERVICE(arg1, arg2, arg3)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
            : ACTUALSERVICE(arg1, arg2, arg3, arg4)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
            : ACTUALSERVICE(arg1, arg2, arg3, arg4, arg5)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
            : ACTUALSERVICE(arg1, arg2, arg3, arg4, arg5, arg6)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
            : ACTUALSERVICE(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }
        template <typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
        Service(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8)
            : ACTUALSERVICE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
            , _referenceCount(0)
        {
            ServiceAdministrator::Instance().AddRef();
        }

    public:
        template <typename INTERFACE>
        static INTERFACE* Create()
        {
            ACTUALSERVICE* object = new Service();

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1>
        static INTERFACE* Create(ARG1 arg1)
        {
            ACTUALSERVICE* object = new Service(arg1);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3, arg4);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3, arg4, arg5);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3, arg4, arg5, arg6);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3, arg4, arg5, arg6, arg7);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        template <typename INTERFACE, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5, typename ARG6, typename ARG7, typename ARG8>
        static INTERFACE* Create(ARG1 arg1, ARG2 arg2, ARG3 arg3, ARG4 arg4, ARG5 arg5, ARG6 arg6, ARG7 arg7, ARG8 arg8)
        {
            ACTUALSERVICE* object = new Service(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);

            return (Extract<INTERFACE>(object, TemplateIntToType<std::is_same<ACTUALSERVICE, INTERFACE>::value>()));
        }
        virtual ~Service()
        {
            ServiceAdministrator::Instance().Release();
        }

    public:
        virtual void AddRef() const
        {
            Core::InterlockedIncrement(_referenceCount);
        }
        virtual uint32_t Release() const
        {
            if (Core::InterlockedDecrement(_referenceCount) == 0) {
                delete this;

                return (Core::ERROR_DESTRUCTION_SUCCEEDED);
            }
            return (Core::ERROR_NONE);
        }

    private:
        template <typename INTERFACE>
        inline static INTERFACE* Extract(ACTUALSERVICE* object, const TemplateIntToType<false>&)
        {
            INTERFACE* result = reinterpret_cast<INTERFACE*>(object->QueryInterface(INTERFACE::ID));

            if (result == nullptr) {
                delete object;
            }

            return (result);
        }
        template <typename INTERFACE>
        inline static INTERFACE* Extract(ACTUALSERVICE* object, const TemplateIntToType<true>&)
        {
            object->AddRef();
            return (object);
        }

    private:
        mutable uint32_t _referenceCount;
    };

    template <typename ACTUALSINK>
    class Sink : public ACTUALSINK {
    private:
        Sink(const Sink<ACTUALSINK>&) = delete;
        Sink<ACTUALSINK> operator=(const Sink<ACTUALSINK>&) = delete;

    public:
        Sink()
            : ACTUALSINK()
            , _referenceCount(0)
        {
        }
        template <typename ARG1>
        Sink(ARG1 arg1)
            : ACTUALSINK(arg1)
            , _referenceCount(0)
        {
        }
        template <typename ARG1, typename ARG2>
        Sink(ARG1 arg1, ARG2 arg2)
            : ACTUALSINK(arg1, arg2)
            , _referenceCount(0)
        {
        }
        ~Sink()
        {
            if (_referenceCount != 0) {
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
    template <typename ACTUALSERVICE, const TCHAR** MODULENAME>
    class ServiceMetadata : public IServiceMetadata {
    private:
        ServiceMetadata() = delete;
        ServiceMetadata(const ServiceMetadata&) = delete;
        ServiceMetadata& operator=(const ServiceMetadata&) = delete;

        template <typename SERVICE>
        class ServiceImplementation : public Service<SERVICE> {
        private:
            ServiceImplementation() = delete;
            ServiceImplementation(const ServiceImplementation<SERVICE>&) = delete;
            ServiceImplementation<SERVICE> operator=(const ServiceImplementation<SERVICE>&) = delete;

        public:
            explicit ServiceImplementation(const Library& library)
                : Service<SERVICE>()
                , _referenceLib(library)
            {
            }
            ~ServiceImplementation()
            {
                ServiceAdministrator::Instance().ReleaseLibrary(_referenceLib);
            }

        private:
            Library _referenceLib;
        };

    public:
        ServiceMetadata(const uint16_t major, uint16_t minor)
            : _version(((major & 0xFFFF) << 16) + minor)
            , _Id(Core::ClassNameOnly(typeid(ACTUALSERVICE).name()).Text())
        {
            Core::ServiceAdministrator::Instance().Register(this);
        }
        ~ServiceMetadata()
        {
            Core::ServiceAdministrator::Instance().Unregister(this);
        }

    public:
        virtual uint32_t Version() const
        {
            return (_version);
        }
        virtual const std::string& Name() const
        {
            return (_Id);
        }
        virtual const TCHAR* Module() const
        {
            return (*MODULENAME);
        }
        virtual void* Create(const Library& library, const uint32_t interfaceNumber)
        {
            void* result = nullptr;
            IUnknown* object = new ServiceImplementation<ACTUALSERVICE>(library);

            if (object != nullptr) {
                // This quety interface will increment the refcount of the Service at least to 1.
                result = object->QueryInterface(interfaceNumber);

                if (result == nullptr) {
                    delete object;
                }
            }
            return (result);
        }

    private:
        uint32_t _version;
        string _Id;
    };

#define SERVICE_REGISTRATION(ACTUALCLASS, MAJOR, MINOR) \
    static Core::ServiceMetadata<ACTUALCLASS, &Core::System::MODULE_NAME> ServiceMetadata_##ACTUALCLASS(MAJOR, MINOR);

#ifdef BEGIN_INTERFACE_MAP
#undef BEGIN_INTERFACE_MAP
#endif
#ifdef INTERFACE_AGGREGATE
#undef INTERFACE_AGGREGATE
#endif
#ifdef END_INTERFACE_MAP
#undef END_INTERFACE_MAP
#endif

#define BEGIN_INTERFACE_MAP(ACTUALCLASS)                                     \
    void* QueryInterface(const uint32_t interfaceNumber) override            \
    {                                                                        \
        if (interfaceNumber == Core::IUnknown::ID) {                         \
            AddRef();                                                        \
            return (static_cast<void*>(static_cast<Core::IUnknown*>(this))); \
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

#define END_INTERFACE_MAP \
    return (nullptr);     \
    }
}
} // namespace Core

#endif // __SERVICES_H

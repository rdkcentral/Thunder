#include "Module.h"
#include "IShell.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(PluginHost::IShell::state)

    { PluginHost::IShell::DEACTIVATED, _TXT("Deactivated") },
    { PluginHost::IShell::DEACTIVATION, _TXT("Deactivation") },
    { PluginHost::IShell::ACTIVATED, _TXT("Activated") },
    { PluginHost::IShell::ACTIVATION, _TXT("Activation") },
    { PluginHost::IShell::PRECONDITION, _TXT("Precondition") },
    { PluginHost::IShell::DESTROYED, _TXT("Destroyed") },

ENUM_CONVERSION_END(PluginHost::IShell::state)

ENUM_CONVERSION_BEGIN(PluginHost::IShell::reason)

    { PluginHost::IShell::REQUESTED, _TXT("Requested") },
    { PluginHost::IShell::AUTOMATIC, _TXT("Automatic") },
    { PluginHost::IShell::FAILURE, _TXT("Failure") },
    { PluginHost::IShell::MEMORY_EXCEEDED, _TXT("MemoryExceeded") },
    { PluginHost::IShell::STARTUP, _TXT("Startup") },
    { PluginHost::IShell::SHUTDOWN, _TXT("Shutdown") },
    { PluginHost::IShell::CONDITIONS, _TXT("Conditions") },

ENUM_CONVERSION_END(PluginHost::IShell::reason)

namespace PluginHost
{
    class EXTERNAL Object : public Core::JSON::Container {
    private:
        class RootObject : public Core::JSON::Container {
        private:
            RootObject(const RootObject&) = delete;
            RootObject& operator=(const RootObject&) = delete;

        public:
            RootObject()
                : Config(false)
            {
                Add(_T("root"), &Config);
            }
            virtual ~RootObject() {}

        public:
            Core::JSON::String Config;
        };

    public:

        enum class ModeType {
            OFF,
            LOCAL,
            CONTAINER,
            REMOTE
        };

        Object()
            : Locator()
            , User()
            , Group()
            , Threads(1)
            , OutOfProcess(true)
            , Mode(ModeType::LOCAL)
            , Configuration(false)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("configuration"), &Configuration);
        }
        Object(const IShell* info)
            : Locator()
            , User()
            , Group()
            , Threads()
            , OutOfProcess(true)
            , Mode(ModeType::LOCAL)
            , Configuration(false)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("configuration"), &Configuration);

            RootObject config;
            config.FromString(info->ConfigLine());

            if (config.Config.IsSet() == true) {
                // Yip we want to go out-of-process
                Object settings;
                settings.FromString(config.Config.Value());

                *this = settings;

                if (Locator.Value().empty() == true) {
                    Locator = info->Locator();
                }
            }
        }
        Object(const Object& copy)
            : Locator(copy.Locator)
            , User(copy.User)
            , Group(copy.Group)
            , Threads(copy.Threads)
            , OutOfProcess(true)
            , Mode(copy.Mode)
            , Configuration(copy.Configuration)
        {
            Add(_T("locator"), &Locator);
            Add(_T("user"), &User);
            Add(_T("group"), &Group);
            Add(_T("threads"), &Threads);
            Add(_T("outofprocess"), &OutOfProcess);
            Add(_T("mode"), &Mode);
            Add(_T("configuration"), &Configuration);
        }
        virtual ~Object()
        {
        }

        Object& operator=(const Object& RHS)
        {

            Locator = RHS.Locator;
            User = RHS.User;
            Group = RHS.Group;
            Threads = RHS.Threads;
            OutOfProcess = RHS.OutOfProcess;
            Mode = RHS.Mode;
            Configuration = RHS.Configuration;

            return (*this);
        }

        RPC::Object::HostType HostType() const {
            RPC::Object::HostType result = RPC::Object::HostType::LOCAL;
            switch( Mode.Value() ) {
                case ModeType::CONTAINER :
                    result = RPC::Object::HostType::CONTAINER;
                    break;
                case ModeType::REMOTE :
                    result = RPC::Object::HostType::REMOTE;
                    break;
                default:
                    result = RPC::Object::HostType::LOCAL;
                    break;
            }
            return result;
        }

    public:
        Core::JSON::String Locator;
        Core::JSON::String User;
        Core::JSON::String Group;
        Core::JSON::DecUInt8 Threads;
        Core::JSON::Boolean OutOfProcess;
        Core::JSON::EnumType<ModeType> Mode; 
        Core::JSON::String Configuration; 
    };

    void* IShell::Root(uint32_t & pid, const uint32_t waitTime, const string className, const uint32_t interface, const uint32_t version)
    {
        void* result = nullptr;
        Object rootObject(this);

        bool inProcessOldConfiguration = ( !rootObject.Mode.IsSet() ) && ( rootObject.OutOfProcess.Value() == false ); //note: when both new and old not set this one will revert to the old default which was true
        bool inProcessNewConfiguration = ( rootObject.Mode.IsSet() ) && ( rootObject.Mode == Object::ModeType::OFF ); // when both set the Old one is ignored

        if ( inProcessNewConfiguration == true || inProcessOldConfiguration == true ) {

            string locator(rootObject.Locator.Value());

            if (locator.empty() == true) {
                result = Core::ServiceAdministrator::Instance().Instantiate(Core::Library(), className.c_str(), version, interface);
            } else {
                string search(PersistentPath() + locator);
                Core::Library resource(search.c_str());

                if (!resource.IsLoaded()) {
                    search = DataPath() + locator;
                    resource = Core::Library(search.c_str());

                    if (!resource.IsLoaded()) {
                        resource = Core::Library(locator.c_str());
                    }
                }
                if (resource.IsLoaded() == true) {
                    result = Core::ServiceAdministrator::Instance().Instantiate(resource, className.c_str(), version, interface);
                }
            }
        } else {
            ICOMLink* handler(COMLink());

            // This method can only be used in the main process. Only this process, can instantiate a new process
            ASSERT(handler != nullptr);

            if (handler != nullptr) {
                string locator(rootObject.Locator.Value());
                if (locator.empty() == true) {
                    locator = Locator();
                }
                RPC::Object definition(locator,
                    className,
                    interface,
                    version,
                    rootObject.User.Value(),
                    rootObject.Group.Value(),
                    rootObject.Threads.Value(),
                    rootObject.HostType(), 
                    rootObject.Configuration.Value());

                result = handler->Instantiate(definition, waitTime, pid, ClassName(), Callsign());
            }
        }

        return (result);
    }
}

ENUM_CONVERSION_BEGIN(PluginHost::Object::ModeType)

    { PluginHost::Object::ModeType::OFF, _TXT("Off") },
    { PluginHost::Object::ModeType::LOCAL, _TXT("Local") },
    { PluginHost::Object::ModeType::CONTAINER, _TXT("Container") },
    { PluginHost::Object::ModeType::REMOTE, _TXT("Remote") },

ENUM_CONVERSION_END(PluginHost::Object::ModeType);

} // namespace 




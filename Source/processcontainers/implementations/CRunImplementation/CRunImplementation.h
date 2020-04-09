#include "processcontainers/ProcessContainer.h"

// TODO: Find more elegant alternative
extern "C" {
    #include <crun/error.h>
    #include <crun/container.h>
    #include <crun/utils.h>
    #include <crun/status.h>
}

namespace WPEFramework {
namespace ProcessContainers {

    class CRunNetworkInterfaceIterator : public NetworkInterfaceIterator
    {
    public:
        CRunNetworkInterfaceIterator();
        ~CRunNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;
        std::string IP(uint32_t id) const override;
    };

    class CRunContainer : public IContainer 
    {
    public:
        CRunContainer(string name, string path, string logPath);
        virtual ~CRunContainer();

        // IContainerMethods
        const string Id() const override;
        uint32_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        NetworkInterfaceIterator* NetworkInterfaces() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;

    private:
        uint32_t ClearLeftovers();
        void OverwriteContainerArgs(libcrun_container_t* container, const string& newComand, IStringIterator& newParameters);

        mutable uint32_t _refCount;
        bool _created; // keeps track if container was created and needs deletion
        string _name;
        string _bundle;
        string _configFile;
        string _logPath;
        libcrun_container_t *_container;
        libcrun_context_t _context;
        mutable Core::OptionalType<uint32_t> _pid;
        libcrun_error_t _error;
    };

    class CRunContainerAdministrator : public IContainerAdministrator 
    {
        friend class CRunContainer;
    public:
        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        CRunContainerAdministrator();
        ~CRunContainerAdministrator();

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;

    private:
        std::list<IContainer*> _containers;
        mutable uint32_t _refCount;
    };
}
}

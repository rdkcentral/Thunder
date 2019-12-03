#include "processcontainers/containers.h"
#include "processcontainers/ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    class CContainerAdministrator : public IContainerAdministrator 
    {
    public:
        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        void Logging(const string& logpath, const string& logid, const string& loggingoptions) override;
        ContainerIterator Containers() override;
    
    private:
        std::list<IContainer*> _containers;
    };

    class CContainer : public IContainer 
    {
    public:
        CContainer(Container_t* container);
        virtual ~CContainer();

        const string Id() const override;
        pid_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        string ConfigPath() const override;
        string LogPath() const override;
        IConstStringIterator NetworkInterfaces() const override;
        std::vector<Core::NodeId> IPs(const string& interface) const override;
        bool IsRunning() const override;

        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() const override;

    private:
        Container_t* _container;
        mutable uint32_t _refCount;
    };
}
}
#include "processcontainers/process_containers.h"
#include "processcontainers/ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    class CNetworkInterfaceIterator : public NetworkInterfaceIterator 
    {
    public:
        CNetworkInterfaceIterator(const ProcessContainer* container);
        ~CNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;

        std::string IP(uint32_t id) const override;
    private:
        ProcessContainerNetworkStatus _networkStatus;
    };
    
    class CContainer : public IContainer 
    {
    public:
        CContainer(ProcessContainer* container);
        virtual ~CContainer();

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
        ProcessContainer* _container;
        mutable uint32_t _refCount;
        std::vector<string> _networkInterfaces;
    };

    class CContainerAdministrator : public IContainerAdministrator 
    {
        friend class CContainer;
    public:
        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        CContainerAdministrator();

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;
    protected:
        void RemoveContainer(IContainer*);

    private:
        std::list<IContainer*> _containers;
        mutable uint32_t _refCount;
    };
}
}
#include "processcontainers/containers.h"
#include "processcontainers/ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {
    class CContainer : public IContainer 
    {
    public:
        CContainer(Container_t* container);
        virtual ~CContainer();

        // IContainerMethods
        const string Id() const override;
        pid_t Pid() const override;
        MemoryInfo Memory() const override;
        CPUInfo Cpu() const override;
        IConstStringIterator NetworkInterfaces() const override;
        std::vector<string> IPs(const string& interface) const override;
        bool IsRunning() const override;

        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;

        void AddRef() const override;
        uint32_t Release() override;

    private:
        void _GetNetworkInterfaces();

    private:
        Container_t* _container;
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

        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

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
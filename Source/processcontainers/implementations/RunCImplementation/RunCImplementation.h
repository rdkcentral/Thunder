#include "processcontainers/ProcessContainer.h"

namespace WPEFramework {
namespace ProcessContainers {

    class RunCNetworkInterfaceIterator : public NetworkInterfaceIterator
    {
    public:
        RunCNetworkInterfaceIterator();
        ~RunCNetworkInterfaceIterator();

        std::string Name() const override;
        uint32_t NumIPs() const override;
        std::string IP(uint32_t id) const override;
    };

    class RunCContainer : public IContainer 
    {
    public:
        RunCContainer(string name, string path, string logPath);
        virtual ~RunCContainer();

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
        mutable uint32_t _refCount;
        string _name;
        string _path;
        string _logPath;
        mutable Core::OptionalType<uint32_t> _pid;
    };

    class RunCContainerAdministrator : public IContainerAdministrator 
    {
        friend class RunCContainer;
    public:
        IContainer* Container(const string& id, 
                                IStringIterator& searchpaths, 
                                const string& logpath,
                                const string& configuration) override; //searchpaths will be searched in order in which they are iterated

        RunCContainerAdministrator();
        ~RunCContainerAdministrator();

        // IContainerAdministrator methods
        void Logging(const string& logDir, const string& loggingOptions) override;
        ContainerIterator Containers() override;

        // Lifetime management
        void AddRef() const override;
        uint32_t Release() override;
    protected:
        void DestroyContainer(const string& name); // make sure that no leftovers from previous launch will cause crash
        void RemoveContainer(IContainer*);

    private:
        std::list<IContainer*> _containers;
        mutable uint32_t _refCount;
    };
}
}

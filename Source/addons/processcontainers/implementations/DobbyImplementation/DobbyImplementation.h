
#pragma once

#include "processcontainers/ContainerAdministrator.h"
#include "processcontainers/common/CGroupContainerInfo.h"

#include <Dobby/DobbyProtocol.h>
#include <Dobby/Public/Dobby/IDobbyProxy.h>

#include <future>

namespace Thunder {
namespace ProcessContainers {

    const string CONFIG_NAME = "/config.json";
    const string CONFIG_NAME_SPEC = "/spec.json";

    class DobbyContainerAdministrator;

    class DobbyContainer : public IContainer  {
    public:
        DobbyContainer(DobbyContainerAdministrator& admin, const string& name, const string& path, const string& logPath, const bool useSpecFile = false);
        DobbyContainer(const DobbyContainer&) = delete;
        DobbyContainer& operator=(const DobbyContainer&) = delete;
        ~DobbyContainer() override;

    public:
        // IContainerMethods
        containertype Type() const override { return IContainer::DOBBY; }
        const string& Id() const override;
        uint32_t Pid() const override;
        bool IsRunning() const override;
        bool Start(const string& command, IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/) override;
        IMemoryInfo* Memory() const override;
        IProcessorInfo* ProcessorInfo() const override;
        INetworkInterfaceIterator* NetworkInterfaces() const override;

    private:
        mutable Core::CriticalSection _adminLock;
        DobbyContainerAdministrator& _admin;
        string _name;
        string _path;
        string _logPath;
        int _descriptor;
        mutable Core::OptionalType<uint32_t> _pid;
        bool _useSpecFile;
    };

    class DobbyContainerAdministrator : public IContainerProducer {
        friend class DobbyContainer;

    public:
        DobbyContainerAdministrator() = default;
        ~DobbyContainerAdministrator() override = default;
        DobbyContainerAdministrator(const DobbyContainerAdministrator&) = delete;
        DobbyContainerAdministrator& operator=(const DobbyContainerAdministrator&) = delete;

    private:
        std::shared_ptr<AI_IPC::IIpcService> mIpcService; // Ipc Service instance
        std::shared_ptr<IDobbyProxy> mDobbyProxy; // DobbyProxy instance

    public:
        // IContainerProducer methods
        uint32_t Initialize(const string& config) override;
        void Deinitialize() override;
        Core::ProxyType<IContainer> Container(const string& id, IStringIterator& searchpaths,
            const string& logpath, const string& configuration) override;

    private:
        void DestroyContainer(const string& name); // make sure that no leftovers from previous launch will cause crash
        bool ContainerNameTaken(const string& name);
        void containerStopCallback(int32_t cd, const std::string& containerId,
            IDobbyProxyEvents::ContainerState state,
            const void* params);

    private:
        std::promise<void> _stopPromise;
    };
}
}

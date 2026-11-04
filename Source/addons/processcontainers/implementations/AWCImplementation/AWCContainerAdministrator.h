#pragma once

#include <mutex>
#include <vector>

#include "AWC.h"
#include "dbus/Client.h"
#include "core/JSON.h"

#include "processcontainers/Messaging.h"
#include "processcontainers/ContainerAdministrator.h"

namespace Thunder {
namespace ProcessContainers {

struct PluginConfig: public Core::JSON::Container
{
    bool useProxy{false};
    std::chrono::milliseconds startTimeout;
    std::chrono::milliseconds stopTimeout;

    PluginConfig(const PluginConfig&) = delete;
    PluginConfig& operator=(const PluginConfig&) = delete;
    PluginConfig(const std::string &path);
};


class AWCContainerAdministrator:
    public IContainerProducer,
    public AWCStateChangeNotifier
{
private:
    awc::AWCClient * awcClient_;
    std::shared_ptr<AWCListener> awcClientListener_;
    dbus::Client dbusClient_;

public:
    AWCContainerAdministrator();
    AWCContainerAdministrator(const AWCContainerAdministrator&) = delete;
    AWCContainerAdministrator& operator=(const AWCContainerAdministrator&) = delete;
    ~AWCContainerAdministrator() override;

    uint32_t Initialize(const string& config) override;
    void Deinitialize() override;
    Core::ProxyType<IContainer> Container(const string& name, IStringIterator& searchpaths,
        const string& containerLogDir, const string& configuration) override;
};

} /* ProcessContainers */
} /* Thunder */

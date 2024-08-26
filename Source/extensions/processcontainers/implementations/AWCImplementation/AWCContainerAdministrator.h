#pragma once

#include <mutex>
#include <vector>


#include "AWC.h"
#include "dbus/Client.h"
#include "processcontainers/common/BaseAdministrator.h"
#include "core/JSON.h"

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
    public BaseContainerAdministrator<IContainer>,
    public AWCStateChangeNotifier
{
    friend class AWCContainer;
    friend class Core::SingletonType<AWCContainerAdministrator>;
private:
    awc::AWCClient * awcClient_;
    std::shared_ptr<AWCListener> awcClientListener_;
    dbus::Client dbusClient_;
    AWCContainerAdministrator();
public:
    AWCContainerAdministrator(const AWCContainerAdministrator&) = delete;
    AWCContainerAdministrator& operator=(const AWCContainerAdministrator&) = delete;
    ~AWCContainerAdministrator() override;
    IContainer* Container(const string& name, IStringIterator& searchpaths, const string& containerLogDir, const string& configuration) override;
    void Logging(const string& globalLogDir, const string& loggingOptions) override {};
};

} /* ProcessContainers */
} /* Thunder */

#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>

#include "AWCContainerBase.h"

namespace Thunder {
namespace ProcessContainers {

class AWCProxyContainer: public AWCContainerBase
{
    using AppState = dbus::AppState;
    using Clock = std::chrono::steady_clock;
    using Lock = std::unique_lock<std::mutex>;
    using milliseconds = std::chrono::milliseconds;

    class DBusInterface: public dbus::Client::Interface
    {
        AWCProxyContainer *container_;

        void onStateChange(int, AppState) override final;
    public:
        DBusInterface(AWCProxyContainer *);
        ~DBusInterface() override final;
    };

    using SharedDBusInterface = std::shared_ptr<DBusInterface>;

    friend class AWCContainerAdministrator;

    AWCProxyContainer(const string &name, const PluginConfig &, dbus::Client *);

    bool waitForStateChange(Lock &, AppState, milliseconds);
public:
    AWCProxyContainer(const AWCProxyContainer&) = delete;
    ~AWCProxyContainer() override final;
    AWCProxyContainer& operator=(const AWCProxyContainer&) = delete;

    uint32_t Pid() const override final;
    bool IsRunning() const override final;
    bool Start(const string &, ProcessContainers::IStringIterator &) override final;
    bool Stop(const uint32_t timeout /*ms*/) override final;
protected:
    uint32_t pid_{0};
    AppState appState_{AppState::UNKNOWN};
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    SharedDBusInterface dbusInterface_;
    // those values can be overwritten by config
    const milliseconds startTimeout_;
    const milliseconds stopTimeout_;
    dbus::Client *const client_;
};

} /* ProcessContainers */
} /* Thunder */

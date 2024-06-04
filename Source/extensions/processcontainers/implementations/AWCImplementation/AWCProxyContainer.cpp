#include "AWCContainerAdministrator.h"
#include "AWCProxyContainer.h"

#define CHECK(cond) ASSERT((cond))

using namespace Thunder::ProcessContainers;

AWCProxyContainer::DBusInterface::DBusInterface(
    AWCProxyContainer *container):
    dbus::Client::Interface{container ? container->Id() : ""},
    container_{container}
{
    TRACE_L1("id=%s(%p) ", id().c_str(), this);
    CHECK(!id().empty());
    CHECK(container_);
}

AWCProxyContainer::DBusInterface::~DBusInterface()
{
    TRACE_L1("id=%s(%p) ", id().c_str(), this);
    container_ = nullptr;
}

void AWCProxyContainer::DBusInterface::onStateChange(int pid, AppState appState)
{
    TRACE_L1("id=%s(%p) ", id().c_str(), this);
    CHECK(container_);

    std::lock_guard<std::mutex> lock(container_->mutex_);
    container_->pid_ = pid;
    container_->appState_ = appState;
    container_->cond_.notify_one();
}

bool AWCProxyContainer::waitForStateChange(
    Lock &lock, AppState state, milliseconds timeout)
{
    TRACE_L1(
        "begin, curr: %s, desired: %s (timeout %d)",
        toStr(appState_), toStr(state),
        int(timeout.count()));

    using namespace std::chrono;

    const auto timestamp = Clock::now();

    auto r = appState_ == state;

    if(!r)
    {
        r = cond_.wait_for(lock, timeout, [=](){return appState_ == state;});

        if(!r) TRACE_L1("timeout");
    }

    const auto diff = duration_cast<milliseconds>(Clock::now() - timestamp);

    TRACE_L1("end, elapsed=%dms result=%d", int(diff.count()), r);
    return r;
}

AWCProxyContainer::AWCProxyContainer(
    const string& name, const PluginConfig &cfg, dbus::Client *client):
        AWCContainerBase(name),
        startTimeout_{cfg.startTimeout},
        stopTimeout_{cfg.stopTimeout},
        client_{client}
{
    TRACE_L1("id=%s(%p) ", Id().c_str(), this);
    CHECK(client_);

    dbusInterface_ = std::make_shared<DBusInterface>(this);
    client_->bind(dbusInterface_);
}

AWCProxyContainer::~AWCProxyContainer()
{
    TRACE_L1("id=%s(%p)", Id().c_str(), this);

    if(IsRunning()) Stop(2000);
    CHECK(client_);
    client_->unbind(dbusInterface_);
    static_cast<AWCContainerAdministrator&>(
        AWCContainerAdministrator::Instance()).RemoveContainer(this);
}

uint32_t AWCProxyContainer::Pid() const
{
    TRACE_L1(
        "id=%s(%p) pid=%d appState=%s",
        Id().c_str(), this, pid_, toStr(appState_));
    std::lock_guard<std::mutex> lock(mutex_);
    return pid_;
}

bool AWCProxyContainer::IsRunning() const
{
    bool isRunning = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        isRunning = AppState::STARTED == appState_;
    }

    TRACE_L1("id=%s(%p) isRunning=%d", Id().c_str(), this, isRunning);
    return isRunning;
}

bool AWCProxyContainer::Start(const string &command, IStringIterator &params)
{
    if(IsRunning())
    {
        TRACE_L1("id=%s(%p) already running", Id().c_str(), this);
        return false;
    }

    std::string cmd;

    while(params.Next())
    {
        if(!cmd.empty()) cmd += ' ';
        cmd += params.Current();
    }

    TRACE_L1(
        "id=%s(%p) command=%s params_num=%d params=%s",
        Id().c_str(), this, command.c_str(), params.Count(), cmd.c_str());


    CHECK(dbusInterface_);
    if(!dbusInterface_) return false;

    using namespace std::chrono;

    const auto now = Clock::now();

    if(!dbusInterface_->startContainer(client_, command, cmd, startTimeout_))
    {
        TRACE_L1("failed to start id=%s(%p)", Id().c_str(), this);
        return false;
    }

    const auto diff = duration_cast<milliseconds>(Clock::now() - now);

    if(diff >= startTimeout_) return false;

    std::unique_lock<std::mutex> lock(mutex_);
    return waitForStateChange(lock, AppState::STARTED, startTimeout_ - diff);
}

/* provided timeout is not used. Thunder provides 0ms timeout by default
 * and if this function returns false next Stop() will be retried after 10s */
bool AWCProxyContainer::Stop(const uint32_t timeout /*ms*/)
{
    if(!IsRunning())
    {
        TRACE_L1("id=%s(%p) not running", Id().c_str(), this);
        return true;
    }

    TRACE_L1(
        "id=%s(%p) timeout=%u pid=%u",
        Id().c_str(), this, timeout, pid_);

    CHECK(dbusInterface_);
    if(!dbusInterface_) return false;

    using namespace std::chrono;

    const auto now = Clock::now();

    if(!dbusInterface_->stopContainer(client_, stopTimeout_))
    {
        TRACE_L1("failed to stop id=%s(%p)", Id().c_str(), this);
        return false;
    }

    const auto diff = duration_cast<milliseconds>(Clock::now() - now);

    if(diff >= stopTimeout_) return false;

    std::unique_lock<std::mutex> lock(mutex_);
    return waitForStateChange(lock, AppState::STOPPED, stopTimeout_ - diff);
}

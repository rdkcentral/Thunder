#pragma once

#include <condition_variable>
#include <mutex>

#include "AWC.h"
#include "AWCContainerBase.h"


namespace Thunder {
namespace ProcessContainers {

class AWCContainer:
    public AWCContainerBase,
    public AWCStateChangeListener
{
public:
    AWCContainer(const string& id, awc::AWCClient * client, AWCStateChangeNotifier * notifier);
    AWCContainer(const AWCContainer&) = delete;
    ~AWCContainer() override;
    AWCContainer& operator=(const AWCContainer&) = delete;

public:
    containertype Type() const override { return IContainer::AWC; }
    uint32_t Pid() const override;
    bool IsRunning() const override;
    bool Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
    bool Stop(const uint32_t timeout /*ms*/) override;
    void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) override;

private:
    uint32_t _pid{0};
    int _runId{-1};
    awc::awc_app_state_t _appState{awc::AWC_STATE_UNKNOWN};
    awc::AWCClient * _client;
    AWCStateChangeNotifier * _notifier;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    bool _waitForResponse{false};
    bool WaitUntilAWCReady() const;
};

} /* ProcessContainers */
} /* Thunder */


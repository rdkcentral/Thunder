#pragma once

#include <mutex>

#include <AWCClient.h>

namespace Thunder {
namespace ProcessContainers {


class AWCStateChangeListener {
public:
    virtual void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) = 0;
};

class AWCStateChangeNotifier {
public:
    AWCStateChangeNotifier();
    void addListener(AWCStateChangeListener * listener);
    void removeListener(AWCStateChangeListener * listener);
    void notify(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid);
private:
    std::vector<AWCStateChangeListener *> _listeners;
    std::mutex _mutex;
};

class AWCListener : public awc::AWCClient::Listener
{
public:
    AWCListener(AWCStateChangeNotifier * notifier);
    ~AWCListener() {};
    void notifyWindowChange(int window_id, awc::AWCClient::awc_window_state_t window_state, unsigned int pid) override;
    void notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid) override;
    void notifyServiceReady();
private:
    AWCStateChangeNotifier * _notifier;
};

} /* ProcessContainers */
} /* Thunder */

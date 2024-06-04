#include "AWC.h"

#include <algorithm>

#include "Tracing.h"


using namespace Thunder::ProcessContainers;


AWCStateChangeNotifier::AWCStateChangeNotifier() : _listeners(), _mutex() {};

void AWCStateChangeNotifier::addListener(AWCStateChangeListener * listener)
{
    if (!listener) return;
    std::lock_guard<std::mutex> lock(_mutex);
    if (std::find(_listeners.begin(), _listeners.end(), listener) == _listeners.end())
    {
        _listeners.push_back(listener);
    }
    else
    {
        TRACE_L1("%s listener already registrered %p", __FUNCTION__, listener);
    }
}

void AWCStateChangeNotifier::removeListener(AWCStateChangeListener * listener)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _listeners.erase(std::remove(_listeners.begin(), _listeners.end(), listener), _listeners.end());
}

void AWCStateChangeNotifier::notify(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto listener : _listeners)
    {
        listener->notifyStateChange(req_id, app_state, status, pid);
    }
}

AWCListener::AWCListener(AWCStateChangeNotifier * notifier)
    : _notifier(notifier)
{}

void AWCListener::notifyWindowChange(int window_id, awc::AWCClient::awc_window_state_t window_state, unsigned int pid)
{
    // intentionally left empty
}

void AWCListener::notifyStateChange(int req_id, awc::awc_app_state_t app_state, int status, unsigned int pid)
{
    TRACE_L3("%s req_id=%d app_state=%d status=%d pid=%u", _TRACE_FUNCTION_, req_id, app_state, status, pid);
    _notifier->notify(req_id, app_state, status, pid);
}

void AWCListener::notifyServiceReady()
{
    TRACE_L3("%s", _TRACE_FUNCTION_);
    // no action
}

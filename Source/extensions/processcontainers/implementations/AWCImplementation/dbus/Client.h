#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "api.h"

namespace Thunder {
namespace ProcessContainers {
namespace dbus {

/* Keep this enum in sync with awc_app_state_t:
 * slauncher/client/AWCClient.h
 * this file is not included to avoid slauncher dep. */
enum class AppState
{
  STARTED = 0,
  STOPPED = 1,
  SUSPENDED = 2,
  CLOSED = 3,
  STARTING = 4,
  STOPPING = 5,
  SUSPENDING = 6,
  UNKNOWN = 7
};

inline
const char *toStr(AppState state)
{
    const char *str[] =
    {
        "STARTED",
        "STOPPED",
        "SUSPENDED",
        "CLOSED",
        "STARTING",
        "STOPPING",
        "SUSPENDING",
        "UNKNOWN"
    };
    return
        int(sizeof(str) / sizeof(str[0])) > int(state)
        ? str[int(state)] : "undefined";
}

struct ThreadGuard
{
    std::thread thread;

    template <typename ...Tn>
    ThreadGuard(Tn &&... args): thread{std::forward<Tn>(args)...} {}
    ~ThreadGuard() {if(thread.joinable()) thread.join();}
};

enum class AsyncResult {Pending, Timeout, OK, NOK};
enum class ConnState {Pending, Connected, Disconnected};

struct Client
{
    friend struct DBusCallbacks;

    using milliseconds = std::chrono::milliseconds;

    class Interface
    {
        friend class Client;
    private:
        const std::string id_;
        virtual void onStateChange(int pid, AppState state) = 0;
    public:
        Interface(std::string);
        virtual ~Interface();
        Interface(const Interface &) = delete;
        Interface &operator=(const Interface &) = delete;

        const std::string &id() const {return id_;};
        void stateChange(int, AppState);
        bool startContainer(
            Client *, const std::string &, const std::string &, milliseconds);
        bool stopContainer(Client *, milliseconds);
    };

    using SharedInterface = std::shared_ptr<Interface>;
private:
    using Lock = std::unique_lock<std::mutex>;
    using Map = std::map<std::string, SharedInterface>;

    std::mutex mutex_;
    std::condition_variable cond_;
    struct {
        std::unique_ptr<ThreadGuard> thread;
        GMainLoop *mainLoop{nullptr};
        ContainerCtrl *api{nullptr};
        ConnState connState{ConnState::Disconnected};

        struct {
            gulong stateChange{0};
        } signalId;
    } dbus_;
    Map map_;

    void connect(gulong &, const char *, GCallback);
    void disconnect(gulong &);
    bool waitForResult(Lock &, AsyncResult &, GCancellable *, milliseconds);
    bool connectionResetIfNeeded(Lock &, milliseconds = milliseconds{500});
    bool connectBus(Lock &, milliseconds timeout);
    void disconnectBus(Lock &);
    void exec();
    SharedInterface findInterface(const std::string &);
    void quit(Lock &);
public:
    Client();
    ~Client();
    void quit();
    bool bind(SharedInterface);
    bool unbind(SharedInterface);
};

} /* dbus */
} /* ProcessContainers */
} /* Thunder */

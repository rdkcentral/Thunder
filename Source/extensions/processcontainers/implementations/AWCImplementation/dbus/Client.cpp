#include <functional>
#include "Client.h"
#include "core/Trace.h"

#define CHECK(cond) ASSERT((cond))

namespace Thunder {
namespace ProcessContainers {
namespace dbus {

struct DBusCallbacks
{
    static
    void handleStateChange(ContainerCtrl *, const char *, int, int, gpointer);
    static
    void asyncReady(GObject *, GAsyncResult *, gpointer);

    static_assert(
        std::is_same<
            decltype(&asyncReady),
            GAsyncReadyCallback>::value, "signature check failed");
};

} /* dbus */
} /* ProcessContainers */
} /* Thunder */

using namespace Thunder::ProcessContainers::dbus;

namespace {

const char *noNull(const char *p) {return p ? p : "";}
const char *dbusInterface() {return "com.lgi.rdk.utils.container.ctrl";}
const char *dbusPath() {return "/com/lgi/rdk/utils/container/ctrl";}

std::unique_ptr<GCancellable, decltype(&g_object_unref)> makeCancellable()
{
    return
     std::unique_ptr<GCancellable, decltype(&g_object_unref)>
     {
         g_cancellable_new(), g_object_unref
     };
}

struct GErrorGuard
{
    GError *error{nullptr};
    ~GErrorGuard()
    {
        if(error) g_error_free(error);
        error = nullptr;
    }
    const char *msg() const {return error ? noNull(error->message) : "";}
    int code() const {return error ? error->code : 0;}
};

struct AsyncReadyCbData
{
    using ReadyCb = std::function<bool(GAsyncResult *, GError **)>;
    /* readyCb will be executed with mutex locked */
    ReadyCb readyCb;
    std::mutex *mutex;
    std::condition_variable *cond;
    AsyncResult result{AsyncResult::Pending};

    AsyncReadyCbData(ReadyCb cb, std::mutex &m, std::condition_variable &c):
        readyCb{std::move(cb)},
        mutex{&m},
        cond{&c}
    {}
};

} /* namespace */

void DBusCallbacks::handleStateChange(
    ContainerCtrl *instance,
    const char *id, int pid, int state,
    gpointer user_data)
{
    TRACE_L1("id=%s, pid=%d, state=%d", noNull(id), pid, state);
    auto client = static_cast<Client *>(user_data);

    CHECK(client);
    if(!client) return;
    auto interface = client->findInterface({noNull(id)});
    if(interface) interface->stateChange(pid, AppState(state));
}

void DBusCallbacks::asyncReady(GObject *, GAsyncResult *result, gpointer payload)
{
    TRACE_L1("begin");
    auto cbData = reinterpret_cast<AsyncReadyCbData *>(payload);
    GErrorGuard errorGuard;

    CHECK(cbData);
    CHECK(cbData->readyCb);
    CHECK(cbData->mutex);
    CHECK(cbData->cond);

    if(!cbData || !cbData->readyCb || !cbData->mutex || !cbData->cond) return;

    std::unique_lock<std::mutex> lock{*cbData->mutex};
    const auto status = cbData->readyCb(result, &errorGuard.error);

    if(!status && errorGuard.error)
    {
        TRACE_L1("failed: %s (%d)", errorGuard.msg(), errorGuard.code());
        cbData->result =
            G_IO_ERROR_CANCELLED == errorGuard.code()
            ? AsyncResult::Timeout
            : AsyncResult::NOK;
    }
    else if(!status) cbData->result = AsyncResult::NOK;
    else cbData->result = AsyncResult::OK;
    cbData->cond->notify_all();
    TRACE_L1("end");
}

Client::Interface::Interface(std::string name): id_{std::move(name)}
{
    TRACE_L1("%s(%p)", id().c_str(), this);
}

Client::Interface::~Interface()
{
    TRACE_L1("%s(%p)", id().c_str(), this);
}

void Client::Interface::stateChange(int pid, AppState appState)
{
    TRACE_L1("id=%s(%p), pid=%d, %s", id().c_str(), this, pid, toStr(appState));
    onStateChange(pid, appState);
}

bool Client::Interface::startContainer(
    Client *client,
    const std::string &cmd, const std::string &param, milliseconds timeout)
{
    TRACE_L1(
        "id=%s(%p), cmd=%s, param=%s, timeout=%dms",
        id().c_str(), this, cmd.c_str(), param.c_str(), int(timeout.count()));

    CHECK(client);

    if(!client)
    {
        TRACE_L1("id=%s(%p), invalid client",  id().c_str(), this);
        return false;
    }

    auto cancellable = makeCancellable();
    std::unique_lock<std::mutex> lock{client->mutex_};

    if(!client->connectionResetIfNeeded(lock)) return false;

    AsyncReadyCbData cbData
    {
        [&](GAsyncResult *result, GError **error)
        {
            return
                container_ctrl_call_start_finish(client->dbus_.api, result, error);
        },
        client->mutex_,
        client->cond_
    };

    container_ctrl_call_start(
        client->dbus_.api,
        id().c_str(), cmd.c_str(), param.c_str(),
        cancellable.get(), DBusCallbacks::asyncReady, &cbData);

    const auto r =
        client->waitForResult(lock, cbData.result, cancellable.get(), timeout);
    TRACE_L1("result=%d", r);
    return r;
}

bool Client::Interface::stopContainer(Client *client, milliseconds timeout)
{
    TRACE_L1("id=%s(%p), timeout=%dms", id().c_str(), this, int(timeout.count()));

    CHECK(client);

    if(!client)
    {
        TRACE_L1("id=%s(%p), invalid client",  id().c_str(), this);
        return false;
    }

    auto cancellable = makeCancellable();
    std::unique_lock<std::mutex> lock{client->mutex_};

    if(!client->connectionResetIfNeeded(lock)) return false;

    AsyncReadyCbData cbData
    {
        [&](GAsyncResult *result, GError **error)
        {
            return
                container_ctrl_call_stop_finish(client->dbus_.api, result, error);
        },
        client->mutex_,
        client->cond_
    };

    container_ctrl_call_stop(
        client->dbus_.api,
        id().c_str(),
        cancellable.get(), DBusCallbacks::asyncReady, &cbData);

    const auto r =
        client->waitForResult(lock, cbData.result, cancellable.get(), timeout);
    TRACE_L1("result=%d", r);
    return r;
}

bool Client::connectionResetIfNeeded(Lock &lock, milliseconds timeout)
{
    if(ConnState::Pending == dbus_.connState)
    {
       TRACE_L1("wait for pending connection");

       cond_.wait(
          lock,
          [&]()
          {
              if(ConnState::Pending != dbus_.connState) return true;
              TRACE_L1("still waiting");
              return false;
          });
    }

    if(ConnState::Connected == dbus_.connState) return true;

    CHECK(ConnState::Disconnected == dbus_.connState);

    if(ConnState::Disconnected != dbus_.connState) return false;

    CHECK(!dbus_.thread);

    if(dbus_.thread) return false;

    TRACE_L1("spawning dbus thread");

    dbus_.thread =
        std::unique_ptr<ThreadGuard>(new ThreadGuard{[=](){exec();}});

    if(!connectBus(lock, timeout))
    {
        TRACE_L1("not connected, timeout=%dms", int(timeout.count()));
        quit(lock);
        /* wait until exec() returns */
        dbus_.thread.reset(nullptr);
        return false;
    }

    return true;
}

void Client::connect(gulong &signal, const char *name, GCallback callback)
{
    CHECK(!mutex_.try_lock());
    CHECK(0 == signal);
    if(0 != signal) return;
    signal = g_signal_connect(dbus_.api, name, callback, this);
}

void Client::disconnect(gulong &signal)
{
    CHECK(!mutex_.try_lock());
    CHECK(0 != signal);
    if(0 == signal) return;
    g_signal_handler_disconnect(dbus_.api, signal);
    signal = 0;
}

bool Client::waitForResult(
    Lock &lock, AsyncResult &result, GCancellable *cancellable, milliseconds timeout)
{
    TRACE_L1("begin, timeout=%d", int(timeout.count()));
    CHECK(cancellable);

    const auto status =
        cond_.wait_for(lock, timeout, [&](){return result != AsyncResult::Pending;});

    if(!status)
    {
        TRACE_L1("timeout: %dms", int(timeout.count()));

        g_cancellable_cancel(cancellable);

        /* must wait for:
         * a) async operation to be cancelled (asyncReady wont be called)
         * and:
         * b) asyncReady finish executing
         * because asyncReady uses AsyncReadyCbData from current thread stack */
        cond_.wait(
            lock, [&]()
            {
                return
                    result != AsyncResult::Pending
                    && g_cancellable_is_cancelled(cancellable);
            });
        TRACE_L1("cancelled async call");
    }

    const auto r = AsyncResult::OK == result;

    TRACE_L1("end, result=%d", r);
    return r;
}

bool Client::connectBus(Lock &lock, milliseconds timeout)
{
    CHECK(!dbus_.api);
    if(dbus_.api) return false;

    CHECK(ConnState::Disconnected == dbus_.connState);
    if(ConnState::Disconnected != dbus_.connState) return false;

    dbus_.connState = ConnState::Pending;

    TRACE_L1("if=%s, path=%s", dbusInterface(), dbusPath());

    AsyncReadyCbData cbData
    {
        [&](GAsyncResult *result, GError **error)
        {
            dbus_.api = container_ctrl_proxy_new_for_bus_finish(result, error);
            return dbus_.api;
        },
        mutex_,
        cond_
    };

    auto cancellable = makeCancellable();

    container_ctrl_proxy_new_for_bus(
        G_BUS_TYPE_SYSTEM,
        G_DBUS_PROXY_FLAGS_NONE,
        dbusInterface(),
        dbusPath(),
        cancellable.get(),
        DBusCallbacks::asyncReady,
        &cbData);

    const auto status =
        waitForResult(lock, cbData.result, cancellable.get(), timeout);

    if(!status)
    {
        TRACE_L1("failed, timeout=%dms", int(timeout.count()));
        dbus_.connState = ConnState::Disconnected;
        CHECK(!dbus_.api);
        return false;
    }

    TRACE_L1("connected");

    connect(
        dbus_.signalId.stateChange,
        "state-change", G_CALLBACK(DBusCallbacks::handleStateChange));
    dbus_.connState = ConnState::Connected;
    return true;
}

void Client::disconnectBus(Lock &)
{
    if(ConnState::Connected != dbus_.connState) return;

    disconnect(dbus_.signalId.stateChange);
    dbus_.connState = ConnState::Disconnected;
    g_object_unref(dbus_.api);
    dbus_.api = nullptr;
}

void Client::exec()
{
    TRACE_L1("begin");

    auto loopHandle =
        std::unique_ptr<GMainLoop, decltype(&g_main_loop_unref)>
        {
            g_main_loop_new(nullptr, FALSE /* not running */),
            g_main_loop_unref
        };

    {
        std::unique_lock<std::mutex> lock(mutex_);
        dbus_.mainLoop = loopHandle.get();
    }

    g_main_loop_run(loopHandle.get());

    {
        std::unique_lock<std::mutex> lock(mutex_);
        dbus_.mainLoop = nullptr;
        disconnectBus(lock);
    }

    TRACE_L1("end");
}

Client::SharedInterface Client::findInterface(const std::string &id)
{
    std::unique_lock<std::mutex> lock{mutex_};

    const auto i = map_.find(id);

    if(std::end(map_) == i) return nullptr;
    return i->second;
}

void Client::quit(Lock &)
{
    TRACE_L1("%p", this);

    if(!dbus_.mainLoop) return;
    if(!g_main_loop_is_running(dbus_.mainLoop)) return;
    g_main_loop_quit(dbus_.mainLoop);
}

Client::Client()
{
    TRACE_L1("%p", this);
}

Client::~Client()
{
    TRACE_L1("%p", this);
    quit();
}

void Client::quit()
{
    TRACE_L1("%p", this);

    std::unique_lock<std::mutex> lock(mutex_);
    quit(lock);
}

bool Client::bind(SharedInterface interface)
{
    CHECK(interface);
    CHECK(!interface->id().empty());

    std::unique_lock<std::mutex> lock{mutex_};

    CHECK(0u == map_.count(interface->id()));

    if(
        !interface
        || interface->id().empty()
        || 0u != map_.count(interface->id()))
    {
        TRACE_L1("failed to bind: %s(%p)", interface->id().c_str(), interface.get());
        return false;
    }

    TRACE_L1("bound: %s(%p)", interface->id().c_str(), interface.get());
    auto id = interface->id();
    map_.insert({std::move(id), std::move(interface)});
    return true;
}

bool Client::unbind(SharedInterface interface)
{
    CHECK(interface);
    CHECK(!interface->id().empty());

    std::unique_lock<std::mutex> lock{mutex_};

    if(
        !interface
        || interface->id().empty()
        || !map_.count(interface->id()))
    {
        TRACE_L1(
            "invalid/not bound: %s(%p)",
            interface ? interface->id().c_str() : "", interface.get());
        return true;
    }

    const auto r = map_.erase(interface->id());

    if(r) TRACE_L1("unbound: %s(%p)", interface->id().c_str(), interface.get());
    return r;
}

#include <algorithm>

#include "AWCContainerAdministrator.h"
#include "AWCImplementation.h"
#include "AWCProxyContainer.h"
#include "processcontainers/Tracing.h"

using namespace Thunder::ProcessContainers;
using namespace Thunder::Core;


PluginConfig::PluginConfig(const std::string &path):
    JSON::Container()
{
    Core::JSON::String useProxyValue;
    Core::JSON::DecUInt32 startTimeoutValue{10 * 1000};
    Core::JSON::DecUInt32 stopTimeoutValue{10 * 1000};

    Add(_T("awc_container"), &useProxyValue);
    Add(_T("start_timeout"), &startTimeoutValue);
    Add(_T("stop_timeout"), &stopTimeoutValue);

    if(path.empty()) return;

    OptionalType<JSON::Error> error;
    FromString(path, error);

    if(error.IsSet())
    {
        TRACE_L1(
                "Parsing %s failed with %s",
                path.c_str(), ErrorDisplayMessage(error.Value()).c_str());
    }

    using namespace std::chrono;

    if(useProxyValue == _T("proxy")) useProxy = true;
    startTimeout = milliseconds{startTimeoutValue.Value()};
    stopTimeout = milliseconds{stopTimeoutValue.Value()};

    Remove(_T("awc_container"));
    Remove(_T("start_timeout"));
    Remove(_T("stop_timeout"));
}

AWCContainerAdministrator::AWCContainerAdministrator()
    : BaseContainerAdministrator()
{
    TRACE_L1("%p", this);
    awcClient_ = awc::AWCClient::getInstance();
    if (awcClient_) {
        awcClientListener_ = std::make_shared<AWCListener>(this);
        awcClient_->setListener(awcClientListener_);
    }
}

AWCContainerAdministrator::~AWCContainerAdministrator()
{
    TRACE_L1("%p", this);
    if (awcClient_ && awcClientListener_) {
        awcClient_->removeListener(awcClientListener_);
    }
}

IContainer* AWCContainerAdministrator::Container(
    const string& name,
    IStringIterator& searchpaths,
    const string& containerLogDir,
    const string& configuration)
{
    std::string config = configuration;

    // logs dont like multi line strings
    std::replace_if(
            std::begin(config), std::end(config),
        [](char c){return '\n' == c;}, ' ');

    TRACE_L1(
        "(%p) callsign=%s, logDir=%s, config=%s",
        this, name.c_str(), containerLogDir.c_str(), config.c_str());

    const PluginConfig cfg{configuration};

    IContainer* container = Get(name);
    const auto append = !container;

    if(!container && cfg.useProxy)
    {
        container = new AWCProxyContainer(name, cfg, &dbusClient_);
    }
    else if(!container)
    {
        container = new AWCContainer(name, awcClient_, this);
    }
    if(append)
    {
        this->InternalLock();
        InsertContainer(container);
        this->InternalUnlock();
    }

    if(!container) TRACE_L1("not supported container type: %s", name.c_str());
    return container;
}

IContainerAdministrator& IContainerAdministrator::Instance()
{
    static AWCContainerAdministrator& myAWCContainerAdministrator = Core::SingletonType<AWCContainerAdministrator>::Instance();
    return myAWCContainerAdministrator;
}

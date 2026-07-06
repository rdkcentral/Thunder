#include <algorithm>

#include "AWCContainerAdministrator.h"
#include "AWCImplementation.h"
#include "AWCProxyContainer.h"

#include "processcontainers/ContainerProducer.h"

namespace Thunder {
namespace ProcessContainers {

using namespace Core;

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
{
    TRACE_L1("%p", this);
    awcClient_ = awc::AWCClient::getInstance();
}

AWCContainerAdministrator::~AWCContainerAdministrator()
{
    TRACE_L1("%p", this);
}

uint32_t AWCContainerAdministrator::Initialize(const string& configuration VARIABLE_IS_NOT_USED)
{
    uint32_t result = Core::ERROR_GENERAL;

    if (awcClient_) {
        awcClientListener_ = std::make_shared<AWCListener>(this);
        awcClient_->setListener(awcClientListener_);
        result = Core::ERROR_NONE;
    }

    return (result);
}

void AWCContainerAdministrator::Deinitialize()
{
    if (awcClient_ && awcClientListener_) {
        awcClient_->removeListener(awcClientListener_);
        awcClientListener_.reset();
    }
}

Core::ProxyType<IContainer> AWCContainerAdministrator::Container(
    const string& name,
    IStringIterator& searchpaths VARIABLE_IS_NOT_USED,
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

    Core::ProxyType<IContainer> container;

    if(cfg.useProxy) {
        container = ContainerAdministrator::Instance().Create<AWCProxyContainer>(name, cfg, &dbusClient_);
    }
    else {
        container = ContainerAdministrator::Instance().Create<AWCContainer>(name, awcClient_, this);
    }

    if(container.IsValid() == false) {
        TRACE_L1("not supported container type: %s", name.c_str());
    }

    return container;
}

// FACTORY REGISTRATION
static ContainerProducerRegistrationType<AWCContainerAdministrator, IContainer::containertype::AWC> registration;

} // namespace ProcessContainers
}

#pragma once
#include "Module.h"

#include "IPluginStarter.h"

using namespace WPEFramework;

/**
 * @brief COM-RPC implementation of a plugin starter
 *
 * Connects to Thunder over COM-RPC and attempts to start a given plugin
 */
class COMRPCStarter : public IPluginStarter
{
public:
    explicit COMRPCStarter(const std::string& pluginName);
    ~COMRPCStarter() override;

    bool activatePlugin(const int maxRetries, const int retryDelayMs) final;

private:
    Core::NodeId getConnectionEndpoint() const;

private:
    const std::string mPluginName;

    Core::ProxyType<RPC::InvokeServerType<1, 0, 4>> mEngine;
    Core::ProxyType<RPC::CommunicatorClient> mClient;
};
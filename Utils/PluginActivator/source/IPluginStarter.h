#pragma once

#include <string>

/**
 * Interface to start a specified plugin
 *
 * Could be implemented with JSON-RPC or COM-RPC
 */
class IPluginStarter
{
public:
    virtual ~IPluginStarter() {};

    /**
     * @brief Activate a Thunder plugin
     *
     * Will block until either the Thunder plugin has activated or until the maximum amount of retires has occurred
     *
     * @param[in]   maxRetries          Maximum amount of attempts to activate the plugin - if plugin is not activated within the amount of retries
     *                                  this method will return false
     * @param[in]   retryDelayMs        Amount of time to wait after a failed activation before retrying again
     */
    virtual bool activatePlugin(const int maxRetries, const int retryDelayMs) = 0;
};
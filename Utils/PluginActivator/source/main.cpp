/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "COMRPCStarter.h"
#include "COMRPCMonitor.h"
#include "Module.h"
#include "Log.h"

#include <memory>

static int gMonitorPlugin = 0;
static int gDeactivatePluginOnExit = 0;
static int gRetryCount = 3;
static int gRetryDelayMs = 3000;
static string gPluginName;
JsonObject gConfigOptions;
Core::Event gSync(false, true);

static void signalHandler(int sig, siginfo_t *info, void *)
{
    fprintf(stderr, "!!! Received signal %d, unblocking main thread !!!\n", sig);
    gSync.Unlock();
}

static bool installHandlerForSignal(int signal)
{
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, signal);

    action.sa_sigaction = &signalHandler;
    action.sa_flags = SA_SIGINFO;

    return !sigaction(signal, &action, 0);
};

static inline bool isLongOption(const char *arg)
{
    return arg && strncmp(arg, "--", 2) == 0;
}

struct JSONConfig
{
    static bool fromString(const char *str, JsonObject &obj) {
        return obj.IElement::FromString(str);
    }

    static bool fromFile(const char *file, JsonObject &obj) {
        Core::File fObj(file);
        if (!fObj.Open(true))
            return false;
        return obj.IElement::FromFile(fObj);
    }
};

/**
 * @brief Display a help message for the tool
 */
static void displayUsage()
{
    printf("Usage: PluginActivator <option(s)> [callsign]\n");
    printf("    Utility that starts a given thunder plugin\n\n");
    printf("    -h, --help              Print this help and exit\n");
    printf("    -r, --retries           Maximum amount of retries to attempt to start the plugin before giving up\n");
    printf("    -d, --delay             Delay (in ms) between each attempt to start the plugin if it fails\n");
    printf("    -c, --config-string     JSON string to override plugin configuration\n");
    printf("    --config-file           JSON config file to override plugin configuration\n");
    printf("    --monitor               Monitor plugin and try to re-activate it on failure/crash \n");
    printf("    --nomonitor             Do no monitor plugin\n");
    printf("    --deactivate-on-exit    Deactivate plugin on exit of this utility, applicable only with --monitor option\n");
    printf("\n");
    printf("    [callsign]          Callsign of the plugin to activate (Required)\n");
    printf("\n");
    printf("Example Uses:\n");
    printf("PluginActivator HtmlApp\n");
    printf("PluginActivator --config-string \"{\\\"environmentvariable\\\":[{\\\"name\\\":\\\"GST_DEBUG\\\",\\\"value\\\":\\\"westeros:*\\\"}]}\" HtmlApp\n");
    printf("PluginActivator --config-file /tmp/override.json HtmlApp\n");
    printf("PluginActivator -- --localstorageenabled true HtmlApp\n");
    printf("PluginActivator --monitor HtmlApp\n");
    printf("PluginActivator --monitor -- --localstorageenabled true --url 'http://www.google.com' HtmlApp\n");
    printf("PluginActivator --monitor --config-file override.json -- --localstorageenabled true HtmlApp\n");
    printf("PluginActivator --monitor --deactivate-on-exit --config-file override.json -- --localstorageenabled true HtmlApp\n");
    printf("LOG_LEVEL=4 ./PluginActivator --monitor --deactivate-on-exit HtmlApp\n");
    printf("LOG_LEVEL=5 ./PluginActivator --monitor --deactivate-on-exit -- --localstorageenabled true HtmlApp\n");
}

/**
 * @brief Parse the provided command line arguments
 *
 * Must be given the name of the plugin to activate, everything else
 * is optional and will fallback to sane defaults
 */
static void parseArgs(const int argc, char** argv)
{
    if (argc == 1) {
        displayUsage();
        exit(EXIT_SUCCESS);
    }

    struct option longopts[] = {
        { "help", no_argument, nullptr, (int)'h' },
        { "monitor", no_argument, &gMonitorPlugin, 1 },
        { "nomonitor", no_argument, &gMonitorPlugin, 0 },
        { "retries", required_argument, nullptr, (int)'r' },
        { "delay", required_argument, nullptr, (int)'d' },
        { "config-string", required_argument, nullptr, (int)'c' },
        { "config-file", required_argument, nullptr, (int)'f' },
        { "deactivate-on-exit", no_argument, &gDeactivatePluginOnExit, (int)1 },
        { nullptr, 0, nullptr, 0 }
    };

    opterr = 0;

    int option;
    int longindex;

    while ((option = getopt_long(argc, argv, "hc:f:r:d:", longopts, &longindex)) != -1) {
        switch (option) {
        case 0:
        case 1:
            break;
        case 'h':
            displayUsage();
            exit(EXIT_SUCCESS);
            break;
        case 'r':
            gRetryCount = std::atoi(optarg);
            if (gRetryCount < 0) {
                fprintf(stderr, "Error: Retry count must be > 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            gRetryDelayMs = std::atoi(optarg);
            if (gRetryDelayMs < 0) {
                fprintf(stderr, "Error: Delay ms must be > 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'c':
        case 'f':
            {
                JsonObject options;
                if ((option == 'c') ? JSONConfig::fromString(optarg, options) : JSONConfig::fromFile(optarg, options)) {
                    auto iterator = options.Variants();
                    while (iterator.Next())
                        gConfigOptions[iterator.Label()] = iterator.Current();
                    printf("Read configurations %d, %d\n", options.Size(), gConfigOptions.Size());
                } else {
                    fprintf(stderr, "Error: Couldn't parse config string, ignoring values");
                }
            }
            break;
        case '?':
            if (optopt == 'c')
                fprintf(stderr, "Warning: Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Warning: Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Warning: Unknown option character `\\x%x'.\n", optopt);

            exit(EXIT_FAILURE);
            break;
        default:
            exit(EXIT_FAILURE);
            break;
        }
    }

    while ((1 + optind) < argc) {
        if (isLongOption(argv[optind + 0]) && !isLongOption(argv[optind + 1])) {
            const char *opt = argv[optind + 0];
            const char *val = argv[optind + 1];
            gConfigOptions[&opt[2]] = val;
            optind += 2;
        } else {
            printf("Unrecognized extra arg : %s, @ %d\n", argv[optind + 0], optind);
            break;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Error: Must provide plugin name to activate\n");
        exit(EXIT_FAILURE);
    }

    gPluginName = argv[optind];

    optind++;
    for (int i = optind; i < argc; i++) {
        printf("Warning: Non-option argument %s ignored\n", argv[i]);
    }
}

int main(int argc, char* argv[])
{
    installHandlerForSignal(SIGINT);
    installHandlerForSignal(SIGTERM);

    initLogging();

    parseArgs(argc, argv);

    // For now, we only implement the starter in COM-RPC but could do a JSON-RPC version
    // in the future
    bool success = false;
    Core::ProxyType<COMRPCStarter> starter;
    COMRPCStarterAndMonitor *monitor = nullptr;

    if (gMonitorPlugin) {
        starter = Core::ProxyType<COMRPCStarterAndMonitor>::Create(gPluginName);
        monitor = dynamic_cast<COMRPCStarterAndMonitor*>(starter.operator->());

        monitor->configureMonitor(gRetryCount, gRetryDelayMs);
        monitor->onReactivationFailure([&]() {
            success = false;
            gSync.Unlock();
        });
    } else {
        starter = Core::ProxyType<COMRPCStarter>::Create(gPluginName);
    }

    if (starter.IsValid()) {
        starter->setConfigOverride(std::move(gConfigOptions));

        success = starter->activatePlugin(gRetryCount, gRetryDelayMs);
        if (success) {
            if (gMonitorPlugin) {
                LOG_INF(gPluginName.c_str(), "Started Monitoring Plugin");
                gSync.Lock();

                if (gDeactivatePluginOnExit) {
                    // Avoid re-activating the plugin hereafter
                    monitor->configureMonitor(0, 0);

                    LOG_INF(gPluginName.c_str(), "Utility was interrupted, proceeding to deactivate plugin");
                    success = starter->deactivatePlugin();
                    if (!success)
                        LOG_ERROR(gPluginName.c_str(), "Error, Failed to deactivate plugin\n");
                }
            }
        } else {
            LOG_ERROR(gPluginName.c_str(), "Error: Failed to activate plugin\n");
        }

        starter.Release();
    } else {
        LOG_ERROR(gPluginName.c_str(), "Error: Unable to create starter instance\n");
    }

    // Destruct the COM-RPC starter so it cleans up after itself before we dispose WPEFramework singletons
    Core::Singleton::Dispose();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

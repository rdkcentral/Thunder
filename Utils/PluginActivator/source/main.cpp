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
#include "Module.h"
#include "Log.h"

#include <memory>

static int gDeactivatePlugin = 0;
static int gRetryCount = 3;
static int gRetryDelayMs = 3000;
static string gPluginName;
JsonObject gConfigOptions;

/**
 * @brief Check given string starts with a '--' to denote they are long options
 */
static inline bool isLongOption(const char *arg)
{
    return arg && strncmp(arg, "--", 2) == 0;
}

/*
 * @brief A wrapper to convert string / a file to JsonObject
 */
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
    printf("    -k, --deactivate        Deactivate given plugin\n");
    printf("    -c, --config-string     JSON string to override plugin configuration\n");
    printf("    --config-file           JSON config file to override plugin configuration\n");
    printf("\n");
    printf("    [callsign]          Callsign of the plugin to activate (Required)\n");
    printf("\n");
    printf("Example Uses:\n");
    printf("PluginActivator HtmlApp\n");
    printf("PluginActivator --config-string \"{\\\"environmentvariable\\\":[{\\\"name\\\":\\\"GST_DEBUG\\\",\\\"value\\\":\\\"westeros:*\\\"}]}\" HtmlApp\n");
    printf("PluginActivator --config-file /tmp/override.json HtmlApp\n");
    printf("PluginActivator -- --localstorageenabled true HtmlApp\n");
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
        { "retries", required_argument, nullptr, (int)'r' },
        { "delay", required_argument, nullptr, (int)'d' },
        { "config-string", required_argument, nullptr, (int)'c' },
        { "config-file", required_argument, nullptr, (int)'f' },
        { "deactivate", no_argument, nullptr, (int)'k' },
        { nullptr, 0, nullptr, 0 }
    };

    opterr = 0;

    int option;
    int longindex;

    while ((option = getopt_long(argc, argv, "hkc:f:r:d:", longopts, &longindex)) != -1) {
        switch (option) {
        case 'k':
            gDeactivatePlugin = 1;
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
                if (gDeactivatePlugin)
                    break;

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
    initLogging();

    parseArgs(argc, argv);

    // For now, we only implement the starter in COM-RPC but could do a JSON-RPC version
    // in the future
    bool success = false;
    std::unique_ptr<COMRPCStarter> starter(new COMRPCStarter(gPluginName));

    if (!gDeactivatePlugin) {
        starter->setConfigOverride(std::move(gConfigOptions));
        success = starter->activatePlugin(gRetryCount, gRetryDelayMs);
    } else {
        success = starter->deactivatePlugin(gRetryCount, gRetryDelayMs);
    }

    if (!success) {
        LOG_ERROR(gPluginName.c_str(), "Error: Failed to %s plugin\n", (gDeactivatePlugin ? "deactivate" : "activate"));
    }

    starter.reset();

    // Destruct the COM-RPC starter so it cleans up after itself before we dispose WPEFramework singletons
    Core::Singleton::Dispose();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

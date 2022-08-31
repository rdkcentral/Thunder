#include "Module.h"
#include "COMRPCStarter.h"

#include <memory>

static int gRetryCount = 100;
static int gRetryDelayMs = 500;
static std::string gPluginName;

/**
 * @brief Display a help message for the tool
 */
static void displayUsage()
{
    printf("Usage: PluginActivator <option(s)> [callsign]\n");
    printf("    Utility that starts a given thunder plugin\n\n");
    printf("    -h, --help          Print this help and exit\n");
    printf("    -r, --retries       Maximum amount of retries to attempt to start the plugin before giving up\n");
    printf("    -d, --delay         Delay (in ms) between each attempt to start the plugin if it fails\n");
    printf("\n");
    printf("    [callsign]          Callsign of the plugin to activate (Required)\n");
}

/**
 * @brief Parse the provided command line arguments
 *
 * Must be given the name of the plugin to activate, everything else
 * is optional and will fallback to sane defaults
 */
static void parseArgs(const int argc, char **argv)
{
    if (argc == 1)
    {
        displayUsage();
        exit(EXIT_SUCCESS);
    }

    struct option longopts[] =
        {
            {"help", no_argument, nullptr, (int)'h'},
            {"retries", required_argument, nullptr, (int)'r'},
            {"delay", required_argument, nullptr, (int)'d'},
            {nullptr, 0, nullptr, 0}};

    opterr = 0;

    int c;
    int longindex;

    while ((c = getopt_long(argc, argv, "hr:d:", longopts, &longindex)) != -1)
    {
        switch (c)
        {
        case 'h':
            displayUsage();
            exit(EXIT_SUCCESS);
            break;
        case 'r':
            gRetryCount = std::atoi(optarg);
            if (gRetryCount < 0)
            {
                fprintf(stderr, "Error: Retry count must be > 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            gRetryDelayMs = std::atoi(optarg);
            if (gRetryDelayMs < 0)
            {
                fprintf(stderr, "Error: Delay ms must be > 0\n");
                exit(EXIT_FAILURE);
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

    if (optind == argc)
    {
        fprintf(stderr, "Error: Must provide plugin name to activate\n");
        exit(EXIT_FAILURE);
    }

    gPluginName = argv[optind];

    optind++;
    for (int i = optind; i < argc; i++)
    {
        printf("Warning: Non-option argument %s ignored\n", argv[i]);
    }
}

int main(int argc, char *argv[])
{
    parseArgs(argc, argv);

    // For now, we only implement the starter in COM-RPC but could do a JSON-RPC version
    // in the future
    std::unique_ptr<IPluginStarter> starter = std::make_unique<COMRPCStarter>(gPluginName);
    if (starter->activatePlugin(gRetryCount, gRetryDelayMs))
    {
        return EXIT_SUCCESS;
    }
    else
    {
        return EXIT_FAILURE;
    }
}
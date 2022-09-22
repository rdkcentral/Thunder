# Plugin Activator
A command-line tool to activate a specified Thunder plugin.

Will automatically retry activation mutliple times if the plugin does not successfully activate.

Designed to be used with systemd where each plugin becomes a standalone systemd service, activated with this tool.

## Usage
```
Usage: PluginActivator <option(s)> [callsign]
    Utility that starts a given thunder plugin

    -h, --help          Print this help and exit
    -r, --retries       Maximum amount of retries to attempt to start the plugin before giving up
    -d, --delay         Delay (in ms) between each attempt to start the plugin if it fails

    [callsign]          Callsign of the plugin to activate (Required)
```
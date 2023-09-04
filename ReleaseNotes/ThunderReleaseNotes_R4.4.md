# Thunder Release Notes R4.4

## introduction

This document describes the changes introduced in Thunder R4.4 compared to R4.3.
See [here](https://github.com/rdkcentral/Thunder/blob/master/ReleaseNotes/ThunderReleaseNotes_R4_Thunder.pdf) for the release notes of Thunder R4.3 (which lists all changes in Thunder R4 up to R4.3).

## changes in R4.4

### General bug fixes

A lot of issues were fixed in Thunder R4.4 improving stability and resource usage. Certainly the message engine (which handles Tracing, logging and waning reporting) has seen significant improvements. 

### Message Engine and Warning Reporting

Warning reporting (introduced in Thunder R3) is now also fully handled by the message engine. Besides benefiting from the general improvements introduced by the message engine this means that warning reporting now has the advantage it can be controlled and configured with all the options and consistency introduced by the message engine. 

A Thunder commandline parameter was introduced to have the message output (so Tracing, logging, warning reporting) go directly to stdout (-f or -F depending on the desired output format) in case the message control plugin is not being used or not active. Note that when this is used in combination with the message control plugin multiple output of the same message could be observed.

### Secure Proxy/Stubs

A first version of secure proxy/stubs is introduced in R4.4.
This feature when turned on adds checks to the generated COMRPC proxy/stubs to detect and prevent the handling of incorrect, or perhaps malicious, data sent over a COMRPC socket which could lead to incorrect behaviour or even crashes.
As this feature will introduce additional checks which will have an influence on performance and resource usage it needs to be activated (build flags PROXYSTUB_GENERATOR_ENABLE_SECURITY and PROXYSTUB_GENERATOR_ENABLE_COHERENCY; note these are separate options as the latter has a bigger influence)

Note in R4.4 a first iteration of secure proxy/stubs it introduced. In future releases it will be extended and improved.

### COM channel per Plugin

It is now possible to have a separate COM channel per Plugin. This is for example convenient for security purposes where one could limit the access to the Thunder Communicator port for example only to the root user but do allow access to a specific plugin for a different user. This can be configured using the communicator option in the Plugin config file, documented [here](https://rdkcentral.github.io/Thunder/plugin/config/#default-options). 

### WebSocket open improvements

Now the state change notification "Socket open" for a WebSocket is only triggered when the full upgrade is completed. Previously it could be triggered before that happened.

### Secure Socket improvements

The secure socket implementation has been improved and is now more in line with the https standard. Proper headers are supported and validation has been improved with an added custom validator option.

### Postmortem improvements

The postmortem handling for Thunder has been improved in case of crashes. Now also the running jobs in the WorkerPool will be logged with details for all JSON and COM RPC jobs.






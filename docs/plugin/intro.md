A Thunder plugin in its most basic sense is a small, self-contained C++ library that implements a specific interface. This interface can then be accessed and invoked by other applications over an RPC communications channel (either [JSON-RPC](../../introduction/architecture/rpc/jsonrpc) or [COM-RPC](../../introduction/architecture/rpc/comrpc)). These applications could be native C++ applications, or web apps running in a browser.

Each plugin should be responsible for a different piece of business functionality, and can be enabled/disabled at runtime. Plugins can communicate with each other if required, although it is recommended to try and avoid this where possible to avoid overly interconnected interfaces.

Examples of common STB/TV functionality that could be implemented in a Thunder plugin:

* Network and WiFi management
* HDMI input control
* Device maintenance tasks

As well as performing actions when invoked, plugins can also be used for running periodic background tasks that do not require user interaction such as housekeeping, software download and updates. Plugin interfaces can define notifications that can be triggered on events to allow for event-driven programming instead of relying on less-efficient polling techniques.

Apart from the Controller plugin, Thunder does not come with any plugins by default. Some reference plugins are available in the `ThunderNanoServices` repository, but there is no requirement to use them.

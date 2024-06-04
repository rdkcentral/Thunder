## Run Thunder

To launch Thunder, execute the `Thunder` process from a command line.

If you have installed Thunder in a non-standard directory on Linux, you will need to set a few environment variables. If Thunder is installed in the standard system directories (e.g. `/usr/bin`) then you can just launch it directly

```shell
$ export LD_LIBRARY_PATH=${PWD}/install/usr/lib:${LD_LIBRARY_PATH}
$ export PATH=${PWD}/install/usr/bin:${PATH}

$ Thunder -f -c ${PWD}/install/etc/Thunder/config.json
```

### Available command-line options

* `-c`: Path to the config file to use
* `-b`: Daemonise Thunder and run in the background
* `-f`: Flush all trace/logging/warning reporting messages directly to the console without any abbreviation
* `-F`: Flush all trace/logging/warning reporting messages directly to the console with some abbreviation
* `-h`: Show usage/help message

For general debugging, it is useful to pass the `-f` option to flush the plugin log/trace messages to the console. Alternatively build and install the MessageControl plugin, which would then take responsibility for handling the messages and processing them.

## Basic Usage

By default, Thunder will listen on 2 HTTP endpoints:

* `http://<interface>:<port>/Service` (HTTP web requests)
* `http://<interface>:<port>/jsonrpc` (JSON-RPC requests)

To test if Thunder is working, make a GET request to the JSON-RPC endpoint. This will return some information about the state of the system including:

* Loaded plugins and their activation state
* Open connection channels
* Running threads

```json
{
    "plugins": [
        {
            "callsign": "Controller",
            "classname": "Controller",
            "configuration": {},
            "startmode": "Activated",
            "state": "activated",
            "observers": 0,
            "module": "Application",
            "version": {
                "major": 1,
                "minor": 0,
                "patch": 0
            }
        }
    ],
    "channel": [
        {
            "remote": "127.0.0.1",
            "state": "WebServer",
            "activity": true,
            "id": 2
        }
    ],
    "server": {
        "threads": [
            {
                "id": "0x7FFFF44B1640",
                "job": "WorkerPool::Timer",
                "runs": 2
            },
            ...
        ]
    }
}
```

### JSON-RPC

The main way to interact with Thunder is JSON-RPC requests. Thunder is JSON-RPC 2.0 compliant - read more about the JSON-RPC specification [here](https://www.jsonrpc.org/specification).

#### JSON-RPC over HTTP

To make a JSON-RPC call to Thunder, make a `POST` request to the `/jsonrpc` endpoint, where the body of the request contains the JSON-RPC object. For this example, curl is used to make the request - but use whatever API testing tool you are familiar with ([Postman](https://www.postman.com/) and [Insomnia](https://insomnia.rest/) are popular choices)

:arrow_right: Request

```shell
$ curl -H "Content-Type: application/json" -X POST \
-d '{"jsonrpc":"2.0","id":1,"method":"Controller.1.activate","params":{"callsign":"TestPlugin"}}' \
http://localhost:55555/jsonrpc
```

:arrow_left: Response

```json
{"jsonrpc":"2.0","id":1,"result":null}
```

#### JSON-RPC over Web Sockets

If possible, it is recommended to use JSON-RPC via a websocket connection. This is more efficient for multiple requests (no need to repeatedly open/close a HTTP connection) and supports notifications/events from plugins.

For this example, [wscat](https://github.com/websockets/wscat) is used to connect to Thunder but again, use whatever websocket testing tool you are familiar with

```shell
$ wscat -c ws://127.0.0.1:55555/jsonrpc
Connected (press CTRL+C to quit)
> {"jsonrpc":"2.0","id":1,"method":"Controller.1.activate","params":{"callsign":"TestPlugin"}}
< {"jsonrpc":"2.0","id":1,"result":null}
```

### Controller Plugin

The Controller plugin is the central plugin in Thunder for managing plugins. It is the only plugin always included as part of a Thunder build, and the code is part of the core Thunder repository. Using this plugin you can activate/deactivate plugins, check their status, suspend or resume compatible plugins and manage plugin configuration.

It provides JSON-RPC APIs for managing plugins

- **Activate** – start a plugin
- **Deactivate** – stop a plugin
- **Unavailable** – mark a plugin as unavailable (useful if the plugin requires libraries/assets that will be downloaded & installed at a later time)
- **Suspend** – put a plugin into suspend mode (if supported)
- **Resume** – resume a previously suspended plugin (if supported)
- **Status** – get the status of Thunder or a particular plugin
- **Configuration** – get/set the configuration of a plugin
- **Hibernate** – checkpoint the state of the plugin to disk to free memory (if supported)

If a plugin is configured as auto-start, then it will automatically be activated when Thunder starts. Otherwise it is another applications responsibility to call the Controller plugin and activate the plugin manually. If a plugin is deactivated, it won't respond to any API requests.

## Systemd Service

An example systemd service to run Thunder daemon is below. This could be used to run Thunder on system startup automatically.

```ini
[Unit]
Description=thunder
Wants=multi-user.target
After=multi-user.target

[Service]
PIDFile=/var/run/Thunder.pid
Environment="WAYLAND_DISPLAY=wayland-0"
Environment="XDG_RUNTIME_DIR=/run"
ExecStart=-/usr/bin/Thunder -b
ExecStop=/bin/kill $MAINPID

[Install]
WantedBy=multi-user.target
```

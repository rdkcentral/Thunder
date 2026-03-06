---
applyTo: '**'
---

# 11 — Configuration and Metadata

> Thunder configuration is hierarchical and token-based. Never hardcode absolute paths. Never read config outside of `Initialize()`.

## Configuration Hierarchy

```
install/etc/Thunder/config.json          ← Daemon global config (one file)
install/etc/Thunder/plugins/             ← Per-plugin config directory
    MyPlugin.json
    NetworkControl.json
    ...
    MyPlugin.json { "configuration": { ... } }   ← Plugin-specific config object
```

The daemon reads its own `config.json` once at startup. Plugin config files are read when the plugin activates. The `configs` field in `config.json` points to the plugin config directory.

## Daemon `config.json` — Key Fields

```json
{
  "binding": "127.0.0.1",
  "port": 55555,
  "communicator": "/tmp/communicator",
  "persistentpath": "/opt/persistent/thunder/",
  "datapath": "/usr/share/Thunder/",
  "systempath": "/usr/lib/thunder/",
  "proxystubpath": "/usr/lib/thunder/proxystubs/",
  "volatilepath": "/tmp/",
  "configs": "/etc/Thunder/plugins/",
  "postmortempath": "/opt/logs/",
  "exitreasons": ["WATCHDOG", "FAILURE"],
  "defaultmessagingcategories": [
    { "category": "Trace", "module": "SysLog", "enabled": true }
  ],
  "observe": {
    "configpath": true,
    "proxystubpath": true
  }
}
```

**New daemon-level config options must be added to `Source/Thunder/Config.h`** as `Core::JSON::Container` members — never as environment variables or compile-time constants.

## Path Substitution Tokens

These tokens are resolved by `Config::Substitute()` at runtime. They must be used in all plugin config path strings — **never hardcode absolute paths**.

| Token | Resolves to | `config.json` field |
|-------|-------------|---------------------|
| `%datapath%` | Read-only data directory | `datapath` |
| `%persistentpath%` | Writable persistent storage | `persistentpath` |
| `%volatilepath%` | Temporary / volatile storage | `volatilepath` |
| `%systempath%` | System library path | `systempath` |
| `%proxystubpath%` | Proxy stub library path | `proxystubpath` |

Plugin config usage:
```json
{
  "configuration": {
    "databasepath": "%persistentpath%mydb/",
    "resourcepath": "%datapath%assets/"
  }
}
```

Plugin code retrieval:
```cpp
// Use service path helpers — they resolve tokens automatically
string dbPath = service->PersistentPath() + _T("mydb/");
string dataPath = service->DataPath() + _T("assets/");
string volatilePath = service->VolatilePath();
```

**Never** use `service->ConfigLine()` for path construction — it returns raw JSON. Use `service->DataPath()`, `service->PersistentPath()`, `service->VolatilePath()` for derived paths.

## Plugin Config JSON File — Full Schema

```json
{
  "callsign": "MyPlugin",
  "classname": "MyPlugin",
  "locator": "libThunderMyPlugin.so",
  "autostart": true,
  "startuporder": 50,
  "precondition": ["NETWORK", "TIME"],
  "termination": ["NOT_NETWORK"],
  "configuration": {
    "retries": 3,
    "endpoint": "127.0.0.1:8080",
    "datapath": "%datapath%mydata/"
  },
  "root": {
    "mode": "Local",
    "threads": 4,
    "stacksize": 0,
    "startup": 3
  }
}
```

| Field | Type | Required | Default | Meaning |
|-------|------|---------|---------|---------|
| `callsign` | string | Yes | — | Unique plugin identifier; used in JSON-RPC URLs (`/MyPlugin/method`) |
| `classname` | string | Yes | — | C++ class name for `PLUGINHOST_ENTRY_POINT` lookup |
| `locator` | string | Yes | — | Shared library filename |
| `autostart` | bool | No | `false` | Activate at daemon startup (after preconditions are met) |
| `startuporder` | uint | No | `50` | Activation order among autostart plugins — lower = earlier |
| `precondition` | array | No | `[]` | Subsystem names that must all be active before activation |
| `termination` | array | No | `[]` | `NOT_*` subsystem names — any going inactive triggers deactivation |
| `configuration` | object | No | `{}` | Plugin-specific config; accessed via `service->ConfigLine()` |
| `root` | object | No | absent | OOP execution config; absence means in-process |

## `startuporder` — Activation Sequencing

Among autostart plugins, lower `startuporder` values activate first. Plugins with the same value activate in unspecified order (relative to each other). Use `startuporder` when plugin A must be active before plugin B starts.

Typical values:
- `10` — fundamental infrastructure plugins (e.g. SecurityAgent)
- `30` — subsystem setters (e.g. PlatformPlugin setting PLATFORM subsystem)
- `50` — default for most plugins
- `80` — plugins that depend on many subsystems

## Precondition and Termination Config

```json
{
  "precondition": ["NETWORK", "TIME"],
  "termination": ["NOT_NETWORK"]
}
```

- `precondition` uses **AND** logic: all listed subsystems must be active before the plugin transitions from `PRECONDITION` to `ACTIVATED`.
- `termination` uses **NOT_ prefix + OR** logic: if any listed `NOT_*` condition is met (i.e., the named subsystem goes inactive), the plugin deactivates.
- Subsystem names are the enum value names from `ISubSystem` without the `ISubSystem::` prefix.

## OOP Execution Config (`root` block)

```json
{
  "root": {
    "mode": "Local",
    "threads": 4,
    "stacksize": 0,
    "startup": 3
  }
}
```

| Field | Values | Meaning |
|-------|--------|---------|
| `mode` | `Local`, `Container`, `Distributed` | Execution environment |
| `threads` | uint | Number of WorkerPool threads in the OOP process |
| `stacksize` | uint | Stack size per thread (0 = OS default) |
| `startup` | uint | Startup order within the OOP host process |

When a `root` block is absent, the plugin runs in-process in the main Thunder daemon.

## Reading Config in Plugins

```cpp
class Config : public Core::JSON::Container {
private:
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

public:
    Config()
        : Core::JSON::Container()
        , Retries(3)
        , Endpoint(_T(""))
    {
        Add(_T("retries"), &Retries);
        Add(_T("endpoint"), &Endpoint);
    }

    Core::JSON::DecUInt32 Retries;
    Core::JSON::String Endpoint;
};

const string Initialize(PluginHost::IShell* service) override
{
    Config config;
    config.FromString(service->ConfigLine());  // parse the "configuration" object

    _retries = config.Retries.Value();
    _endpoint = config.Endpoint.Value();
    return {};
}
```

**Read config only in `Initialize()`** — never in constructors, destructors, or static initializers.

## Hot-Reload Behavior

| What changes | Behavior |
|-------------|---------|
| Plugin config `.json` file (in `configs/` dir) | `ConfigObserver` detects change → plugin is deactivated, config updated, re-activated (if autostart) |
| Proxy stub `.so`/`.dylib` added to `proxystubpath` | Hot-loaded immediately — stubs registered without daemon restart |
| Proxy stub `.so`/`.dylib` removed from `proxystubpath` | Ignored — stubs are **never** unloaded at runtime (use-after-free risk) |
| Daemon `config.json` | **Not** hot-reloaded — daemon restart required |

To force a plugin config refresh without touching the daemon: `touch install/etc/Thunder/plugins/MyPlugin.json`.

## `BUILD_REFERENCE` — Version Traceability

Pass the git commit hash at CMake configure time:
```bash
-DBUILD_REFERENCE=$(git -C Thunder rev-parse HEAD)
```

This is compiled into the binary and surfaced via the Controller's `/status` endpoint. It is mandatory for production builds — omitting it makes version tracing impossible in deployed systems.

## Plugin Config Template (`.conf.in`)

Plugin config files are templated by `ConfigGenerator`. The `.conf.in` file contains CMake-variable substitution placeholders:

```json
{
  "callsign": "MyPlugin",
  "classname": "@PLUGIN_MYPLUGIN_CLASSNAME@",
  "locator": "lib@PLUGIN_MYPLUGIN_LOCATOR@.so",
  "autostart": @PLUGIN_MYPLUGIN_AUTOSTART@,
  "startuporder": @PLUGIN_MYPLUGIN_STARTUPORDER@,
  "configuration": {
    "endpoint": "@PLUGIN_MYPLUGIN_ENDPOINT@"
  }
}
```

CMake variables are set in the plugin's `CMakeLists.txt`:
```cmake
set(PLUGIN_MYPLUGIN_CLASSNAME "MyPlugin" CACHE STRING "MyPlugin class name")
set(PLUGIN_MYPLUGIN_AUTOSTART false CACHE BOOL "MyPlugin auto-start on boot")
set(PLUGIN_MYPLUGIN_STARTUPORDER 50 CACHE STRING "MyPlugin startup order")
set(PLUGIN_MYPLUGIN_ENDPOINT "127.0.0.1:8080" CACHE STRING "MyPlugin endpoint")
```

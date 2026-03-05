---
applyTo: '**'
---

# 06 — COM-RPC Fundamentals

> Cross-reference: `05-object-lifecycle-and-memory.md` for ref-counting rules, `07-interface-driven-development.md` for interface design, `01-architecture.md` for the OOP announce handshake.

## COM-RPC vs JSON-RPC

| Aspect | COM-RPC | JSON-RPC |
|--------|---------|---------|
| Transport | Unix domain socket / TCP (binary, proprietary protocol) | WebSocket / HTTP (text, JSON-RPC 2.0) |
| Audience | C++ plugin-to-plugin, daemon-to-plugin, external C++ tools | JavaScript clients, test tools, ThunderUI, remote management |
| Type safety | Full C++ type safety via generated proxy/stubs | Schema-defined, validated at dispatch time |
| Performance | Low latency, minimal serialization overhead | Higher overhead (JSON serialization) |
| Use when | Plugin acquires another plugin's interface; OOP plugin communication | External UI/apps communicate with Thunder |

COM-RPC is the **primary mechanism for all inter-plugin and daemon-plugin communication**. JSON-RPC is a translation layer on top for external clients only.

## Interface ID Registration

Every COM interface must have a globally unique `uint32_t` ID declared as:
```cpp
enum { ID = RPC::ID_MY_INTERFACE };
```

**Where to register new IDs**:
- IDs for `Source/com/` internal interfaces → `Source/com/Ids.h`
- IDs for all plugin exchange interfaces → `ThunderInterfaces/interfaces/Ids.h`
- Never create arbitrary ID values — always allocate from the appropriate `Ids.h` by appending after the last defined ID

ID ranges in `ThunderInterfaces/interfaces/Ids.h` are organized by subsystem. Follow the existing convention when adding new IDs.

Iterator interface IDs follow the same rule:
```cpp
using IMyItemIterator = RPC::IIteratorType<IMyItem*, RPC::ID_MY_ITEM_ITERATOR>;
// ID_MY_ITEM_ITERATOR must be declared in Ids.h
```

## Proxy/Stub Generation

Proxy and stub files are **auto-generated** from interface headers by `ProxyStubGenerator`. The generator reads annotations in the header.

**Input**: `ThunderInterfaces/interfaces/IMyInterface.h`
**Output**: `ThunderInterfaces/generated/ProxyStubs/ProxyStub_MyInterface.cpp`

The generated file contains:
- `ProxyStub::UnknownProxyType<IMyInterface>` subclass (proxy — serializes outgoing calls)
- `ProxyStub::UnknownStubType<IMyInterface, METHOD_COUNT>` subclass (stub — deserializes incoming calls)
- Static `Instantiation` object in an anonymous namespace that calls `RPC::Administrator::Instance().Announce<Proxy, Stub>()`

**Never hand-edit generated files.** Edit the source interface header, then let CMake regenerate.

**Regeneration is triggered automatically** by CMake when interface headers change. Manual regeneration:
```bash
python3 install/bin/ProxyStubGenerator/StubGenerator.py \
  --include install/include \
  ThunderInterfaces/interfaces/IMyInterface.h
```

## `RPC::Administrator` — Stub Registry

`Administrator` is a process-wide singleton that maps interface IDs to stub and proxy factories.

**Registration** happens automatically at shared library load time via the generated `Instantiation` static object:
```cpp
// Auto-generated, in the generated .cpp — do not write this manually
namespace {
    class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<MyProxy, MyStub>();
        }
    } _announcement;
}
```

**Dispatch**: when an IPC frame arrives, `Administrator` uses the interface ID in the frame to look up the registered stub, then calls `Stub::Handle()` which deserializes parameters and calls the real implementation.

An unregistered interface ID at the OOP boundary causes a runtime error (`Core::ERROR_UNAVAILABLE`). This is the most common symptom of a missing proxy/stub library or incorrect ID.

## `RPC::Communicator` — Transport Layer

| Class | Role |
|-------|------|
| `RPC::Communicator::Server` | Opened by the Thunder daemon; accepts incoming COM-RPC connections |
| `RPC::CommunicatorClient` | Opened by OOP plugin processes to connect back to the daemon |

Socket path is configured via the `communicator` field in `config.json`:
- Default: `/tmp/communicator` (Unix domain socket)
- TCP debug mode: `"127.0.0.1:62000"` — enables Wireshark capture

**Never create raw sockets for IPC** — always use the `Communicator` / `CommunicatorClient` infrastructure.

**Timeout**: `RPC::CommunicationTimeOut` (3000 ms in release builds, infinite in debug builds) applies to every remote method call. Calls exceeding this timeout return `Core::ERROR_TIMEDOUT`.

## OOP Plugin Announce Handshake

Full sequence when activating an OOP plugin (see also `01-architecture.md`):

1. `Service::Activate()` calls `Communicator::Create()` → `RemoteConnectionMap::Create()`.
2. `CreateStarter()` creates a `LocalProcess` (or `ContainerProcess`) launcher with a unique `exchangeId`.
3. `Process::Launch()` forks `ThunderPlugin` with CLI args: `-l locator -c classname -C callsign -r socket -i ifaceId -x exchangeId`.
4. The parent blocks on a `Core::Event` waiting for the child's announcement.
5. The child opens a `CommunicatorClient` connection to the parent's COM-RPC socket.
6. The child sends an `AnnounceMessage` containing: PID, class name, interface ID, version, implementation pointer, `exchangeId`.
7. The parent's `ChannelServer::AnnounceHandler::Procedure()` receives it → `RemoteConnectionMap::Announce()`.
8. `Announce()` matches `exchangeId` to the pending `Create()`, sets the `Core::Event`.
9. The parent unblocks and receives the proxied interface pointer.

**`AnnounceMessage` is an `IPCMessageType<1, ...>`** — use `Label()` + `static_cast` to identify it (not `dynamic_cast`) for macOS compatibility. See `05-object-lifecycle-and-memory.md`.

## COM-RPC Error Detection

Two distinct categories of errors can be returned from a COM-RPC call:

| Error type | Bit test | Meaning |
|------------|---------|---------|
| Transport / COM error | `(result & COM_ERROR) != 0` | IPC framework failure: timeout, disconnected, marshal error |
| Plugin error | `(result & COM_ERROR) == 0` | The plugin method itself returned an error code |

Always distinguish these before deciding on remediation:
```cpp
Core::hresult result = iface->DoSomething(param);
if (result & COM_ERROR) {
    // Transport failed — plugin may be dead or unreachable; do not retry logic
    TRACE_L1("COM transport error: %s", Core::ErrorToString(result));
} else if (result != Core::ERROR_NONE) {
    // Plugin returned a logical error — inspect and handle per-plugin semantics
}
```

## `IStringIterator` and `IValueIterator`

Pre-built in `Source/com/`:
- `RPC::IStringIterator` — iterator over `string` values
- `RPC::IValueIterator` — iterator over `uint32_t` values

Reuse these for simple scalar collections — do not define new iterator interfaces for these types:
```cpp
// In an interface method:
virtual Core::hresult Callsigns(RPC::IStringIterator*& out) const = 0;

// In implementation:
Core::hresult Callsigns(RPC::IStringIterator*& out) const override
{
    std::vector<string> names = { _T("Plugin1"), _T("Plugin2") };
    out = Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(names);
    return Core::ERROR_NONE;
}
```

## `SmartInterfaceType<T>` — External COM-RPC Client

For external tools and test apps (not plugins) that connect to Thunder via COM-RPC:

```cpp
class MyClient : public RPC::SmartInterfaceType<Exchange::IMyInterface> {
public:
    void Operational(const bool upAndRunning) override
    {
        if (upAndRunning) {
            TRACE_L1("Connected to MyPlugin");
        } else {
            TRACE_L1("Disconnected from MyPlugin");
        }
    }
};

int main()
{
    Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), _T("/tmp/communicator"));
    MyClient client;
    client.Open(5000 /* ms timeout */);

    auto* iface = client.Interface();
    if (iface != nullptr) {
        iface->DoSomething();
        iface->Release();
    }

    client.Close(Core::infinite);
    Core::Singleton::Dispose();
    return 0;
}
```

`SmartInterfaceType<T>` handles reconnection, interface re-acquisition, and proxy lifecycle automatically.

## ThunderShark — COM-RPC Traffic Debugging

To capture and decode COM-RPC traffic:

**Step 1**: Switch the communicator to TCP in `config.json`:
```json
{ "communicator": "127.0.0.1:62000" }
```

**Step 2**: Capture traffic:
```bash
tcpdump -i lo port 62000 -w /tmp/comrpc-traffic.pcap
```

**Step 3**: Generate the Wireshark Lua dissector:
```bash
cd ThunderBuild
Thunder/GenerateLua.sh Thunder/ ThunderInterfaces/
# Produces: protocol-thunder-comrpc.data
```

**Step 4**: Open in Wireshark, load the dissector data, filter `thunder-comrpc`.
- Failed calls: `thunder-comrpc.invoke.hresult != 0`
- Shows method names, parameter values, results, and call durations

## Adding a New COM Interface — Checklist

1. Create `ThunderInterfaces/interfaces/IMyFeature.h` (see `07-interface-driven-development.md`).
2. Allocate a new ID in `ThunderInterfaces/interfaces/Ids.h` — append after the last ID in the relevant range.
3. If the interface includes collections, define `IMyItemIterator = RPC::IIteratorType<...>` and allocate an iterator ID.
4. Add `@stubgen:omit` on any method with parameters that cannot be marshalled.
5. Rebuild `ThunderInterfaces` — CMake triggers `ProxyStubGenerator` automatically.
6. Verify the generated `.cpp` contains the `Instantiation` static object with the `Announce<>` call.
7. In the plugin implementation, add the new interface to the `BEGIN_INTERFACE_MAP` / `END_INTERFACE_MAP`.
8. In the plugin's CMakeLists.txt, ensure the proxy/stub library is listed in `target_link_libraries` or is in `proxystubpath`.

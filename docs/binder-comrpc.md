# Binder COMRPC Transport

This document describes the Android Binder-backed COMRPC transport added to Thunder.  
It covers the design, the source files that were added or changed, and step-by-step build and run instructions.

---

## Background

Thunder's native COMRPC transport runs over Unix domain sockets (`Core::IPCChannel` → `Communicator`).  
On platforms that carry the Linux Android Binder kernel driver (`/dev/binder`), inter-process communication can instead use binder — the same IPC mechanism used by Android itself.

Key properties of binder IPC:

- **Zero-copy in kernel** — the kernel maps the sender's buffer directly into the receiver's address space.
- **Strict process isolation** — the kernel assigns per-fd handle tables; a process _cannot_ send a transaction to itself over a different fd to the same process (the driver returns `BR_FAILED_REPLY`).
- **Context manager** — one process calls `BINDER_SET_CONTEXT_MGR` to own handle 0.  All other processes look up named services through that context manager.

---

## Architecture

### Layered design

```
┌─────────────────────────────────────────────────────┐
│                  Application / Plugin                │
│         (uses Thunder COMRPC interfaces)             │
└──────────────────────┬──────────────────────────────┘
                       │  COM-RPC (ProxyStub + Administrator)
┌──────────────────────▼──────────────────────────────┐
│  Thunder/Source/com  —  "com" layer                  │
│                                                      │
│  BinderCommunicator      COMRPC server               │
│  BinderCommunicatorClient  COMRPC client             │
│  BinderIPCChannel        Core::IPCChannel impl       │
│  BinderEndpoint          N-thread looper base        │
│  BinderServiceManager    In-process context manager  │
│  BinderServiceManagerProxy  SVC_MGR client proxy     │
│  IServiceManager         Pure-virtual SM interface   │
└──────────────────────┬──────────────────────────────┘
                       │  BinderTransport / BinderBuffer
┌──────────────────────▼──────────────────────────────┐
│  Thunder/Source/core  —  "core" layer                │
│                                                      │
│  BinderTransport     fd, mmap, ioctl, Loop/Call      │
│  BinderBuffer        RAII read/write cursor wrapper  │
└──────────────────────┬──────────────────────────────┘
                       │  ioctl(BINDER_WRITE_READ)
┌──────────────────────▼──────────────────────────────┐
│  Linux kernel  /dev/binder  (binderfs)               │
└─────────────────────────────────────────────────────┘
```

### Three-process model

Linux binder **rejects same-process cross-fd transactions** (`BR_FAILED_REPLY`), so a working deployment always uses at least three separate processes:

```
Process A  BinderServiceManager          BINDER_SET_CONTEXT_MGR,
           (context manager, handle 0)   answers SVC_MGR_{ADD,GET,LIST}_SERVICE

Process B  BinderCommunicator            Registers its binder node with A via
           (server)                      RegisterNode(); serves ANNOUNCE + INVOKE

Process C  BinderCommunicatorClient      Looks up service via LookupHandle() to A,
           (client)                      sends ANNOUNCE, then INVOKE per method call
```

---

## Source files

### New files in `Source/core/`

| File | Purpose |
|---|---|
| `BinderBuffer.h` | C++ RAII wrapper replacing AOSP `bio_*` C functions.  Manages a read/write cursor and binder-object offset table over a caller-supplied backing buffer.  No dependency on AOSP `struct binder_io` (absent from the Linux kernel header). |
| `BinderTransport.h/.cpp` | Single `/dev/binder` fd.  Provides `Open()`, `Close()`, `BecomeContextManager()`, `Call()` (blocking synchronous transaction), `Loop()` (blocking server event loop), `Acquire()`, `Release()`, `LinkToDeath()`. |

### New files in `Source/com/`

| File | Purpose |
|---|---|
| `IServiceManager.h` | Pure-virtual interface: `AddService` / `GetService` / `ListService`. |
| `BinderEndpoint.h/.cpp` | Abstract base for any binder server endpoint.  Opens the transport, spawns N looper threads each driving `BinderTransport::Loop()`.  Derived classes implement `HandleTransaction()`. |
| `BinderServiceManager.h/.cpp` | Singleton in-process context manager.  Calls `BecomeContextManager()`, handles `SVC_MGR_*` protocol on transactions that arrive at handle 0.  Also exposes the in-process `IServiceManager` API directly (bypassing binder IPC). |
| `BinderServiceManagerProxy.h/.cpp` (part of `BinderServiceManager.cpp`) | Client-side proxy that calls the context manager over real binder IPC.  `RegisterNode()` sends `SVC_MGR_ADD_SERVICE` with a `BINDER_TYPE_BINDER` object using the caller's own transport fd — this is how the kernel associates server handle assignment with the server's fd. `LookupHandle()` sends `SVC_MGR_GET_SERVICE`. |
| `BinderIPCChannel.h/.cpp` | `Core::IPCChannel` implementation backed by binder.  Two modes: **server-side** (zero outbound handle; `SetReplyBuffer()` + `ReportResponse()` fill the binder reply); **client-side** (`Execute()` serialises params, calls `BinderTransport::Call(BINDER_COMRPC_INVOKE)`, deserialises response). |
| `BinderCommunicator.h/.cpp` | COMRPC server.  `Open()` calls `BinderEndpoint::Open()` then `BinderServiceManagerProxy::RegisterNode()` using the server's own fd.  Handles `BINDER_COMRPC_ANNOUNCE` (create session, call `Acquire()`) and `BINDER_COMRPC_INVOKE` (dispatch to `_handler`). |

### Wire protocol

Two custom binder transaction codes are defined in `BinderIPCChannel.h`:

```cpp
BINDER_COMRPC_ANNOUNCE = 0x4E415243   // 'C','R','A','N'
BINDER_COMRPC_INVOKE   = 0x56495243   // 'C','R','I','V'
```

**ANNOUNCE** (client → server, one-shot per session):
```
Request:  [ uint32 initLen ][ Data::Init bytes ]
Reply:    [ uint32 session_id ][ uint32 setupLen ][ Data::Setup bytes ]
```
`Data::Setup` carries the server-side `instance_id` (impl pointer cast to integer) and the `proxyStubPath`.

**INVOKE** (client → server, once per method call):
```
Request:  [ uint32 session_id ][ uint32 paramsLen ][ Data::Frame params ]
Reply:    [ uint32 respLen ][ Data::Frame response ]
```
The `Data::Frame` encoding is identical to the existing Thunder COM-RPC socket transport.

### Modified files

| File | Change |
|---|---|
| `Source/core/CMakeLists.txt` | Added `BinderTransport.cpp`, `BinderBuffer.h` under `if(BINDER_COMRPC)`.  Sets `BINDER_COMRPC_ENABLED` define on `ThunderCore`. |
| `Source/com/CMakeLists.txt` | Added all `Binder*.cpp` / `Binder*.h` / `IServiceManager.h` under `if(BINDER_COMRPC)`.  Sets `BINDER_COMRPC_ENABLED` define on `ThunderCOM`. |
| `Tests/binder/CMakeLists.txt` | Six test executables linked against `ThunderCore`, `ThunderCOM`, `ThunderMessaging`. |
| `build.sh` (ThunderBinder root) | Added `-DBINDER_COMRPC=ON` to the `configure_Thunder` step. |

### Key design decisions

**No dependency on AOSP `struct binder_io` or `struct binder_death`.**  
Those structs exist only in AOSP user-space source; the Linux kernel header `<linux/android/binder.h>` does not define them.  
`BinderBuffer` uses its own `_data`, `_data0`, `_offs`, `_offs0`, `_data_avail`, `_offs_avail`, `_flags` fields.  
`DeathEntry` stores only the `IDeathObserver*`; the handle value is used directly as the cookie in `BC_REQUEST_DEATH_NOTIFICATION`.

**`RegisterNode` uses the server's own transport fd.**  
Calling `BINDER_SET_CONTEXT_MGR` on fd A and then registering a service using fd B would cause the kernel to associate the binder node with fd B's process context.  Future transactions routed to that node will be delivered to whichever fd last called `BC_ENTER_LOOPER`.  Using the server's own fd in `RegisterNode` ensures the kernel routes incoming calls to the server's looper threads.

---

## Prerequisites for testing

### Tested platform

All tests described in this document were run on the following platform:

| Property | Value |
|---|---|
| OS | Ubuntu 22.04.5 LTS (Jammy Jellyfish) |
| Architecture | AArch64 (ARM64) |
| Kernel | 5.15.167 (custom build — see below) |
| Binder device | `/dev/binder` → symlink to `/dev/binderfs/binder` |

### Why a custom kernel is required on Ubuntu 22.04

The Ubuntu 22.04 stock kernel (`5.15.0-xxx-generic`) is built **without** Android Binder support.  
You must use a kernel compiled with the following options:

```
CONFIG_ANDROID_BINDER_IPC=y
CONFIG_ANDROID_BINDERFS=y
CONFIG_ANDROID_BINDER_DEVICES="binder,hwbinder,vndbinder"
CONFIG_ANDROID_BINDER_IPC_SELFTEST=y   # optional, adds kernel self-test
```

!!! note
    `CONFIG_ANDROID_BINDERFS` is required in addition to `CONFIG_ANDROID_BINDER_IPC`.  
    Without it the kernel will not create `/dev/binderfs/` and the `/dev/binder` symlink will not exist.

### Verifying binder support on your system

**Step 1 — Check the kernel config:**

```bash
# Method 1: read the saved config for the running kernel
grep CONFIG_ANDROID_BINDER /boot/config-$(uname -r)

# Method 2: if CONFIG_IKCONFIG_PROC=y, read directly from /proc
zcat /proc/config.gz | grep CONFIG_ANDROID_BINDER
```

Expected output:
```
CONFIG_ANDROID_BINDER_IPC=y
CONFIG_ANDROID_BINDERFS=y
CONFIG_ANDROID_BINDER_DEVICES="binder,hwbinder,vndbinder"
CONFIG_ANDROID_BINDER_IPC_SELFTEST=y
```

If these lines are absent or set to `=m` (module) instead of `=y`, binder is not compiled in and the tests will not run.

**Step 2 — Check that the device nodes exist:**

```bash
ls -la /dev/binder /dev/binderfs/
```

Expected output:
```
lrwxrwxrwx 1 root root 20 /dev/binder -> /dev/binderfs/binder

/dev/binderfs/:
crw----rw- 1 root root 237, 1 binder
crw------- 1 root root 237, 0 binder-control
crw------- 1 root root 237, 2 hwbinder
crw------- 1 root root 237, 3 vndbinder
```

If `/dev/binder` does not exist, binderfs is not mounted.  Check whether it is mounted:

```bash
mount | grep binderfs
# Expected: none         on /dev/binderfs type binder (rw,relatime)
```

If it is not mounted, mount it manually (requires root):

```bash
sudo mkdir -p /dev/binderfs
sudo mount -t binder none /dev/binderfs
sudo ln -sf /dev/binderfs/binder /dev/binder
```

To make the mount persistent across reboots, add the following line to `/etc/fstab`:

```
none  /dev/binderfs  binder  defaults  0  0
```

### `sudo` and permission requirements

The binder context manager (`BINDER_SET_CONTEXT_MGR` ioctl) requires `CAP_SYS_ADMIN`.  
In practice this means the process must run as **root** or via `sudo`.

The default permissions of `/dev/binderfs/binder` are `crw------- root root` on a fresh boot.  
All non-root users are denied.  There are two ways to allow non-root testing:

**Option A — Run tests as root (simplest):**

```bash
sudo LD_LIBRARY_PATH=build/Thunder/Source/core:build/Thunder/Source/com:build/Thunder/Source/messaging \
  build/Thunder/Tests/binder/test_binder_invoke
```

**Option B — Grant group or world access to the device (persists until reboot):**

```bash
# Grant world read-write access to /dev/binderfs/binder only
# (binder-control must remain root-only)
sudo chmod o+rw /dev/binderfs/binder
```

After this, non-root users can open `/dev/binder` but `BecomeContextManager()` will still require  
`CAP_SYS_ADMIN`, so the `BinderServiceManager` process still needs `sudo`.

!!! warning
    Do **not** apply `chmod o+rw` to `binder-control`.  That device controls binderfs node allocation  
    and must remain root-only.

### Build tool prerequisites

```
cmake >= 3.15
ninja-build
g++ with C++14 support  (tested: g++ 11.4.0 on Ubuntu 22.04)
```

---

## Build instructions

### 1. Build ThunderTools (prerequisite — run once)

```bash
cd /path/to/ThunderBinder
bash build.sh ThunderTools configure
cmake --build build/ThunderTools --target install
```

### 2. Configure Thunder with Binder COMRPC enabled

```bash
cmake -G Ninja \
  -S Thunder \
  -B build/Thunder \
  -DBINDING="127.0.0.1" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX=install \
  -DPORT=55555 \
  -DTOOLS_SYSROOT="${PWD}" \
  -DINITV_SCRIPT=OFF \
  -DENABLED_TRACING_LEVEL=3 \
  -DBINDER_COMRPC=ON
```

Expected CMake output lines confirming the feature is enabled:
```
-- Binder COMRPC transport enabled
-- Binder COMRPC com layer enabled
```

### 3. Build

```bash
cmake --build build/Thunder --target ThunderCore ThunderCOM
```

To also build all binder tests:
```bash
cmake --build build/Thunder --target \
  test_binder_transport    \
  test_binder_svcmgr       \
  test_binder_svcmgr_fail  \
  test_binder_announce     \
  test_binder_invoke       \
  test_binder_multi_client
```

---

## Run instructions

Set the runtime library search path for all test commands:

```bash
export LD_LIBRARY_PATH=\
build/Thunder/Source/core:\
build/Thunder/Source/com:\
build/Thunder/Source/messaging
```

All tests live in `build/Thunder/Tests/binder/`.

### `/dev/binder` permissions

`BINDER_SET_CONTEXT_MGR` requires `CAP_SYS_ADMIN`.  Either run as root or grant group access:

```bash
# One-time, survives until next reboot:
sudo chmod o+rw /dev/binderfs/binder
```

### Test descriptions and expected output

#### `test_binder_transport` — kernel driver smoke test

Tests `Open()`, `BecomeContextManager()`, `Close()` directly against `/dev/binder`.  
Requires root (or `chmod o+rw` above) for `BecomeContextManager`.

```bash
sudo ./test_binder_transport
```
```
[PASS] Open /dev/binder
[PASS] IsOpen() after Open
[PASS] BecomeContextManager
[PASS] Close
[PASS] IsOpen() is false after Close

ALL TESTS PASSED (0 failure(s))
```

#### `test_binder_svcmgr_fail` — service manager negative-path tests

Verifies input validation (empty name, handle=0, unknown service, out-of-range index) and proxy error returns.  
Does **not** require root.

```bash
./test_binder_svcmgr_fail
```
```
[PASS] First Start succeeds
[PASS] Second Start returns INPROGRESS
[PASS] AddService with empty name returns error
[PASS] AddService handle=0 returns error
[PASS] GetService unknown returns UNAVAILABLE
...
ALL TESTS PASSED (0 failure(s))
```

#### `test_binder_svcmgr` — in-process service manager

Exercises `AddService`, `GetService`, `ListService` using the in-process `BinderServiceManager` API.  
Requires root.

```bash
sudo ./test_binder_svcmgr
```
```
[PASS] ServiceManager::Start
[PASS] AddService "test.service" handle=1
[PASS] GetService "test.service" returns handle=1
...
ALL TESTS PASSED (0 failure(s))
```

#### `test_binder_invoke` — full end-to-end COMRPC test *(primary E2E test)*

Forks three separate processes, registers an `ICalculator` ProxyStub pair inline, and makes real `Add()` calls over binder COMRPC:

| Process | Role |
|---|---|
| A (fork child 1) | `BinderServiceManager` — context manager |
| B (fork child 2) | `CalcServer` — registers with A, serves ANNOUNCE + INVOKE |
| C (parent) | `BinderCommunicatorClient` — acquires proxy, calls `Add(3,4)` and `Add(100,200)` |

```bash
sudo ./test_binder_invoke
```
```
[svcmgr] running
[server] registered test.calculator
[PASS] BinderCommunicatorClient::Open
[PASS] BINDER_COMRPC_ANNOUNCE: session established
[PASS] Acquire<ICalculator>: valid proxy returned
[PASS] Add(3,4): return code OK
[PASS] Add(3,4) == 7
[PASS] Add(100,200): return code OK
[PASS] Add(100,200) == 300

ALL TESTS PASSED (0 failure(s))
```

#### `test_binder_announce` — ANNOUNCE-only E2E test

Similar to `test_binder_invoke` but validates the session setup path in isolation.  
Requires root.

```bash
sudo ./test_binder_announce
```

#### `test_binder_multi_client` — concurrent stress test

Spawns 8 client threads each making 10 sequential `INVOKE` calls against the same server.  Validates correct multiplexing by the looper thread pool.  
Requires root.

```bash
sudo ./test_binder_multi_client
```

---

## Writing a service using Binder COMRPC

### 1. Define an interface

```cpp
struct IMyService : public Core::IUnknown {
    enum { ID = 0x00001000 };
    virtual uint32_t DoWork(uint32_t input, uint32_t& output) = 0;
};
```

### 2. Write the ProxyStub pair

Follow the pattern in `Tests/binder/test_binder_invoke.cpp` (`ProxyStubs::CalculatorProxy` / `CalculatorStub`), or use `ThunderTools/ProxyStubGenerator` with `BINDER_COMRPC_ENABLED` defined.

### 3. Implement and serve

```cpp
class MyServer : public RPC::BinderCommunicator {
public:
    MyServer()
        : BinderCommunicator("my.service", "/usr/lib/thunderproxystubs",
                             Core::ProxyType<Core::IIPCServer>())
        , _impl()
    {}
protected:
    void* Acquire(const string&, uint32_t id, uint32_t) override {
        if (id == IMyService::ID) return &_impl;
        return nullptr;
    }
private:
    MyServiceImpl _impl;
};

// In main (separate process from the context manager):
RPC::BinderServiceManager::Instance().Start(4); // or use external svcmgr
MyServer server;
server.Open(4);           // registers "my.service" with the context manager
pause();
```

### 4. Call from a client

```cpp
// In a different process:
RPC::BinderCommunicatorClient client("my.service");
client.Open();

IMyService* svc = client.Acquire<IMyService>(
    RPC::CommunicationTimeOut, "MyServiceImpl", IMyService::ID);
if (svc) {
    uint32_t result = 0;
    svc->DoWork(42, result);
    svc->Release();
}
client.Close();
```

---

## Known limitations

| Limitation | Notes |
|---|---|
| Same-process binder IPC | The kernel returns `BR_FAILED_REPLY` for transactions between fds in the same process.  The three-process model (svcmgr / server / client) is **mandatory**. |
| ProxyStub path not loaded | `proxyStubPath` is serialised in ANNOUNCE `Data::Setup` but not acted on by the client.  Dynamic `.so` loading (`DynamicLoaderPaths`) must be wired up separately if cross-package ProxyStubs are needed. |
| `ThunderProxyStubs` plugin | The generated `ProxyStubsMetadata.cpp` requires `com/ProxyStubs.h` from the fully-installed Thunder, which is not present in a source-only build.  This target fails but does not affect `ThunderCore`, `ThunderCOM`, or any binder test. |
| `BecomeContextManager` privilege | Requires `CAP_SYS_ADMIN` on most kernels.  In production, the context manager should run as a privileged service, and all other processes use user-space handles. |

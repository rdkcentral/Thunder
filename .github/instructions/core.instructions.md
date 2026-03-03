---
name: Thunder-Core-Library
description: 'Rules for working in Source/core/ — the standalone platform-primitive library'
applyTo: 'Source/core/**'
---

# Thunder `core/` Library

`Source/core/` is a **self-contained, dependency-free** platform-primitive library. It has zero dependency on `Source/com/`, `Source/plugins/`, `Source/Thunder/`, or any ThunderInterfaces. Every addition must remain usable without the plugin framework.

## Dependency Rule
- **Never** `#include` from `Source/plugins/`, `Source/com/`, or `Source/Thunder/` inside `Source/core/`.
- Allowed includes: other `core/` headers, OS system headers, `Module.h`, `Portability.h`.
- Use `#ifdef __LINUX__` / `#ifdef __WINDOWS__` / `#ifdef __POSIX__` / `#ifdef __UNIX__` for OS-specific code — always guarded, never assumed.

## I/O & Resource Readiness — `IResource` / `ResourceMonitor`
- All file descriptor readiness (sockets, pipes, doors, eventfds) must go through `ResourceMonitor`.
- Implement `Core::IResource`: provide `Descriptor()`, `Events()` (returns `POLLIN`/`POLLOUT`/`POLLERR` bits), and `Handle(events)`.
- Register with `Core::ResourceMonitor::Instance().Register(*this)` and unregister with `Unregister(*this)`.
- **Never** poll, busy-wait, or call `select()`/`epoll_wait()` directly — `ResourceMonitor` owns the event loop.

## Threading — `WorkerPool` / `IWorkerPool`
- Never create raw `std::thread` or `pthread_create` in `core/` code intended for plugins.
- Dispatch jobs via `Core::IWorkerPool::Instance().Submit(job)` where `job` is a `Core::ProxyType<Core::IDispatch>`.
- Implement `Core::IDispatch` (single `Dispatch()` method) for any unit of async work.
- `Core::TimerType<T>` schedules timed callbacks — never use `sleep()`, `usleep()`, or `std::this_thread::sleep_for()`.

## Synchronization — `Sync.h`
- Use `Core::CriticalSection` for mutual exclusion (wraps `pthread_mutex_t` recursively on Linux).
- Use `Core::BinarySemaphore` / `Core::CountingSemaphore` for signalling.
- Use `Core::Event` for one-shot or auto-reset wait/signal.
- `Core::SafeSyncType<LOCK>` provides an RAII lock guard — prefer it over manual `Lock()`/`Unlock()`.
- **Never** hold a `CriticalSection` across a blocking call or any call that acquires another lock — document lock order when nesting is unavoidable.

## JSON — `JSON.h` / `JsonObject.h`
- All structured data uses `Core::JSON::Container` subclasses — never hand-parse JSON strings.
- Scalar types: `Core::JSON::DecUInt32`, `Core::JSON::String`, `Core::JSON::Boolean`, `Core::JSON::EnumType<E>`, `Core::JSON::ArrayType<T>`.
- Register fields in the constructor with `Add(_T("key"), &Member)`.
- Deserialize with `.FromString(str)` / `.ToString(out)`.

## Smart Pointers — `Proxy.h`
- `Core::ProxyType<T>`: ref-counted shared ownership. Obtain via `Core::ProxyType<T>::Create(args...)`.
- `Core::ProxyObject<T>`: the heap-allocated ref-counted wrapper — do not use directly; accessed through `ProxyType`.
- Raw `IUnknown*` pointers returned by `QueryInterface()` carry a ref — caller must call `Release()`.
- Never `delete` a `Core::IUnknown` subclass directly — always `Release()`.

## Error Handling
- **No exceptions** (`-fno-exceptions`). Never `throw`. Signal errors via `uint32_t` / `Core::hresult` return codes.
- Use `Core::ERROR_NONE` (== 0) for success.
- `ASSERT(condition)` for developer invariants — compiled out in release. Never use `assert()` from `<cassert>`.
- `TRACE_L1(fmt, ...)` for lightweight diagnostic prints (compiled out in `MinSizeRel`).

## Platform Portability
- Use `string` from `Portability.h` (not `std::string` directly) where the code must be wide-char-safe on Windows.
- Use `_T("literal")` for string literals passed to Thunder APIs.
- Integer widths: prefer `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t` — never `int` for protocol fields.
- File paths: use `Core::FileSystem` helpers, never hardcode separators.

## Platform Preprocessor Guards
- Use `#ifdef __LINUX__` for Linux-specific code (includes Android).
- Use `#ifdef __APPLE__` for macOS-specific code.
- Use `#ifdef __POSIX__` for POSIX-common code (covers both Linux and macOS).
- Use `#ifdef __WINDOWS__` for Windows-specific code.
- Use `#ifdef __UNIX__` for general Unix (POSIX + others).
- Always provide an `#else` or `#error` for unsupported platforms in new platform-abstraction code.
- **Never** assume Linux-only — always check if macOS needs a different implementation.

## Network Info (`NetworkInfo.h` / `NetworkInfo.cpp`)
`AdapterIterator` provides a cross-platform abstraction over network interface enumeration:

### Linux Implementation
- Uses `netlink` sockets (`AF_NETLINK`, `NETLINK_ROUTE`) for interface enumeration.
- Reads `/proc/net/if_inet6` for IPv6 addresses.
- Sends `RTM_GETADDR` / `RTM_GETLINK` requests to query interface state.

### macOS Implementation
- Uses `getifaddrs()` for address enumeration (IPv4 + IPv6) and interface flags.
- Uses `sysctl()` with `PF_ROUTE` / `CTL_NET` / `RTM_IFLIST2` for MAC address and interface statistics.
- **Key differences from Linux**:
  - No `netlink` — all queries through BSD `sysctl` and `ioctl`.
  - `AF_LINK` sockaddr contains MAC address (via `LLADDR()` macro from `<net/if_dl.h>`).
  - `struct if_msghdr2` provides per-interface byte/packet counters.
  - Interface index ↔ name mapping via `if_indextoname()` / `if_nametoindex()`.
  - No `/proc` filesystem — never reference `/proc/**` in macOS code paths.

### DHCPClient
- Linux: uses raw sockets (`AF_PACKET`) for DHCP discovery.
- macOS: raw DHCP sockets require root privileges — conditional compilation guards needed.
- Always guard DHCP-specific code with `#ifndef __APPLE__` or implement a macOS alternative.

## NodeId (`NodeId.h` / `NodeId.cpp`)
- `Core::NodeId` wraps socket addresses (IPv4, IPv6, Unix domain, Netlink).
- `getaddrinfo()` is used for hostname resolution — handle `EAI_NONAME` and `EAI_AGAIN` as non-fatal (especially for numeric-only addresses).
- On macOS, `EAI_NONAME` can trigger excessive syslog spam if logged at warning level — log at `TRACE_L1` only.
- Unix domain socket paths: max length is `sizeof(sockaddr_un::sun_path) - 1` (typically 104 on macOS, 108 on Linux).

## IPC Message System (`IPCConnector.h`)
The IPC framework is built on a type-erased message hierarchy:

### Core Types
| Type | Location | Role |
|------|----------|------|
| `IIPC` | `IPCConnector.h:251` | Abstract interface: `Label()`, `IParameters()`, `IResponse()` |
| `IReferenceCounted` | `IPCConnector.h` | Ref-counting mixin: `AddRef()`, `Release()` |
| `IPCMessageType<ID, P, R>` | `IPCConnector.h:266` | Template: inherits BOTH `IIPC` and `IReferenceCounted` |
| `IIPCServer` | `IPCConnector.h:259` | Handler: `Procedure(IPCChannel&, ProxyType<IIPC>&)` |

### `IPCMessageType<ID, PARAMETERS, RESPONSE>`
- `ID` is a compile-time `uint32_t` discriminator — returned by `Label()` and `::Id()`.
- `PARAMETERS` and `RESPONSE` are serializable message body types.
- Inner classes `RawSerializedType<PACKAGE, REALIDENTIFIER>` implement `IMessage` for each direction.
- Reference counting is forwarded from the inner parameter/response objects to the parent `IPCMessageType`.

### IPCChannel Message Routing
- `IPCChannel::Register(label, handler)` associates a label ID with an `IIPCServer` handler.
- `IPCChannel::Unregister(label)` removes the handler.
- Incoming frames are deserialized, matched by label, and dispatched to the registered handler.
- The handler receives `Core::ProxyType<Core::IIPC>&` — a type-erased smart pointer to the message.
- **Critical**: converting this `ProxyType<IIPC>` to a specific `ProxyType<IPCMessageType<...>>` uses `dynamic_cast` internally, which **fails on macOS** for template types across dylibs. Use `Label()` + `static_cast` instead (see com.instructions.md).

## `ResourceMonitor` Details
- Singleton: `Core::ResourceMonitor::Instance()`.
- Uses `poll()` on POSIX (or `epoll` on Linux when available).
- Registered resources must unregister before destruction — failure to unregister causes use-after-free.
- **Unregister timing**: `Unregister()` may be called from the `Handle()` callback itself (re-entrant safe), but the resource must not be destroyed until `Unregister()` returns.

## New File Checklist
- Include guard for new headers: use classic guards `#ifndef __MYFILE_H` / `#define __MYFILE_H` / `#endif` (double-underscore); existing headers that already use `#pragma once` may remain unchanged.
- License header (Apache 2.0, Metrological copyright) at top.
- `EXTERNAL` macro on any class/function exported from the shared library.
- Export the new header from `core/core.h` if it is part of the public API.
- If the feature has platform-specific behavior, provide both `__LINUX__` and `__APPLE__` implementations from the start.

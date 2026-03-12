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
- Use `#ifdef __POSIX__` for POSIX-common code.
- Use `#ifdef __WINDOWS__` for Windows-specific code.
- Use `#ifdef __UNIX__` for general Unix (POSIX + others).
- Always provide an `#else` or `#error` for unsupported platforms in new platform-abstraction code.

## Network Info (`NetworkInfo.h` / `NetworkInfo.cpp`)
`AdapterIterator` provides a cross-platform abstraction over network interface enumeration.

### Linux Implementation
- Uses `netlink` sockets (`AF_NETLINK`, `NETLINK_ROUTE`) for interface enumeration.
- Reads `/proc/net/if_inet6` for IPv6 addresses.
- Sends `RTM_GETADDR` / `RTM_GETLINK` requests to query interface state.

### DHCPClient
- Uses raw sockets (`AF_PACKET`) for DHCP discovery.

## NodeId (`NodeId.h` / `NodeId.cpp`)
- `Core::NodeId` wraps socket addresses (IPv4, IPv6, Unix domain, Netlink).
- `getaddrinfo()` is used for hostname resolution — handle `EAI_NONAME` and `EAI_AGAIN` as non-fatal (especially for numeric-only addresses).
- Unix domain socket paths: max length is `sizeof(sockaddr_un::sun_path) - 1` (108 bytes on Linux).

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
- **Critical**: never convert `ProxyType<IIPC>` to a specific `ProxyType<IPCMessageType<...>>` via `dynamic_cast` — use `Label()` to identify the message type and `static_cast` to convert (see `com.instructions.md`). `dynamic_cast` on template specialisations is not reliable across dylib boundaries.

## `ResourceMonitor` Details
- Singleton: `Core::ResourceMonitor::Instance()`.
- Uses `poll()` on POSIX (or `epoll` on Linux when available).
- Registered resources must unregister before destruction — failure to unregister causes use-after-free.
- **Unregister timing**: `Unregister()` may be called from the `Handle()` callback itself (re-entrant safe), but the resource must not be destroyed until `Unregister()` returns.

## New File Checklist
- Include guard for new headers: use classic guards `#ifndef __MYFILE_H__` / `#define __MYFILE_H__` / `#endif` (double-underscore prefix and suffix); existing headers that already use `#pragma once` may remain unchanged.
- License header (Apache 2.0, Metrological copyright) at top.
- `EXTERNAL` macro on any class/function exported from the shared library.
- Export the new header from `core/core.h` if it is part of the public API.

## Dependency Inversion Rule

`Source/core/` is a **provider of abstractions** — it must never depend on upper layers. Upper layers (`com/`, `plugins/`, `Thunder/`) implement the abstractions. Concretely:
- `core/` defines `IWorkerPool`, `IResource`, `IDispatch` — it does not implement a `WorkerPool` that knows about plugins.
- `com/` and `Thunder/` provide the concrete `WorkerPool` and `ResourceMonitor` implementations that are injected at runtime.
- If a new abstraction in `core/` needs a concrete implementation, define an interface in `core/` and provide the implementation in `Thunder/` or `com/`.

## Singleton Pattern in `core/`

`Core::Singleton<T>` provides a process-lifetime singleton with explicit disposal:
```cpp
// Access:
T& instance = Core::Singleton<T>::Instance();

// Disposal (must be called at process exit to ensure proper shutdown order):
Core::Singleton::Dispose();
```

- Use singletons only for process-lifetime services: `WorkerPool`, `ResourceMonitor`, `Administrator`.
- Do **not** use `Core::Singleton<T>` for objects that have a meaningful lifetime shorter than the process.
- Prefer passing instances explicitly over using singletons in new code where possible.
- `Core::Singleton::Dispose()` must be called in `main()` after all other cleanup — it tears down all registered singletons in reverse construction order.

## `FileSystem`, `File`, `Directory` Utilities

- Use `Core::File` for file operations — never `fopen`/`fclose` directly.
- Use `Core::Directory` for directory traversal.
- Use `Core::FileSystem::PathName()` / `Core::FileSystem::FileName()` for path manipulation — never `strchr`/`strrchr` on path strings.
- `Core::FileObserver` registers for file change notifications via `IResource` — use it for hot-reload patterns.

## `DataElement` and `DataElementFile`

- `Core::DataElement` is a reference-counted view over a raw byte buffer — used for zero-copy IPC payload passing.
- `Core::DataElementFile` memory-maps a file as a `DataElement`.
- Never copy large binary payloads — pass `Core::DataElement` or `Core::ProxyType<Core::DataElement>` instead.

## Cross-Reference

- For IPC message type safety (`Label()` + `static_cast` pattern): see `com.instructions.md`.
- For `NodeId` and `NetworkInfo` usage from the plugin layer: see `com.instructions.md` (Communicator socket configuration).

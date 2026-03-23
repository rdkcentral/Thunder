/*
 * test_binder_invoke.cpp
 *
 * End-to-end cross-process COMRPC test over the Android Binder transport.
 *
 * Architecture (three separate processes via fork):
 *
 *   Process A (svcmgr_pid)  — BinderServiceManager context manager
 *   Process B (server_pid)  — CalcServer registers with A, loops serving requests
 *   Process C (this/parent) — BinderCommunicatorClient acquires ICalculator proxy
 *                             and makes real Add() calls over binder COMRPC
 *
 * The ICalculator ProxyStub pair is defined inline below — no code generation
 * needed.  Both the proxy (C) and stub (B) are registered in every process
 * before fork, so the fork-copies carry the registration automatically.
 *
 * Validation:
 *   - BINDER_COMRPC_ANNOUNCE round-trip: SessionId() != 0
 *   - BINDER_COMRPC_INVOKE: calc->Add(3,4) == 7
 *   - BINDER_COMRPC_INVOKE: calc->Add(100,200) == 300
 *
 * Requires /dev/binder and CAP_SYS_ADMIN (run as root).
 *
 * NOTE: Linux binder rejects same-process cross-fd transactions (BR_FAILED_REPLY);
 *       three separate processes are mandatory.
 */

#include <cstdio>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

#include "../../Source/com/BinderCommunicator.h"
#include "../../Source/com/BinderServiceManager.h"
#include "../../Source/com/Administrator.h"
#include "../../Source/com/IUnknown.h"
#include "../../Source/core/Errors.h"

using namespace Thunder;
using namespace Thunder::Core;
using namespace Thunder::RPC;

static int failures = 0;

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s\n", msg); \
            ++failures; \
        } else { \
            printf("[PASS] %s\n", msg); \
        } \
    } while (0)

// ==========================================================================
// ICalculator interface
// ==========================================================================
struct ICalculator : public Core::IUnknown {
    // Method indices after IUnknown (0=AddRef,1=Release,2=QueryInterface)
    enum MethodIndex { ADD = 0 };
    enum { ID = 0xCA1C0001 };
    ~ICalculator() override = default;
    virtual uint32_t Add(uint32_t a, uint32_t b, uint32_t& result) = 0;
};

// ==========================================================================
// ICalculator ProxyStub (inline — no code generation needed)
// ==========================================================================
namespace ProxyStubs {

// --------------------------------------------------------------------------
// Stub: server-side handler invoked by BinderCommunicator::HandleInvoke
// --------------------------------------------------------------------------
static ProxyStub::MethodHandler CalculatorStubMethods[] = {
    // (0) virtual uint32_t Add(uint32_t a, uint32_t b, uint32_t& result)
    [](Core::ProxyType<Core::IPCChannel>& /*channel*/,
       Core::ProxyType<RPC::InvokeMessage>& message)
    {
        ICalculator* impl =
            reinterpret_cast<ICalculator*>(message->Parameters().Implementation());
        ASSERT(impl != nullptr);

        RPC::Data::Frame::Reader reader(message->Parameters().Reader());
        const uint32_t a = reader.Number<uint32_t>();
        const uint32_t b = reader.Number<uint32_t>();

        uint32_t result = 0;
        const uint32_t hr = impl->Add(a, b, result);

        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<uint32_t>(hr);
        writer.Number<uint32_t>(result);
    },
    nullptr
};

// --------------------------------------------------------------------------
// Proxy: client-side call marshalling sent via BinderIPCChannel::Execute
// --------------------------------------------------------------------------
class CalculatorProxy final : public ProxyStub::UnknownProxyType<ICalculator> {
public:
    CalculatorProxy(const Core::ProxyType<Core::IPCChannel>& channel,
                    const Core::instance_id impl,
                    const bool outbound)
        : BaseClass(channel, impl, outbound)
    {}

    uint32_t Add(uint32_t a, uint32_t b, uint32_t& result) override
    {
        IPCMessage message(
            static_cast<const ProxyStub::UnknownProxy&>(*this).Message(
                ICalculator::ADD));

        RPC::Data::Frame::Writer writer(message->Parameters().Writer());
        writer.Number<uint32_t>(a);
        writer.Number<uint32_t>(b);

        const Core::hresult hresult =
            static_cast<const ProxyStub::UnknownProxy&>(*this).Invoke(message);
        if (hresult == Core::ERROR_NONE) {
            RPC::Data::Frame::Reader reader(message->Response().Reader());
            const uint32_t hr = reader.Number<uint32_t>();
            result             = reader.Number<uint32_t>();
            return hr;
        }
        return Core::ERROR_RPC_CALL_FAILED;
    }
};

// --------------------------------------------------------------------------
// Registration — static initialiser registers in every process (including
// the fork-children, because fork copies the initialiser state)
// --------------------------------------------------------------------------
typedef ProxyStub::UnknownStubType<ICalculator, CalculatorStubMethods> CalculatorStub;

namespace {
    struct CalculatorRegistration {
        CalculatorRegistration() {
            RPC::Administrator::Instance().Announce<ICalculator,
                                                    CalculatorProxy,
                                                    CalculatorStub>();
        }
        ~CalculatorRegistration() {
            RPC::Administrator::Instance().Recall<ICalculator>();
        }
    } sCalcReg;
}

} // namespace ProxyStubs

// ==========================================================================
// Concrete implementation (used only in Process B)
// ==========================================================================
class Calculator : public ICalculator {
public:
    uint32_t AddRef()  const override { return 1; }
    uint32_t Release() const override { return 1; }
    void*    QueryInterface(uint32_t id) override {
        if (id == ICalculator::ID || id == Core::IUnknown::ID) return this;
        return nullptr;
    }
    uint32_t Add(uint32_t a, uint32_t b, uint32_t& result) override {
        result = a + b;
        return Core::ERROR_NONE;
    }
};

class CalcServer : public BinderCommunicator {
public:
    CalcServer()
        : BinderCommunicator("test.calculator", "",
                             Core::ProxyType<Core::IIPCServer>())
        , _impl()
    {}
protected:
    void* Acquire(const string& /*cls*/, uint32_t id, uint32_t /*ver*/) override {
        if (id == ICalculator::ID || id == Core::IUnknown::ID) return &_impl;
        return nullptr;
    }
private:
    Calculator _impl;
};

// ==========================================================================
// Process entry points
// ==========================================================================

static int RunSvcmgr()
{
    uint32_t r = BinderServiceManager::Instance().Start(2);
    if (r != Core::ERROR_NONE) {
        fprintf(stderr, "[svcmgr] Start failed: %u\n", r);
        return 1;
    }
    fprintf(stderr, "[svcmgr] running\n");
    pause(); // block until SIGTERM from parent
    return 0;
}

static int RunServer()
{
    CalcServer server;
    uint32_t r = server.Open(2);
    if (r != Core::ERROR_NONE) {
        fprintf(stderr, "[server] Open failed: %u\n", r);
        return 1;
    }
    fprintf(stderr, "[server] registered test.calculator\n");
    pause(); // block until SIGTERM from parent
    return 0;
}

// ==========================================================================
// Main — forks A and B, then acts as the COMRPC client
// ==========================================================================
int main()
{
    // -- Fork Process A: context manager -----------------------------------
    pid_t svcmgr_pid = fork();
    if (svcmgr_pid < 0) { perror("fork svcmgr"); return 1; }
    if (svcmgr_pid == 0) { return RunSvcmgr(); }

    usleep(200000); // 200 ms: let svcmgr become context manager

    // -- Fork Process B: calculator server ---------------------------------
    pid_t server_pid = fork();
    if (server_pid < 0) {
        kill(svcmgr_pid, SIGTERM); waitpid(svcmgr_pid, nullptr, 0);
        perror("fork server"); return 1;
    }
    if (server_pid == 0) { return RunServer(); }

    usleep(200000); // 200 ms: let server register with svcmgr

    // -- Process C: client -------------------------------------------------
    {
        BinderCommunicatorClient client("test.calculator");
        uint32_t r = client.Open();
        CHECK(r == Core::ERROR_NONE, "BinderCommunicatorClient::Open");

        if (r == Core::ERROR_NONE) {
            ICalculator* calc = client.Acquire<ICalculator>(
                RPC::CommunicationTimeOut, "Calculator", ICalculator::ID);

            CHECK(client.SessionId() != 0,
                  "BINDER_COMRPC_ANNOUNCE: session established");

            CHECK(calc != nullptr,
                  "Acquire<ICalculator>: valid proxy returned");

            if (calc != nullptr) {
                uint32_t sum = 0;

                r = calc->Add(3, 4, sum);
                CHECK(r == Core::ERROR_NONE, "Add(3,4): return code OK");
                CHECK(sum == 7,              "Add(3,4) == 7");

                r = calc->Add(100, 200, sum);
                CHECK(r == Core::ERROR_NONE, "Add(100,200): return code OK");
                CHECK(sum == 300,            "Add(100,200) == 300");

                calc->Release();
            }
            client.Close();
        }
    }

    // -- Clean up child processes ------------------------------------------
    kill(server_pid,  SIGTERM);
    kill(svcmgr_pid,  SIGTERM);
    waitpid(server_pid,  nullptr, 0);
    waitpid(svcmgr_pid,  nullptr, 0);

    printf("\n%s (%d failure(s))\n",
           failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           failures);
    return (failures == 0) ? 0 : 1;
}

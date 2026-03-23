/*
 * test_binder_multi_client.cpp
 *
 * Stress-test: 8 concurrent clients each acquire the same service and make
 * 10 sequential COMRPC invoke calls.  Verifies that concurrent binder
 * transactions are corrrectly multiplexed by the looper thread pool.
 *
 * Expected output: all 80 calls return the correct results.
 */

#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

#include "../../Source/com/BinderCommunicator.h"
#include "../../Source/com/BinderServiceManager.h"
#include "../../Source/core/Errors.h"

using namespace Thunder;
using namespace Thunder::Core;
using namespace Thunder::RPC;

static std::atomic<int> failures{0};
static std::atomic<int> passes{0};

#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            printf("[FAIL] %s\n", msg); \
            ++failures; \
        } else { \
            ++passes; \
        } \
    } while (0)

// Minimal echo interface
struct IEcho : public Core::IUnknown {
    enum { ID = 0xEC110001 };
    ~IEcho() override = default;
    virtual uint32_t Echo(uint32_t value, uint32_t& out) = 0;
};

class EchoImpl : public IEcho {
public:
    uint32_t AddRef()  const override { return 1; }
    uint32_t Release() const override { return 1; }
    void*    QueryInterface(uint32_t id) override {
        if (id == IEcho::ID || id == Core::IUnknown::ID) return this;
        return nullptr;
    }
    uint32_t Echo(uint32_t value, uint32_t& out) override {
        out = value;
        return ERROR_NONE;
    }
};

class EchoServer : public BinderCommunicator {
public:
    EchoServer()
        : BinderCommunicator("test.echo.multi",
                             "",
                             Core::ProxyType<Core::IIPCServer>())
        , _impl()
    {}

protected:
    void* Acquire(const string&, uint32_t id, uint32_t) override {
        if (id == IEcho::ID) return &_impl;
        return nullptr;
    }
private:
    EchoImpl _impl;
};

static void ClientWorker(int id, int* failCount)
{
    BinderCommunicatorClient client("test.echo.multi");
    if (client.Open() != ERROR_NONE) {
        printf("[FAIL] worker %d: Open failed\n", id);
        ++(*failCount);
        return;
    }

    IEcho* echo = client.Acquire<IEcho>(
        RPC::CommunicationTimeOut, "Echo", IEcho::ID);

    if (echo == nullptr) {
        printf("[FAIL] worker %d: Acquire failed\n", id);
        ++(*failCount);
        client.Close();
        return;
    }

    for (int i = 0; i < 10; ++i) {
        uint32_t out = 0;
        uint32_t result = echo->Echo(static_cast<uint32_t>(id * 100 + i), out);
        if (result != ERROR_NONE || out != static_cast<uint32_t>(id * 100 + i)) {
            printf("[FAIL] worker %d iteration %d: result=%u out=%u\n",
                   id, i, result, out);
            ++(*failCount);
        }
    }

    echo->Release();
    client.Close();
}

int main()
{
    BinderServiceManager& svcmgr = BinderServiceManager::Instance();
    if (svcmgr.Start(4) != ERROR_NONE) {
        printf("SKIP: Cannot start context manager\n");
        return 77;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EchoServer server;
    if (server.Open(4) != ERROR_NONE) {
        printf("[FAIL] EchoServer::Open\n");
        svcmgr.Stop();
        return 1;
    }
    svcmgr.AddService("test.echo.multi", 0, true);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    constexpr int kWorkers = 8;
    std::vector<std::thread> threads;
    std::vector<int> workerFails(kWorkers, 0);

    threads.reserve(kWorkers);
    for (int i = 0; i < kWorkers; ++i) {
        threads.emplace_back(ClientWorker, i, &workerFails[i]);
    }

    int totalFails = 0;
    for (int i = 0; i < kWorkers; ++i) {
        threads[i].join();
        totalFails += workerFails[i];
    }

    server.Close();
    svcmgr.Stop();

    printf("\n%d workers × 10 calls = 80 total\n", kWorkers);
    printf("%s (%d failure(s))\n",
           totalFails == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED",
           totalFails);
    return (totalFails == 0) ? 0 : 1;
}

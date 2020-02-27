#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>

using namespace WPEFramework;
using namespace WPEFramework::Core;

#define THREADPOOL_COUNT 4

static std::thread::id g_parentworkerId;
static bool g_workerthreadDone = false;

class WorkerPoolImplementation : public Core::WorkerPool {
    public:
        WorkerPoolImplementation() = delete;
        WorkerPoolImplementation(const WorkerPoolImplementation&) = delete;
        WorkerPoolImplementation& operator=(const WorkerPoolImplementation&) = delete;

        WorkerPoolImplementation(const uint8_t threads, const uint32_t stackSize)
            : WorkerPool(threads, reinterpret_cast<uint32_t*>(::malloc(sizeof(uint32_t) * threads)))
            , _minions()
        {
            for (uint8_t index = 1; index < threads; index++) {
                _minions.emplace_back();
            }

        }
        ~WorkerPoolImplementation()
        {
            // Diable the queue so the minions can stop, even if they are processing and waiting for work..
            Stop();
        }
     public:
        void Run() {

            Core::WorkerPool::Run();
            Core::WorkerPool::Join();
        }
        void Stop() {
            Core::WorkerPool::Stop();
        }

    protected:
        virtual Core::WorkerPool::Minion& Index(const uint8_t index) override {
            uint8_t count = index;
            std::list<Core::WorkerPool::Minion>::iterator element (_minions.begin());

            while ((element != _minions.end()) && (count > 1)) {
                count--;
                element++;
            }

            ASSERT (element != _minions.end());

            return (*element);
        }

        virtual bool Running() override {
            return true;
        }
    private:
        std::list<Core::WorkerPool::Minion> _minions;
};
class WorkerPoolTypeImplementation : public Core::WorkerPoolType<THREADPOOL_COUNT> {
public:
    WorkerPoolTypeImplementation() = delete;
    WorkerPoolTypeImplementation(const WorkerPoolImplementation&) = delete;
    WorkerPoolTypeImplementation& operator=(const WorkerPoolImplementation&) = delete;

    WorkerPoolTypeImplementation(const uint32_t stackSize)
        : Core::WorkerPoolType<THREADPOOL_COUNT>(stackSize)
        {
            Core::WorkerPool::Minion minion(Core::Thread::DefaultStackSize());
        }
        virtual ~WorkerPoolTypeImplementation()
        {
        }
};

Core::ProxyType<WorkerPoolImplementation> workerpool = Core::ProxyType<WorkerPoolImplementation>::Create(2, Core::Thread::DefaultStackSize());

class WorkerThreadClass : public Core::Thread {
private:
    WorkerThreadClass(const WorkerThreadClass&) = delete;
    WorkerThreadClass& operator=(const WorkerThreadClass&) = delete;

public:
    WorkerThreadClass()
        : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
    {
    }

    virtual ~WorkerThreadClass()
    {
    }

    virtual uint32_t Worker() override
    {
        while (IsRunning() && (!g_workerthreadDone)) {
            EXPECT_TRUE(g_parentworkerId != std::this_thread::get_id());
            ::SleepMs(250);
            g_workerthreadDone = true;
            workerpool->Stop();
        }
        return (Core::infinite);
    }
};

class WorkerJob : public Core::IDispatch {
private:
    WorkerJob(const WorkerJob&) = delete;
    WorkerJob& operator=(const WorkerJob&) = delete;

public:
    WorkerJob()
    {
    }
    ~WorkerJob()
    {
    }
    virtual void Dispatch() override
    {
        EXPECT_NE(g_parentworkerId, std::this_thread::get_id());
        g_workerthreadDone = true;
    }
};
TEST(test_workerpool, simple_workerpool)
{
    WorkerThreadClass object;
    object.Run();
    workerpool->Run();
    workerpool->Id(0);
    EXPECT_EQ(workerpool->Id(1),0u);
    EXPECT_EQ(workerpool->Id(-1),0u);
    workerpool->Snapshot();
    Core::ProxyType<Core::IDispatch> job(Core::ProxyType<WorkerJob>::Create());
    workerpool->Submit(job);
    workerpool->Schedule(Core::infinite, job);
    workerpool->Revoke(job);
    workerpool->Instance();
    EXPECT_TRUE(workerpool->IsAvailable());

    object.Stop();
}
TEST(test_workerjobpooltype, simple_workerjobpooltype)
{
    workerpool.Release();
    WorkerPoolTypeImplementation workerpooltype(Core::Thread::DefaultStackSize());
    workerpooltype.ThreadId(0);
    Core::Singleton::Dispose();
}

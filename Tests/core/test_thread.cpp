#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace WPEFramework {
namespace Tests {

    class ThreadClass : public Core::Thread {
    public:
        ThreadClass() = delete;
        ThreadClass(const ThreadClass&) = delete;
        ThreadClass& operator=(const ThreadClass&) = delete;

        ThreadClass(bool* threadDone, std::mutex* mutex,std::condition_variable* cv, std::thread::id parentTid)
            : Core::Thread(Core::Thread::DefaultStackSize(), _T("Test"))
            , _done(threadDone)
            , _threadMutex(mutex)
            , _threadCV(cv)
            , _parentTid(parentTid)
        {
        }

        virtual ~ThreadClass()
        {
        }

        virtual uint32_t Worker() override
        {
            while (IsRunning() && (!*_done)) {
                EXPECT_NE(_parentTid, std::this_thread::get_id());
                std::unique_lock<std::mutex> lk(*_threadMutex);
                *_done = true;
                _threadCV->notify_one();
            }
            return (Core::infinite);
        }

    private :
        bool* _done;
        std::mutex* _threadMutex;
        std::condition_variable* _threadCV;
        std::thread::id _parentTid;
    };

    class Job : public Core::IDispatch {
    public:
        Job(const Job&) = delete;
        Job& operator=(const Job&) = delete;

        Job()
        {
        }
        ~Job()
        {
        }
        virtual void Dispatch() override
        {
            EXPECT_NE(_parentTPid, std::this_thread::get_id());
            std::unique_lock<std::mutex> lk(_mutex);
            _threadDone = true;
            _cv.notify_one();
        }
        static bool GetState()
        {
            return _threadDone;
        }

    private:
        static bool _threadDone;
        static std::thread::id _parentTPid;

    public:
        static std::mutex _mutex;
        static std::condition_variable _cv;
    };

    bool Job::_threadDone = false;
    std::mutex Job::_mutex;
    std::condition_variable Job::_cv;
    std::thread::id Job::_parentTPid = std::this_thread::get_id();

    TEST(Core_Thread, SimpleThread)
    {
        std::thread::id parentTid = std::this_thread::get_id();
        bool threadDone = false;
        std::mutex mutex;
        std::condition_variable cv;

        ThreadClass object(&threadDone, &mutex, &cv, parentTid);
        object.Run();
        EXPECT_EQ(object.State(), Core::Thread::RUNNING);
        std::unique_lock<std::mutex> lk(mutex);
        while(!threadDone) {
            cv.wait(lk);
        }
        object.Stop();
        EXPECT_EQ(object.State(), Core::Thread::STOPPING);
        object.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED | Core::Thread::STOPPING, Core::infinite);
    }

    TEST(Core_Thread, ThreadPool)
    {
        Core::ProxyType<Core::IDispatch> job(Core::ProxyType<Job>::Create());
        Core::ThreadPoolType<Core::Job, 1> executor(0, _T("TestPool"));
        executor.Submit(Core::Job(job), Core::infinite);

        std::unique_lock<std::mutex> lk(Job::_mutex);
        while(!Job::GetState()) {
            Job::_cv.wait(lk);
        }
        Core::Singleton::Dispose();
    }
} // Tests
} // WPEFramework

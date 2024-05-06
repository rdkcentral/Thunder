#include <chrono>
#include <thread>
#include "core.h"

// Test job class
class TestJob : public Core::IDispatch
{
    public:

    TestJob() = delete;
    TestJob(const TestJob& copy) = delete;
    TestJob& operator=(const TestJob& RHS) = delete;
    ~TestJob() override = default;
    TestJob(const uint32_t waitTime = 0,)
        : _waitTime(waitTime)
    {
    }

public:

    void Dispatch() override
    {
        int input;
        std::cout << "Enter the number: " << std::endl;
        std::cin >> input;
        Fibonacci(input);
    }

private:

    int Fibonacci(const int n) {
        int a = 0;
        int b = 1;
        int c = 0;
        if (n <= 1)
            return n;

        for(int i = 2; i <= n; i++) {
            c = a + b;
            a = b;
            b = c;
        }
        return b;
    }

    uint32_t _waitTime;
};

// Creates job and submits it in WorkerPool
void CreateAndSubmitJob()
{
    std::cout << "Starting first test" << std::endl;
    // Creating WorkerPool instance
    Core::Workerpool workerpool;
    workerpool.Join();

    // Now we are creating and submiting job
    Core::ProxyType<Core::IDispatch> job_one = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(0));
    workerpool.Submit(job);

    // Now run the job and wait for it to complete
    workerpool.Run();

    workerpool.Stop();
}

// Creates three jobs and submits it in WorkerPool
void ScheduleJobs()
{
    std::cout << "Starting second test" << std::endl;

    // Creating WorkerPool instance
    Core::Workerpool workerpool;
    workerpool.Join();

    // Now we are creating 3 jobs
    Core::ProxyType<Core::IDispatch> job_one = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(50));
    Core::ProxyType<Core::IDispatch> job_two = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(100));
    Core::ProxyType<Core::IDispatch> job_three = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(150));

    // Now we submit one job and schedule another two
    workerpool.Submit(job_one);
    workerpool.Schedule(Core::Time::Now() + 10, job_two);
    workerpool.Schedule(Core::Time::Now() + 20, job_three);
    
    // Before we run them lets reschedule first one to run after job_two
    if(workerpool.Reschedule(Core::Time::Now() + 15, job_one))
    {
        std::cout << "Job one rescheduled" << std::endl;
    }
    else std::cout << "Error while rescheduling occured" << std::endl;

    // Run jobs and revoke third one
    workerpool.Run();
    if(uint32_t errorCode = workerpool.Revoke(job_three) == 0) job_three.Cancel();

    // Print error code to make sure job three was revoked 
    std::cout << "Job three error code: " << errorCode << std::endl;

    workerpool.Stop();
}


// Run test of your choice
int main() {
    for(1)
    {
        std::cout << "Choose test to run: " << std::endl;
        std::cout << "1 - Creating and submiting job" << std::endl;
        std::cout << "2 - Scheduling jobs" << std::endl;
        std::cout << "Press anything else to quit" << std::endl;
        int input;
        std::cin >> input;
        switch(input)
        {
            case 1: CreateAndSubmitJob();
            case 2: ScheduleJobs();
            default: break;
        }
    }

    return 0;
}
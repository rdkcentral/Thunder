#include "Module.h"

// Test job class
class TestJob : public Core::IDispatch
{
    public:
    enum Status {
         INITIATED,
         CANCELED,
         COMPLETED,
    };

    TestJob() = delete;
    TestJob(const TestJob& copy) = delete;
    TestJob& operator=(const TestJob& RHS) = delete;
    ~TestJob() override = default;
    TestJob(const Status status, const uint32_t waitTime = 0,)
        : _status(status)
        , _waitTime(waitTime)
    {
    }

public:
    Status GetStatus()
    {
        return _status;
    }
    void Cancelled()
    {
        _status = (_status != COMPLETED) ? CANCELED : _status;
    }
    void Dispatch() override
    {
        _status = COMPLETED;
        usleep(_waitTime);
    }

private:
    Status _status;
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
    Core::ProxyType<Core::IDispatch> job_one = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(TestJob::Status INITIATED, 0));
    workerpool.Submit(job);

    // Print job status to check if it is initiated
    std::cout << "Job status: " << job.GetStatus() << std::endl;

    // Now run the job and wait for it to complete
    workerpool.Run();
    std::this_thread::sleep_for::(std::chrono::seconds(2));

    // Print job status to check if it is completed
    std::cout << "Job status: " << job.GetStatus() << std::endl;

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
    Core::ProxyType<Core::IDispatch> job_one = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(TestJob::Status INITIATED, 50));
    Core::ProxyType<Core::IDispatch> job_two = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(TestJob::Status INITIATED, 100));
    Core::ProxyType<Core::IDispatch> job_three = Core::ProxyType<Core::IDispatch>(Core::ProxyType<TestJob>(TestJob::Status INITIATED, 150));

    // Print jobs status to check if they are initiated
    std::cout << "Job one status: " << job_one.GetStatus() << std::endl;
    std::cout << "Job two status: " << job_two.GetStatus() << std::endl;
    std::cout << "Job three status: " << job_three.GetStatus() << std::endl;

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
    if(uint32_t errorCode = workerpool.Revoke(job_three) == 0) job_three.Cancelled();

    // Print error code to make sure job three was revoked 
    std::cout << "Job three error code: " << errorCode << std::endl;

    // Wait for jobs to complete and print their status
    std::this_thread::sleep_for::(std::chrono::seconds(20));
    std::cout << "Job one status: " << job_one.GetStatus() << std::endl;
    std::cout << "Job two status: " << job_two.GetStatus() << std::endl;
    std::cout << "Job three status: " << job_three.GetStatus() << std::endl;

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
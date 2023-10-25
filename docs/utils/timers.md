# Scheduler
## Overview
`Workerpool` has the ability to schedule `jobs` using the `Scheduler` class. This can be very helpful when you do not want to start a `job` immediately but at a specific time. To learn more about how to create a `job` read [Workerpool](../threading/worker-pool) section of documentation. 

 To schedule a `job` we use several methods for this:

* `Schedule()`
* `Reschedule()`
* `Revoke()`

## Scheduler methods

### Schedule
`Schedule()` is used to plan when to perform the `job` we specified. Using this method is very simple. We specify the `job` and the date on which to start performing it in the appropriate format.
!!! note
    In case the given time would point to the past, the task will start executing immediately.

```cpp
void Schedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) override
        {
            if (time > Core::Time::Now()) {
                ASSERT(job.IsValid() == true);
                ASSERT(_timer.HasEntry(Timer(this, job)) == false);

                _timer.Schedule(time, Timer(this, job));
            }
            else {
                _threadPool.Submit(job, Core::infinite);
            }
        }
```


### Reschedule
`Reschedule()` method is used to change the execution date of an already scheduled `job`. It is used in the same way as the `Schedule()` method. It works as follows, first the scheduled `job` is attempted to be canceled and then the Schedule with the newly specified time is used.
!!! note
    `Reschedule()` returns true when the `job` cancellation succeeds and false when it fails, this can happen when the given task was not previously scheduled using `Schedule()`

```cpp
bool Reschedule(const Core::Time& time, const Core::ProxyType<IDispatch>& job) override
        {
            ASSERT(job.IsValid() == true);

            bool revoked = (Revoke(job) != Core::ERROR_UNKNOWN_KEY);
            Schedule(time, job);
            return (revoked);
        }
```


### Revoke
`Revoke()` is used to cancel the execution of a `job`. We use it by specifying the `job` we want to cancel and the time after which it should be cancelled. We do not need to specify the time, if we leave the default value, the `job` will be canceled immediately. The `Reschedule` method returns a numeric value. 

 Error codes that can be returned:

* `ERROR_NONE` - 0 (no errors)
* `ERROR_UNKNOWN_KEY` - 22

```cpp
uint32_t Revoke(const Core::ProxyType<IDispatch>& job, const uint32_t waitTime = Core::infinite) override
        {
            uint32_t result(_timer.Revoke(Timer(this, job)) ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY);

            uint32_t report = _threadPool.Revoke(job, waitTime);

            if (report == Core::ERROR_UNKNOWN_KEY) {
                report = _external.Completed(job, waitTime);
                
                if ( (report != Core::ERROR_UNKNOWN_KEY) && (result == Core::ERROR_UNKNOWN_KEY) ) {
                    result = report;
                }
            }

            return (result);
        }
```

## Example
Below is an example how to use `Scheduler` from unit tests.
```cpp
void ScheduleJobs(Core::ProxyType<IDispatch>& job, const uint16_t scheduledTime)
    {
        InsertJobData(job, scheduledTime);
        _parent.Schedule(Core::Time::Now().Add(scheduledTime), job);
    }
void RescheduleJobs(Core::ProxyType<IDispatch>& job, const uint16_t scheduledTime)
    {
        InsertJobData(job, scheduledTime);
        _parent.Reschedule(Core::Time::Now().Add(scheduledTime), job);
    }
```

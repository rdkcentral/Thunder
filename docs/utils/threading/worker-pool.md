The purpose of this document is to illuminate the underutilized potential of the Worker Pool. Even though multiple examples can be found across many repositories where Worker Pool is, in fact, properly utilized, many developers possibly unaware of this feature within Thunder still resort to alternative, less optimal thread creation methods. By delving into the nuances of multithreading and basic thread creation, we aim to pave the way for demonstrating the unparalleled advantages of embracing the Worker Pool.

## Multithreading

Multithreading, a well-known programming paradigm, empowers a program to execute multiple tasks simultaneously, optimizing CPU usage to its fullest potential. In the realm of multithreading, each independently executing section of a program is referred to as a thread. Considered through this lens, multithreading offers a multitude of advantages.

Most significant among them is the ability to concurrently execute multiple instructions, a potential game-changer for plugins aiming for elevated performance. The appeal of faster execution is undeniable, but, as with any approach, it brings its own set of considerations and challenges, particularly in embedded environments.

!!! note
	In the context of embedded systems, where resources are often constrained, embracing multithreading may lead to noteworthy drawbacks. One prominent concern is the increased memory usage, a critical factor necessary to consider when optimizing the performance in such environments.

## Ways to create threads

### Prior to C++11

In the pre-C++11 era, a prominent method for thread creation involved the use of `pthreads` - an acronym denoting `POSIX` threads. Notably, `pthreads` proved highly effective in Unix/Linux operating systems, leveraging their capabilities in multiprocessor or multicore environments to achieve optimal execution speed. However, this approach encounters a significant roadblock when transitioning to Windows, where native support for `pthreads` is lacking.

While successful in its own right, the reliance on `pthreads` highlighted a critical issue: the absence of standardized language support for thread creation. This deficiency resulted in substantial portability challenges, limiting the seamless transition of threaded code across different platforms.

### Post C++11

With C++11, `std::thread` was released. It is an improvement upon `boost::thread` but offers the advantage of cross-platform compatibility without relying on additional dependencies.

The natural thought might be to use `std::thread` universally to avoid compatibility issues. While this holds true for standard computers and modern mobile devices, the scenario changes when dealing with resource-constrained embedded systems.

Nevertheless, there is no need to worry - there are already ways to deal with all this and more in Thunder.

!!! warning
	Simply using `std::thread` or other standard ways of creating threads in your plugins is not the optimal approach when it comes to embedded systems.

## Thread Pool concept

Thread pools are software design patterns that help achieve concurrency in the execution of the computer application. The thread pool provides multiple threads that wait for tasks to be allocated for simultaneous execution by a supervising program.

!!! note
	We could say that a thread pool is a collection of *worker threads* that effectively perform asynchronous callbacks for the application and that it is mainly used to decrease the number of application threads, but also at the same time to provide management of the worker threads.

Additionally, the threads are not terminated immediately - when one of the threads completes its task, it becomes idle and ready to be sent off to another assignment. If there is no task, the thread will simply wait.

## Advantages of using Thread Pool

To truly understand why it is much more efficient to use something like a thread pool instead of simply creating your own threads, we need to look at it from various points of view. There are three main perspectives to this issue:

* memory usage,

* scalability,

* portability.

### Memory usage

Comprehending the problem of memory usage might not be easy for everyone, especially people who are used to coding applications for PC or mobile devices, which nowadays basically have unlimited memory, in particular compared to embedded systems.

!!! warning
	Higher memory usage may not seem like a huge deal when creating a few additional threads now and again on the PC with many GBs of RAM, but it definitely can be very noticeable when done on an embedded device with, for example, 512 MB.

You could be asking yourself now - why does it have to be like that, would it not be easier for embedded devices to have at least a little bit more memory, so that we as developers would not have to worry about it that much?

Unfortunately, the answer is no, and there is a very good reason for that. These embedded devices have to be as cost-efficient as possible, because saving even a tiny percentage of their production cost makes a huge difference when millions of them are being manufactured. With that in mind, it is very profitable in the long run to spend quite a bit of money to improve the software as much as possible, so it is feasible to reduce the production cost and save much more.

### Scalability

Imagine now that we are not only in an embedded environment but also in a system where dozens of plugins are running concurrently. Now, if every one of these plugins were creating new threads whenever it would like, we would for sure quickly run into a memory shortage problem. This is a serious scalability issue, and we can all agree that from an architectural point of view, it is a terrible approach.

Once again, it can be noticed that issues like that usually do not happen, for example, when we are building a relatively small PC application, but in the embedded system it is a significant concern that we as developers need to consider.

On top of that, it is worth mentioning that the very process of creating a new thread is sometimes much more resource-consuming than the actual operations which are performed by this thread. To avoid that, we need to use a thread pool design pattern, and luckily for us, that is already implemented. 

!!! note
	In summary, more threads use more memory, whereas a thread pool can be configured to split the work among existing threads and not to use too much memory, which could substantially slow down our system or even cause a crash.

### Portability

One of the main reasons for using the functionalities available in Thunder is to make the system as portable as possible. But how is that exactly achieved and what does it mean? Well, a system is considered portable if it requires very little effort to run on different platforms. Furthermore, a generalized abstraction between application logic and system interfaces is a prerequisite for portability.

!!! note
	That is exactly one of the main goals of Thunder, namely, providing an abstraction layer between plugins and the OS. The general rule of thumb is: do not do something that directly targets the OS in a plugin, since we most likely have an abstraction for that in Thunder; use these abstractions.

You may be wondering now why it is so important. Imagine that in the future, we would like to enable our system to work on a new platform, maybe even one that has not been developed yet. Of course, it would be a huge task anyhow, but think about how much easier it would be if abstractions were used in every plugin instead of each one of them directly targeting the OS on their own.

From an architectural point of view, the difference is enormous. If each plugin uses the same abstraction layer, we only need to make changes to this functionality, and we are good to go. On the other hand, if everyone targets the OS on their own, we would literally have to rewrite each and every plugin to be portable to this new environment, which obviously scales really badly with the size of the system.

## How to use Worker Pool

The whole time up to this point the concept of a thread pool has been discussed as a design pattern. As was mentioned, we have such functionality inside the Thunder core, that is, in `ThreadPool.h`. But the title of this document is Worker Pool and it is time to introduce its main features. First, it can be located in `Thunder/Source/core/WorkerPool.h`. We could say that it is an interface of sorts that simplifies the usage of a thread pool concept. It actually makes that quite easy, which will be shown later with some examples. So, no worries, it is not like you will have to learn to use something much more complex than, for example, `std::thread`.

### Most important features and methods

The main purpose of this document is to indicate how to use Worker Pool, and, of course, it cannot be done without showcasing some of its features. It would be completely unnecessary to go through the entire file and explain everything in detail because we want to use Thunder interfaces so that we do not have to worry about how everything works underneath. However, in some cases, it might be easier to understand some features when diving a bit deeper, but we shall try not to get carried away, so that you will not be scared off from using it in the process.

``` c++
template <typename IMPLEMENTATION>
class JobType : public ThreadPool::JobType<IMPLEMENTATION> {
public:
    JobType(const JobType<IMPLEMENTATION>&) = delete;
    JobType<IMPLEMENTATION>& operator=(const JobType<IMPLEMENTATION>&) = delete;

    template <typename... Args>
    JobType(Args&&... args)
        : ThreadPool::JobType<IMPLEMENTATION>(std::forward<Args>(args)...)
    {
    }
    ~JobType()
    {
        Revoke();
    }
```

The first thing worth discussing is a class template `JobType<>`, which can be seen in the above listing. Without going into too many details, this class template will allow us to create *jobs*.

!!! tip
	The concept of a job could be described as follows: inside the plugin, we implement a piece of code that should be executed in a separate thread; afterwards, we submit our job to Thunder (a piece of code responsible for that is in the listing below), and it takes care of it for us.

In a bit more detail, the Worker Pool will first try to find a worker (simply a thread inside the thread pool) who is currently not doing anything and
will wait until such a worker is found. Next, it will assign the job to a worker, and that is about it.

``` c++
bool Submit()
{
    ProxyType<IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Submit());

    if (job.IsValid()) {
        IWorkerPool::Instance().Submit(job);
    }
 
    return (ThreadPool::JobType<IMPLEMENTATION>::IsIdle() == false);
}
```

So now you know how to create a job and what that means. You have yet to find out how to do that in the code, but that is going to be covered in
the next section. Additionally, it was mentioned that the job can be submitted, but that is not the only thing you can do with a job. It is also possible to reschedule or revoke a job. All of this can be done with the use of very simple methods, namely `Submit()`, `Reschedule()` and `Revoke()`, which can be found in the following listing. On top of that, scheduling jobs will be described further in the following section. 

``` c++
void Revoke()
{
    Core::ProxyType<IDispatch> job(ThreadPool::JobType<IMPLEMENTATION>::Revoke());
    if (job.IsValid() == true) {
        Core::IWorkerPool::Instance().Revoke(job);
        ThreadPool::JobType<IMPLEMENTATION>::Revoked();
    }
}
```

You might be wondering what exactly stands behind a job, that is, what actually happens, for example, when the job is submitted. As mentioned above, the job is an object of a class template `JobType<>`. The key word here is template.

!!! tip
	When creating a job, you should include a class reference in its template. Then, you need to create a method called `Dispatch()` inside this class, and in this method you put everything that should be executed - about as difficult as using `std::thread`, but infinitely more efficient.

### Coding examples

In this section, we show what steps to take to code an explanatory job. First, we need to create it as a private member of a class, which can be seen below:

``` c++
Core::WorkerPool::JobType<className&> _job;
```

As you could have guessed, you simply substitute `className` with the name of a class that will be used inside a plugin to submit a job, and that is almost it. After declaring the member variable `_job`, we have yet to initialize it with some value. If you predicated that the value was simply going to be a *pointer to our class*, you were right. The easiest way to do that would be inside every constructor of the class, and an example of this can be found below:

``` c++
FileObserver()
    : _callback(nullptr)
    , _position(0)
    , _path()
    , _job(*this)
{
}
```

After that, it is only necessary to create the `Dispatch()` method, and we will be able to submit our job with `_job.Submit()`. Here is a `Dispatch() ` method from `FileObserver` class, but keep in mind that this can be virtually anything that you want to be done for your plugin in a separate thread:

```c++
void Dispatch()
{
    TRACE(Trace::Information, (_T("FileObserver: job is dispatched")));
    std::ifstream file(_path);
    if (file) {
        file.seekg(_position, file.beg);
        std::string str;
        while ((std::getline(file, str)) && (str.size() > 0)) {
            ASSERT(_callback != nullptr);
            _callback->NewLine(str);
        }
    }
    _position = GetCurrentPosition(_path);
}
```

!!! warning
	It is important not to forget about the `Revoke()` method and to know when to call it.

Consider a situation where the job is submitted and the class is destructed afterwards. It is essential to remember what happens after a job submission inside the Worker Pool, namely, it is waiting for any worker to be available. If the class is destroyed, either before a worker is assigned or before an actual job is finished, you will surely run into some problems. Because of that, it is worth to keep in mind the lifetime of the object you pass into the `JobType<>` template as a parameter, meaning that you must make sure it is kept alive as long as the job is submitted and/or running. Depending on the situation, the `Revoke()` method will either synchronously stop the potential run or wait for the run to complete.

If, for example, we did not pass a class to the template as a reference, the class would become a composite of the `JobType<>` object and its lifetime would then always be equal to the `JobType<>` object, which is, of course, not intended.

!!! warning
	The fact that we pass a reference to a class makes it mandatory to call `Revoke()` in its destructor, and the same applies to the `Unregister()` method when callbacks are used, as you can see in the following listing.

``` c++
~FileObserver()
{
    _job.Revoke();
    ASSERT(_callback == nullptr);
    if (_callback != nullptr)
    {
        Unregister();
    }
}
```

A basic example of properly using the Worker Pool to perform a relatively easy task can be found inside `rdkcentral/ThunderNanoServices/FileTransfer/FileTransfer.h` in the `FileObserver` class, which by the way is another useful and underused
functionality in Thunder.

## Conclusions

To sum up, the main idea is not to reinvent the wheel. When creating plugins, developers should keep in mind they are working on a large system in an embedded environment.

Because of that, from an architectural point of view, a different set of rules applies than when working on developing PC or even mobile applications. We all have to be aware of limitations like the low amount of memory available, the difficulty of keeping a system with dozens of plugins scalable, or even the necessity to use abstractions to achieve portability.

These are the main reasons why it is essential to use functionalities that are already given, instead of making things suboptimally on your own.

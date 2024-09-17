#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <thread>
#include <iostream>
#include <sstream>
#include <random>

#include <mutex>
#include <condition_variable>

#define MODULE_NAME unraveller

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>

using namespace Thunder;

namespace App {
struct IThreader {
public:
    enum Type : uint8_t {
        NONE = 0x0,
        PTHREAD = 0x1,
        STDTHREAD = 0x2,
    };

    struct ICallback {
        virtual ~ICallback() = default;
        virtual uint32_t Work() = 0;
    };

    virtual uint32_t Run(const uint32_t nThreads) = 0;
    virtual uint32_t Stop() = 0;

    virtual bool IsRunning() const = 0;

    virtual ~IThreader() = default;

    static Core::ProxyType<IThreader> Instance(const Type type, ICallback* callback);
};

static std::unordered_map<std::string, IThreader::Type> const threadTypeTable = {
    { "pthread", IThreader::Type::PTHREAD },
    { "stdthread", IThreader::Type::STDTHREAD }
};

class Printer {
public:
    Printer(Printer&&) = delete;
    Printer(const Printer&) = delete;
    Printer& operator=(Printer&&) = delete;
    Printer& operator=(const Printer&) = delete;

    Printer(std::ostream& output = std::cout)
        : _lock(std::unique_lock<std::mutex>(_mutex))
        , _output(output)
    {
    }

    template <typename TYPE>
    Printer& operator<<(const TYPE& _t)
    {
        _output << _t;
        return *this;
    }

    Printer& operator<<(std::ostream& (*fp)(std::ostream&))
    {
        _output << fp;
        return *this;
    }

private:
    std::unique_lock<std::mutex> _lock;
    std::ostream& _output;
    static std::mutex _mutex;
};

std::mutex Printer::_mutex;

class MemProber {
public:
    typedef struct {
        unsigned long size, resident, share, text, lib, data, dt;
    } mstat_t;

    MemProber();
    ~MemProber() = default;

    MemProber(MemProber&&) = delete;
    MemProber(const MemProber&) = delete;
    MemProber& operator=(MemProber&&) = delete;
    MemProber& operator=(const MemProber&) = delete;

    static bool Probe(mstat_t& mstat)
    {
        constexpr TCHAR mstatPath[] = _T("/proc/self/statm");
        bool result;

        FILE* mstatFile = fopen(mstatPath, "r");
        result = (7 != fscanf(mstatFile, "%ld %ld %ld %ld %ld %ld %ld", &mstat.size, &mstat.resident, &mstat.share, &mstat.text, &mstat.lib, &mstat.data, &mstat.dt));
        fclose(mstatFile);

        return result;
    }

    static void Dump(const mstat_t& mstat, std::ostream& ostream = std::cout)
    {
        const int32_t deltaRss(mstat.resident - _cache.resident);
        const int32_t deltaVsz(mstat.size - _cache.size);
        ostream << " PID: " << getpid() << ", RSS: " << mstat.resident << "kB(" << ((deltaRss > 0) ? "+" : "") << deltaRss << "), VSZ: " << mstat.size << "kB (" << ((deltaVsz > 0) ? "+" : "") << deltaVsz << ")";
    }

    static void Dump(std::ostream& ostream)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        mstat_t mstat;
        memset(&mstat, 0, sizeof(mstat));

        Probe(mstat);

        Dump(mstat, ostream);

        _cache = std::move(mstat);
    }

    static std::string Dump()
    {
        std::lock_guard<std::mutex> lock(_mutex); 

        std::stringstream s;

        mstat_t mstat;
        memset(&mstat, 0, sizeof(mstat));

        Probe(mstat);

        Dump(mstat, s);

        _cache = std::move(mstat); 

        return s.str();
    }
    private:
        static std::mutex _mutex;
        static mstat_t _cache;
};

std::mutex MemProber::_mutex;
MemProber::mstat_t MemProber::_cache;

MemProber::MemProber(){
    memset(&_cache, 0, sizeof(mstat_t));
}

class Randomizer {
public:
    Randomizer()
        : _engine()
        , _x()
    {
        Reseed();
    }

    ~Randomizer() = default;

    template <typename TYPE>
    const TYPE Generate()
    {
        TYPE ret(0);

        do {
            std::uniform_int_distribution<TYPE> random(0, TYPE(~0));
            ret = random(_engine);
        } while (ret == 0);

        return ret;
    }

private:
    void Reseed()
    {
        int y;

        void* z = std::malloc(sizeof(int));
        free(z);

        std::seed_seq seed{
            static_cast<long long>(reinterpret_cast<intptr_t>(&_x)),
            static_cast<long long>(reinterpret_cast<intptr_t>(&y)),
            static_cast<long long>(reinterpret_cast<intptr_t>(z)),
            static_cast<long long>(++_x),
            static_cast<long long>(std::hash<std::thread::id>()(std::this_thread::get_id())),
            static_cast<long long>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
        };

        _engine.seed(seed);
    }

    std::mt19937 _engine;
    uint32_t _x;
};

class ConsoleOptions : public Core::Options {
public:
    ConsoleOptions(ConsoleOptions&&) = delete;
    ConsoleOptions(const ConsoleOptions&) = delete;
    ConsoleOptions& operator=(ConsoleOptions&&) = delete;
    ConsoleOptions& operator=(const ConsoleOptions&) = delete;

    ConsoleOptions(int argumentCount, TCHAR* arguments[])
        : Core::Options(argumentCount, arguments, _T("t:c:s:h"))
        , Count(0)
        , Stack(0)
        , Type(IThreader::Type::NONE)
        , executableName(Core::FileNameOnly(arguments[0]))
    {
        Parse();
    }

    ~ConsoleOptions()
    {
    }

private:
    std::string Sanitize(const TCHAR data[])
    {
        std::string text(data);

        text.erase(
            std::remove_if(
                text.begin(), text.end(), [](char const c) {
                    return ' ' == c || '"' == c || '\'' == c;
                }),
            text.end());

        return text;
    }

    virtual void Option(const TCHAR option, const TCHAR* argument)
    {
        switch (option) {
        case 't': {
            auto it = threadTypeTable.find(Sanitize(argument));

            if (it != threadTypeTable.end()) {
                Type = (it->second);
            }
            break;
        }
        case 'c': {
            Count = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
            break;
        }
        case 's': {
            Stack = Core::NumberType<uint32_t>(Core::TextFragment(argument)).Value();
            break;
        }
        case 'h':
        default: {
            Printer(std::cerr) << "Usage: " << executableName << " -s<uint32> -c<uint32> -t <type>" << std::endl
                               << " -c    The number of threads" << std::endl
                               << " -s    The stack allocation in Kb" << std::endl
                               << " -t    Type of threading to use <pthread/stdthread>" << std::endl;

            exit(EXIT_FAILURE);
            break;
        }
        }
    }

public:
    uint32_t Count;
    uint32_t Stack;
    IThreader::Type Type;

private:
    const std::string executableName;
};

class CommonThreader : virtual public IThreader {
public:
    CommonThreader(CommonThreader&&) = delete;
    CommonThreader(const CommonThreader&) = delete;
    CommonThreader& operator=(CommonThreader&&) = delete;
    CommonThreader& operator=(const CommonThreader&) = delete;
    CommonThreader() = delete;

    bool IsRunning() const override
    {
        return _running;
    }
    void Arm() const
    {
        std::unique_lock<std::mutex> lock(_mutex);
    }

    void Trigger() const
    {
        _cv.notify_all();
    }

protected:
    CommonThreader(ICallback* callback)
        : _callback(callback)
        , _running(false)
        , _mutex()
        , _cv()
    {
    }

    const uint32_t Work(uint64_t tid)
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock);
        lock.unlock();

        Printer() << tid << ": Work start: " << MemProber::Dump() << std::endl;

        uint16_t sum(0);

        if ((_callback != nullptr) && (_running == true)) {
            sum = _callback->Work();
        }

        Printer() << tid << ": Work done: " << sum << MemProber::Dump() << std::endl;

        return sum;
    }

    ICallback* _callback;
    std::atomic_bool _running;

    mutable std::mutex _mutex;
    mutable std::condition_variable _cv;

    MemProber _mem_probe;
};

class PThreader : public CommonThreader {
public:
    PThreader(PThreader&&) = delete;
    PThreader(const PThreader&) = delete;
    PThreader& operator=(PThreader&&) = delete;
    PThreader& operator=(const PThreader&) = delete;
    PThreader() = delete;

    uint32_t Run(const uint32_t nThreads) override
    {

        Arm();

        _running = true;

        for (uint32_t i = 0; i < nThreads; i++) {
            pthread_t new_thread;
            pthread_create(&new_thread, NULL, Process, this);
            _threads.push_back(new_thread);
        }

        Printer() << "PThreads starting" << MemProber::Dump() << std::endl;

        Trigger();

        return Core::ERROR_NONE;
    }

    uint32_t Stop() override
    {
        Printer() << "PThreads stopping" << MemProber::Dump() << std::endl;

        _running = false;

        Trigger();

        for (auto& thread : _threads) {
            // Printer() << "Joining Thread ID: " << thread << std::endl;
            pthread_join(thread, NULL);
        }

        _threads.clear();

        return Core::ERROR_NONE;
    }

    PThreader(ICallback* callback)
        : CommonThreader(callback)
        , _threads()
    {
    }

    ~PThreader() = default;

private:
    static void* Process(void* data)
    {
        ASSERT(data != nullptr);

        PThreader* context = reinterpret_cast<PThreader*>(data);

        const uint64_t tid(pthread_self());

        if (context) {
            const uint16_t sum(context->Work(tid));
        }

        return nullptr;
    }

private:
    std::vector<pthread_t> _threads;
};

class StdThreader : public CommonThreader {
public:
    StdThreader(StdThreader&&) = delete;
    StdThreader(const StdThreader&) = delete;
    StdThreader& operator=(StdThreader&&) = delete;
    StdThreader& operator=(const StdThreader&) = delete;
    StdThreader() = delete;

    uint32_t Run(const uint32_t nThreads) override
    {
        Arm();

        _running = true;

        for (uint32_t i = 0; i < nThreads; i++) {
            _threads.emplace_back(Process, this);
        }

        Printer() << "STDThreads Starting " << MemProber::Dump() << std::endl;

        Trigger();

        return Core::ERROR_NONE;
    }

    uint32_t Stop() override
    {
        Printer() << "STDThreads Stopping " << MemProber::Dump() << std::endl;

        _running = false;

        Trigger();

        for (auto& thread : _threads) {
            // Printer() << "Joining Thread ID: " << thread.get_id() << std::endl;
            thread.join();
        }

        _threads.clear();

        return Core::ERROR_NONE;
    }

    StdThreader(ICallback* callback)
        : CommonThreader(callback)
        , _threads()
    {
    }

    ~StdThreader() = default;

private:
    static void Process(StdThreader* context)
    {
        ASSERT(context != nullptr);

        const uint64_t tid(static_cast<uint32_t>(std::hash<std::thread::id>()(std::this_thread::get_id())));

        if (context) {
            const uint16_t sum(context->Work(tid));


        }
    }

private:
    std::vector<std::thread> _threads;
};

class WorkLoad : public IThreader::ICallback {
    static constexpr uint16_t Kilobyte = 1024;

public:
    WorkLoad(const uint16_t sizeKb)
        : _sizeKb(sizeKb)
        , _mutex()
        , _cv()
        , _random()
    {
    }

    uint32_t Work()
    {
        const uint32_t bytes(_sizeKb * Kilobyte);

        uint8_t* data = static_cast<uint8_t*>(::alloca(bytes));

        for (uint32_t i = 0; i < (bytes / sizeof(uint32_t)); i++) {
            data[i * sizeof(uint32_t)] = _random.Generate<uint32_t>();
        }

        Crypto::MD5 md5(data, _sizeKb);
        
        uint16_t sum;

        ::memcpy(&sum, md5.Result(), md5.Length);

        Printer() << " - Waiting to be released: " << sum << MemProber::Dump() << std::endl;

        // wait until release
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock);
        lock.unlock();

        return sum;
    }

    void Arm()
    {
        std::unique_lock<std::mutex> lock(_mutex);
    }

    void Trigger()
    {
        _cv.notify_all();
    }

private:
    // void ReserveBlock(uint16_t kb)
    // {
    //     if (kb > 0) {
    //         uint32_t kBytes[Kilobyte / 4];

    //         for (uint16_t i = 0; i < (Kilobyte / 4); i++) {
    //             kBytes[i] = _random.Generate<uint32_t>();
    //         }

    //         ReserveBlock(--kb);
    //     } else {
    //         Printer() << "Job finished... " << std::endl;
    //         // wait until release
    //         std::unique_lock<std::mutex> lock(_mutex);
    //         _cv.wait(lock);
    //         lock.unlock();
    //         Printer() << "IJSVRIJ!" << std::endl;
    //     }
    // }

private:
    const uint16_t _sizeKb;
    std::mutex _mutex;
    std::condition_variable _cv;
    Randomizer _random;
};

Core::ProxyType<IThreader> IThreader::Instance(const IThreader::Type type, ICallback* callback)
{
    Core::ProxyType<IThreader> iface;
    static Core::ProxyListType<IThreader> _ifaces;

    switch (type) {
    case IThreader::Type::PTHREAD:
        iface = _ifaces.Instance<PThreader>(callback);
        break;

    case IThreader::Type::STDTHREAD:
        iface = _ifaces.Instance<StdThreader>(callback);
        break;

    default:
        break;
    }

    return iface;
}
}

int main(int argc, char** argv)
{
    App::ConsoleOptions consoleOptions(argc, argv);

    Core::ProxyType<App::IThreader> thread;

    if (consoleOptions.Type != App::IThreader::Type::NONE) {

        App::Printer() << "Test with " << consoleOptions.Count << " threads allocating " << consoleOptions.Stack << "kB" << std::endl;

        App::WorkLoad job(consoleOptions.Stack);

        std::atomic_bool started(false);

        thread = App::IThreader::Instance(consoleOptions.Type, &job);

        App::Printer() << "Threads initialised" << App::MemProber::Dump() << std::endl;

        char element;

        do {
            element = toupper(getchar());

            switch (element) {
            case 'S':
                if (thread->IsRunning() == false) {
                    job.Arm();
                    thread->Run(consoleOptions.Count);
                } else {
                    job.Trigger();
                    thread->Stop();
                    App::Printer() << "Released threads" << App::MemProber::Dump() << std::endl;
                }
                break;
            case 'I':
                // info
                break;
            case 'Q':
                break;
            default: {
            }
            }
        } while (element != 'Q');

    } else {
        App::Printer(std::cerr) << "Please provide a proper thread implementation." << std::endl;
    }

    if (thread.IsValid() == true) {
        thread.Release();
    }

    Core::Singleton::Dispose();

    App::Printer() << "Exit" << App::MemProber::Dump() << std::endl;

    return (0);
}

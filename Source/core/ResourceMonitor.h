#ifndef RESOURCE_MONITOR_TYPE_H
#define RESOURCE_MONITOR_TYPE_H

#include "Module.h"
#include "Portability.h"
#include "Singleton.h"
#include "Thread.h"
#include "Trace.h"

namespace WPEFramework {

namespace Core {

    struct EXTERNAL IResource {
        virtual ~IResource() {}

        typedef signed int handle;

        virtual handle Descriptor() const = 0;
        virtual uint16_t Events() = 0;
        virtual void Handle(const uint16_t events) = 0;
    };

    template <typename RESOURCE, typename WATCHDOG = Void>
    class ResourceMonitorType {
    private:
        static constexpr uint8_t FileDescriptorAllocation = 32;

        typedef ResourceMonitorType<RESOURCE> Parent;

        ResourceMonitorType(const ResourceMonitorType&) = delete;
        ResourceMonitorType& operator=(const ResourceMonitorType&) = delete;

        class MonitorWorker : public Core::Thread {
        private:
            MonitorWorker(const MonitorWorker&) = delete;
            MonitorWorker& operator=(const MonitorWorker&) = delete;

        public:
            MonitorWorker(Parent& parent)
                : Core::Thread(Thread::DefaultStackSize(), parent.Name())
                , _parent(parent)
            {
                Thread::Init();
            }
            virtual ~MonitorWorker()
            {
                Stop();
                _parent.Break();
                Wait(Thread::BLOCKED | Thread::STOPPED, Core::infinite);
            }

        public:
#ifdef __LINUX__
            virtual bool Initialize()
            {
                return ((Thread::Initialize() == true) && (_parent.Initialize() == true));
            }
#endif
            virtual uint32_t Worker()
            {
                return (_parent.Worker());
            }

        private:
            Parent& _parent;
        };

    public:
        ResourceMonitorType()
            : _monitor(nullptr)
            , _adminLock()
            , _resourceList()
            , _monitorRuns(0)
            , _watchDog()
            , _name(_T("Monitor::") + ClassNameOnly(typeid(RESOURCE).name()).Text())
#ifdef __WIN32__
            , _action(WSACreateEvent())
#else
            , _descriptorArrayLength(FileDescriptorAllocation)
            , _descriptorArray(static_cast<struct pollfd*>(::malloc(sizeof(::pollfd) * (_descriptorArrayLength + 1))))
            , _signalDescriptor(-1)
#endif
        {
        }
        ~ResourceMonitorType()
        {

            // All resources should be gone !!!
            ASSERT(_resourceList.size() == 0);

            if (_monitor != nullptr) {
                _adminLock.Lock();

                _resourceList.clear();

                _monitor->Block();
                Break();

                _adminLock.Unlock();

                delete _monitor;
            }

#ifdef __LINUX__
            ::free(_descriptorArray);
            if (_signalDescriptor != -1) {
                ::close(_signalDescriptor);
            }
#endif
#ifdef __WIN32__
            WSACloseEvent(_action);
#endif
        }

    public:
        const TCHAR* Name() const
        {
            return (_name.c_str());
        }
        uint32_t Runs() const
        {
            return (_monitorRuns);
        }
        ::ThreadId Id() const
        {
            return (_monitor != nullptr ? _monitor->Id() : 0);
        }
        void Register(RESOURCE& resource)
        {
            _adminLock.Lock();

            // Make sure this entry does not exist, only register resources once !!!
            ASSERT(std::find(_resourceList.begin(), _resourceList.end(), &resource) == _resourceList.end());

            _resourceList.push_back(&resource);

            if (_resourceList.size() == 1) {
                if (_monitor == nullptr) {
                    _monitor = new MonitorWorker(*this);

                    // Wait till we are at least initialized
                    _monitor->Wait(Thread::BLOCKED | Thread::STOPPED);
                }

                _monitor->Run();
            } else {
                Break();
            }

            _adminLock.Unlock();
        }
        void Unregister(RESOURCE& resource)
        {
            _adminLock.Lock();

            // Make sure this entry does not exist, only register resources once !!!
            typename std::list<RESOURCE*>::iterator index(std::find(_resourceList.begin(), _resourceList.end(), &resource));

            if (index != _resourceList.end()) {
                *index = nullptr;
                Break();
            }

            _adminLock.Unlock();
        }
        inline void Break()
        {

            ASSERT(_monitor != nullptr);

#ifdef __APPLE__
            int data = 0;
            ::sendto(_signalDescriptor
                    & data,
                sizeof(data), 0,
                _signalNode,
                _signalNode.Size());
#elif defined(__LINUX__)
            _monitor->Signal(SIGUSR2);
#elif defined(__WIN32__)
            ::WSASetEvent(_action);
#endif
        };

    private:
        HAS_MEMBER(Arm, hasArm);

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<hasArm<TYPE, void (TYPE::*)()>::value, void>::type
        Arm()
        {
            _watchDog.Arm();
        }

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<!hasArm<TYPE, void (TYPE::*)()>::value, void>::type
        Arm()
        {
        }

        HAS_MEMBER(Reset, hasReset);

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<hasReset<TYPE, void (TYPE::*)()>::value, void>::type
        Reset()
        {
            _watchDog.Reset();
        }

        template <typename TYPE>
        inline typename Core::TypeTraits::enable_if<!hasReset<TYPE, void (TYPE::*)()>::value, void>::type
        Reset()
        {
        }

#ifdef __LINUX__
        bool Initialize()
        {
#ifdef __APPLE__

            if ((_signalDescriptor = ::socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
                TRACE_L1("Error on creating socket SOCKET. Error %d", errno);
            } else if (SetNonBlocking(_signalDescriptor) == false) {
                _signalDescriptor = -1;
                TRACE_L1("Error on etting socket to non blocking. Error %d", errno);
            } else {
                char* file = mktemp("/tmp/ResourceMonitor.XXXXXX");

                // Do we need to find something to bind to or is it pre-destined
                _signalNode = Core::NodeId(file);
                if (::bind(_signalDescriptor, _signalNode, _signalNode.Size()) != 0) {
                    _signalDescriptor = -1;
                }
            }

#else

            sigset_t sigset;

            /* Create a sigset of all the signals that we're interested in */
            int VARIABLE_IS_NOT_USED err = sigemptyset(&sigset);
            ASSERT(err == 0);
            err = sigaddset(&sigset, SIGUSR2);
            ASSERT(err == 0);

            /* We must block the signals in order for signalfd to receive them */
            err = pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
            assert(err == 0);

            /* Create the signalfd */
            _signalDescriptor = signalfd(-1, &sigset, 0);

#endif

            ASSERT(_signalDescriptor != -1);

            _descriptorArray[0].fd = _signalDescriptor;
            _descriptorArray[0].events = POLLIN;
            _descriptorArray[0].revents = 0;

            return (_signalDescriptor != -1);
        }
#endif

#ifdef __LINUX__
        uint32_t Worker()
        {
            uint32_t delay = 0;

            _monitorRuns++;

            // Add entries not in the Array before we start !!!
            _adminLock.Lock();

            // Do we have enough space to allocate all file descriptors ?
            if ((_resourceList.size() + 1) > _descriptorArrayLength) {
                _descriptorArrayLength = ((((_resourceList.size() + 1) / FileDescriptorAllocation) + 1) * FileDescriptorAllocation);

                ::free(_descriptorArray);

                // Resize the array to fit..
                _descriptorArray = static_cast<::pollfd*>(::malloc(sizeof(::pollfd) * _descriptorArrayLength));

                _descriptorArray[0].fd = _signalDescriptor;
                _descriptorArray[0].events = POLLIN;
                _descriptorArray[0].revents = 0;
            }

            int filledFileDescriptors = 1;
            typename std::list<RESOURCE*>::iterator index = _resourceList.begin();

            // Fill in all entries required/updated..
            while (index != _resourceList.end()) {
                RESOURCE* entry = (*index);

                uint16_t events;

                if ((entry == nullptr) || ((events = entry->Events()) == 0)) {
                    index = _resourceList.erase(index);
                } else {
                    _descriptorArray[filledFileDescriptors].fd = entry->Descriptor();
                    _descriptorArray[filledFileDescriptors].events = events;
                    _descriptorArray[filledFileDescriptors].revents = 0;
                    filledFileDescriptors++;
                    index++;
                }
            }

            if (filledFileDescriptors > 1) {
                _adminLock.Unlock();

                int result = poll(_descriptorArray, filledFileDescriptors, -1);

                _adminLock.Lock();

                if (result == -1) {
                    TRACE_L1("poll failed with error <%d>", errno);

                } else if (_descriptorArray[0].revents & POLLIN) {
#ifdef __APPLE__
                    int info;
#else
                    /* We have a valid signal, read the info from the fd */
                    struct signalfd_siginfo info;
#endif
                    uint32_t VARIABLE_IS_NOT_USED bytes = read(_signalDescriptor, &info, sizeof(info));
                    ASSERT(bytes == sizeof(info) || bytes == 0);
                }

                // We are only interested in the filedescriptors that have a corresponding client.
                // We also know that once a file descriptor is not found, we handled them all...
                int fd_index = 1;
                index = _resourceList.begin();

                while (fd_index < filledFileDescriptors) {
                    ASSERT(index != _resourceList.end());

                    RESOURCE* entry = (*index);

                    // The entry might have been removed from observing in the mean time...
                    if (entry != nullptr) {

                        // As we are the only ones that take out the resources from the list, we
                        // always make sure that the iterator is on the right spot/filedescriptor.
                        ASSERT(entry->Descriptor() == _descriptorArray[fd_index].fd);

                        uint16_t flagsSet = _descriptorArray[fd_index].revents;

                        Arm<WATCHDOG>();

                        // Event if the flagsSet == 0, call handle, maybe a break was issued by this RESOURCE..
                        entry->Handle(flagsSet);

                        Reset<WATCHDOG>();
                    }

                    index++;
                    fd_index++;
                }
            } else {
                _monitor->Block();
                delay = Core::infinite;
            }

            _adminLock.Unlock();

            return (delay);
        }
#endif

#ifdef __WIN32__
        uint32_t Worker()
        {
            uint32_t delay = 0;
            typename std::list<RESOURCE*>::iterator index;

            _monitorRuns++;

            _adminLock.Lock();

            // Now iterate over the sockets and determine their states..
            index = _resourceList.begin();

            while (index != _resourceList.end()) {

                RESOURCE* entry = (*index);

                ASSERT(entry != nullptr);

                uint16_t events = entry->Events();

                if (events == 0) {
                    index = _resourceList.erase(index);
                } else {
                    if ((events & 0x8000) != 0) {
                        ::WSAEventSelect((*index)->Descriptor(), _action, (events & 0x7FFF));
                    }
                    index++;
                }
            }

            if (_resourceList.size() > 0) {

                _adminLock.Unlock();

                WaitForSingleObject(_action, Core::infinite);

                _adminLock.Lock();

                // Find all "pending" sockets and signal them..
                index = _resourceList.begin();

                ::WSAResetEvent(_action);

                while (index != _resourceList.end()) {
                    RESOURCE* entry = (*index);

                    ASSERT(index != _resourceList.end());
                    ASSERT(entry != nullptr);

                    WSANETWORKEVENTS networkEvents;

                    // Re-enable monitoring for the next round..
                    ::WSAEnumNetworkEvents(entry->Descriptor(), nullptr, &networkEvents);

                    uint16_t flagsSet = static_cast<uint16_t>(networkEvents.lNetworkEvents);

                    Arm<WATCHDOG>();

                    // Event if the flagsSet == 0, call handle, maybe a break was issued by this RESOURCE..
                    entry->Handle(flagsSet);

                    Reset<WATCHDOG>();

                    index++;
                }
            } else {
                delay = Core::infinite;
                _monitor->Block();
            }

            _adminLock.Unlock();

            return (delay);
        }
#endif

    private:
        MonitorWorker* _monitor;
        mutable Core::CriticalSection _adminLock;
        std::list<RESOURCE*> _resourceList;
        uint32_t _monitorRuns;
        WATCHDOG _watchDog;
        string _name;

#ifdef __LINUX__
        uint32_t _descriptorArrayLength;
        struct ::pollfd* _descriptorArray;
        int _signalDescriptor;
#endif

#ifdef __WIN32__
        HANDLE _action;
#endif
#ifdef __APPLE__
        Core::NodeId _signalNode;
#endif
    };

#ifdef WATCHDOG_ENABLED
    class ResourceMonitorHandler {
    private:
        ResourceMonitorHandler(const ResourceMonitorHandler& rhs) = delete;
        ResourceMonitorHandler& operator=(const ResourceMonitorHandler& rhs) = delete;

    public:
        ResourceMonitorHandler()
        {
        }
        ~ResourceMonitorHandler()
        {
        }

    private:
        uint32_t Expired()
        {
            fprintf(stderr, "===> Resource monitor thread is taking too long.");
            return Core::infinite;
        }
    };

    typedef ResourceMonitorType<IResource, WatchDogType<ResourceMonitorHandler>> ResourceMonitorBase;
#else
    typedef ResourceMonitorType<IResource> ResourceMonitorBase;
#endif

    class EXTERNAL ResourceMonitor : public ResourceMonitorBase {
    private:
        ResourceMonitor()
            : ResourceMonitorBase()
        {
        }
        ResourceMonitor(const ResourceMonitor&) = delete;
        ResourceMonitor& operator=(const ResourceMonitor&) = delete;

        friend class SingletonType<ResourceMonitor>;

    public:
        static ResourceMonitor& Instance();
        ~ResourceMonitor() {}
    };
}
} // namespace WPEFramework::Core

#endif // RESOURCE_MONITOR_TYPE_H

#pragma once

#include "Thread.h"

namespace WPEFramework {
namespace Core {

class ExitHandler : public Core::Thread {
    private:
        ExitHandler(const ExitHandler&) = delete;
        ExitHandler& operator=(const ExitHandler&) = delete;

    public:
        ExitHandler()
            : Core::Thread(Core::Thread::DefaultStackSize(), nullptr)
        {
        }
        virtual ~ExitHandler()
        {
            Stop();
            Wait(Core::Thread::STOPPED, Core::infinite);
        }

        static void Construct() {
            _adminLock.Lock();
            if (_instance == nullptr) {
                _instance = new WPEFramework::Core::ExitHandler();
                _instance->Run();
            }
            _adminLock.Unlock();
        }
        static void Destruct() {
            _adminLock.Lock();
            if (_instance != nullptr) {
                delete _instance; //It will wait till the worker execution completed
                _instance = nullptr;
            } else {
                _instance->CloseDown();
            }
            _adminLock.Unlock();
        }

    private:
        virtual uint32_t Worker() override
        {
            ASSERT(_instance != nullptr);
            _instance->CloseDown();

            Block();
            return (Core::infinite);
        }
        void CloseDown();
    private:
        static ExitHandler* _instance;
        static Core::CriticalSection _adminLock;
};

} // Core
} // WPEFramework

#ifndef __SYNCHRONIZE_H
#define __SYNCHRONIZE_H

#include "Module.h"
#include "Portability.h"

namespace WPEFramework {
namespace Core {
    template <typename MESSAGE>
    class SynchronizeType {
    private:
        SynchronizeType(const SynchronizeType<MESSAGE>&);
        SynchronizeType<MESSAGE> operator=(const SynchronizeType<MESSAGE>&);

    public:
        SynchronizeType()
            : _response(nullptr)
            , _signal(false, true)
        {
        }
        virtual ~SynchronizeType()
        {
        }

    public:
        template <typename INBOUND>
        bool Evaluate(const INBOUND& newMessage)
        {
            bool result = false;

            _adminLock.Lock();

            if ((_response != nullptr) && (_response->Copy(newMessage) == true)) {
                // It is the message we where waiting on. Signal succesfull retrieval.
                result = true;
                _response = nullptr;
                _signal.SetEvent();
            }

            _adminLock.Unlock();

            return (result);
        }
        inline void Flush()
        {
            _signal.SetEvent();
        }
        void Load(MESSAGE& response)
        {
            _adminLock.Lock();

            ASSERT(_response == nullptr);

            _signal.ResetEvent();

            _response = &response;

            _adminLock.Unlock();
        }
        void Load()
        {
            _adminLock.Lock();

            ASSERT(_response == nullptr);

            _signal.ResetEvent();

            _response = static_cast<MESSAGE*>(~0);

            _adminLock.Unlock();
        }
        void Evaluate()
        {
            _adminLock.Lock();

            if (_response == static_cast<MESSAGE*>(~0)) {
                _response = nullptr;
                _signal.SetEvent();
            }

            _adminLock.Unlock();
        }
        uint32_t Aquire(const uint32_t duration)
        {
            uint32_t result = _signal.Lock(duration);

            _adminLock.Lock();

            if (_response != nullptr) {
                // Seems it failed, question why?
                result = (_signal.IsSet() ? Core::ERROR_ASYNC_ABORTED : Core::ERROR_TIMEDOUT);
                _response = nullptr;
            }

            _adminLock.Unlock();

            return (result);
        }
        inline void Lock()
        {
            _adminLock.Lock();
        }
        inline void Unlock()
        {
            _adminLock.Unlock();
        }

    private:
        MESSAGE* _response;
        Core::Event _signal;
        Core::CriticalSection _adminLock;
    };
}
} // namespace Core

#endif // __SYNCHRONIZE_H

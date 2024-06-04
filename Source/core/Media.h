 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __IMEDIA_H
#define __IMEDIA_H

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    struct IMedia {
        virtual ~IMedia() = default;

        virtual string RemoteId() const = 0;
        virtual uint32_t Open(const uint32_t waitTime) = 0;
        virtual uint32_t Close(const uint32_t waitTime) = 0;
        virtual uint32_t Reset() = 0;
        virtual uint32_t Write(uint16_t& nSize,
            const uint8_t szBytes[],
            const uint32_t nTime = Core::infinite)
            = 0;
        virtual uint32_t Read(uint16_t& nSize,
            uint8_t szBytes[],
            const uint32_t nTime = Core::infinite)
            = 0;
    };

    template <typename SOURCE>
    class Media : public IMedia {
    private:
        class Handler : public SOURCE {
        public:
            Handler() = delete;
            Handler(const Handler&) = delete;
            Handler(Handler&&) = delete;
            Handler& operator=(const Handler&) = delete;
            Handler& operator=(Handler&&) = delete;

        public:
            Handler(Media<SOURCE>& parent)
                : SOURCE()
                , _parent(parent)
            {
            }
            template <typename Arg1>
            Handler(Media<SOURCE>& parent, Arg1 arg1)
                : SOURCE(arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            Handler(Media<SOURCE>& parent, Arg1 arg1, Arg2 arg2)
                : SOURCE(arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            Handler(Media<SOURCE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : SOURCE(arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            Handler(Media<SOURCE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : SOURCE(arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            Handler(Media<SOURCE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : SOURCE(arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            Handler(Media<SOURCE>& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : SOURCE(arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
            {
            }
            ~Handler()
            {
            }

        protected:
            virtual void StateChange()
            {
                _parent.Reevaluate();
            }
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }
            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

        private:
            Media<SOURCE>& _parent;
        };

        Media(const Media<SOURCE>&) = delete;
        Media<SOURCE> operator=(const Media<SOURCE>&) = delete;

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        Media()
            : IMedia()
            , _channel(*this)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1>
        Media(Arg1 arg1)
            : IMedia()
            , _channel(*this, arg1)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2>
        Media(Arg1 arg1, Arg2 arg2)
            : IMedia()
            , _channel(*this, arg1, arg2)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        Media(Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : IMedia()
            , _channel(*this, arg1, arg2, arg3)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        Media(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : IMedia()
            , _channel(*this, arg1, arg2, arg3, arg4)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        Media(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : IMedia()
            , _channel(*this, arg1, arg2, arg3, arg4, arg5)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        Media(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : IMedia()
            , _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
            , _sendSignal(false, true)
            , _sendBuffer(nullptr)
            , _sendSize(0)
            , _receiveSignal(false, true)
            , _receiveBuffer(nullptr)
            , _receiveSize(0)
            , _aborting(false)
            , _adminLock()
        {
        }
POP_WARNING()
        ~Media() override = default;

    public:
        SOURCE& Source()
        {
            return (_channel);
        }
        virtual string RemoteId() const
        {
            return (_channel.RemoteId());
        }
        virtual uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        virtual uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        virtual uint32_t Reset()
        {
            _adminLock.Lock();

            if ((_sendBuffer != nullptr) || (_receiveBuffer != nullptr)) {
                _aborting = true;

                // Make sure the blocking threads see the state change
                Reevaluate();

                _adminLock.Unlock();

                // Wait till the flag is down again..
                while (_aborting == true) {
                    // give up our slice, we wait for action..
                    SleepMs(10);
                }
            } else {
                _adminLock.Unlock();
            }

            _channel.Flush();

            return (ERROR_NONE);
        }
        virtual uint32_t Write(uint16_t& sendSize, const uint8_t dataFrame[], const uint32_t duration)
        {
            uint32_t result = ERROR_ILLEGAL_STATE;

            Core::Time currentTime(Core::Time::Now());
            uint32_t current = static_cast<uint32_t>(currentTime.Ticks());

            currentTime.Add(duration);

            uint32_t endTick = static_cast<uint32_t>(currentTime.Ticks());

            _adminLock.Lock();

            if ((_sendBuffer == nullptr) && (_aborting == false)) {
                _sendSize = sendSize;
                _sendBuffer = &dataFrame[0];

                _sendSignal.ResetEvent();

                _adminLock.Unlock();

                _channel.Trigger();

                while ((_channel.IsOpen() == true) && (_sendSize != 0) && (current < endTick) && (_aborting == false)) {
                    uint32_t ticksLeft = (endTick - current) / 1000;

                    TRACE_L5(_T("Waiting for %d ms"), ticksLeft);

                    _sendSignal.Lock(ticksLeft);

                    _sendSignal.ResetEvent();

                    current = static_cast<uint32_t>(Core::Time::Now().Ticks());
                }

                _adminLock.Lock();

                _sendBuffer = nullptr;
                sendSize -= _sendSize;
                result = (_sendSize == 0 ? ERROR_NONE : (_aborting ? ERROR_ASYNC_ABORTED : (_channel.IsOpen() == false ? ERROR_UNAVAILABLE : ERROR_TIMEDOUT)));

                // If we were the last, triggered by the Abort, clear the Abort.
                if (_receiveBuffer == nullptr) {
                    _aborting = false;
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual uint32_t Read(uint16_t& receiveSize, uint8_t dataFrame[], const uint32_t duration)
        {
            uint32_t result = ERROR_ILLEGAL_STATE;

            Core::Time currentTime(Core::Time::Now());
            uint32_t current = static_cast<uint32_t>(currentTime.Ticks());

            currentTime.Add(duration);

            uint32_t endTick = static_cast<uint32_t>(currentTime.Ticks());

            _adminLock.Lock();

            if ((_receiveBuffer == nullptr) && (_aborting == false)) {
                _receiveSize = receiveSize;
                _receiveBuffer = &dataFrame[0];

                _adminLock.Unlock();

                _receiveSignal.ResetEvent();

                while ((_channel.IsOpen() == true) && (_receiveSize != 0) && (current < endTick) && (_aborting == false)) {
                    uint32_t ticksLeft = (endTick - current) / 1000;

                    TRACE_L5(_T("Read waiting for %d ms"), ticksLeft);

                    _receiveSignal.Lock(ticksLeft);

                    _receiveSignal.ResetEvent();

                    current = static_cast<uint32_t>(Core::Time::Now().Ticks());
                }

                _adminLock.Lock();

                _receiveBuffer = nullptr;
                receiveSize -= _receiveSize;
                result = (_receiveSize == 0 ? ERROR_NONE : (_aborting ? ERROR_ASYNC_ABORTED : (_channel.IsOpen() == false ? ERROR_UNAVAILABLE : ERROR_TIMEDOUT)));

                // If we were the last, triggered by the Abort, clear the Abort.
                if (_sendBuffer == nullptr) {
                    _aborting = false;
                }
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        // Signal a state change, Opened, Closed or Accepted
        void Reevaluate()
        {
            // Let the process reevaluate what we did...
            _sendSignal.SetEvent();
            _receiveSignal.SetEvent();
        }
        // Methods to extract and insert data into the socket buffers
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _adminLock.Lock();

            if ((_sendBuffer != nullptr) && (_aborting == false) && (_sendSize != 0)) {
                result = (maxSendSize > _sendSize ? _sendSize : maxSendSize);

                ASSERT(result != 0);

                // copy our buffer in, as far as we can..
                ::memcpy(dataFrame, _sendBuffer, result);

                _sendSize -= result;
                _sendBuffer = &(_sendBuffer[result]);

                _sendSignal.SetEvent();
            }

            _adminLock.Unlock();

            return (result);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
        {
            uint16_t result = 0;

            _adminLock.Lock();

            if ((_receiveBuffer != nullptr) && (_aborting == false) && (_receiveSize != 0)) {
                result = (availableData > _receiveSize ? _receiveSize : availableData);

                ASSERT(result != 0);

                // copy our buffer in, as far as we can..
                ::memcpy(_receiveBuffer, dataFrame, result);

                _receiveSize -= result;
                _receiveBuffer = &(_receiveBuffer[result]);

                _receiveSignal.SetEvent();
            }

            _adminLock.Unlock();

            return (result);
        }

    private:
        Handler _channel;

        Core::Event _sendSignal;
        const uint8_t* _sendBuffer;
        uint16_t _sendSize;

        Core::Event _receiveSignal;
        uint8_t* _receiveBuffer;
        uint16_t _receiveSize;

        bool _aborting;
        Core::CriticalSection _adminLock;
    };
}
} // namespace Core

#endif // __IMEDIA_H

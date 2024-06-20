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
 
#pragma once

#include "Module.h"

namespace Thunder {

namespace Core {

    struct IOutbound {

        struct ICallback {
            virtual ~ICallback() = default;

            virtual void Updated(const Core::IOutbound& data, const uint32_t error_code) = 0;
        };

        virtual ~IOutbound() = default;

        virtual uint16_t Serialize(uint8_t stream[], const uint16_t length) const = 0;
        virtual void Reload() const = 0;
    };

    struct IInbound {

        virtual ~IInbound() = default;

        enum state : uint8_t {
            INPROGRESS,
            RESEND,
            COMPLETED
        };

        virtual uint16_t Deserialize(const uint8_t stream[], const uint16_t length) = 0;
        virtual state IsCompleted() const = 0;
    };

    template <typename CHANNEL>
    class SynchronousChannelType : public CHANNEL {
    private:
        SynchronousChannelType() = delete;
        SynchronousChannelType(const SynchronousChannelType<CHANNEL>&) = delete;
        SynchronousChannelType<CHANNEL>& operator=(const SynchronousChannelType<CHANNEL>&) = delete;

        class Frame {
        private:
            Frame() = delete;
            Frame(const Frame&) = delete;
            Frame& operator=(Frame&) = delete;

            enum state : uint8_t {
                IDLE = 0x00,
                INBOUND = 0x01,
                COMPLETE = 0x02
            };

        public:
            Frame(const IOutbound& message, IInbound* response)
                : _message(message)
                , _response(response)
                , _state(IDLE)
                , _expired(0)
                , _callback(nullptr)
            {
                _message.Reload();
            }
            Frame(const IOutbound& message, IInbound* response, IOutbound::ICallback* callback, const uint32_t waitTime)
                : _message(message)
                , _response(response)
                , _state(IDLE)
                , _expired(Core::Time::Now().Add(waitTime).Ticks())
                , _callback(callback)
            {
                _message.Reload();
            }
            ~Frame()
            {
            }

        public:
            IOutbound::ICallback* Callback() const
            {
                return (_callback);
            }
            const IOutbound& Outbound() const
            {
                return (_message);
            }
            const IInbound* Inbound() const
            {
                return (_response);
            }
            uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                uint16_t result = _message.Serialize(dataFrame, maxSendSize);

                if (result == 0) {
                    _state = (_response == nullptr ? COMPLETE : INBOUND);
                }
                return (result);
            }
            uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData)
            {
                uint16_t result = 0;

                if (_response != nullptr) {
                    result = _response->Deserialize(dataFrame, availableData);
                    IInbound::state newState = _response->IsCompleted();
                    if (newState == IInbound::COMPLETED) {
                        _state = COMPLETE;
                    } else if (newState == IInbound::RESEND) {
                        _message.Reload();
                        _state = IDLE;
                    }
                }
                return (result);
            }
            bool operator==(const IOutbound& message) const
            {
                return (&_message == &message);
            }
            bool operator!=(const IOutbound& message) const
            {
                return (&_message != &message);
            }
            bool IsSend() const
            {
                return (_state != IDLE);
            }
            bool CanBeRemoved() const
            {
                return ((_state == COMPLETE) && (_expired != 0));
            }
            bool IsComplete() const
            {
                return (_state == COMPLETE);
            }
            bool IsExpired() const
            {
                return ((_expired != 0) && (_expired < Core::Time::Now().Ticks()));
            }

        private:
            const IOutbound& _message;
            IInbound* _response;
            state _state;
            uint64_t _expired;
            IOutbound::ICallback* _callback;
        };

    public:
        template <typename... Args>
        SynchronousChannelType(Args... args)
            : CHANNEL(args...)
            , _adminLock()
            , _queue()
            , _reevaluate(false, true)
            , _waitCount(0)
        {
        }
        ~SynchronousChannelType() override
        {
            CHANNEL::Close(Core::infinite);
        }

        uint32_t Exchange(const uint32_t waitTime, const IOutbound& message)
        {

            message.Reload();

            _adminLock.Lock();

            Cleanup();

            _queue.emplace_back(message, nullptr);

            uint32_t result = Completed(_queue.back(), waitTime);

            _adminLock.Unlock();

            return (result);
        }

        uint32_t Exchange(const uint32_t waitTime, const IOutbound& message, IInbound& response)
        {

            message.Reload();

            _adminLock.Lock();

            Cleanup();

            _queue.emplace_back(message, &response);

            uint32_t result = Completed(_queue.back(), waitTime);

            _adminLock.Unlock();

            return (result);
        }
        void Revoke(const IOutbound& message)
        {
            bool trigger = false;

            _adminLock.Lock();

            Cleanup();

            typename std::list<Frame>::iterator index = std::find(_queue.begin(), _queue.end(), message);
            if (index != _queue.end()) {
                trigger = (_queue.size() > 1) && (index == _queue.begin());

                if (index->Callback() != nullptr) {
                    index->Callback()->Updated(index->Outbound(), Core::ERROR_ASYNC_ABORTED);
                }
                _queue.erase(index);
            }

            _adminLock.Unlock();

            if (trigger == true) {
                CHANNEL::Trigger();
            }
        }

    protected:
        void Send(const uint32_t waitTime, const IOutbound& message, IOutbound::ICallback* callback, IInbound* response)
        {
            message.Reload();

            _adminLock.Lock();

            Cleanup();

            _queue.emplace_back(message, response, callback, waitTime);
            bool trigger = (_queue.size() == 1);

            _adminLock.Unlock();

            if (trigger == true) {
                CHANNEL::Trigger();
            }
        }
        int Handle() const
        {
            return (static_cast<const Core::IResource&>(*this).Descriptor());
        }
        virtual void StateChange() override
        {
            _adminLock.Lock();
            Reevaluate();
            _adminLock.Unlock();
        }
        virtual uint16_t Deserialize(const uint8_t* dataFrame, const uint16_t availableData) = 0;

    private:
        void Cleanup()
        {
            while ((_queue.size() > 0) && (_queue.front().IsExpired() == true)) {
                Frame& frame = _queue.front();
                if (frame.Callback() != nullptr) {
                    frame.Callback()->Updated(frame.Outbound(), Core::ERROR_TIMEDOUT);
                }
                _queue.pop_front();
            }
        }
        void Reevaluate()
        {
            _reevaluate.SetEvent();

            while (_waitCount.load() != 0) {
                std::this_thread::yield();
            }

            _reevaluate.ResetEvent();
        }
        uint32_t Completed(const Frame& request, const uint32_t allowedTime)
        {

            Core::Time now = Core::Time::Now();
            Core::Time endTime = Core::Time(now).Add(allowedTime);
            uint32_t result = Core::ERROR_ASYNC_ABORTED;

            if (&(_queue.front()) == &request) {

                _adminLock.Unlock();

                CHANNEL::Trigger();

                _adminLock.Lock();
            }

            while ((CHANNEL::IsOpen() == true) && (request.IsComplete() == false) && (endTime > now)) {
                uint32_t remainingTime = (endTime.Ticks() - now.Ticks()) / Core::Time::TicksPerMillisecond;

                _waitCount++;

                _adminLock.Unlock();

                result = _reevaluate.Lock(remainingTime);

                _waitCount--;

                _adminLock.Lock();

                now = Core::Time::Now();
            }

            typename std::list<Frame>::iterator index = std::find(_queue.begin(), _queue.end(), request.Outbound());
            if (index != _queue.end()) {
                if (request.IsComplete() == true) {
                    result = Core::ERROR_NONE;
                }
                _queue.erase(index);
            }

            return (result);
        }

        // Methods to extract and insert data into the socket buffers
        virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize) override
        {
            uint16_t result = 0;

            _adminLock.Lock();

            // First clear all potentially expired objects...
            Cleanup();

            if (_queue.size() > 0) {
                Frame& frame = _queue.front();

                if (frame.IsSend() == false) {
                    result = frame.SendData(dataFrame, maxSendSize);
                    if (frame.CanBeRemoved() == true) {
                        _queue.pop_front();
                        if (frame.Callback() != nullptr) {
                            frame.Callback()->Updated(frame.Outbound(), Core::ERROR_NONE);
                        }
                        Reevaluate();
                    } else if (frame.IsSend() == true) {
                        Reevaluate();
                    }
                }
            }

            _adminLock.Unlock();

            return (result);
        }
        virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t availableData) override
        {
            uint16_t result = 0;

            _adminLock.Lock();

            // First clear all potentially expired objects...
            Cleanup();

            if (_queue.size() > 0) {
                Frame& frame = _queue.front();

                result = frame.ReceiveData(dataFrame, availableData);

                if (frame.CanBeRemoved() == true) {
                    ASSERT(frame.Inbound() != nullptr);
                    _queue.pop_front();
                    if (frame.Callback() != nullptr) {
                        frame.Callback()->Updated(frame.Outbound(), Core::ERROR_NONE);
                    }
                    if (_queue.size() > 0) {
                        CHANNEL::Trigger();
                    }
                } else if (frame.IsComplete()) {
                    Reevaluate();
                } else if (frame.IsSend() == false) {
                    CHANNEL::Trigger();
                }
            }

            _adminLock.Unlock();

            if (result < availableData) {
                result += Deserialize(&(dataFrame[result]), availableData - result);
            }

            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        std::list<Frame> _queue;
        Core::Event _reevaluate;
        volatile std::atomic<uint32_t> _waitCount;
    };

} // namespace Core

} // namespace Thunder

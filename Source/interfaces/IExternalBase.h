/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

// ---- Include system wide include files ----
#include "Module.h"
#include "IExternal.h"

// ---- Include local include files ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Referenced classes and types ----

// ---- Class Definition ----

// @stubgen:skip

namespace WPEFramework {
namespace Exchange {

    template <enum IExternal::identification IDENTIFIER>
    class ExternalBase : public IExternal {
    private:
        ExternalBase(const ExternalBase<IDENTIFIER>&) = delete;
        ExternalBase<IDENTIFIER>& operator=(const ExternalBase<IDENTIFIER>&) = delete;

        class Job : public Core::IDispatch {
        private:
            Job() = delete;
            Job(const Job&) = delete;
            Job& operator=(const Job&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            Job(ExternalBase* parent)
                : _parent(*parent)
                , _submitted(false)
                , _job()
            {
                ASSERT(parent != nullptr);
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            virtual ~Job()
            {
            }

        public:
            inline void Load()
            {
                _job = Core::ProxyType<Core::IDispatch>(*this);
                _job.AddRef();
            }
            void Submit()
            {

                _parent.Lock();
                if (_submitted == false) {
                    _submitted = true;
                    _parent.Schedule(Core::Time(), _job);
                }
                _parent.Unlock();
            }
            void Dispose()
            {
                _parent.Lock();
                if (_submitted == true) {
                    _parent.Revoke(_job);
                    _submitted = false;
                }
                _job.Release();
                _parent.Unlock();
            }
            virtual void Dispatch()
            {
                _parent.Lock();
                std::list<IExternal::INotification*>::iterator index(_parent._clients.begin());
                _submitted = false;
                _parent.RecursiveCall(index);
            }

        private:
            ExternalBase& _parent;
            bool _submitted;
            Core::ProxyType<Core::IDispatch> _job;
        };
        class Timed : public Core::IDispatch {
        private:
            Timed() = delete;
            Timed(const Timed&) = delete;
            Timed& operator=(const Timed&) = delete;

        public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
            Timed(ExternalBase* parent)
                : _parent(*parent)
                , _nextTime(0)
                , _periodicity(0)
                , _job()
            {
                ASSERT(parent != nullptr);
            }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif
            virtual ~Timed()
            {
            }

        public:
            inline void Load()
            {
                _job = Core::ProxyType<Core::IDispatch>(*this);
                _job.AddRef();
            }
            inline uint16_t Period() const
            {
                return (_periodicity / (1000 * Core::Time::TicksPerMillisecond));
            }
            void Period(const uint16_t periodicity)
            {

                _parent.Lock();
                if (_job.IsValid()) {

                    _periodicity = 0;

                    _parent.Revoke(_job);
                } else if (periodicity != 0) {
                    _job = Core::ProxyType<Core::IDispatch>(*this);
                }

                _parent.Unlock();

                if (periodicity != 0) {

                    ASSERT(_job.IsValid() == true);
                    _periodicity = periodicity * 1000 * Core::Time::TicksPerMillisecond;
                    _nextTime = Core::Time::Now().Ticks();

                    _parent.Schedule(Core::Time(), _job);
                } else {
                    _job.Release();
                }
            }
            virtual void Dispatch()
            {
                _parent.Trigger();

                _parent.Lock();
                if (_periodicity != 0) {

                    Core::ProxyType<Core::IDispatch> job(*this);

                    _nextTime += _periodicity;
                    _parent.Schedule(Core::Time(_nextTime), job);
                }
                _parent.Unlock();
            }

        private:
            ExternalBase& _parent;
            uint64_t _nextTime;
            uint32_t _periodicity;
            Core::ProxyType<Core::IDispatch> _job;
        };

    public:
#ifdef __WINDOWS__
#pragma warning(disable : 4355)
#endif
        inline ExternalBase(const uint32_t id, const basic base, const specific spec, const dimension dim, const uint8_t decimals)
            : _adminLock()
            , _id(IDENTIFIER | (id & 0x0FFFFFFF))
            , _type((dim << 19) | ((decimals & 0x07) << 16) | (base << 12) | spec)
            , _condition(IExternal::constructing)
            , _clients()
            , _job(this)
            , _timed(this)
        {
            _job.Load();
            _timed.Load();
        }
        inline ExternalBase(const uint32_t id, const uint32_t type)
            : _adminLock()
            , _id(IDENTIFIER | (id & 0x0FFFFFFF))
            , _type(type)
            , _condition(IExternal::constructing)
            , _clients()
            , _job(this)
            , _timed(this)
        {
            _job.Load();
            _timed.Load();
        }
#ifdef __WINDOWS__
#pragma warning(default : 4355)
#endif

        virtual ~ExternalBase()
        {
            _job.Dispose();
            _timed.Period(0);
            _job.CompositRelease();
            _timed.CompositRelease();
        }

    public:
        // ------------------------------------------------------------------------
        // Convenience methods to extract interesting information from the type
        // ------------------------------------------------------------------------
        inline basic Basic() const
        {
            return (static_cast<basic>((_type >> 12) & 0xF));
        }
        inline dimension Dimension() const
        {
            return (static_cast<dimension>((_type >> 19) & 0x1FFF));
        }
        inline specific Specific() const
        {
            return (static_cast<specific>(_type & 0xFFF));
        }
        inline uint8_t Decimals() const
        {
            return ((_type >> 16) & 0x07);
        }

        // ------------------------------------------------------------------------
        // Polling interface methods
        // ------------------------------------------------------------------------
        // Define the polling time in Seconds. This value has a maximum of a 24 hour.
        inline uint16_t Period() const
        {
            return (_timed.Period());
        }
        inline void Period(const uint16_t value)
        {
            _timed.Period(value);
        }

        // ------------------------------------------------------------------------
        // IExternal default interface implementation
        // ------------------------------------------------------------------------
        // Pushing notifications to interested sinks
        virtual void Register(IExternal::INotification* sink) override
        {

            Lock();

            std::list<IExternal::INotification*>::iterator index = std::find(_clients.begin(), _clients.end(), sink);

            if (index == _clients.end()) {
                sink->AddRef();
                _clients.push_back(sink);
                sink->Update();
            }

            Unlock();
        }

        virtual void Unregister(IExternal::INotification* sink) override
        {

            Lock();

            std::list<IExternal::INotification*>::iterator index = std::find(_clients.begin(), _clients.end(), sink);

            if (index != _clients.end()) {
                sink->Release();
                _clients.erase(index);
            }

            Unlock();
        }

        virtual condition Condition() const override
        {
            return (_condition.load());
        }

        // Identification of this element.
        virtual uint32_t Identifier() const override
        {
            return (_id);
        }

        // Characteristics of this element
        virtual uint32_t Type() const override
        {
            return (_type);
        }

        virtual int32_t Minimum() const override
        {
            int32_t result = 0;

            switch (Dimension()) {
            case logic:
            case percentage:
            case kwh:
            case kvah:
            case pulses: {
                break;
            }
            case degrees: {
                result = -127;
                break;
            }
            case units: {
                result = Core::NumberType<int32_t>::Min();
                break;
            }
            }
            return (result);
        }

        virtual int32_t Maximum() const override
        {
            int32_t result = 1;
            switch (Dimension()) {
            case logic: {
                break;
            }
            case percentage: {
                result = 100;
                break;
            }
            case kwh:
            case kvah:
            case pulses:
            case units: {
                result = Core::NumberType<int32_t>::Max();
                break;
            }
            case degrees: {
                result = +512;
                break;
            }
            }
            return (result);
        }
        virtual void Activate()
        {
            IExternal::condition expected = IExternal::constructing;

            if (_condition.compare_exchange_strong(expected, IExternal::activated) == true) {
                _job.Submit();
            }
        }
        virtual void Deactivate()
        {
            IExternal::condition expected = IExternal::activated;

            if (_condition.compare_exchange_strong(expected, IExternal::deactivated) == true) {
                _job.Submit();
                _timed.Period(0);
            } else {
                expected = IExternal::constructing;
                _condition.compare_exchange_strong(expected, IExternal::deactivated);
            }
        }

        virtual void Trigger() = 0;
        virtual uint32_t Get(int32_t& value) const = 0;
        virtual uint32_t Set(const int32_t value) = 0;
        virtual void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch>& job) = 0;
        virtual void Revoke(const Core::ProxyType<Core::IDispatch>& job) = 0;

        BEGIN_INTERFACE_MAP(ExternalBase)
        INTERFACE_ENTRY(Exchange::IExternal)
        END_INTERFACE_MAP

    protected:
        inline void Updated()
        {
            Lock();
            _job.Submit();
            Unlock();
        }
        inline void Lock() const
        {
            _adminLock.Lock();
        }
        inline void Unlock() const
        {
            _adminLock.Unlock();
        }
        inline void ChangeTypeId(const uint32_t id, const uint32_t type)
        {
            _id = id;
            _type = type;
        }

    private:
        void RecursiveCall(std::list<IExternal::INotification*>::iterator& position)
        {
            if (position == _clients.end()) {
                Unlock();
            } else {
                IExternal::INotification* client(*position);
                client->AddRef();
                position++;
                RecursiveCall(position);
                client->Update();
                client->Release();
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        uint32_t _id;
        uint32_t _type;
        std::atomic<condition> _condition;
        std::list<IExternal::INotification*> _clients;
        Core::ProxyObject<Job> _job;
        Core::ProxyObject<Timed> _timed;
    };
}

} // Namespace Exchange

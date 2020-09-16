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

#ifndef __PLUGIN_FRAMEWORK_REQUEST_H
#define __PLUGIN_FRAMEWORK_REQUEST_H

#include "Module.h"

namespace WPEFramework {
namespace PluginHost {

    // Forward declaration. We only have a smart pointer to a service.
    class Service;

    // Request related classes
    // These classes realize the allocation and supply of requests. Whenever
    // a reuest is coming in, all information, gathered during the loading
    // of the request, is stored in the request. The pool, recycles requests
    // used but are not actively used.
    class EXTERNAL Request : public Web::Request {
    public:
        enum enumState : uint8_t {
            INCOMPLETE = 0x01,
            OBLIVIOUS,
            MISSING_CALLSIGN,
            INVALID_VERSION,
            COMPLETE,
            UNAUTHORIZED,
            SERVICE_CALL = 0x80	
        };

    private:
        Request(const Request&) = delete;
        Request& operator=(const Request&) = delete;

    public:
        Request();
        virtual ~Request();

    public:
		// This method tells us if this call was received over the 
		// Service prefix path or (if it is false) over the JSONRPC
		// prefix path.
        inline bool ServiceCall() const
        {
            return ((_state & SERVICE_CALL) != 0);
        }
        inline enumState State() const
        {
            return (static_cast<enumState>(_state & 0x7F));
        }
        inline Core::ProxyType<PluginHost::Service>& Service()
        {
            return (_service);
        }
		inline void Unauthorized() {
            _state = ((_state & 0x80) | UNAUTHORIZED);
		}

        void Clear();
        void Service(const uint32_t errorCode, const Core::ProxyType<PluginHost::Service>& service, const bool serviceCall);

    private:
        uint8_t _state;
        Core::ProxyType<PluginHost::Service> _service;
    };

#ifdef THUNDER_PERFORMANCE
    class PerformanceAdministrator {
    public:
        enum StoreType : uint8_t {
            Serialization,
            Deserialization,
            Execution,
            ThreadPoolWait,
            CommunicatorWait,
            Total,
            Invalid
         };
    public:
        class Statistics {
        public:
            class Tuple {
            public:
                Tuple() {
                    Clear();
                }
                Tuple(const Tuple& copy) {
                    _minimum = copy._minimum;
                    _maximum = copy._maximum;
                    _average = copy._average;
                    _count = copy._count;
                }
                Tuple& operator= (const Tuple& rhs) {
                    _minimum = rhs._minimum;
                    _maximum = rhs._maximum;
                    _average = rhs._average;
                    _count = rhs._count;

                    return (*this);
                }
                ~Tuple() = default;

            public:
                void Clear() {
                    _minimum = ~0;
                    _maximum = 0;
                    _average = 0;
                    _count = 0;
                }
                void Measurement(const uint32_t data) {
                    if (data < _minimum) { _minimum = data; }
                    if (data > _maximum) { _maximum = data; }
                    _average = ((_count * _average) + data) / (_count + 1);
                    _count++;
                }
                uint32_t Measurement() const {
                    return _average;
                }
                uint32_t Count() const {
                    return _count;
                }

            private:
                uint32_t _minimum;
                uint32_t _maximum;
                uint32_t _average;
                uint32_t _count;
            };

        public:
            Statistics()
                : _serialization()
                , _deserialization()
                , _execution()
                , _threadPoolWait()
                , _communicatorWait()
                , _total() {
            }
            Statistics(const Statistics& copy) {
                _deserialization = copy._deserialization;
                _serialization = copy._serialization;
                _execution = copy._execution;
                _threadPoolWait = copy._threadPoolWait;
                _communicatorWait = copy._communicatorWait;
                _total = copy._total;
            }
            Statistics& operator= (const Statistics& rhs) {
                _deserialization = rhs._deserialization;
                _serialization = rhs._serialization;
                _execution = rhs._execution;
                _threadPoolWait = rhs._threadPoolWait;
                _communicatorWait = rhs._communicatorWait;
                _total = rhs._total;
                return (*this);
            }
            inline bool operator<(const Statistics& rhs) const
            {
                return true;
            }

            ~Statistics()  = default;

        public:
            void Clear() {
                _serialization.Clear();
                _deserialization.Clear();
                _execution.Clear();
                _threadPoolWait.Clear();
                _total.Clear();
            }
            void Serialization(const uint32_t data) {
                _serialization.Measurement(data);
            }
            uint32_t Serialization() const {
                return _serialization.Measurement();
            }
            void Deserialization(const uint32_t data) {
                _deserialization.Measurement(data);
            }
            uint32_t Deserialization() const {
                return _deserialization.Measurement();
            }
            void Execution(const uint32_t data) {
                _execution.Measurement(data);
            }
            uint32_t Execution() const {
                return _execution.Measurement();
            }
            void ThreadPoolWait(const uint32_t data) {
                _threadPoolWait.Measurement(data);
            }
            uint32_t ThreadPoolWait() const {
                return _threadPoolWait.Measurement();
            }
            void CommunicatorWait(const uint32_t data) {
                _communicatorWait.Measurement(data);
            }
            uint32_t CommunicatorWait() const {
                return _communicatorWait.Measurement();
            }
            void Total(const uint32_t data) {
                _total.Measurement(data);
            }
            uint32_t Total() const {
                return _total.Measurement();
            }

        private:
            Tuple _serialization;
            Tuple _deserialization;
            Tuple _execution;
            Tuple _threadPoolWait;
            Tuple _communicatorWait;
            Tuple _total;
        };

        using StatisticsList = std::list< std::pair<const uint32_t, Statistics> >;

    private:
        PerformanceAdministrator()
            : _statistics() {
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(100,  Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(200,  Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(400,  Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(800,  Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(1600, Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(3200, Statistics()));
            _statistics.emplace_back(std::pair<const uint32_t, Statistics>(6400, Statistics()));
        }

    public:
        PerformanceAdministrator(const PerformanceAdministrator&) = delete;
        PerformanceAdministrator& operator= (const PerformanceAdministrator&) = delete;

        static PerformanceAdministrator& Instance() {
        static PerformanceAdministrator singleton;
            return (singleton);
        }

        void Store(StoreType type, const uint32_t packageSize, const uint32_t time) {
            StatisticsList::iterator index(std::lower_bound(_statistics.begin(), _statistics.end(), std::pair<const uint32_t, Statistics>(packageSize, Statistics())));
            if (index != _statistics.end()) {
                switch (type) {
                case Serialization: {
                    index->second.Serialization(time);
                    break;
                }
                case Deserialization: {
                    index->second.Serialization(time);
                    break;
                }
                case Execution: {
                    index->second.Execution(time);
                    break;
                }
                case ThreadPoolWait: {
                    index->second.ThreadPoolWait(time);
                    break;
                }
                case CommunicatorWait: {
                    index->second.CommunicatorWait(time);
                    break;
                }
                case Total: {
                    index->second.Total(time);
                    break;
                }
                default:
                    ASSERT(true);
                }
            }
        }

        bool Retrieve(StoreType type, const uint32_t packageSize, uint32_t& time) const {
            StatisticsList::const_iterator index(std::lower_bound(_statistics.begin(), _statistics.end(), std::pair<const uint32_t, Statistics>(packageSize, Statistics())));
            if (index != _statistics.end()) {
                switch (type) {
                case Serialization: {
                    time = index->second.Serialization();
                    break;
                }
                case Deserialization: {
                    time = index->second.Serialization();
                    break;
                }
                case Execution: {
                    time = index->second.Execution();
                    break;
                }
                case ThreadPoolWait: {
                    time = index->second.ThreadPoolWait();
                    break;
                }
                case CommunicatorWait: {
                    time = index->second.CommunicatorWait();
                    break;
                }
                case Total: {
                    time = index->second.Total();
                    break;
                }
                default:
                    ASSERT(true);
                }
            }
            return (index != _statistics.end());
        }

    private:
        StatisticsList _statistics;
    };

    class TrackingJSONRPC : public Web::JSONBodyType<Core::JSONRPC::Message> {
    public:
        TrackingJSONRPC(const TrackingJSONRPC&) = delete;
        TrackingJSONRPC& operator= (const TrackingJSONRPC&) = delete;

        TrackingJSONRPC() = default;
        ~TrackingJSONRPC() override = default;

    public:
        void Clear() {
            _in = 0;
            _out = 0;
        }
	void In(const uint32_t data) {
            if (data == 0) {
                uint64_t now = Core::Time::Now().Ticks();
                _deserialization = static_cast<uint32_t>(now - _stamp);
		_stamp = now;
                PerformanceAdministrator::Instance().Store(PerformanceAdministrator::Deserialization, _in, _deserialization);
            }
            _in += data;
        }
	void Out(const uint32_t data) {
            if (data == 0) {
                uint64_t now = Core::Time::Now().Ticks();
                _communicatorWait = static_cast<uint32_t>(now - _stamp);
                _stamp = now;
                PerformanceAdministrator::Instance().Store(PerformanceAdministrator::CommunicatorWait, _in, _communicatorWait);
            }
            _out += data;
        }
        void Dispatch() {
            uint64_t now = Core::Time::Now().Ticks();
            _threadPoolWait = static_cast<uint32_t>(now - _stamp);
            _stamp = now;
            PerformanceAdministrator::Instance().Store(PerformanceAdministrator::ThreadPoolWait, _in, _threadPoolWait);
        }
	void Execution() {
            uint64_t now = Core::Time::Now().Ticks();
            _execution = static_cast<uint32_t>(now - _stamp);
            _stamp = now;
            PerformanceAdministrator::Instance().Store(PerformanceAdministrator::Execution, _in, _execution);
        }
        void Acquire() {
            _stamp = Core::Time::Now().Ticks();
	    _begin = _stamp;
        }
        void Relinquish() {
            uint64_t now = Core::Time::Now().Ticks();
            uint32_t total = static_cast<uint32_t>(now - _begin);
            _serialization = static_cast<uint32_t>(now - _stamp);
            PerformanceAdministrator::Instance().Store(PerformanceAdministrator::Serialization, _in, _serialization);

            // Time to register all data...We are completed, we got:
            // _deserialisationTime = Time it took to receive the full message till completion and its size in _in (bytes)
            // _threadPoolWait = Time it took for the deserialized message to be picked up by the workerpool to be executed.
            // _execution = Time it took for the request to reach completion. Actual execution time of the request.
            // _communicatorWait = Time it took after the response message was dropped for the resource monitor to pick it up for sending.
            // _serializationTime = Time it took for the response to be fully serialized and send over the line. size is in _out (bytes)
            // total = Time it took from entering the system and leaving the system. This should pretty mucch be the sum of all times.
            PerformanceAdministrator::Instance().Store(PerformanceAdministrator::Total, _in, total);
        }

    private:
        uint32_t _in;
        uint32_t _out;
        uint64_t _begin;
        uint64_t _stamp;
	uint32_t _size;
	uint32_t _deserialization;
	uint32_t _execution;
	uint32_t _serialization;
        uint32_t _threadPoolWait;
        uint32_t _communicatorWait;
    };
    using JSONRPCMessage = TrackingJSONRPC;
#else
    using JSONRPCMessage = Web::JSONBodyType<Core::JSONRPC::Message>;
#endif

    typedef Core::ProxyPoolType<PluginHost::Request> RequestPool;
}
}

#endif // __PLUGIN_FRAMEWORK_REQUEST_

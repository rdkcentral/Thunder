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

#ifndef __PLUGIN_FRAMEWORK_REQUEST_H
#define __PLUGIN_FRAMEWORK_REQUEST_H

#include "Module.h"
#include "Config.h"

namespace Thunder {
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

#if THUNDER_PERFORMANCE
    class PerformanceAdministrator {
    public:
        class Statistics {
        public:
            struct DataSet {
                uint32_t Deserialization;
                uint32_t ThreadPool;
                uint32_t Execution;
                uint32_t Communication;
                uint32_t Serialization;
                uint32_t Total;
            };
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
                Tuple(Tuple&& move) {
                    _minimum = move._minimum;
                    _maximum = move._maximum;
                    _average = move._average;
                    _count = move._count;
                    move.Clear();
                }
                Tuple& operator=(const Tuple& rhs) {
                    _minimum = rhs._minimum;
                    _maximum = rhs._maximum;
                    _average = rhs._average;
                    _count = rhs._count;

                    return (*this);
                }
                Tuple& operator=(Tuple*& move) {
                    if (this != &move) {
                        _minimum = move._minimum;
                        _maximum = move._maximum;
                        _average = move._average;
                        _count = move._count;
                        move.Clear();
		    }

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
                uint32_t Minimum() const {
                    return _minimum;
                }
                uint32_t Average() const {
                    return _average;
                }
                uint32_t Maximum() const {
                    return _maximum;
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
            Statistics() = delete;
            Statistics(Statistics&& move) = delete;
            Statistics(const Statistics& copy) = delete;
            Statistics& operator=(Statistics&& rhs) = delete;
            Statistics& operator=(const Statistics& rhs) = delete;

            Statistics(const uint32_t uptill)
                : _adminLock()
                , _limit(uptill)
		, _deserialization()
                , _threadPool()
                , _execution()
                , _communication()
                , _serialization()
                , _total() {
            }
            ~Statistics() = default;

        public:
            void Clear() {
                _deserialization.Clear();
                _threadPool.Clear();
                _execution.Clear();
                _communication.Clear();
                _serialization.Clear();
                _total.Clear();
            }
            void Add(const DataSet& data) {
		_adminLock.Lock();
                _deserialization.Measurement(data.Deserialization);
                _threadPool.Measurement(data.ThreadPool);
                _execution.Measurement(data.Execution);
                _communication.Measurement(data.Communication);
                _serialization.Measurement(data.Serialization);
                _total.Measurement(data.Total);
		_adminLock.Unlock();
            }
            uint32_t Limit() const {
                return (_limit);
            }
            Tuple Deserialization() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _deserialization;
            }
            Tuple ThreadPool() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _threadPool;
            }
            Tuple Execution() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _execution;
            }
            Tuple Communication() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _communication;
            }
            Tuple Serialization() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _serialization;
            }
            Tuple Total() const {
		Core::SafeSyncType<Core::CriticalSection> lock (_adminLock);
                return _total;
            }

        private:
            mutable Core::CriticalSection _adminLock;
            uint32_t _limit;
            Tuple _deserialization;
            Tuple _threadPool;
            Tuple _execution;
            Tuple _communication;
            Tuple _serialization;
            Tuple _total;
        };

        using StatisticsList = std::list< Statistics >;

    private:
        PerformanceAdministrator()
            : _statistics() {
            _statistics.emplace_back(100);
            _statistics.emplace_back(200);
            _statistics.emplace_back(400);
            _statistics.emplace_back(800);
            _statistics.emplace_back(1600);
            _statistics.emplace_back(3200);
            _statistics.emplace_back(6400);
            _statistics.emplace_back(Core::NumberType<uint32_t>::Max());
        }

    public:
        PerformanceAdministrator(PerformanceAdministrator&&) = delete;
        PerformanceAdministrator(const PerformanceAdministrator&) = delete;
        PerformanceAdministrator& operator=(PerformanceAdministrator&&) = delete;
        PerformanceAdministrator& operator=(const PerformanceAdministrator&) = delete;

        static PerformanceAdministrator& Instance() {
            static PerformanceAdministrator singleton;
            return (singleton);
        }

        void Clear() {
            StatisticsList::iterator index(_statistics.begin());
            while (index != _statistics.end()) {
                index->Clear();
                index++;
            }
        }

        void Store(const uint32_t packageSize, const Statistics::DataSet& statistics) {
            StatisticsList::iterator index(_statistics.begin());
            while ( (index != _statistics.end()) && (packageSize > index->Limit()) ) {
                index++;
            }
            ASSERT (index != _statistics.end());
            index->Add(statistics);
        }

        const Statistics& Retrieve(const uint32_t packageSize) const {
            StatisticsList::const_iterator index (_statistics.cbegin());
            while ( (index != _statistics.cend()) && (packageSize > index->Limit()) ) {
                index++;
            }
            ASSERT (index != _statistics.end());
            return (*index);
        }

    private:
        StatisticsList _statistics;
    };

    class TrackingJSONRPC : public  Web::JSONRPC::Body {
    public:
        TrackingJSONRPC(TrackingJSONRPC&&) = delete;
        TrackingJSONRPC(const TrackingJSONRPC&) = delete;
        TrackingJSONRPC& operator=(TrackingJSONRPC&&) = delete;
        TrackingJSONRPC& operator=(const TrackingJSONRPC&) = delete;

        TrackingJSONRPC() = default;
        ~TrackingJSONRPC() override = default;

    public:
        void Clear() {
            _in = 0;
            _out = 0;
            Web::JSONRPC::Body::Clear();

        }
	void In(const uint32_t data) {
            if (data == 0) {
                uint64_t now = Core::Time::Now().Ticks();
                _statistics.Deserialization = static_cast<uint32_t>(now - _stamp);
		        _stamp = now;
            }
            _in += data;
        }
	void Out(const uint32_t data) {
            if (data == 0) {
                uint64_t now = Core::Time::Now().Ticks();
                _statistics.Communication = static_cast<uint32_t>(now - _stamp);
                _stamp = now;
            }
            _out += data;
        }
        void Dispatch() {
            uint64_t now = Core::Time::Now().Ticks();
            _statistics.ThreadPool = static_cast<uint32_t>(now - _stamp);
            _stamp = now;
        }
	void Execution() {
            uint64_t now = Core::Time::Now().Ticks();
            _statistics.Execution = static_cast<uint32_t>(now - _stamp);
            _stamp = now;
        }
        void Acquire() {
            _stamp = Core::Time::Now().Ticks();
	    _begin = _stamp;
        }
        void Relinquish() {
            uint64_t now = Core::Time::Now().Ticks();
            _statistics.Total = static_cast<uint32_t>(now - _begin);
            _statistics.Serialization = static_cast<uint32_t>(now - _stamp);

            // Time to register all data...We are completed, we got:
            // _deserialisationTime = Time it took to receive the full message till completion and its size in _in (bytes)
            // _threadPoolWait = Time it took for the deserialized message to be picked up by the workerpool to be executed.
            // _execution = Time it took for the request to reach completion. Actual execution time of the request.
            // _communicatorWait = Time it took after the response message was dropped for the resource monitor to pick it up for sending.
            // _serializationTime = Time it took for the response to be fully serialized and send over the line. size is in _out (bytes)
            // total = Time it took from entering the system and leaving the system. This should pretty mucch be the sum of all times.
            PerformanceAdministrator::Instance().Store(_in, _statistics);
        }

    private:
        uint32_t _in;
        uint32_t _out;
        uint64_t _begin;
        uint64_t _stamp;
        PerformanceAdministrator::Statistics::DataSet _statistics;
    };
    using JSONRPCMessage = TrackingJSONRPC;
#else
    using JSONRPCMessage = Web::JSONRPC::Body;
#endif

    typedef Core::ProxyPoolType<PluginHost::Request> RequestPool;
}
}

#endif // __PLUGIN_FRAMEWORK_REQUEST_

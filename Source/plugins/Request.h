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
        class Statistics {
        public:
            class Tuple {
            public:
                Tuple(const Tuple&) = delete;
                Tuple& operator= (const Tuple&) = delete;
                Tuple() {
                    Clear();
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

            private:
                uint32_t _minimum;
                uint32_t _maximum;
                uint32_t _average;
                uint32_t _count;
            };

        public:
            Statistics(const Statistics&) = delete;
            Statistics& operator= (const Statistics&) = delete;
            Statistics()
                : _deserialization()
                , _execution()
                , _serialization()
                , _total() {
            }
            ~Statistics()  = default;

        public:
            void Clear() {
                _deserialization.Clear();
                _execution.Clear();
                _serialization.Clear();
                _total.Clear();
            }
            void Deserialization(const uint32_t data) {
                _deserialization.Measurement(data);
            }
            void Execution(const uint32_t data) {
                _execution.Measurement(data);
            }
            void Serialization(const uint32_t data) {
                _serialization.Measurement(data);
            }
            void Total(const uint32_t data) {
                _total.Measurement(data);
            }

        private:
            Tuple _deserialization;
            Tuple _execution;
            Tuple _serialization;
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

        void Serialization(const uint32_t time, const uint32_t packageSize) {
            //auto it = _statistics.lower_bound(packageSize);
            //if (it != _statistics.end()) {
            //    it->second.Serialization(time);
            //}
        }

        void Deserialization(const uint32_t time, const uint32_t packageSize) {
            //auto it = _statistics.lower_bound(packageSize);
            //if (it != _statistics.end()) {
            //    it->second.Deserialization(time);
            //}
        }
        void Execution(const uint32_t time) {
        }
        void Total(const uint32_t time, const uint32_t packageSize) {
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
                PerformanceAdministrator::Instance().Deserialization(_deserialization, _in);
		_stamp = now;
            }
            _in += data;
        }
	void Out(const uint32_t data) {
            if (data == 0) {
                uint64_t now = Core::Time::Now().Ticks();
                _communicatorWait = static_cast<uint32_t>(now - _stamp);
                _stamp = now;
            }
            _out += data;
        }
        void Dispatch() {
            uint64_t now = Core::Time::Now().Ticks();
            _threadPoolWait = static_cast<uint32_t>(now - _stamp);
            _stamp = now;
        }
	void Execution() {
            uint64_t now = Core::Time::Now().Ticks();
            _execution = static_cast<uint32_t>(now - _stamp);
            PerformanceAdministrator::Instance().Execution(_execution);
            _stamp = now;
        }
        void Acquire() {
            _stamp = Core::Time::Now().Ticks();
	    _begin = _stamp;
        }
        void Relinquish() {
            uint64_t now = Core::Time::Now().Ticks();
            uint32_t total = static_cast<uint32_t>(now - _begin);
            _serialization = static_cast<uint32_t>(now - _stamp);
            PerformanceAdministrator::Instance().Deserialization(_serialization, _out);

            // Time to register all data...We are completed, we got:
            // _deserialisationTime = Time it took to receive the full message till completion and its size in _in (bytes)
            // _threadPoolWait = Time it took for the deserialized message to be picked up by the workerpool to be executed.
            // _execution = Time it took for the request to reach completion. Actual execution time of the request.
            // _communicatorWait = Time it took after the response message was dropped for the resource monitor to pick it up for sending.
            // _serializationTime = Time it took for the response to be fully serialized and send over the line. size is in _out (bytes)
            // total = Time it took from entering the system and leaving the system. This should pretty mucch be the sum of all times.
            PerformanceAdministrator::Instance().Total(total, 0);
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

#pragma once

#include "Module.h"
#include "Portability.h"
#include "SystemInfo.h"

namespace WPEFramework {
namespace Core {

	class StopWatch {
    public:
        StopWatch(const StopWatch&) = delete;
        StopWatch& operator= (const StopWatch&) = delete;

		StopWatch() : _systemInfo(SystemInfo::Instance()) {
            _lastMeasurement = _systemInfo.Ticks();
		}
        ~StopWatch() {
		}

	public:
		inline uint64_t Elapsed() const {
            return (_systemInfo.Ticks() - _lastMeasurement);
		}
        inline uint64_t Reset() {
            uint64_t now = _systemInfo.Ticks();
			uint64_t result = now - _lastMeasurement;
            _lastMeasurement = now;
            return (result);
        }

    private:
        SystemInfo& _systemInfo;
        uint64_t _lastMeasurement;
    };

} } // namespace WPEFramework::Core
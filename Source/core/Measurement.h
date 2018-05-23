#ifndef __MEASUREMENT_H
#define __MEASUREMENT_H

#include "Module.h"
#include "Portability.h"
#include "Number.h"

namespace WPEFramework {
namespace Core {
    template <typename TYPE>
    class MeasurementType {
    public:
        MeasurementType()
            : _min(Core::NumberType<TYPE>::Max())
            , _max(Core::NumberType<TYPE>::Min())
            , _last(0)
            , _average(0)
            , _measurements(0)
        {
        }
        MeasurementType(const MeasurementType<TYPE>& copy)
            : _min(copy._min)
            , _max(copy._max)
            , _last(copy._last)
            , _average(copy._average)
            , _measurements(copy._measurements)
        {
        }
        ~MeasurementType()
        {
        }

        MeasurementType<TYPE>& operator=(const MeasurementType<TYPE>& RHS)
        {
            _min = RHS._min;
            _max = RHS._max;
            _last = RHS._last;
            _average = RHS._average;
            _measurements = RHS._measurements;

            return (*this);
        }

    public:
        void Reset()
        {
            _min = Core::NumberType<TYPE>::Max();
            _max = Core::NumberType<TYPE>::Min();
            _last = 0;
            _average = 0;
            _measurements = 0;
        }
        void Set(const TYPE value)
        {
            if (value < _min) {
                _min = value;
            }
            if (value > _max) {
                _max = value;
            }
            _last = value;

            uint64_t collected = (static_cast<uint64_t>(_average * _measurements) + value);
            _measurements++;
            _average = static_cast<TYPE>(collected / _measurements);
        }
        inline TYPE Min() const
        {
            return (_min);
        }
        inline TYPE Max() const
        {
            return (_max);
        }
        inline TYPE Last() const
        {
            return (_last);
        }
        inline TYPE Average() const
        {
            return (_average);
        }
        inline uint32_t Measurements() const
        {
            return (_measurements);
        }

    private:
        TYPE _min;
        TYPE _max;
        TYPE _last;
        TYPE _average;
        uint32_t _measurements;
    };
}
}

#endif // __MEASUREMENT_H
#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Browser to change
    // Browser specific properties like displayed URL.
    struct IPerformance : virtual public Core::IUnknown {

        enum { ID = ID_PERFORMANCE };

        virtual ~IPerformance() {}

        virtual uint32_t Send(const uint16_t sendSize, const uint8_t buffer[]) = 0;
        virtual uint32_t Receive(uint16_t& bufferSize, uint8_t buffer[]) const = 0;
        virtual uint32_t Exchange(uint16_t& bufferSize, uint8_t buffer[], const uint16_t maxBufferSize) = 0;
    };
}
}

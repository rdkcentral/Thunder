#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This is an example to show the workings and how to develope a COMRPC/JSONRPC method/interface
    struct IMath : virtual public Core::IUnknown {

        enum { ID = ID_MATH };

        virtual ~IMath() {}

        virtual uint32_t Add(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const = 0;
        virtual uint32_t Sub(const uint16_t A, const uint16_t B, uint16_t& sum /* @out */)  const = 0;
    };
}
}

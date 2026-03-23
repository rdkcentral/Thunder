#pragma once

#include "Module.h"
#include <string>

namespace Thunder {
    namespace Exchange {
        struct EXTERNAL IEcho : virtual public Core::IUnknown {

            enum { ID = ID_ECHO };

            ~IEcho() override = default;

            virtual std::string Echo(const std::string& message) = 0;        
        };
    }
}
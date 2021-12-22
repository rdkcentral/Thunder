#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    struct EXTERNAL IMessageOutput {
        virtual ~IMessageOutput() = default;
        virtual void Output(const string& message) = 0;
    };

}
}
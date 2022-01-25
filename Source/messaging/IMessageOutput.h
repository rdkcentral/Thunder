#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    struct EXTERNAL IMessageOutput {
        virtual ~IMessageOutput() = default;
        virtual void Output(const Core::Messaging::Information& info, const Core::Messaging::IEvent* message) = 0;
    };

}
}

#ifndef __ITRACEMEDIA_H
#define __ITRACEMEDIA_H

#include "ITraceControl.h"
#include "Module.h"

namespace WPEFramework {
namespace Trace {
    struct EXTERNAL ITraceMedia {
        virtual ~ITraceMedia(){};
        virtual void Output(const char fileName[], const uint32_t lineNumber, const char className[], const ITrace* information) = 0;
    };
}
} // namespace Trace

#endif // __ITRACEMEDIA_H

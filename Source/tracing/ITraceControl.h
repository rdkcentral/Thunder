#ifndef __ITRACECONTROL_H
#define __ITRACECONTROL_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

namespace WPEFramework {
namespace Trace {
    const uint16_t TRACINGBUFFERSIZE = 1024;

    struct ITraceControl {
        virtual ~ITraceControl() {}
        virtual void Destroy() = 0;
        virtual const char* Category() const = 0;
        virtual const char* Module() const = 0;
        virtual bool Enabled() const = 0;
        virtual void Enabled(const bool enabled) = 0;
    };

    struct ITrace {
		virtual const char* Category() const = 0;
		virtual const char* Module() const = 0;
		virtual const char* Data() const = 0;
		virtual uint16_t Length() const = 0;
	};
}
}

#endif // __ITRACECONTROL_H

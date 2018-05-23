#ifndef __PROXYSTUB_ITRACING_H
#define __PROXYSTUB_ITRACING_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

namespace WPEFramework {
namespace Trace {
    struct ITraceIterator : virtual public Core::IUnknown {
        enum { ID = 0x00000003 };

        virtual ~ITraceIterator(){};

        virtual void Reset() = 0;
        virtual bool Info(bool& enabled, string& module, string& category) const = 0;
    };

    struct ITraceController : virtual public Core::IUnknown {
        enum { ID = 0x00000004 };

        virtual ~ITraceController(){};

        virtual void Enable(const bool enabled, const string& module, const string& category) = 0;
    };
}
}

#endif // __PROXYSTUB_ITRACING_H


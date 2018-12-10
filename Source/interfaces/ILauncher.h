#ifndef ILAUNCHER_H
#define ILAUNCHER_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct ILauncher : virtual public Core::IUnknown {
        enum { ID = 0x00000058 };

        virtual void Launch(const string&, const string&) = 0;
    };
}
}

#endif // ILAUNCHER_H

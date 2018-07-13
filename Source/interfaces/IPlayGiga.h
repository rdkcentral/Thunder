#ifndef _PLAYGIGA_H
#define _PLAYGIGA_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IPlayGiga : virtual public Core::IUnknown {
        enum { ID = 0x00000043 };

        virtual void Launch(const string&, const string&) = 0;
    };
}
}

#endif // _PLAYGIGA_H

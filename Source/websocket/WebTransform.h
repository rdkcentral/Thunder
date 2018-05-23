#ifndef __WEBTRANSFORM_H
#define __WEBTRANSFORM_H

#include "Module.h"

namespace WPEFramework {
namespace Web {
    class NoTransform {
    private:
        NoTransform(const NoTransform&);
        NoTransform& operator=(const NoTransform&);

    public:
        inline NoTransform()
        {
        }
        inline ~NoTransform()
        {
        }
    };
}
}

#endif // __WEBTRANSFORM_H
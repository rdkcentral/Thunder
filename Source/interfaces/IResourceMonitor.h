#ifndef __IRESOURCEMONITOR_H
#define __IRESOURCEMONITOR_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IResourceMonitor : virtual public Core::IUnknown {

        enum { ID = ID_RESOURCEMONITOR };

        virtual ~IResourceMonitor() {}
        
        virtual uint32_t Configure(PluginHost::IShell* framework) = 0;

        virtual string CompileMemoryCsv() = 0;
    };
}
}

#endif // __IRESOURCEMONITOR_H

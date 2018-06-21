#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#include "Module.h"

namespace WPEFramework {

namespace Exchange {

    // This interface gives direct access to a Blutooth server instance, running as a plugin in the framework.
    struct IPluginBluetooth : virtual public Core::IUnknown {

        enum { ID = 0x00000068 };

        virtual ~IPluginBluetooth() {}
        virtual uint32 Configure(PluginHost::IShell* service) = 0;
        virtual bool StartScan() = 0;
        virtual bool StopScan() = 0;
        virtual string ShowDeviceList() = 0;
        virtual bool Pair(string) = 0;
        virtual bool Connect(string) = 0;
        virtual bool Disconnect(string) = 0;
    };

} }

#endif // _BLUETOOTH_H

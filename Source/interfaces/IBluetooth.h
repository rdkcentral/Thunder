#ifndef _BLUETOOTH_H
#define _BLUETOOTH_H

#include "Module.h"
#include <string>

namespace WPEFramework {

namespace Exchange {

    // This interface gives direct access to a Blutooth server instance, running as a plugin in the framework.
    struct IBluetooth : virtual public Core::IUnknown {

        enum { ID = 0x00000070 };

        virtual ~IBluetooth() {}
        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
        virtual bool Scan() = 0;
        virtual bool StopScan() = 0;
        virtual string DiscoveredDevices() = 0;
        virtual string PairedDevices() = 0;
        virtual bool Pair(string) = 0;
        virtual bool Connect(string) = 0;
        virtual bool Disconnect() = 0;
        virtual bool IsScanning() = 0;
        virtual string Connected() = 0;
    };

} }

#endif // _BLUETOOTH_H

#ifndef _TUNER_H
#define _TUNER_H

#include "Module.h"

#define TUNER_PROCESS_NODE_ID "/tmp/tunerProcess0"

namespace WPEFramework {
namespace Exchange {

    struct IStreaming : virtual public Core::IUnknown {
        enum { ID = 0x00000068 };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000067 };

            virtual ~INotification() {}

            virtual void ScanningStateChanged(const uint32_t) = 0;
            virtual void CurrentChannelChanged(const string&) = 0;

            virtual void TestNotification(const string&) = 0; // XXX: Just for test
        };

        virtual ~IStreaming() {}

        virtual void Register(IStreaming::INotification*) = 0;
        virtual void Unregister(IStreaming::INotification*) = 0;

        virtual uint32_t Configure(PluginHost::IShell*) = 0;
        virtual void StartScan() = 0;
        virtual void StopScan() = 0;
        virtual void SetCurrentChannel(const string&) = 0;
        virtual const string GetCurrentChannel() = 0;
        virtual bool IsScanning() = 0;

        virtual void Test(const string&) = 0;                 // XXX: Just for test
    };
}
}

#endif // _TUNER_H

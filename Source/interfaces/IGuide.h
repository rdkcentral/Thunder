#ifndef __ISICONTROL_H
#define __ISICONTROL_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a SIControl to change
    struct IGuide : virtual public Core::IUnknown {
        enum { ID = 0x00000061 };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = 0x00000062 };

            virtual ~INotification() {}
            virtual void EITBroadcast() = 0;
            virtual void EmergencyAlert() = 0;
            virtual void ParentalControlChanged() = 0;
            virtual void ParentalLockChanged(const string&) = 0;
            virtual void TestNotification(const string&) = 0; // XXX: Just for test
        };

        virtual ~IGuide() {}
                            
        virtual uint32_t StartParser(PluginHost::IShell*) = 0;
        virtual const string GetChannels() = 0;
        virtual const string GetPrograms() = 0;
        virtual const string GetCurrentProgram(const string&) = 0;
        virtual const string GetAudioLanguages(const uint32_t) = 0;
        virtual const string GetSubtitleLanguages(const uint32_t) = 0;
        virtual bool SetParentalControlPin(const string&, const string&) = 0;
        virtual bool SetParentalControl(const string&, const bool) = 0;
        virtual bool IsParentalControlled() = 0;
        virtual bool SetParentalLock(const string&, const bool, const string&) = 0;
        virtual bool IsParentalLocked(const string&) = 0;

        virtual void Register(IGuide::INotification*) = 0;
        virtual void Unregister(IGuide::INotification*) = 0;
    };
}
}

#endif // __ISICONTROL_H

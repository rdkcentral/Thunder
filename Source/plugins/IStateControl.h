#ifndef __ISTATECONTROL_H
#define __ISTATECONTROL_H

#include "IShell.h"
#include "Module.h"

namespace WPEFramework {
namespace PluginHost {

    // This interface gives direct access to change occuring on the remote object
    struct IStateControl
        : virtual public Core::IUnknown {

        enum {
            ID = 0x0000002A
        };

        enum command {
            SUSPEND = 0x0001,
            RESUME = 0x0002
        };

        enum state {
            UNINITIALIZED = 0x0000,
            SUSPENDED = 0x0001,
            RESUMED = 0x0002,
            EXITED = 0x0003
        };

        struct INotification
            : virtual public Core::IUnknown {
            enum {
                ID = 0x0000002B
            };

            virtual ~INotification()
            {
            }

            virtual void StateChange(const IStateControl::state state) = 0;
        };

        static const TCHAR* ToString(const state value);
        static const TCHAR* ToString(const command value);

        virtual ~IStateControl()
        {
        }

        virtual uint32_t Configure(PluginHost::IShell* framework) = 0;
        virtual state State() const = 0;
        virtual uint32_t Request(const command state) = 0;

        virtual void Register(IStateControl::INotification* notification) = 0;
        virtual void Unregister(IStateControl::INotification* notification) = 0;
    };
}

namespace Core {

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IStateControl::command>*
    EnumerateType<PluginHost::IStateControl::command>::Table(const uint16_t);

    template <>
    EXTERNAL /* static */ const EnumerateConversion<PluginHost::IStateControl::state>*
    EnumerateType<PluginHost::IStateControl::state>::Table(const uint16_t);

} // namespace PluginHost
} // namespace WPEFramework

#endif // __ISTATECONTROL_H

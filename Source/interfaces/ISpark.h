#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // This interface gives direct access to a Spark instance
    struct ISpark : virtual public Core::IUnknown {

        enum { ID = ID_SPARK };

        enum state {
            PLAYING = 0x0001,
            STOPPED = 0x0002,
            SUSPENDING = 0x0004
        };

        struct INotification : virtual public Core::IUnknown {
            enum { ID = ID_SPARK_NOTIFICATION };

            virtual ~INotification() {}

            virtual void StateChange(const ISpark::state state) = 0;
        };

        virtual ~ISpark() {}

        virtual void Register(ISpark::INotification* spark) = 0;
        virtual void Unregister(ISpark::INotification* spark) = 0;

    };
}
}

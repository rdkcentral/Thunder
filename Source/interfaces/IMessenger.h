#pragma once

#include "Module.h"

// @stubgen:skip

namespace WPEFramework {

namespace Exchange {

struct IRoomAdministrator : virtual public Core::IUnknown {
    enum { ID = ID_ROOMADMINISTRATOR };

    struct INotification : virtual public Core::IUnknown {
        enum { ID = ID_ROOMADMINISTRATOR_NOTIFICATION };

        virtual void Created(const string& id) = 0;
        virtual void Destroyed(const string& id) = 0;
    };

    struct IRoom : virtual public Core::IUnknown {
        enum { ID = ID_ROOMADMINISTRATOR_ROOM };

        struct ICallback : virtual public Core::IUnknown {
           enum { ID = ID_ROOMADMINISTRATOR_ROOM_CALLBACK };

           virtual void Joined(const string& userId) = 0;
           virtual void Left(const string& userId) = 0;
        };

        struct IMsgNotification : virtual public Core::IUnknown {
           enum { ID = ID_ROOMADMINISTRATOR_ROOM_MSGNOTIFICATION };

           virtual void Message(const string& message, const string& userId) = 0;
        };

        virtual void SendMessage(const string& message) = 0;
        virtual void SetCallback(ICallback* callback) = 0;
    };

    virtual void Register(INotification* sink) = 0;
    virtual void Unregister(const INotification* sink) = 0;

    // Returns a nullptr if the userId is already taken.
    virtual IRoom* Join(const string& roomId, const string& userId, IRoom::IMsgNotification* messageSink) = 0;
};

} // namespace Exchange

} // namespace WPEFramework

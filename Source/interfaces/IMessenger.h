/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Module.h"

// @stubgen:skip

namespace WPEFramework {

namespace Exchange {

struct IRoomAdministrator : virtual public Core::IUnknown {
    enum { ID = ID_ROOMADMINISTRATOR };

    struct EXTERNAL INotification : virtual public Core::IUnknown {
        enum { ID = ID_ROOMADMINISTRATOR_NOTIFICATION };

        virtual void Created(const string& id) = 0;
        virtual void Destroyed(const string& id) = 0;
    };

    struct EXTERNAL IRoom : virtual public Core::IUnknown {
        enum { ID = ID_ROOMADMINISTRATOR_ROOM };

        struct EXTERNAL ICallback : virtual public Core::IUnknown {
           enum { ID = ID_ROOMADMINISTRATOR_ROOM_CALLBACK };

           virtual void Joined(const string& userId) = 0;
           virtual void Left(const string& userId) = 0;
        };

        struct EXTERNAL IMsgNotification : virtual public Core::IUnknown {
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

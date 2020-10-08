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

#define WPEPLAYER_PROCESS_NODE_ID "/tmp/player"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IStream : virtual public Core::IUnknown {
        enum { ID = ID_STREAM };

        enum class state : uint8_t {
            Idle = 0,
            Loading,
            Prepared,
            Controlled,
            Error
        };

        enum class streamtype : uint8_t {
            Undefined = 0,
            Cable = 1,
            Handheld = 2,
            Satellite = 4,
            Terrestrial = 8,
            DAB = 16,
            RF = 31,
            Unicast = 32,
            Multicast = 64,
            IP = 96
       };

        enum class drmtype : uint8_t {
            None = 0,
            ClearKey,
            PlayReady,
            Widevine,
            Unknown
        };

        struct EXTERNAL IElement : virtual public Core::IUnknown {
            enum { ID = ID_STREAM_ELEMENT};

            struct EXTERNAL IIterator : virtual public Core::IUnknown {
                enum { ID = ID_STREAM_ELEMENT_ITERATOR };

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;
                virtual IElement* Current() = 0;
            };

            enum class type : uint8_t {
                Unknown = 0,
                Audio,
                Video,
                Subtitles,
                Teletext,
                Data
            };

            virtual type Type() const = 0;
        };

        struct EXTERNAL IControl : virtual public Core::IUnknown {
            enum { ID = ID_STREAM_CONTROL };

            struct EXTERNAL IGeometry : virtual public Core::IUnknown {
                enum { ID = ID_STREAM_CONTROL_GEOMETRY };

                virtual uint32_t X() const = 0;
                virtual uint32_t Y() const = 0;
                virtual uint32_t Z() const = 0;
                virtual uint32_t Width() const = 0;
                virtual uint32_t Height() const = 0;
            };

            struct EXTERNAL ICallback : virtual public Core::IUnknown {
                enum { ID = ID_STREAM_CONTROL_CALLBACK };

                virtual void Event(const uint32_t eventid) = 0;
                virtual void TimeUpdate(const uint64_t position) = 0;
            };

            virtual RPC::IValueIterator* Speeds() const = 0;
            virtual void Speed(const int32_t request) = 0;
            virtual int32_t Speed() const = 0;
            virtual void Position(const uint64_t absoluteTime) = 0;
            virtual uint64_t Position() const = 0;
            virtual void TimeRange(uint64_t& begin /* @out */, uint64_t& end /* @out */) const = 0;
            virtual IGeometry* Geometry() const = 0;
            virtual void Geometry(const IGeometry* settings) = 0;
            virtual void Callback(IControl::ICallback* callback) = 0;
        };

        struct EXTERNAL ICallback : virtual public Core::IUnknown {
            enum { ID = ID_STREAM_CALLBACK };

            virtual void DRM(const uint32_t state) = 0;
            virtual void StateChange(const state newState) = 0;
            virtual void Event(const uint32_t eventid) = 0;
        };

        virtual string Metadata() const = 0;
        virtual streamtype Type() const = 0;
        virtual drmtype DRM() const = 0;
        virtual IControl* Control() = 0;
        DEPRECATED virtual void Callback(IStream::ICallback* callback) {};
        virtual void Attach(IStream::ICallback* callback) = 0;
        virtual uint32_t Detach(IStream::ICallback* callback) = 0;
        virtual state State() const = 0;
        virtual uint32_t Load(const string& configuration) = 0;
        virtual uint32_t Error() const = 0;

        virtual IElement::IIterator* Elements() = 0;
    };

    struct EXTERNAL IPlayer : virtual public Core::IUnknown {
        enum { ID = ID_PLAYER };

        virtual IStream* CreateStream(const IStream::streamtype streamType) = 0;
        virtual uint32_t Configure(PluginHost::IShell* service) = 0;
    };

} // namespace Exchange
}

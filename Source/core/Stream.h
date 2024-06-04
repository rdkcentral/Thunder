 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#ifndef __STREAMTYPE_H
#define __STREAMTYPE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace Thunder {
namespace Core {
    struct IStream {
        virtual ~IStream() = default;
        virtual bool IsOpen() const = 0;
        virtual bool IsClosed() const = 0;
        virtual string LocalId() const = 0;
        virtual string RemoteId() const = 0;
        virtual uint32_t Open(const uint32_t waitTime) = 0;
        virtual uint32_t Close(const uint32_t waitTime) = 0;
        virtual void Trigger() = 0;
    };

    template <typename BASESTREAM>
    class StreamType : public BASESTREAM, public IStream {
    public:
        StreamType(StreamType<BASESTREAM>&&) = delete;
        StreamType(const StreamType<BASESTREAM>&) = delete;
        StreamType<BASESTREAM>& operator=(StreamType<BASESTREAM>&&) = delete;
        StreamType<BASESTREAM>& operator=(const StreamType<BASESTREAM>&) = delete;

    public:
        template <typename... Args>
        StreamType(Args... args)
            : BASESTREAM(args...)
        {
        }
        ~StreamType() override = default;

    public:
        virtual bool IsOpen() const
        {
            return (BASESTREAM::IsOpen());
        }
        virtual bool IsClosed() const
        {
            return (BASESTREAM::IsClosed());
        }
        virtual string LocalId() const
        {
            return (BASESTREAM::LocalId());
        }
        virtual string RemoteId() const
        {
            return (BASESTREAM::RemoteId());
        }
        virtual uint32_t Open(const uint32_t waitTime)
        {
            return (BASESTREAM::Open(waitTime));
        }
        virtual uint32_t Close(const uint32_t waitTime)
        {
            return (BASESTREAM::Close(waitTime));
        }
        virtual void Trigger()
        {
            BASESTREAM::Trigger();
        }
    };
}

} // namespace Core

#endif /// __STREAMTYPE_H

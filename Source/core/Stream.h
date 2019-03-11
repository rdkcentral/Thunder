#ifndef __STREAMTYPE_H
#define __STREAMTYPE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Core {
    struct IStream {
        virtual ~IStream(){};
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
    private:
        StreamType(const StreamType<BASESTREAM>&);
        StreamType<BASESTREAM>& operator=(const StreamType<BASESTREAM>&);

    public:
        template <typename... Args>
        StreamType(Args... args)
            : BASESTREAM(args...)
        {
        }
        virtual ~StreamType()
        {
        }

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

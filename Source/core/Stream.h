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
        StreamType()
            : BASESTREAM()
        {
        }
        template <typename arg1>
        inline StreamType(arg1 a_Arg1)
            : BASESTREAM(a_Arg1)
        {
        }
        template <typename arg1, typename arg2>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2)
            : BASESTREAM(a_Arg1, a_Arg2)
        {
        }
        template <typename arg1, typename arg2, typename arg3>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3)
            : BASESTREAM(a_Arg1, a_Arg2, a_Arg3)
        {
        }
        template <typename arg1, typename arg2, typename arg3, typename arg4>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4)
            : BASESTREAM(a_Arg1, a_Arg2, a_Arg3, a_Arg4)
        {
        }
        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5)
            : BASESTREAM(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5)
        {
        }
        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6)
            : BASESTREAM(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6)
        {
        }
        template <typename arg1, typename arg2, typename arg3, typename arg4, typename arg5, typename arg6, typename arg7>
        inline StreamType(arg1 a_Arg1, arg2 a_Arg2, arg3 a_Arg3, arg4 a_Arg4, arg5 a_Arg5, arg6 a_Arg6, arg7 a_Arg7)
            : BASESTREAM(a_Arg1, a_Arg2, a_Arg3, a_Arg4, a_Arg5, a_Arg6, a_Arg7)
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

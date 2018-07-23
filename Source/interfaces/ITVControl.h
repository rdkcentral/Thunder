#ifndef _ITVCONTROL_H
#define _ITVCONTROL_H

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct IStream : virtual public Core::IUnknown {
        enum { ID = 0x0000006D };

        enum Stat {
            TBD
        };

        enum StreamType {
            None
        };

        enum DRMType {
            PlayReady,
            Widevine
        };

        struct IControl : virtual public Core::IUnknown {
            enum { ID = 0x0000006E };

            struct IGeometry : virtual public Core::IUnknown {
                enum { ID = 0x0000006F };

                virtual uint32_t X() const = 0;;
                virtual uint32_t Y() const = 0;
                virtual uint32_t Z() const = 0;
                virtual uint32_t Width() const = 0;
                virtual uint32_t Height() const = 0;
            };

            struct ICallback {
               virtual void DRM(uint32_t) = 0;
               virtual void StateChange(Stat) = 0;
            };

            virtual ~IControl() {};

            virtual void Speed(const uint32_t) = 0;
            virtual uint32_t Speed() const = 0;
            virtual void Position(const uint64_t) = 0;
            virtual uint64_t Position() const = 0;
            virtual void TimeRange(uint64_t&, uint64_t&) const = 0;
            virtual IGeometry* Geometry() const = 0;
            //virtual void Geometry(const IGeometry&) = 0; TODO: discuss and identify mechanism to handle class reference
            //virtual void Callback(IControl::ICallback*) = 0; TODO: discuss and identify mechanism to handle callback
        };

        struct ICallback {
            virtual void TimeUpdate(uint64_t) = 0;
        };
        virtual ~IStream() {};

        virtual StreamType Type() const = 0;
        virtual DRMType DRM() const = 0;
        virtual IControl* Control() = 0;
        //virtual void Callback(IStream::ICallback*) = 0; TODO: discuss and identify mechanism to handle callback
        virtual Stat State() const = 0;
        virtual uint32_t Load(std::string) = 0;
    };   
}
}
#endif //_ITVCONTROL_H

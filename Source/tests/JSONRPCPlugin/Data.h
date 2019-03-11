#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Data {
    struct Window {
        uint32_t X;
        uint32_t Y;
        uint32_t Width;
        uint32_t Height;
    };

    class EXTERNAL Geometry : public Core::JSON::Container {
    private:
        Geometry(const Geometry&) = delete;
        Geometry& operator=(const Geometry&) = delete;

    public:
        Geometry()
            : Core::JSON::Container()
            , X(0)
            , Y(0)
            , Width(~0)
            , Height(~0)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);
        }
        Geometry(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
            : Core::JSON::Container()
            , X(0)
            , Y(0)
            , Width(~0)
            , Height(~0)
        {
            Add(_T("x"), &X);
            Add(_T("y"), &Y);
            Add(_T("width"), &Width);
            Add(_T("height"), &Height);

            X = x;
            Y = y;
            Width = width;
            Height = height;
        }
        ~Geometry()
        {
        }

    public:
        Core::JSON::DecUInt32 X;
        Core::JSON::DecUInt32 Y;
        Core::JSON::DecUInt32 Width;
        Core::JSON::DecUInt32 Height;
    };
    class EXTERNAL Parameters : public Core::JSON::Container {
    private:
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;

    public:
        Parameters()
            : Core::JSON::Container()
            , Location(_T("DefaultValue"))
            , UTC(false)
        {
            Add(_T("location"), &Location);
            Add(_T("utc"), &UTC);
        }
        Parameters(const string location, const bool utc)
            : Core::JSON::Container()
            , Location(_T("DefaultValue"))
            , UTC(false)
        {
            Add(_T("location"), &Location);
            Add(_T("utc"), &UTC);
            Location = location;
            UTC = utc;
        }
        virtual ~Parameters()
        {
        }

    public:
        Core::JSON::String Location;
        Core::JSON::Boolean UTC;
    };
    class EXTERNAL Response : public Core::JSON::Container {
    private:
        Response(const Response&) = delete;
        Response& operator=(const Response&) = delete;

    public:
        enum state {
            ACTIVE,
            INACTIVE,
            IDLE,
            FAILURE
        };

    public:
        Response()
            : Core::JSON::Container()
            , State(FAILURE)
            , Time(0)
        {
            Add(_T("state"), &State);
            Add(_T("time"), &Time);
        }
        virtual ~Response()
        {
        }

    public:
        Core::JSON::EnumType<state> State;
        Core::JSON::DecUInt64 Time;
    };
}
}
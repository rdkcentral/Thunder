#pragma once

#include "ITraceControl.h"
#include "Module.h"
#include "TraceCategories.h"

#include <stdarg.h>

namespace WPEFramework {
namespace Logging {

#define SYSLOG(CATEGORY, PARAMETERS)                                                 \
    if (Trace::TraceType<CATEGORY, &Logging::MODULE_LOGGING>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                \
        Trace::TraceType<CATEGORY, &Logging::MODULE_LOGGING> __message__(__data__);  \
        Logging::SysLog(__FILE__, __LINE__, &__message__);                           \
    }

    void EXTERNAL SysLog(const char filename[], const uint32_t line, const Trace::ITrace* data);
    void EXTERNAL SysLog(const bool toConsole);
    extern EXTERNAL const char* MODULE_LOGGING;

    class EXTERNAL Startup {
    private:
        Startup() = delete;
        Startup(const Startup& a_Copy) = delete;
        Startup& operator=(const Startup& a_RHS) = delete;

    public:
        Startup(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Startup(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Startup()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    class EXTERNAL Shutdown {
    private:
        Shutdown() = delete;
        Shutdown(const Shutdown& a_Copy) = delete;
        Shutdown& operator=(const Shutdown& a_RHS) = delete;

    public:
        Shutdown(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Shutdown(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Shutdown()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };

    class EXTERNAL Notification {
    private:
        Notification() = delete;
        Notification(const Notification& a_Copy) = delete;
        Notification& operator=(const Notification& a_RHS) = delete;

    public:
        Notification(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Notification(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Notification()
        {
        }

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        std::string _text;
    };
}
} // namespace Logging

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

#pragma once

#include "ITraceControl.h"
#include "Module.h"
#include "TraceCategories.h"
#include "TraceUnit.h"

#include <stdarg.h>

namespace WPEFramework {
namespace Logging {

#define SYSLOG(CATEGORY, PARAMETERS)				\
    if (Logging::LoggingType<CATEGORY>::IsEnabled() == true) {  \
        Logging::LoggingType<CATEGORY> __data__ PARAMETERS;     \
        Logging::SysLog(__FILE__, __LINE__, &__data__);		\
    }

    void EXTERNAL DumpException(const string& exceptionType);
    void EXTERNAL DumpSystemFiles(const Core::process_t pid);
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
            Core::Format(_text, formatter, ap);
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
            Core::Format(_text, formatter, ap);
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
            Core::Format(_text, formatter, ap);
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

    class EXTERNAL Crash {
    public:
        Crash() = delete;
        Crash(const Crash& a_Copy) = delete;
        Crash& operator=(const Crash& a_RHS) = delete;

        Crash(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Crash(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Crash()
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

    class EXTERNAL ParsingError {
    private:
        ParsingError() = delete;
        ParsingError(const ParsingError& a_Copy) = delete;
        ParsingError& operator=(const ParsingError& a_RHS) = delete;

    public:
        ParsingError(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit ParsingError(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~ParsingError()
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

    class EXTERNAL Error {
    private:
        Error() = delete;
        Error(const Error& a_Copy) = delete;
        Error& operator=(const Error& a_RHS) = delete;

    public:
        Error(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Error(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Error()
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

    class EXTERNAL Fatal {
    private:
        Fatal() = delete;
        Fatal(const Fatal& a_Copy) = delete;
        Fatal& operator=(const Fatal& a_RHS) = delete;

    public:
        Fatal(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Fatal(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Fatal()
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

    template <typename CATEGORY>
    class LoggingType : public Trace::ITrace {
    private:
        template <typename CONTROLCATEGORY>
        class LoggingControl : public Trace::ITraceControl {
        private:
            LoggingControl(const LoggingControl<CONTROLCATEGORY>&) = delete;
            LoggingControl<CONTROLCATEGORY>& operator=(const LoggingControl<CONTROLCATEGORY>&) = delete;

        public:
            LoggingControl()
                : m_CategoryName(Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text())
                , m_Enabled(0x02)
            {
                // Register Our trace control unit, so it can be influenced from the outside
                // if nessecary..
                Trace::TraceUnit::Instance().Announce(*this);

                // Logs, by default, are enabled.
                m_Enabled |= 0x01;
            }
            ~LoggingControl() override
            {
                Destroy();
            }

        public:
            inline bool IsEnabled() const
            {
                return ((m_Enabled & 0x01) != 0);
            }
            virtual const char* Category() const
            {
                return (m_CategoryName.c_str());
            }
            virtual const char* Module() const
            {
                return (Logging::MODULE_LOGGING);
            }
            virtual bool Enabled() const
            {
                return (IsEnabled());
            }
            virtual void Enabled(const bool enabled)
            {
                m_Enabled = (m_Enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }
            virtual void Destroy()
            {
                if ((m_Enabled & 0x02) != 0) {
                    // Register Our trace control unit, so it can be influenced from the outside
                    // if nessecary..
                    Trace::TraceUnit::Instance().Revoke(*this);
                    m_Enabled = 0;
                }
            }

        protected:
            const string m_CategoryName;
            uint8_t m_Enabled;
        };

    public:
        LoggingType(const LoggingType<CATEGORY>&) = delete;
        LoggingType<CATEGORY>& operator=(const LoggingType<CATEGORY>&) = delete;

        template <typename... Args>
        LoggingType(Args... args)
            : _traceInfo(args...)
        {
        }
        virtual ~LoggingType() = default;

    public:
        // No dereference etc.. 1 straight line to enabled or not... Quick method..
        inline static bool IsEnabled()
        {
            return (s_LogControl.IsEnabled());
        }

        inline static void Enable(const bool status)
        {
            s_LogControl.Enabled(status);
        }

        virtual const char* Category() const
        {
            return (s_LogControl.Category());
        }

        virtual const char* Module() const
        {
            return (s_LogControl.Module());
        }

        virtual bool Enabled() const
        {
            return (s_LogControl.Enabled());
        }

        virtual void Enabled(const bool enabled)
        {
            s_LogControl.Enabled(enabled);
        }

        virtual void Destroy()
        {
            s_LogControl.Destroy();
        }

        virtual const char* Data() const
        {
            return (_traceInfo.Data());
        }
        virtual uint16_t Length() const
        {
            return (_traceInfo.Length());
        }

    private:
        CATEGORY _traceInfo;
        static LoggingControl<CATEGORY> s_LogControl;
    };

    template <typename CATEGORY>
    typename LoggingType<CATEGORY>::template LoggingControl<CATEGORY> LoggingType<CATEGORY>::s_LogControl;
}
} // namespace Logging

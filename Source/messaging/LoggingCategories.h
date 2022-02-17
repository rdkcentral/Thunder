/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

namespace WPEFramework {
namespace Logging {

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
}
}
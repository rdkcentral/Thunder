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
#include "TraceControl.h"


namespace WPEFramework {
namespace Trace {

    // for backwards compatibility 
    template <typename... Args>
    inline void Format(string& dst, Args&&... args) { Core::Format(dst, std::forward<Args>(args)...); }
    template <typename... Args>
    inline string Format(Args&&... args) { return Core::Format(std::forward<Args>(args)...); }

    class EXTERNAL Text {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Text(const Text& a_Copy) = delete;
        Text& operator=(const Text& a_RHS) = delete;

    public:
        inline Text()
        {
        }
        inline Text(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        inline Text(const std::string& text)
            : _text(Core::ToString(text.c_str()))
        {
        }
        inline Text(const char text[])
            : _text(Core::ToString(text))
        {
        }
#ifndef __CORE_NO_WCHAR_SUPPORT__
        inline Text(const std::wstring& text)
            : _text(Core::ToString(text.c_str()))
        {
        }
        inline Text(const wchar_t text[])
            : _text(Core::ToString(text))
        {
        }
#endif
        ~Text()
        {
        }

        inline void Set(const string& text)
        {
            _text = Core::ToString(text.c_str());
        }
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

    class EXTERNAL Constructor {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Constructor(const Constructor& a_Copy) = delete;
        Constructor& operator=(const Constructor& a_RHS) = delete;

    public:
        Constructor()
        {
        }
        ~Constructor()
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
        static const std::string _text;
    };

    class EXTERNAL Destructor {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Destructor(const Destructor& a_Copy) = delete;
        Destructor& operator=(const Destructor& a_RHS) = delete;

    public:
        Destructor()
        {
        }
        ~Destructor()
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
        static const std::string _text;
    };

    class EXTERNAL CopyConstructor {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        CopyConstructor(const CopyConstructor& a_Copy) = delete;
        CopyConstructor& operator=(const CopyConstructor& a_RHS) = delete;

    public:
        CopyConstructor()
        {
        }
        ~CopyConstructor()
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
        static const std::string _text;
    };

    class EXTERNAL AssignmentOperator {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        AssignmentOperator(const AssignmentOperator& a_Copy) = delete;
        AssignmentOperator& operator=(const AssignmentOperator& a_RHS) = delete;

    public:
        AssignmentOperator()
        {
        }
        ~AssignmentOperator()
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
        static const std::string _text;
    };

    class EXTERNAL MethodEntry {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        MethodEntry() = delete;
        MethodEntry(const MethodEntry& a_Copy) = delete;
        MethodEntry& operator=(const MethodEntry& a_RHS) = delete;

    public:
#ifndef __CORE_NO_WCHAR_SUPPORT__
        MethodEntry(const wchar_t MethodName[])
            : _text("Entered Method: ")
        {
            std::string secondPart;
            Core::ToString(MethodName, secondPart);
            _text += secondPart;
        }
#endif
        MethodEntry(const char MethodName[])
            : _text(std::string("Entered Method: ") + std::string(MethodName))
        {
        }
        ~MethodEntry()
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

    class EXTERNAL MethodExit {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        MethodExit() = delete;
        MethodExit(const MethodExit& a_Copy) = delete;
        MethodExit& operator=(const MethodExit& a_RHS) = delete;

    public:
#ifndef __CORE_NO_WCHAR_SUPPORT__
        MethodExit(const wchar_t MethodName[])
            : _text("Exit Method: ")
        {
            std::string secondPart;
            Core::ToString(MethodName, secondPart);
            _text += secondPart;
        }
#endif
        MethodExit(const char MethodName[])
            : _text(std::string("Exit Method: ") + std::string(MethodName))
        {
        }
        ~MethodExit()
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

    class EXTERNAL Information {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Information() = delete;
        Information(const Information& a_Copy) = delete;
        Information& operator=(const Information& a_RHS) = delete;

    public:
        Information(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Information(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Information()
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

    class EXTERNAL Warning {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Warning() = delete;
        Warning(const Warning& a_Copy) = delete;
        Warning& operator=(const Warning& a_RHS) = delete;

    public:
        Warning(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Warning(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Warning()
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
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
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
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
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

    class EXTERNAL Initialisation {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Initialisation() = delete;
        Initialisation(const Initialisation& a_Copy) = delete;
        Initialisation& operator=(const Initialisation& a_RHS) = delete;

    public:
        Initialisation(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Initialisation(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~Initialisation()
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

    class EXTERNAL Assert {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        Assert(const Assert& a_Copy) = delete;
        Assert& operator=(const Assert& a_RHS) = delete;

    public:
        Assert()
            : _text("Assertion: <<No description supplied>>")
        {
        }
        Assert(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        explicit Assert(const string& text)
            : _text(std::string("Assertion: ") + (Core::ToString(text)))
        {
        }
        ~Assert()
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

    class EXTERNAL Duration {
    public:
        Duration() = delete;
        Duration(const Duration& a_Copy) = delete;
        Duration& operator=(const Duration& a_RHS) = delete;

        Duration(const Core::Time& startTime, const TCHAR formatter[], ...)
        : _text()
        {
            uint64_t duration(Core::Time::Now().Ticks() - startTime.Ticks());

            std::string message;

            va_list ap;
            va_start(ap, formatter);
            Core::Format(message, formatter, ap);
            va_end(ap);
            
            Message(_T("%" PRIu64 "us, %s"), duration, message.c_str());
        }
        explicit Duration(const Core::Time& startTime)
        : _text()
        {
            uint64_t duration(Core::Time::Now().Ticks() - startTime.Ticks());
            Message(_T("%" PRIu64 "us"), duration);
        }
        ~Duration()
        {
        }
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }
    private:
        void Message(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Core::Format(_text, formatter, ap);
            va_end(ap);
        }
        std::string _text;
    };
}
} // namespace Trace


#pragma once

#include "Module.h"

#include "../tracing/tracing.h"

namespace WPEFramework {
namespace ProcessContainers {

    class EXTERNAL ProcessContainerization {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        ProcessContainerization(const ProcessContainerization& a_Copy) = delete;
        ProcessContainerization& operator=(const ProcessContainerization& a_RHS) = delete;

    public:
        ProcessContainerization(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        ProcessContainerization(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~ProcessContainerization() = default;

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
        string _text;
    };


} // ProcessContainers
} // WPEFramework


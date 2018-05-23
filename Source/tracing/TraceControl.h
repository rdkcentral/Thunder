#ifndef __TRACECONTROL_H
#define __TRACECONTROL_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "TraceUnit.h"
#include "ITraceControl.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----
#define TRACE(CATEGORY, PARAMETERS)                                                                              \
    if (Trace::TraceType<CATEGORY, &Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                            \
        Trace::TraceType<CATEGORY, &Core::System::MODULE_NAME> __message__(__data__);  \
        Trace::TraceUnit::Instance().Trace(                                                           \
            __FILE__,                                                                                            \
            __LINE__,                                                                                            \
            typeid(*this).name(),                                                                                \
            &__message__);                                                                                       \
    }

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                                                       \
    if (Trace::TraceType<CATEGORY, &Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                                            \
        Trace::TraceType<CATEGORY, &Core::System::MODULE_NAME> __message__(__data__);  \
        Trace::TraceUnit::Instance().Trace(                                                           \
            __FILE__,                                                                                            \
            __LINE__,                                                                                            \
            "<<Global>>",                                                                                        \
            &__message__);                                                                                       \
    }

// ---- Helper functions ----

// ---- Class Definition ----
namespace WPEFramework {
namespace Trace {
    template <typename CATEGORY, const char** MODULENAME>
    class TraceType : public ITrace {
    private:
        template <typename CONTROLCATEGORY, const char** CONTROLMODULE>
        class TraceControl : public ITraceControl {
        private:
            // -------------------------------------------------------------------
            // This object should not be copied or assigned. Prevent the copy
            // constructor and assignment constructor from being used. Compiler
            // generated assignment and copy methods will be blocked by the
            // following statments.
            // Define them but do not implement them, compile error/link error.
            // -------------------------------------------------------------------
            TraceControl(const TraceControl<CONTROLCATEGORY, CONTROLMODULE>&);
            TraceControl<CONTROLCATEGORY, CONTROLMODULE>& operator=(const TraceControl<CONTROLCATEGORY, CONTROLMODULE>&);

        public:
            TraceControl()
                : m_CategoryName(Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text())
                , m_Enabled(0x02)
            {
                // Register Our trace control unit, so it can be influenced from the outside
                // if nessecary..
                TraceUnit::Instance().Announce(*this);

                bool enabled = false;
                if (TraceUnit::Instance().IsDefaultCategory(*CONTROLMODULE, m_CategoryName, enabled)) {
                    if (enabled) {
                        // Better not to use virtual Enabled(...), because derived classes aren't finished yet.
                        m_Enabled = m_Enabled | 0x01;
                    }
                }
            }
            virtual ~TraceControl()
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
                return (*CONTROLMODULE);
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
                    TraceUnit::Instance().Revoke(*this);
                    m_Enabled = 0;
                }
            }

        protected:
            const string m_CategoryName;
            uint8_t m_Enabled;
        };

    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        TraceType(const TraceType<CATEGORY, MODULENAME>&) = delete;
        TraceType<CATEGORY, MODULENAME>& operator=(const TraceType<CATEGORY, MODULENAME>&) = delete;

    public:
        TraceType(CATEGORY& category)
            : _traceInfo(category)
        {
        }
        virtual ~TraceType()
        {
        }

    public:
        // No dereference etc.. 1 straight line to enabled or not... Quick method..
        inline static bool IsEnabled()
        {
            return (s_TraceControl.IsEnabled());
        }

        inline static void Enable(const bool status)
        {
            s_TraceControl.Enabled(status);
        }

        virtual const char* Category() const
        {
            return (s_TraceControl.Category());
        }

        virtual const char* Module() const
        {
            return (s_TraceControl.Module());
        }

        virtual bool Enabled() const
        {
            return (s_TraceControl.Enabled());
        }

        virtual void Enabled(const bool enabled)
        {
            s_TraceControl.Enabled(enabled);
        }

        virtual void Destroy()
        {
            s_TraceControl.Destroy();
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
        CATEGORY& _traceInfo;
        static TraceControl<CATEGORY, MODULENAME> s_TraceControl;
    };

    template <typename CATEGORY, const char** MODULENAME>
    EXTERNAL_HIDDEN typename TraceType<CATEGORY, MODULENAME>::template TraceControl<CATEGORY, MODULENAME> TraceType<CATEGORY, MODULENAME>::s_TraceControl;
}

} // namespace Trace

#endif // __TRACECONTROL_H

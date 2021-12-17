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

#ifndef __TRACECONTROL_H
#define __TRACECONTROL_H

// ---- Include system wide include files ----
#include <inttypes.h>

// ---- Include local include files ----
#include "ITraceControl.h"
#include "Module.h"
#include "TraceUnit.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----
#define TRACE(CATEGORY, PARAMETERS)                                                    \
    if (WPEFramework::Trace::TraceType<CATEGORY, &WPEFramework::Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                  \
        WPEFramework::Trace::TraceType<CATEGORY, &WPEFramework::Core::System::MODULE_NAME> __message__(__data__);  \
        WPEFramework::Trace::TraceUnit::Instance().Trace(                                            \
            __FILE__,                                                                  \
            __LINE__,                                                                  \
            typeid(*this).name(),                                                      \
            &__message__);                                                             \
    }

#define TRACE2(CATEGORY, PARAMETERS)                                                                                                       \
    if (WPEFramework::Trace::ControlLifetime<CATEGORY, &WPEFramework::Core::System::MODULE_NAME>::IsEnabled() == true) {                   \
        CATEGORY __data__ PARAMETERS;                                                                                                      \
        WPEFramework::Core::MessageInformation __info__(WPEFramework::Core::MessageMetaData::MessageType::TRACING,                         \
            Core::ClassNameOnly(typeid(CATEGORY).name()).Text(),                                                                           \
            WPEFramework::Core::System::MODULE_NAME,                                                                                       \
            __FILE__,                                                                                                                      \
            __LINE__);                                                                                                                     \
        WPEFramework::Trace::Trace<CATEGORY, &WPEFramework::Core::System::MODULE_NAME> __message__(typeid(*this).name(), __data__.Data()); \
        WPEFramework::Core::MessageUnit::Instance().Push(__info__, &__message__);                                                          \
    }

#define TRACE_GLOBAL(CATEGORY, PARAMETERS)                                             \
    if (WPEFramework::Trace::TraceType<CATEGORY, &WPEFramework::Core::System::MODULE_NAME>::IsEnabled() == true) { \
        CATEGORY __data__ PARAMETERS;                                                  \
        WPEFramework::Trace::TraceType<CATEGORY, &WPEFramework::Core::System::MODULE_NAME> __message__(__data__);  \
        WPEFramework::Trace::TraceUnit::Instance().Trace(                                            \
            __FILE__,                                                                  \
            __LINE__,                                                                  \
            __FUNCTION__,                                                              \
            &__message__);                                                             \
    }

#define TRACE_DURATION(CODE, ...)                                         \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now();     \
    CODE                                                                  \
    TRACE(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));

#define TRACE_DURATION_GLOBAL(CODE, ...)                                  \
    WPEFramework::Core::Time start = WPEFramework::Core::Time::Now();     \
    CODE                                                                  \
    TRACE_GLOBAL(WPEFramework::Trace::Duration, (start, ##__VA_ARGS__));

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

        public:
            TraceControl(const TraceControl<CONTROLCATEGORY, CONTROLMODULE>&) = delete;
            TraceControl<CONTROLCATEGORY, CONTROLMODULE>& operator=(const TraceControl<CONTROLCATEGORY, CONTROLMODULE>&) = delete;

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


    public:
        TraceType(const TraceType<CATEGORY, MODULENAME>&) = delete;
        TraceType<CATEGORY, MODULENAME>& operator=(const TraceType<CATEGORY, MODULENAME>&) = delete;

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

namespace WPEFramework {
namespace Trace {
    template <typename CATEGORY, const char** MODULENAME>
    class Trace : public Core::IMessageEvent {
    public:
        Trace() = default;
        Trace(const string& classname, const string& text)
            : _moduleName(MODULENAME != nullptr ? *MODULENAME : _T("MODULE_UNKNOWN"))
            , _classname(classname)
            , _text(text)
        {
        }

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            writer.NullTerminatedText(_moduleName);
            writer.NullTerminatedText(_classname);
            writer.NullTerminatedText(_text);

            return writer.Offset();
        }
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize) override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Reader reader(frame, 0);

            _moduleName = reader.NullTerminatedText();
            _classname = reader.NullTerminatedText();
            _text = reader.NullTerminatedText();

            return _moduleName.size() + 1 + _classname.size() + 1 + _text.size() + 1;
        }

        void ToString(string& text) const override
        {
            text = Core::Format("[%s][%s]", _moduleName.c_str(), _text.c_str());
        }

    private:
        string _moduleName;
        string _classname;
        string _text;
    };

    class Factory : public Core::IMessageEventFactory {
    private:
        class DefaultTrace : public Trace<DefaultTrace, nullptr> {
        public:
            using Base = Trace<DefaultTrace, nullptr>;

            DefaultTrace()
            {
            }
        };

    public:
        Factory()
            : _tracePool(40)
        {
        }

        Core::ProxyType<Core::IMessageEvent> Create() override
        {
            Core::ProxyType<DefaultTrace> proxy = _tracePool.Element();
            return Core::ProxyType<Core::IMessageEvent>(proxy);
        }

    private:
        Core::ProxyPoolType<DefaultTrace> _tracePool;
    };

    template <typename CATEGORY, const char** MODULENAME>
    class ControlLifetime {
    private:
        template <typename CONTROLCATEGORY, const char** CONTROLMODULE>
        class Control : public Core::IControl {
        public:
            Control(const Control<CONTROLCATEGORY, CONTROLMODULE>&) = delete;
            Control<CONTROLCATEGORY, CONTROLMODULE>& operator=(const Control<CONTROLCATEGORY, CONTROLMODULE>&) = delete;

            Control()
                : _enabled(0x02)
                , _type(Core::MessageMetaData::MessageType::TRACING)
                , _category(Core::ClassNameOnly(typeid(CONTROLCATEGORY).name()).Text())
                , _module(CONTROLMODULE != nullptr ? *CONTROLMODULE : _T("UNKNOWN_MODULE"))
            {
                // Register Our trace control unit, so it can be influenced from the outside
                // if nessecary..
                Core::MessageUnit::Instance().Announce(this);

                //todo add checking

                bool isEnabled = false;
                bool isDefault = false;
                Core::MessageUnit::Instance().FetchDefaultSettingsForCategory(this, isEnabled, isDefault);

                if (isDefault) {
                    if (isEnabled) {
                        _enabled = _enabled | 0x01;
                    }
                }
            }
            ~Control() override
            {
                Destroy();
            }

        public:
            Core::MessageMetaData::MessageType Type() const override
            {
                return Core::MessageMetaData::MessageType::TRACING;
            }

            string Category() const override
            {
                return _category;
            }

            string Module() const override
            {
                return _module;
            }

            //non virtual method, so it can be called faster
            inline bool IsEnabled() const
            {
                return ((_enabled & 0x01) != 0);
            }

            bool Enable() const override
            {
                return IsEnabled();
            }

            virtual void Enable(const bool enabled) override
            {
                _enabled = (_enabled & 0xFE) | (enabled ? 0x01 : 0x00);
            }
            void Destroy() override
            {
                if ((_enabled & 0x02) != 0) {
                    // Register Our trace control unit, so it can be influenced from the outside
                    // if nessecary..
                    Core::MessageUnit::Instance().Revoke(this);
                    _enabled = 0;
                }
            }

        private:
            uint8_t _enabled;
            Core::MessageMetaData::MessageType _type;
            string _category;
            string _module;
        };

    public:
        ControlLifetime(const ControlLifetime<CATEGORY, MODULENAME>&) = delete;
        ControlLifetime<CATEGORY, MODULENAME>& operator=(const ControlLifetime<CATEGORY, MODULENAME>&) = delete;
        ControlLifetime() = default;
        ~ControlLifetime() = default;

    public:
        inline static bool IsEnabled()
        {
            return (_MessageControl.IsEnabled());
        }

    private:
        static Control<CATEGORY, MODULENAME> _MessageControl;
    };

    template <typename CATEGORY, const char** MODULENAME>
    EXTERNAL_HIDDEN typename ControlLifetime<CATEGORY, MODULENAME>::template Control<CATEGORY, MODULENAME> ControlLifetime<CATEGORY, MODULENAME>::_MessageControl;

}

} // namespace Trace

#endif // __TRACECONTROL_H

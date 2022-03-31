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

#ifndef __TRACEUNIT_H
#define __TRACEUNIT_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "ITraceMedia.h"
#include "Module.h"

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Trace {
    // ---- Referenced classes and types ----
    struct ITraceControl;
    struct ITrace;

    constexpr uint32_t CyclicBufferSize = ((16 * 1024) - (sizeof(struct Core::CyclicBuffer::control))); /* 16Kb */
    extern EXTERNAL const TCHAR* CyclicBufferName;

    // ---- Class Definition ----
    class EXTERNAL TraceUnit {
    public:
        class Setting {
        public:
            class JSON : public Core::JSON::Container {
            public:
                JSON& operator=(const JSON&) = delete;
                JSON()
                    : Core::JSON::Container()
                    , Module()
                    , Category()
                    , Enabled(false)
                {
                    Add(_T("module"), &Module);
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);
                }
                JSON(const JSON& copy)
                    : Core::JSON::Container()
                    , Module(copy.Module)
                    , Category(copy.Category)
                    , Enabled(copy.Enabled)
                {
                    Add(_T("module"), &Module);
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);
                }
                JSON(const Setting& rhs)
                    : Core::JSON::Container()
                    , Module()
                    , Category()
                    , Enabled()
                {
                    Add(_T("module"), &Module);
                    Add(_T("category"), &Category);
                    Add(_T("enabled"), &Enabled);

                    if (rhs.HasModule()) {
                        Module = rhs.Module();
                    }
                    if (rhs.HasCategory()) {
                        Category = rhs.Category();
                    }
                    Enabled = rhs.Enabled();
                }
                ~JSON() override = default;

            public:
                Core::JSON::String Module;
                Core::JSON::String Category;
                Core::JSON::Boolean Enabled;
            };

        public:
            Setting(const JSON& source) 
                : _module()
                , _category()
                , _enabled(source.Enabled.Value()) {
                if (source.Module.IsSet()) {
                    _module = source.Module.Value();
                }
                if (source.Category.IsSet()) {
                    _category = source.Category.Value();
                }
            }
            Setting(const Setting& copy)
                : _module(copy._module)
                , _category(copy._category)
                , _enabled(copy._enabled) {
            }
            ~Setting() {
            }

        public:
            bool HasModule() const {
                return (_module.IsSet());
            }
            bool HasCategory() const {
                return (_category.IsSet());
            }
            const string& Module() const {
                return (_module);
            }
            const string& Category() const {
                return (_category);
            }
            bool Enabled() const {
                return (_enabled);
            }

        private:
            Core::OptionalType<string> _module;
            Core::OptionalType<string> _category;
            bool _enabled;
        };

    public:
        typedef std::list<Setting> Settings;
        typedef std::list<ITraceControl*> TraceControlList;
        typedef Core::IteratorType<TraceControlList, ITraceControl*> Iterator;

    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statements.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        TraceUnit(const TraceUnit&) = delete;
        TraceUnit& operator=(const TraceUnit&) = delete;

        class EXTERNAL TraceBuffer : public Core::CyclicBuffer {
        private:
            TraceBuffer() = delete;
            TraceBuffer(const TraceBuffer&) = delete;
            TraceBuffer& operator=(const TraceBuffer&) = delete;

        public:
            TraceBuffer(const string& doorBell, const string& name);
            ~TraceBuffer();

        public:
            virtual uint32_t GetOverwriteSize(Cursor& cursor) override;
            inline void Ring() {
                _doorBell.Ring();
            }
            inline void Acknowledge() {
                _doorBell.Acknowledge();
            }
            inline uint32_t Wait (const uint32_t waitTime) {
                return (_doorBell.Wait(waitTime));
            }
            inline void Relinquish()
            {
                return (_doorBell.Relinquish());
            }

        private:
            virtual void DataAvailable() override;

        private:
            Core::DoorBell _doorBell;
        };

    protected:
        TraceUnit();

    public:
        virtual ~TraceUnit();

    public:
        static TraceUnit& Instance();

        uint32_t Open(const uint32_t identifier);
        uint32_t Open(const string& pathName);
        uint32_t Close();

        void Announce(ITraceControl& Category);
        void Revoke(ITraceControl& Category);
        Iterator GetCategories();
        uint32_t SetCategories(const bool enable, const char* module, const char* category);

        // Default enabled/disabled categories: set via config.json.
        bool IsDefaultCategory(const string& module, const string& category, bool& enabled) const;
        string Defaults() const;
        void Defaults(const string& jsonCategories);
        void Defaults(Core::File& file);

        void Trace(const char fileName[], const uint32_t lineNumber, const char className[], const ITrace* const information);

        inline Core::CyclicBuffer* CyclicBuffer()
        {
            return (m_OutputChannel);
        }
        inline bool HasDirectOutput() const
        {
            return (m_DirectOut);
        }
        inline void DirectOutput(const bool enabled)
        {
            m_DirectOut = enabled;
        }
        inline void Announce() {
            ASSERT (m_OutputChannel != nullptr);
            m_OutputChannel->Ring();
        }
        inline void Acknowledge() {
            ASSERT (m_OutputChannel != nullptr);
            m_OutputChannel->Acknowledge();
        }
        inline uint32_t Wait (const uint32_t waitTime) {
            ASSERT (m_OutputChannel != nullptr);
            return (m_OutputChannel->Wait(waitTime));
        }
        inline void Relinquish() {
            ASSERT(m_OutputChannel != nullptr);
            return (m_OutputChannel->Relinquish());
        }

    private:
        inline uint32_t Open(const string& doorBell, const string& fileName) 
        {
            ASSERT(m_OutputChannel == nullptr);

            m_OutputChannel = new TraceBuffer(doorBell, fileName);

            ASSERT(m_OutputChannel->IsValid() == true);

            return (m_OutputChannel->IsValid() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }
        void UpdateEnabledCategories(const Core::JSON::ArrayType<Setting::JSON>& info);

        TraceControlList m_Categories;
        Core::CriticalSection m_Admin;
        TraceBuffer* m_OutputChannel;
        Settings m_EnabledCategories;
        bool m_DirectOut;
    };
}
} // namespace Trace

#endif // __TRACEUNIT_H

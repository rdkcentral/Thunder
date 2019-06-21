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

    constexpr uint32_t CyclicBufferSize = ((8 * 1024) - (sizeof(struct Core::CyclicBuffer::control))); /* 8Kb */
    extern EXTERNAL const TCHAR* CyclicBufferName;

    // ---- Class Definition ----
    class EXTERNAL TraceUnit {
    private:
        class EnabledCategory : public Core::JSON::Container {
        private:
            EnabledCategory& operator=(const EnabledCategory&) = delete;

        public:
            EnabledCategory()
                : Core::JSON::Container()
                , Module()
                , Category()
                , Enabled(false)
            {
                Add(_T("module"), &Module);
                Add(_T("category"), &Category);
                Add(_T("enabled"), &Enabled);
            }
            EnabledCategory(const EnabledCategory& copy)
                : Core::JSON::Container()
                , Module(copy.Module)
                , Category(copy.Category)
                , Enabled(copy.Enabled)
            {
                Add(_T("module"), &Module);
                Add(_T("category"), &Category);
                Add(_T("enabled"), &Enabled);
            }
            virtual ~EnabledCategory()
            {
            }

        public:
            Core::JSON::String Module;
            Core::JSON::String Category;
            Core::JSON::Boolean Enabled;
        };

        typedef Core::JSON::ArrayType<EnabledCategory> EnabledCategories;

    public:
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
        uint32_t Open(const string& pathName, const uint32_t identifier);
        uint32_t Close();

        void Announce(ITraceControl& Category);
        void Revoke(ITraceControl& Category);
        Iterator GetCategories();
        uint32_t SetCategories(const bool enable, const char* module, const char* category);

        // Default enabled/disabled categories: set via config.json.
        bool IsDefaultCategory(const string& module, const string& category, bool& enabled) const;
        void GetDefaultCategoriesJson(string& jsonCategories);
        void SetDefaultCategoriesJson(const string& jsonCategories);

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
        void UpdateEnabledCategories();

        TraceControlList m_Categories;
        Core::CriticalSection m_Admin;
        TraceBuffer* m_OutputChannel;
        EnabledCategories m_EnabledCategories;
        bool m_DirectOut;
    };
}
} // namespace Trace

#endif // __TRACEUNIT_H

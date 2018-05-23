#ifndef __TRACEUNIT_H
#define __TRACEUNIT_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "ITraceMedia.h"

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Trace {
    // ---- Referenced classes and types ----
    struct ITraceControl;
    struct ITrace;

    #define TRACE_CYCLIC_BUFFER_ENVIRONMENT _T("TRACE_PATH")
    #define TRACE_CYCLIC_BUFFER_SIZE ( (8 * 1024) - (sizeof (struct Core::CyclicBuffer::control)) ) /* 8Kb */
    #define TRACE_CYCLIC_BUFFER_PREFIX _T("tracebuffer")

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
			TraceBuffer& operator= (const TraceBuffer&) = delete;

		public:
			TraceBuffer(const string& name);
			~TraceBuffer();

		public:
			Core::DoorBell& DoorBell() {
				return (_doorBell);
			}

			virtual uint32_t GetOverwriteSize(Cursor& cursor) override;

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

        uint32_t Open (const string& pathName);
        uint32_t Close ();

        void Announce(ITraceControl& Category);
        void Revoke(ITraceControl& Category);
        Iterator GetCategories();
        uint32_t SetCategories(const bool enable, const char* module, const char* category);

        // Default enabled/disabled categories: set via config.json.
        bool IsDefaultCategory(const string& module, const string& category, bool& enabled) const;
        void GetDefaultCategoriesJson(string& jsonCategories);
        void SetDefaultCategoriesJson(const string& jsonCategories);

        void Trace(const char fileName[], const uint32_t lineNumber, const char className[], const ITrace* const information);

	inline Core::DoorBell& TraceAnnouncement() {
	    ASSERT(m_OutputChannel != nullptr);
	    return (m_OutputChannel->DoorBell());
	}

        inline Core::CyclicBuffer* CyclicBuffer () {
            return (m_OutputChannel);
        }
        inline bool HasDirectOutput() const {
            return (m_DirectOut);
        }
        inline void DirectOutput(const bool enabled) {
            m_DirectOut = enabled;
        }

    private:
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

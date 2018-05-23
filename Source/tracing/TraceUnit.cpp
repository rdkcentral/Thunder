#include "TraceUnit.h"
#include "TraceCategories.h"

namespace WPEFramework {
namespace Trace {

// In windows you need the newest compiler for this...
//template <typename First, typename... Rest> const string Format(const First* first, const Rest&... rest) {
//	TCHAR buffer[2057];

//	::vsnprintf_s (buffer, sizeof(buffer),sizeof(buffer), first, rest...);

//	return (string(buffer));
//}

#ifndef va_copy
#ifdef _MSC_VER
#define va_copy(dst, src) dst=src
#elif !(__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
#define va_copy(dst, src) memcpy((void*)dst, (void*)src, sizeof(*src))
#endif
#endif

	///
	/// \brief Format message
	/// \param dst String to store formatted message
	/// \param format Format of message
	/// \param ap Variable argument list
	///
	static void toString(string &dst, const TCHAR format[], va_list ap) {
		int length;
		va_list apStrLen;
		va_copy(apStrLen, ap);
		length = vsnprintf(nullptr, 0, format, apStrLen);
		va_end(apStrLen);
		if (length > 0) {
			dst.resize(length);
			vsnprintf((char *)dst.data(), dst.size() + 1, format, ap);
		}
		else {
			dst = "Format error! format: ";
			dst.append(format);
		}
	}

	///
	/// \brief Format message
	/// \param dst String to store formatted message
	/// \param format Format of message
	/// \param ... Variable argument list
	///
	void Format(string &dst, const TCHAR format[], ...) {
		va_list ap;
		va_start(ap, format);
		toString(dst, format, ap);
		va_end(ap);
	}

	///
	/// \brief Format message
	/// \param format Format of message
	/// \param ... Variable argument list
	///
	string Format(const TCHAR format[], ...) {
		string dst;
		va_list ap;
		va_start(ap, format);
		toString(dst, format, ap);
		va_end(ap);
		return dst;
	}

	///
	/// \brief Format message
	/// \param format Format of message
	/// \param ap Variable argument list
	///
	void Format(string& dst, const TCHAR format[], va_list ap) {
		toString(dst, format, ap);
	}
    TraceUnit::TraceUnit()
        : m_Categories()
        , m_Admin()
        , m_OutputChannel(nullptr)
        , m_DirectOut(false)
    {
        string pathName;

        Core::SystemInfo::GetEnvironment(TRACE_CYCLIC_BUFFER_ENVIRONMENT, pathName);

        if (pathName.empty() == false) 
        {
            Open (pathName);
        }
    }

	TraceUnit::TraceBuffer::TraceBuffer(const string& name) :
		Core::CyclicBuffer(name + '.' + Core::NumberType<uint32_t>(Core::ProcessInfo().Id()).Text(), TRACE_CYCLIC_BUFFER_SIZE, true),
		_doorBell(TRACE_CYCLIC_BUFFER_PREFIX)
	{
	}

	TraceUnit::TraceBuffer::~TraceBuffer() {
	}

	/* virtual */ uint32_t TraceUnit::TraceBuffer::GetOverwriteSize(Cursor& cursor)
	{
		while(cursor.Offset() < cursor.Size()) {
			uint16_t chunkSize = 0;
			cursor.Peek(chunkSize);

            		TRACE_L1("Flushing TRACE data !!! %d", __LINE__);

			cursor.Forward(chunkSize);
		}

		return cursor.Offset();
	}

	/* virtual */ void TraceUnit::TraceBuffer::DataAvailable() {
		_doorBell.Ring();
	}

    /* static */ TraceUnit& TraceUnit::Instance()
    {
        return (Core::SingletonType<TraceUnit>::Instance());
    }

    TraceUnit::~TraceUnit()
    {
        m_Admin.Lock();

        if (m_OutputChannel != nullptr) {
            Close();
        }

        while (m_Categories.size() != 0) {
            m_Categories.front()->Destroy();
        }

        m_Admin.Unlock();
    }

    uint32_t TraceUnit::Open (const string& pathName)
    {
        ASSERT (m_OutputChannel == nullptr);

		string actualPath(Core::Directory::Normalize(pathName) + TRACE_CYCLIC_BUFFER_PREFIX);
			
        m_OutputChannel = new TraceBuffer(actualPath);

        ASSERT(m_OutputChannel->IsValid());

		return (Core::ERROR_NONE);
    }

    uint32_t TraceUnit::Close ()
    {
        m_Admin.Lock();

        ASSERT (m_OutputChannel != nullptr);

        if (m_OutputChannel != nullptr) {
            delete m_OutputChannel;
        }

        m_OutputChannel = nullptr;

        m_Admin.Unlock();

		return (Core::ERROR_NONE);
	}

    void TraceUnit::Announce(ITraceControl& Category)
    {
        m_Admin.Lock();

        m_Categories.push_back(&Category);

        m_Admin.Unlock();
    }

    void TraceUnit::Revoke(ITraceControl& Category)
    {
        m_Admin.Lock();

        std::list<ITraceControl*>::iterator index(std::find(m_Categories.begin(), m_Categories.end(), &Category));

        if (index != m_Categories.end()) {
            m_Categories.erase(index);
        }

        m_Admin.Unlock();
    }

    TraceUnit::Iterator TraceUnit::GetCategories()
    {
        return (Iterator(m_Categories));
    }

    uint32_t TraceUnit::SetCategories(const bool enable, const char* module, const char* category)
    {
        uint32_t modifications = 0;

        TraceControlList::iterator index(m_Categories.begin());

        while (index != m_Categories.end()) {
            const char* thisModule = (*index)->Module();
            const char* thisCategory = (*index)->Category();

            if (((*module == '\0') || (::strcmp(module, thisModule) == 0)) && ((*category == '\0') || (::strcmp(category, thisCategory) == 0))) {
                modifications++;
                (*index)->Enabled(enable);
            }
            index++;
        }

        return (modifications);
    }

    void TraceUnit::GetDefaultCategoriesJson(string& jsonCategories)
    {
        m_EnabledCategories.ToString(jsonCategories);
    }
    
    void TraceUnit::SetDefaultCategoriesJson(const string& jsonCategories)
    {
        m_EnabledCategories.FromString(jsonCategories);

        // Deal with existing categories that might need to be enable/disabled.
        UpdateEnabledCategories();
    }
    
    void TraceUnit::UpdateEnabledCategories()
    {
        for (ITraceControl * traceControl : m_Categories) {
            // TODO: should be ConstIterator
            EnabledCategories::Iterator i = m_EnabledCategories.Elements();
            while (i.Next()) {
                EnabledCategory& category = i.Current();
                
                if ((!category.Module.Value().empty()) && (category.Module != traceControl->Module()))
                    continue;
                
                if ((!category.Category.Value().empty()) && (category.Category != traceControl->Category()))
                    continue;

                if (category.Enabled != traceControl->Enabled())
                    traceControl->Enabled(category.Enabled);
            }
        }
    }

    bool TraceUnit::IsDefaultCategory(const string& module, const string& category, bool& enabled) const
    {
        bool isDefaultCategory = false;

        EnabledCategories::ConstIterator i = m_EnabledCategories.Elements();
        while (i.Next()) {
            const EnabledCategory& enabledCategory = i.Current();
             
            if ((!enabledCategory.Module.Value().empty()) && (enabledCategory.Module.Value() != module))
                continue;

            if ((!enabledCategory.Category.Value().empty()) && (enabledCategory.Category.Value() != category))
                continue;

            // Register match of category and update enabled flag.
            isDefaultCategory = true;
            enabled = enabledCategory.Enabled.Value();
        }
        
        return isDefaultCategory;
    }  

    void TraceUnit::Trace(const char file[], const uint32_t lineNumber, const char className[], const ITrace* const information)
    {
	const char* fileName (Core::FileNameOnly(file));

        m_Admin.Lock();

        if ( m_OutputChannel != nullptr) {

            const char* category(information->Category());
            const char* module(information->Module());
            const uint64_t current = Core::Time::Now().Ticks();
            const uint16_t fileNameLength = static_cast<uint16_t>(strlen(fileName) + 1); // File name.
            const uint16_t moduleLength = static_cast<uint16_t>(strlen(module) + 1); // Module.
            const uint16_t categoryLength = static_cast<uint16_t>(strlen(category) + 1); // Cateogory.
            const uint16_t classNameLength = static_cast<uint16_t>(strlen(className) + 1); // Class name.
            const uint16_t informationLength = information->Length(); // Actual data (no '\0' needed).

            // Trace entry has been simplified: 16 bit size followed by fields:
            // length(2 bytes) - clock ticks (8 bytes) - line number (4 bytes) - file/module/category/className
            const uint16_t headerLength  = 2 + 8 + 4 + fileNameLength + moduleLength + categoryLength + classNameLength; 

            const uint32_t fullLength = informationLength + headerLength; // Actual data (no '\0' needed).

            // Tell the buffer how much we are going to write.
            const uint32_t actualLength = m_OutputChannel->Reserve(fullLength);


            if (actualLength >= headerLength) {
                const uint16_t convertedLength = static_cast<uint16_t>(actualLength);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&convertedLength), 2);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&current), 8);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(&lineNumber), 4);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(fileName), fileNameLength);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(module), moduleLength);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(category), categoryLength);
                m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(className), classNameLength);


                if (actualLength >= fullLength) {
                    // We can write the whole information.
                    m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(information->Data()), informationLength);
                }
                else {
                    // Can only write information partially
                    const uint16_t dropLength = actualLength - headerLength;

                    m_OutputChannel->Write(reinterpret_cast<const uint8_t*>(information->Data()), dropLength);
                }
            }
        }

        if (m_DirectOut == true) {
            string time (Core::Time::Now().ToRFC1123(true));
            Core::TextFragment cleanClassName(Core::ClassNameOnly(className));

            fprintf (stdout, "[%s]:[%s:%d]:[%s] %s: %s\n",time.c_str(), fileName, lineNumber, cleanClassName.Data(), information->Category(), information->Data());
            fflush (stdout);
        }

        m_Admin.Unlock();
    }
}
} // namespace WPEFramework::Trace

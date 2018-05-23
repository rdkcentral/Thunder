#include <errno.h>

#include "MessageException.h"
#include "Serialization.h"

namespace WPEFramework {
namespace Core {
#ifdef __WIN32__
#pragma warning(disable : 4996)
#endif

    MessageException::MessageException(const string& message, bool inclSysMsg) throw()
        : m_Message(message)
    {
        if (inclSysMsg) {
            m_Message.append(_T(": "));
            m_Message.append(Core::ToString(strerror(errno)));
        }
    }

#ifdef __WIN32__
#pragma warning(default : 4996)
#endif

    MessageException::~MessageException() throw()
    {
    }

    const TCHAR* MessageException::Message() const throw()
    {
        return m_Message.c_str();
    }
}
} // namespace Solution::Core

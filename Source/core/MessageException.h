#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception> // For exception class

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"

namespace WPEFramework {
namespace Core {

    class MessageException : public std::exception {
    private:
        MessageException();

    public:
        MessageException(const string& message, bool inclSysMsg = false) throw();
        ~MessageException() throw();

        const TCHAR* Message() const throw();

    private:
        string m_Message; // Exception message
    };
}
} // namespace Exceptions

#endif // EXCEPTION_H

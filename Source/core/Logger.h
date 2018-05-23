#ifndef __LOGGER_H
#define __LOGGER_H

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"
#include "Time.h"

#include "ILogService.h"

namespace WPEFramework {
namespace Core {

    class ILogService;

    class Logger {
    private:
        Logger(const Logger& logger);
        Logger& operator=(const Logger& logger);

    public:
        Logger(const string& className);
        ~Logger();

        inline void Debug(const string& msg) const
        {
            Filter(llDebug, msg);
        }

        inline void Info(const string& msg) const
        {
            Filter(llInfo, msg);
        }

        inline void Warning(const string& msg) const
        {
            Filter(llWarning, msg);
        }

        inline void Error(const string& msg) const
        {
            Filter(llError, msg);
        }

        inline void Fatal(const string& msg) const
        {
            Filter(llFatal, msg);
        }

    private:
        void Filter(LogLevel, const string&) const;

        string m_ClassName;
    };
}
} // namespace Core

#endif // __LOGGER_H

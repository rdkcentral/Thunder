#ifndef __ILOGSERVICE_H
#define __ILOGSERVICE_H

#include "Module.h"
#include "Portability.h"
#include "TextFragment.h"

namespace WPEFramework {
namespace Core {

    enum LogLevel {
        llDebug,
        llInfo,
        llWarning,
        llError,
        llFatal
    };

    class ILogService {
    public:
        virtual ~ILogService() {}
        virtual void Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg) = 0;

        static ILogService* Instance();
    };
}
} // namespace Core

#endif // __ILOGSERVICE_H

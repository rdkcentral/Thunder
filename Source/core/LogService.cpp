#include <iostream>
#include <sstream>
#include <typeinfo>

#include "LogService.h"
#include "MessageException.h"

#ifdef __WIN32__
// W4 -- This function or variable may be unsafe. Consider using XXXXXX instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details
#pragma warning(disable : 4996)
#endif

namespace WPEFramework {
namespace Core {

    static ConsoleLogging InternalDefaultLogger(nullptr, llDebug);
    ILogService* DefaultLogger = &InternalDefaultLogger;
    static uint32_t StartLogTimeStamp = static_cast<uint32_t>(Core::Time::Now().Ticks());

    static const string FormatLogLine(uint32_t timeStamp, const string& className, const string& message)
    {
        static uint32_t SequenceNumber = 0;

        SequenceNumber++;

#ifdef _UNICODE
        std::wstringstream fullMessage;
#else
        std::stringstream fullMessage;
#endif

        char buf[256];
        uint32_t relativeTime = timeStamp - StartLogTimeStamp;
        uint32_t sec = relativeTime / 1000;
        uint32_t ms = relativeTime % 1000;
        sprintf(buf, "#%d %d.%03dms {%s} ", SequenceNumber, sec, ms, "LoggerThread");

        // Time to format the message !!!
        fullMessage << buf << className << ": " << message;

        return (fullMessage.str());
    }

    ConsoleLogging::ConsoleLogging(ILogService* nextLogger, const LogLevel logLevel)
        : m_LogLevel(logLevel)
        , m_NextLogger(nextLogger)
    {
        uint32_t stamp = static_cast<uint32_t>(Core::Time::Now().Ticks());

        Log(stamp, llInfo, Core::ToString(typeid(*this).name()), Core::LogMessage(Core::ToString(__FILE__).c_str(), __LINE__, _T("Logging available !!!!")));
    }

    /* virtual */ ConsoleLogging::~ConsoleLogging()
    {
    }

    /* virtual */ void ConsoleLogging::Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg)
    {
        if (m_NextLogger != nullptr) {
            m_NextLogger->Log(timeStamp, level, className, msg);
        }

        if (level >= m_LogLevel) {
            std::string logLine;

            Core::ToString(FormatLogLine(timeStamp, className, msg).c_str(), logLine);

            std::cerr << logLine << std::endl;
        }
    }

    SocketLogging::SocketLogging(ILogService* logService, const LogLevel logLevel)
        : m_LogLevel(logLevel)
        , m_NextLogger(logService)
        , m_Socket(NodeId(_T("0.0.0.0"), 12345))
    {
    }
    /* virtual */ SocketLogging::~SocketLogging()
    {
    }

    /* virtual */ void SocketLogging::Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg)
    {
        if (m_NextLogger != nullptr) {
            m_NextLogger->Log(timeStamp, level, className, msg);
        }

        if (level >= m_LogLevel) {
            std::string logLine;
            ToString(FormatLogLine(timeStamp, className, msg).c_str(), logLine);

            try {
                uint32_t size = logLine.length();

                // Repeatedly send the std::string (not including \0) to the server
                //m_Socket.Write(size, reinterpret_cast<const uint8_t*>(logLine.c_str()), 100);
            }
            catch (MessageException& e) {
                if (m_NextLogger != nullptr) {
                    m_NextLogger->Log(timeStamp, llError, Core::ToString(typeid(*this).name()), Core::ToString(e.what()));
                }
            }
        }
    }

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif

    DecoupledLogging::DecoupledLogging(ILogService* logService)
        : m_NextLogger(logService)
        , m_Decoupling(*this)
        , m_LogLinesQueued(100)
    {
        // This logger has no use if it does not forward its log line to a "real" logger !!!
        ASSERT(m_NextLogger != nullptr);
        m_Decoupling.Run();
    }

#ifdef __WIN32__
#pragma warning(default : 4355)
#endif

    /* virtual */ DecoupledLogging::~DecoupledLogging()
    {
        m_Decoupling.Block();
        m_LogLinesQueued.Disable();
        m_Decoupling.Wait(Thread::BLOCKED | Thread::STOPPED);
    }

    /* virtual */ void DecoupledLogging::Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg)
    {
        LogInfo newEntry = { timeStamp, level, className, msg };

        // Put it in the queue
        m_LogLinesQueued.Insert(newEntry, Core::infinite);
    }

    void DecoupledLogging::Process()
    {
        LogInfo entry;

        while (m_LogLinesQueued.Extract(entry, Core::infinite) == true) {
            m_NextLogger->Log(entry.Time, entry.LogLevel, entry.ClassName, entry.Message);
        }
    }
}
} // namespace Solution::Core

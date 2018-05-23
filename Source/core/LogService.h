#ifndef __LOGSERVICE_H
#define __LOGSERVICE_H

#include "Module.h"
#include "Portability.h"
#include "ILogService.h"
#include "TextFragment.h"
#include "Thread.h"
#include "Queue.h"
//#include "SocketPort.h"

namespace WPEFramework {
namespace Core {

    class EXTERNAL ConsoleLogging : public ILogService {
    private:
        ConsoleLogging();
        ConsoleLogging(const ConsoleLogging&);
        ConsoleLogging& operator=(const ConsoleLogging&);

    public:
        ConsoleLogging(ILogService* nextLogger, const LogLevel logLevel);
        virtual ~ConsoleLogging();

        virtual void Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg);
        LogLevel GetLogLevel() const
        {
            return (m_LogLevel);
        }

        void SetLogLevel(const LogLevel newLevel)
        {
            m_LogLevel = newLevel;
        }

    private:
        LogLevel m_LogLevel;
        ILogService* m_NextLogger;
    };

    class EXTERNAL SocketLogging : public ILogService {
    private:
        SocketLogging();
        SocketLogging(const SocketLogging&);
        SocketLogging& operator=(const SocketLogging&);

    public:
        SocketLogging(ILogService* logService, const LogLevel logLevel);
        virtual ~SocketLogging();

        virtual void Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg);

        LogLevel GetLogLevel() const
        {
            return (m_LogLevel);
        }

        void SetLogLevel(const LogLevel newLevel)
        {
            m_LogLevel = newLevel;
        }

    private:
        LogLevel m_LogLevel;
        ILogService* m_NextLogger;
        //SocketDatagram      m_Socket;
    };

    class EXTERNAL DecoupledLogging : public ILogService {
    private:
        DecoupledLogging();
        DecoupledLogging(const DecoupledLogging&);
        DecoupledLogging& operator=(const DecoupledLogging&);

    private:
        struct LogInfo {
            uint32_t Time;
            Core::LogLevel LogLevel;
            string ClassName;
            string Message;
        };

        class WorkerThread : public Thread {
        private:
            WorkerThread();
            WorkerThread(const WorkerThread&);
            WorkerThread& operator=(const WorkerThread&);

        public:
            WorkerThread(DecoupledLogging& parent)
                : m_Content(parent)
            {
            }
            ~WorkerThread()
            {
            }

        public:
            virtual uint32_t Worker()
            {
                m_Content.Process();

                return (0);
            }

        private:
            DecoupledLogging& m_Content;
        };

    public:
        DecoupledLogging(ILogService* logService);
        virtual ~DecoupledLogging();

        virtual void Log(const uint32_t timeStamp, const LogLevel level, const string& className, const string& msg);

    private:
        void Process();

    private:
        ILogService* m_NextLogger;
        WorkerThread m_Decoupling;
        QueueType<LogInfo> m_LogLinesQueued;
    };

    extern ILogService* DefaultLogger;
}
} // Namespace Core

#endif // __LOGSERVICE_H

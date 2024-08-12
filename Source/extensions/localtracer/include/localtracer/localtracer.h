#pragma once

#include <core/core.h>
#include <messaging/messaging.h>

#include <iomanip>
#include <iostream>

namespace Thunder {
namespace Messaging {
    class EXTERNAL LocalTracer {
    public:
        struct EXTERNAL ICallback {
            virtual ~ICallback() = default;

            virtual void Message(const Core::Messaging::Metadata& metadata, const string& message) = 0;
        };

    private:
        class MessageSettings : public Messaging::MessageUnit::Settings {
        private:
            MessageSettings()
            {
                Messaging::MessageUnit::Settings::Load();
            };

        public:
            MessageSettings(const MessageSettings&) = delete;
            MessageSettings& operator=(const MessageSettings&) = delete;

            static MessageSettings& Instance()
            {
                static MessageSettings singleton;
                return (singleton);
            }
        };

    private:
        class WorkerThread : private Core::Thread {
        public:
            WorkerThread(LocalTracer& parent)
                : Core::Thread()
                , _parent(parent)
            {
            }
            void Start()
            {
                Thread::Run();
            }
            void Stop()
            {
                Thread::Block();
            }

        private:
            uint32_t Worker() override
            {
                if (Thread::IsRunning()) {
                    _parent.Dispatch();
                }

                return Core::infinite;
            }

            LocalTracer& _parent;
        };

    public:
        LocalTracer(const LocalTracer&) = delete;
        LocalTracer& operator=(const LocalTracer&) = delete;

        virtual ~LocalTracer()
        {
            _worker.Stop();

            _client.SkipWaiting();
            _client.ClearInstances();
        }

        void Close()
        {
            _adminLock.Lock();
            _callback = nullptr;
            _adminLock.Unlock();

            Messaging::MessageUnit::Instance().Close();

            Core::Directory(_path.c_str()).Destroy();

            LocalTracer*& singleton = LockSingleton(true);

            singleton = nullptr;

            LockSingleton(false);

            delete this;
        }

        static LocalTracer& Open()
        {
            static Core::CriticalSection lock;

            LocalTracer*& singleton = LockSingleton(true);

            if (singleton == nullptr) {

                char dir[] = "/tmp/localTracer.XXXXXX";
                Messaging::MessageUnit::Settings::Config configuration;

                string Path(::mkdtemp(dir));

                Core::Directory(Path.c_str()).CreatePath();

                Messaging::MessageUnit::Instance().Open(Path, configuration, false, Messaging::MessageUnit::OFF);

                singleton = new LocalTracer(Path);
            }

            LockSingleton(false);

            return *singleton;
        }

    private:
        static LocalTracer*& LockSingleton(const bool lock)
        {
            static LocalTracer* singleton = nullptr;

            static Core::CriticalSection s_lock;

            (lock == true) ? s_lock.Lock() : s_lock.Unlock();

            return (singleton);
        }

        LocalTracer(const string& path)
            : _adminLock()
            , _path(path)
            , _worker(*this)
            , _client(MessageSettings::Instance().Identifier(), MessageSettings::Instance().BasePath(), MessageSettings::Instance().SocketPort())
            , _tracingFactory()
            , _loggingFactory()
            , _callback(nullptr)
        {
            _client.AddInstance(0);
            _client.AddFactory(Core::Messaging::Metadata::type::TRACING, &_tracingFactory);
            _client.AddFactory(Core::Messaging::Metadata::type::LOGGING, &_loggingFactory);

            _worker.Start();

            // check if data is already available
            _client.SkipWaiting();
        }

    public:
        uint32_t EnableMessage(const std::string& moduleName, const std::string& categoryName, const bool enable)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            Core::Messaging::Metadata metaData(Core::Messaging::Metadata::type::TRACING, categoryName, moduleName);

            _client.Enable(metaData, enable);

            return Core::ERROR_NONE;
        }

        void Dispatch()
        {
            _client.WaitForUpdates(Core::infinite);
            _client.PopMessagesAndCall([this](const Core::ProxyType<Core::Messaging::MessageInfo>& metadata, const Core::ProxyType<Core::Messaging::IEvent>& message) {
                Message(*metadata, message->Data());
            });
        }

        void Callback(ICallback* callback)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            ASSERT((_callback != nullptr) ^ (callback != nullptr));

            _callback = callback;
        }

    private:
        void Message(const Core::Messaging::MessageInfo& metadata, const string& message)
        {
            Core::SafeSyncType<Core::CriticalSection> scopedLock(_adminLock);

            if (_callback != nullptr) {
                _callback->Message(metadata, message);
            }
        }

    private:
        mutable Core::CriticalSection _adminLock;
        const string _path;

        WorkerThread _worker;
        MessageClient _client;
        TraceFactoryType<Core::Messaging::IStore::Tracing, Messaging::TextMessage> _tracingFactory;
        TraceFactoryType<Core::Messaging::IStore::Logging, Messaging::TextMessage> _loggingFactory;

        ICallback* _callback;
    };

    class ConsolePrinter : public Messaging::LocalTracer::ICallback {
    public:
        ConsolePrinter(const ConsolePrinter&) = delete;
        ConsolePrinter& operator=(const ConsolePrinter&) = delete;

        ConsolePrinter(const bool abbreviated)
            : _abbreviated(abbreviated)
        {
        }
        virtual ~ConsolePrinter() = default;

        void Message(const Core::Messaging::Metadata& metadata, const string& message) override
        {
            string output;

            ASSERT(metadata.Type() == Core::Messaging::Metadata::type::TRACING);

            ASSERT(dynamic_cast<const Core::Messaging::IStore::Tracing*>(&metadata) != nullptr);
            const Core::Messaging::IStore::Tracing& trace = static_cast<const Core::Messaging::IStore::Tracing&>(metadata);
            const Core::Time now(trace.TimeStamp());

            if (_abbreviated == true) {
                const string time(now.ToTimeOnly(true));
                output = Core::Format("[%s]:[%s]:[%s]: %s",
                    time.c_str(),
                    metadata.Module().c_str(),
                    metadata.Category().c_str(),
                    message.c_str());
            } else {
                const string time(now.ToRFC1123(true));
                output = Core::Format("[%s]:[%s]:[%s:%u]:[%s]:[%s]: %s",
                    time.c_str(),
                    metadata.Module().c_str(),
                    Core::FileNameOnly(trace.FileName().c_str()),
                    trace.LineNumber(),
                    trace.ClassName().c_str(),
                    metadata.Category().c_str(),
                    message.c_str());
            }
            std::cout << output << std::endl
                      << std::flush;
        }

    private:
        bool _abbreviated;
    };
}
}

#include "ConnectorType.h"

namespace WPEFramework {

namespace RPC {

Core::ProxyType<RPC::IIPCServer> DefaultInvokeServer()
{
    class Engine : public RPC::InvokeServerType<1, 0, 8> {
    private:
        class AnnouncementSink : public Core::IIPCServer {
        public:
            AnnouncementSink(const AnnouncementSink&) = delete;
            AnnouncementSink& operator=(const AnnouncementSink&) = delete;

            AnnouncementSink() = default;
            ~AnnouncementSink() override = default;

        public:
            void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message) override
            {
                CommunicatorClient* client = dynamic_cast<CommunicatorClient*>(&source);
                client->Announcement()->Procedure(source, message);
            }
        };

    public:
        Engine()
        {
            Announcements(&_sink);
        }

        ~Engine() override
        {
        }

    private:
        AnnouncementSink _sink;

    };

    return Core::ProxyType<RPC::IIPCServer>(Core::SingletonProxyType<Engine>::Instance());
};

Core::ProxyType<RPC::IIPCServer> WorkerPoolInvokeServer()
{
    class Engine : public RPC::InvokeServer {
    private:
        class AnnouncementSink : public Core::IIPCServer {
        public:
            AnnouncementSink(const AnnouncementSink&) = delete;
            AnnouncementSink& operator=(const AnnouncementSink&) = delete;

            AnnouncementSink() = default;
            ~AnnouncementSink() override = default;

        public:
            void Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& message) override
            {
                CommunicatorClient* client = dynamic_cast<CommunicatorClient*>(&source);
                client->Announcement()->Procedure(source, message);
            }
        };

    public:
        Engine(Core::IWorkerPool* workerPool) : RPC::InvokeServer(workerPool)
        {
            Announcements(&_sink);
        }

        ~Engine() override
        {
        }

    private:
        AnnouncementSink _sink;
    };

    return Core::ProxyType<RPC::IIPCServer>(Core::SingletonProxyType<Engine>::Instance(&Core::IWorkerPool::Instance()));
};


} } 

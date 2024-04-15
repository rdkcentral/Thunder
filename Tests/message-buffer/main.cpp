#include "Module.h"

namespace Thunder {
namespace Test {

static Core::NodeId GetConnectionNode()
{
    string nodeName;
    Core::SystemInfo::GetEnvironment(string(_T("COMMUNICATOR_CONNECTOR")), nodeName);
    return (Core::NodeId(nodeName.c_str()));
}

static class PluginHost {
private:
    PluginHost(const PluginHost&) = delete;
    PluginHost& operator=(const PluginHost&) = delete;

public:
    PluginHost()
        : _engine(Thunder::Core::ProxyType<Thunder::RPC::InvokeServerType<2, 0, 4>>::Create())
        , _comClient(Core::ProxyType<RPC::CommunicatorClient>::Create(GetConnectionNode(), Core::ProxyType<Core::IIPCServer>(_engine)))
    {
    }
    ~PluginHost()
    {
        Deinitialize();
    }

public:
    void Initialize()
    {
        uint32_t result = _comClient->Open(RPC::CommunicationTimeOut);

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T("Could not open connection to node %s. Error: %s"), _comClient->Source().RemoteId(), Core::NumberType<uint32_t>(result).Text()));
        } else {
            Messaging::MessageUnit::Instance().Open(_comClient->ConnectionId());
        }
    }

    void Deinitialize()
    {
        if (_comClient.IsValid() == true) {
            _comClient.Release();
        }
        if (_engine.IsValid() == true) {
            _engine.Release();
        }
        Core::Singleton::Dispose();
    }

    void Trace()
    {
        TRACE(Trace::Information, (_T("test trace")));
    }

    void Syslog()
    {
        SYSLOG(Logging::Notification,(_T("test syslog")));
    }

private:
    Core::ProxyType<RPC::InvokeServerType<2, 0, 4> > _engine;
    Core::ProxyType<RPC::CommunicatorClient> _comClient;
} _thunderClient;

}
}

void help() {
    printf ("I -> Initialize the connection\n");
    printf ("D -> Deinitialize the connection\n");
    printf ("T -> Trace\n");
    printf ("S -> Syslog\n");
    printf ("Q -> Quit\n>");
}

int main(int argc, char** argv)
{
    {
        int element;

        help();
        do {
            element = toupper(getchar());

            switch (element) {
                case 'I': {
                    Thunder::Test::_thunderClient.Initialize();
                    fprintf(stdout, "PluginHost initialized..\n");
                    fflush(stdout);
                    break;
                }
                case 'D': {
                    Thunder::Test::_thunderClient.Deinitialize();
                    fprintf(stdout, "PluginHost deinitialized..\n");
                    fflush(stdout);
                    break;
                }
                case 'T': {
                    Thunder::Test::_thunderClient.Trace();
                    fprintf(stdout, "Trace sent..\n");
                    fflush(stdout);
                    break;
                }
                case 'S': {
                    Thunder::Test::_thunderClient.Syslog();
                    fprintf(stdout, "Syslog sent..\n");
                    fflush(stdout);
                    break;
                }
                case 'Q': break;
                default: {
                }
            }
        } while (element != 'Q');
    }

    return (0);
}

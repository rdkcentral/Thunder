#include "interface/IEcho.h"
#include "core/IServiceManager.h"
#include "com/BinderCommunicator.h"
#include <iostream>

using namespace Thunder::BinderIPC;
using namespace Thunder::RPC;

class COMServer : public RPC::BinderCommunicator {
private:
    class Implementation : public Exchange::IEcho {
    public:
        Implementation(const Implementation&) = delete;
        Implementation& operator= (const Implementation&) = delete;

        Implementation() {
        }
        ~Implementation() override = default;

    public:
        std::string Echo(const std::string& message) override {
            return message;
        }

        BEGIN_INTERFACE_MAP(Implementation)
            INTERFACE_ENTRY(Exchange::IEcho)
        END_INTERFACE_MAP
    };

public:
    COMServer() = delete;
    COMServer(const COMServer&) = delete;
    COMServer& operator=(const COMServer&) = delete;

    COMServer(
        const string& serviceName,
        const string& proxyServerPath,
        const Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >& engine)
        : RPC::BinderCommunicator(
            serviceName, 
            proxyServerPath, 
            Core::ProxyType<Core::IIPCServer>(engine),
            _T("@SimpleComBinderServer"))
        , _remoteEntry(nullptr)
    {
        // Open the communicator -> this will trigger registering the service
        // with the service manager
        Open(Core::infinite);
    }
    ~COMServer() override
    {
        Close(Core::infinite);
    }

private:
    void* Acquire(const string& className, const uint32_t interfaceId, const uint32_t versionId) override
    {
        void* result = nullptr;

        if ((versionId == 1) || (versionId == static_cast<uint32_t>(~0))) {
            
            if (interfaceId == ::Exchange::IEcho::ID) {
                result = Core::ServiceType<Implementation>::Create<Exchange::IEcho>();
            }
            else if (interfaceId == Core::IUnknown::ID) {
                result = Core::ServiceType<Implementation>::Create<Core::IUnknown>();
            }
        }
        return (result);
    }
    void Offer(Core::IUnknown* remote, const uint32_t interfaceId) override
    {

    }
    void Revoke(const Core::IUnknown* remote, const uint32_t interfaceId) override
    {
    }
};


int main()
{
    printf("Starting EchoService Binder COM server...\n");
    {
        int element;
        string psPath = "/usr/lib/Thunder/ProxyStubs"; //Get this from args ideally
        COMServer server("EchoService", psPath, Core::ProxyType< RPC::InvokeServerType<1, 0, 4> >::Create());

        do {
            printf("\n>");
            element = toupper(getchar());

            switch (element) {
            case 'Q': break;
            default: break;
            }

        } while (element != 'Q');
    }

    Core::Singleton::Dispose();

    return 0;
}

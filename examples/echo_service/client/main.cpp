#include "core/IServiceManager.h"
#include "com/BinderCommunicator.h"
#include <iostream>
#include <string>

using namespace Thunder::BinderIPC;
using namespace Thunder::RPC;

int main() {

    {
        int element;
        Exchange::IEcho* echo_service(nullptr);
        Core::ProxyType<RPC::BinderCommunicatorClient> client(Core::ProxyType<RPC::BinderCommunicatorClient>::Create());

        if (client->IsOpen() == false) {
            //This should open the binder channel.
            client->Open(2000);
        }

        if (client->IsOpen() == false) {
            printf("Could not open a connection to the server. No exchange of interfaces happened!\n");
        } else {
                //This should acquire the IEcho service from the service manager via binder
                echo_service = client->Acquire<Exchange::IEcho>(3000, _T("EchoImplementation"), ~0);
        }

        if (echo_service == nullptr) {
            printf("Tried aquiring the IEcho service, but it is not available\n");
        } else {
            //This should call the generated proxy code of the IEcho interface
            // The proxy internally calls UnknownProxyType::Invoke(message)
            // which uses the IPCChannel (which in our case is BinderIPCChannelType) to call 
            // the remote side.
            std::string response = echo_service->Echo("Hello, Thunder!");
            std::cout << "Echo response: " << response << std::endl;
        }
    }
    Core::Singleton::Dispose();
    return 0;
}

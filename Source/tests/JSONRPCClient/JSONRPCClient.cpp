#define MODULE_NAME JSONRPC_Test

#include <core/core.h>
#include <jsonrpc/jsonrpc.h>

using namespace WPEFramework;

bool ParseOptions(int argc, char** argv, Core::NodeId& jsonrpcChannel)
{
    int index = 1;
    bool domain = false;
    bool showHelp = false;

    while ((index < argc) && (!showHelp)) {
        if (strcmp(argv[index], "-domain") == 0)
            domain = true;
        else if (strcmp(argv[index], "-h") == 0)
            showHelp = true;
        index++;
    }

    if (!showHelp) {
        if (domain) {
			jsonrpcChannel = Core::NodeId("/tmp/testserver0");
        }
        else {
			jsonrpcChannel = Core::NodeId("127.0.0.1", 80);
        }
    }

    return (showHelp);
}

void ShowMenu()
{
    printf("Enter\n"
		"\tI : Invoke a synchronous method for getting the server time\n"
		"\tR : Register for a-synchronous feedback\n"
        "\tU : Unregister for a-synchronous feedback\n"
        "\tH : Help\n"
        "\tQ : Quit\n");
}

namespace Handlers {

	// The methods to be used for handling the incoming methods can be static methods or
	// methods in classes. In the client tthere is a demonstration of a static method,
	// in the server is a demonstartion of an object method.
	// To avoid name clashses it is recomended to put the handlers in a namespace (clock,
	// for example, alreay exists in the global namespace and you get very interesting 
	// compiler warnings if there is a name clash)
	static void clock(const Core::JSON::String& parameters) {
		printf("Received a new time: %s\n", parameters.Value().c_str());
	}
}

int main(int argc, char** argv)
{
	Core::NodeId jsonrpcChannel;
    ShowMenu();
	int element;

	ParseOptions(argc, argv, jsonrpcChannel);

	// The JSONRPC Client library is expecting the THUNDER_ACCESS environment variable to be set and pointing to the 
	// WPEFramework, this can be a domain socket (use at least 1 sleash in it, or a TCP address.
	Core::SystemInfo::SetEnvironment(_T("THUNDER_ACCESS"), (_T("127.0.0.1:80")));

	// This is not mandatory, just an easy way to use the VisualStudio environment to Start 2 projects at once, 1 project 
	// being the WPEFramework running the plaugins and the other one this client. However, give the sevrver a bit of time 
	// to bring up Plugin JSONRPCExamplePlugin, before we hook up to it. If one starts this APp, after the WPEFramework is
	// up and running this is not nessecary.
	SleepMs(2000);

	// Create a remoteObject.  This is the way we can communicate with the WPEFramework.
	// The parameters:
	// 1. [mandatory] This is the designator of the module we will connect to.
	// 2. [optional]  This is the designator used for the code we have on my side.
	// 3. [optional]  should the websocket under the hood call directly the plugin 
	//                or will it be rlayed through thejsonrpc dispatcher (default, 
	//                use jsonrpc dispatcher)  
	JSONRPC::Client remoteObject(_T("JSONRPCExamplePlugin.1"), _T("client.events.1"));

    do {
        printf("\n>");
        element = toupper(getchar());

        switch (element) {
		case 'I': {
			// Lets trigger some action on server side to get some feedback. The regular synchronous RPC call.
			// The parameters:
			// 1. [mandatory] Time to wait for the round trip to complete to the server to register.
			// 2. [mandatory] Method name to call (See JSONRPCExamplePlugin::JSONRPCExamplePlugin - 14)
			// 3. [mandatory] Parameters to be send to the other side.
			// 4. [mandatory] Response to be received from the other side.
			Core::JSON::String result;
			remoteObject.Invoke<string, Core::JSON::String>(1000, _T("time"), _T(""), result);
			printf("received time: %s\n", result.Value().c_str());
			break;
		}
        case 'R': { 
			// We have a handler, called Handlers::clock to handle the events coming from the WPEFramework.
			// If we register this handler, it will also automatically be register this handler on the server side.
			// The parameters:
			// 1. [mandatory] Time to wait for the round trip to complete to the server to register.
			// 2. [mandatory] Event name to subscribe to on server side (See JSONRPCExamplePlugin::SendTime - 44)
			// 3. [mandatory] Code to handle this event, it is allowed to use a lambda here, or a object method (see plugin)
			if (remoteObject.Subscribe<Core::JSON::String>(1000, _T("clock"), &Handlers::clock) == Core::ERROR_NONE) {
				printf("Installed a notification handler and registered for the notifications\n");
			}
			else {
				printf("Failed to install a notification handler\n");
			}
            break;
        }
		case 'U': {
			// We are no longer interested inm the events, ets get ride of the notifications.
			// The parameters:
			// 1. [mandatory] Time to wait for the round trip to complete to the server to register.
			// 2. [mandatory] Event name which was used during the registration
			remoteObject.Unsubscribe(1000, _T("clock"));
			printf("Unregistered and renmoved a notification handler\n");
			break;
		}
		case 'H' :
            ShowMenu();
        }

    } while (element != 'Q');

    printf("Leaving app.\n");

    Core::Singleton::Dispose();

    return (0);
}

#include "Module.h"

#include <core/core.h>
#include <cryptalgo/cryptalgo.h>
#include <websocket/websocket.h>

using namespace Thunder;

class WebClient : public Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> {
private:
    typedef Web::WebLinkType<Crypto::SecureSocketPort, Web::Response, Web::Request, Thunder::Core::ProxyPoolType<Web::Response>&> BaseClass;

    // All requests needed by any instance of this socket class, is coming from this static factory.
    // This means that all Requests, received are shared among all WebServer sockets, hopefully limiting
    // the number of requests that need to be created.
    static Thunder::Core::ProxyPoolType<Web::Response> _responseFactory;

public:
    WebClient() = delete;
    WebClient(const WebClient& copy) = delete;
    WebClient& operator=(const WebClient&) = delete;

    WebClient(const Thunder::Core::NodeId& remoteNode)
        : BaseClass(5, _responseFactory, Core::SocketPort::STREAM, remoteNode.AnyInterface(), remoteNode, 2048, 2048)
        , _textBodyFactory(2)
    {
    }
    virtual ~WebClient()
    {
        BaseClass::Close(Thunder::Core::infinite);
    }

public:
    // Notification of a Partial Request received, time to attach a body..
    virtual void LinkBody(Core::ProxyType<Thunder::Web::Response>& element)
    {
        // Time to attach a String Body
        element->Body<Web::TextBody>(_textBodyFactory.Element());
    }
    virtual void Received(Core::ProxyType<Thunder::Web::Response>& element)
    {
        string received;
        element->ToString(received);
        printf(_T("Received: %s\n"), received.c_str());
    }
    virtual void Send(const Core::ProxyType<Thunder::Web::Request>& response)
    {
        string send;
        response->ToString(send);
        printf(_T("Send: %s\n"), send.c_str());
    }
    virtual void StateChange()
    {
        printf(_T("State change: "));
        if (IsOpen()) {
            printf(_T("[SecureSocket] Open - OK\n"));
        } else if (IsClosed()) {
            printf(_T("[SecureSocket] Close - OK\n"));
        }
    }
    void Submit()
    {
        if (IsOpen()) {
            Core::ProxyType<Thunder::Web::Request> request = Core::ProxyType<Thunder::Web::Request>::Create();

            printf("Creating Request!!!!.\n");
            request->Verb = Web::Request::HTTP_GET;
            request->Host = _T("httpbin.org");
            request->Path = _T("/get");
            request->Query = _T("test=OK&this=me");
            printf("Submitting Request!!!!.\n");
            BaseClass::Submit(request);
        }
    }
    void SubmitAuthRequest()
    {
        if (IsOpen()) {
            Core::ProxyType<Thunder::Web::Request> request = Core::ProxyType<Thunder::Web::Request>::Create();

            printf("Creating Auth Request!!!!.\n");
            request->Verb = Web::Request::HTTP_GET;
            request->Host = _T("httpbin.org");
            request->Path = _T("/basic-auth/user/passwd");

            string input("user:passwd");
            uint16_t inputLength = static_cast<uint16_t>(input.length());
            uint16_t outputLength = (((inputLength * 8) / 6) + 4) * sizeof(TCHAR);
            TCHAR output[(((inputLength * 8) / 6) + 4) * sizeof(TCHAR)];
            uint16_t convertedLength = Core::URL::Base64Encode(
                reinterpret_cast<const uint8_t*>(input.c_str()), inputLength * sizeof(TCHAR),
                output, sizeof(output));

            string header = string(output, convertedLength);
            header +='=';
            request->WebToken = Web::Authorization(Web::Authorization::BASIC, _T(header.c_str()));
            printf("Submitting Auth Request!!!!.\n");
            BaseClass::Submit(request);
        }
    }
    void Close()
    {
        printf(_T("[SecureSocket] Close - calling\n"));
        BaseClass::Close(0);
        while (IsClosed() == false);
        printf(_T("[SecureSocket] Close - OK\n"));
    }
    Core::ProxyPoolType<Web::TextBody> _textBodyFactory;
};

/* static */ Thunder::Core::ProxyPoolType<Web::Response> WebClient::_responseFactory(5);

#ifdef __WIN32__
int _tmain(int argc, _TCHAR**)
#else
int main(int argc, const char* argv[])
#endif
{
    fprintf(stderr, "%s build: %s\n"
                    "licensed under LGPL2.1\n", argv[0], __TIMESTAMP__);

#ifdef __WIN32__
#pragma warning(disable : 4355)
#endif

    // Scope to make sure that the sockets are destructed before we close the singletons !!!
    {
        int keyPress;

        WebClient webConnector(Core::NodeId("httpbin.org", 443));
        Core::ProxyType<Thunder::Web::Request> webRequest(Core::ProxyType<Thunder::Web::Request>::Create());

        printf("Ready to start processing events, options to start \n 0 -> connect \n 1 -> disconnect \n 2 -> Submit Request \n 3 -> Submit with Authorization Header \n");
        do {
            printf("\nEnter Key : ");
            keyPress = toupper(getchar());

            switch (keyPress) {
            case '0': {
                printf("Opening up sockets...\n");
                uint32_t result = webConnector.Open(0);

                if ((result != Core::ERROR_NONE) && (result != Core::ERROR_INPROGRESS)) {
                    printf("Could not open the connection, error: %d\n", result);
                } else {
                    printf("Waiting 5 seconds...\n");
                    SleepMs(5000);
                } 

                break;
            }
            case '1': {
                webConnector.Close();
                break;
            }
            case '2': {
                webConnector.Submit();
                break;
            }
            case '3': {
                webConnector.SubmitAuthRequest();
                break;
            }

            }

        } while (keyPress != 'Q');
    }

    // Clear the factory we created..
    Thunder::Core::Singleton::Dispose();

    printf("\nClosing down!!!\n");

    return 0;
}

//
// generated automatically from "IWebServer.h"
//
// implements RPC proxy stubs for:
//   - class IWebServer
//

#include "IWebServer.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IWebServer interface stub definitions
    //
    // Methods:
    //  (0) virtual void AddProxy(const string&, const string&, const string&) = 0
    //  (1) virtual void RemoveProxy(const string&) = 0
    //  (2) virtual string Accessor() const = 0
    //

    ProxyStub::MethodHandler WebServerStubMethods[] = {
        // virtual void AddProxy(const string&, const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();
            const string param2 = reader.Text();

            // call implementation
            IWebServer* implementation = input.Implementation<IWebServer>();
            ASSERT((implementation != nullptr) && "Null IWebServer implementation pointer");
            implementation->AddProxy(param0, param1, param2);
        },

        // virtual void RemoveProxy(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IWebServer* implementation = input.Implementation<IWebServer>();
            ASSERT((implementation != nullptr) && "Null IWebServer implementation pointer");
            implementation->RemoveProxy(param0);
        },

        // virtual string Accessor() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IWebServer* implementation = input.Implementation<IWebServer>();
            ASSERT((implementation != nullptr) && "Null IWebServer implementation pointer");
            const string output = implementation->Accessor();

            // write return value
            writer.Text(output);
        },

        nullptr
    }; // WebServerStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IWebServer interface proxy definitions
    //
    // Methods:
    //  (0) virtual void AddProxy(const string&, const string&, const string&) = 0
    //  (1) virtual void RemoveProxy(const string&) = 0
    //  (2) virtual string Accessor() const = 0
    //

    class WebServerProxy final : public ProxyStub::UnknownProxyType<IWebServer> {
    public:
        WebServerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void AddProxy(const string& param0, const string& param1, const string& param2) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);
            writer.Text(param2);

            // invoke the method handler
            Invoke(newMessage);
        }

        void RemoveProxy(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        string Accessor() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class WebServerProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IWebServer, WebServerStubMethods> WebServerStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IWebServer, WebServerProxy, WebServerStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

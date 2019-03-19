//
// generated automatically from "IPlayGiga.h"
//
// implements RPC proxy stubs for:
//   - class IPlayGiga
//

#include "IPlayGiga.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IPlayGiga interface stub definitions
    //
    // Methods:
    //  (0) virtual void Launch(const string&, const string&) = 0
    //

    ProxyStub::MethodHandler PlayGigaStubMethods[] = {
        // virtual void Launch(const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();

            // call implementation
            IPlayGiga* implementation = input.Implementation<IPlayGiga>();
            ASSERT((implementation != nullptr) && "Null IPlayGiga implementation pointer");
            implementation->Launch(param0, param1);
        },

        nullptr
    }; // PlayGigaStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IPlayGiga interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Launch(const string&, const string&) = 0
    //

    class PlayGigaProxy final : public ProxyStub::UnknownProxyType<IPlayGiga> {
    public:
        PlayGigaProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Launch(const string& param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class PlayGigaProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IPlayGiga, PlayGigaStubMethods> PlayGigaStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IPlayGiga, PlayGigaProxy, PlayGigaStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

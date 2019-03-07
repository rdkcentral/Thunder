//
// generated automatically from "IWebDriver.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IWebDriver
//

#include "IWebDriver.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// IWebDriver interface stub definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//

ProxyStub::MethodHandler WebDriverStubMethods[] = {
    // virtual uint32_t Configure(PluginHost::IShell*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        PluginHost::IShell* param0 = reader.Number<PluginHost::IShell*>();
        PluginHost::IShell* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<PluginHost::IShell>(channel, param0, true);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of PluginHost::IShell proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of PluginHost::IShell proxy");
            }
        }

        RPC::Data::Frame::Writer writer(message->Response().Writer());

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IWebDriver* implementation = input.Implementation<IWebDriver>();
            ASSERT((implementation != nullptr) && "Null IWebDriver implementation pointer");
            const uint32_t output = implementation->Configure(param0_proxy);

            // write return value
            writer.Number<const uint32_t>(output);
        }
        else {
            // return error code
            writer.Number<const uint32_t>(Core::ERROR_RPC_CALL_FAILED);
        }

        if ((param0_proxy != nullptr) && (RPC::Administrator::Instance().Release(param0_proxy, writer) != Core::ERROR_NONE)) {
            TRACE_L1("Warning: PluginHost::IShell proxy destroyed");
        }
    },

    nullptr
}; // WebDriverStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// IWebDriver interface proxy definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//

class WebDriverProxy final : public ProxyStub::UnknownProxyType<IWebDriver> {
public:
    WebDriverProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    uint32_t Configure(PluginHost::IShell* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<PluginHost::IShell*>(param0);

        uint32_t output{};
        if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
            // read return value
            output = Number<uint32_t>(newMessage->Response());

            Complete(newMessage->Response());
        }

        return output;
    }
}; // class WebDriverProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::UnknownStubType<IWebDriver, WebDriverStubMethods> WebDriverStub;

static class Instantiation {
public:
    Instantiation()
    {
        RPC::Administrator::Instance().Announce<IWebDriver, WebDriverProxy, WebDriverStub>();
    }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


//
// generated automatically from "IDsgccClient.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IDsgccClient
//

#include "IDsgccClient.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// IDsgccClient interface stub definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (1) virtual void DsgccClientSet(const string&) = 0
//  (2) virtual string DsgccClientGet() const = 0
//

ProxyStub::MethodHandler DsgccClientStubMethods[] = {
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
            IDsgccClient* implementation = input.Implementation<IDsgccClient>();
            ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
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

    // virtual void DsgccClientSet(const string&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        IDsgccClient* implementation = input.Implementation<IDsgccClient>();
        ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
        implementation->DsgccClientSet(param0);
    },

    // virtual string DsgccClientGet() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        RPC::Data::Frame::Writer writer(message->Response().Writer());

        // call implementation
        const IDsgccClient* implementation = input.Implementation<IDsgccClient>();
        ASSERT((implementation != nullptr) && "Null IDsgccClient implementation pointer");
        const string output = implementation->DsgccClientGet();

        // write return value
        writer.Text(output);
    },

    nullptr
}; // DsgccClientStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// IDsgccClient interface proxy definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (1) virtual void DsgccClientSet(const string&) = 0
//  (2) virtual string DsgccClientGet() const = 0
//

class DsgccClientProxy final : public ProxyStub::UnknownProxyType<IDsgccClient> {
public:
    DsgccClientProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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

    void DsgccClientSet(const string& param0) override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        Invoke(newMessage);
    }

    string DsgccClientGet() const override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        string output{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output = Text(newMessage->Response());
        }

        return output;
    }
}; // class DsgccClientProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::UnknownStubType<IDsgccClient, DsgccClientStubMethods> DsgccClientStub;

static class Instantiation {
public:
    Instantiation()
    {
        RPC::Administrator::Instance().Announce<IDsgccClient, DsgccClientProxy, DsgccClientStub>();
    }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


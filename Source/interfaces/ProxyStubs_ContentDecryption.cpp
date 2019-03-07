//
// generated automatically from "IContentDecryption.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IContentDecryption
//

#include "IContentDecryption.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// IContentDecryption interface stub definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (1) virtual uint32_t Reset() = 0
//  (2) virtual RPC::IStringIterator* Systems() const = 0
//  (3) virtual RPC::IStringIterator* Designators(const string&) const = 0
//  (4) virtual RPC::IStringIterator* Sessions(const string&) const = 0
//

ProxyStub::MethodHandler ContentDecryptionStubMethods[] = {
    // virtual uint32_t Configure(PluginHost::IShell*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        PluginHost::IShell* param0 = reader.Number<PluginHost::IShell*>();
        PluginHost::IShell* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<PluginHost::IShell>(channel, param0);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of PluginHost::IShell proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of PluginHost::IShell proxy");
            }
        }

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IContentDecryption* implementation = input.Implementation<IContentDecryption>();
            ASSERT((implementation != nullptr) && "Null IContentDecryption implementation pointer");
            const uint32_t output = implementation->Configure(param0_proxy);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        }
        else {
            // return error code
            message->Response().Writer().Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
        }

        if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
            TRACE_L1("Warning: PluginHost::IShell proxy destroyed");
        }
    },

    // virtual uint32_t Reset() = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        IContentDecryption* implementation = input.Implementation<IContentDecryption>();
        ASSERT((implementation != nullptr) && "Null IContentDecryption implementation pointer");
        const uint32_t output = implementation->Reset();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<const uint32_t>(output);
    },

    // virtual RPC::IStringIterator* Systems() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        const IContentDecryption* implementation = input.Implementation<IContentDecryption>();
        ASSERT((implementation != nullptr) && "Null IContentDecryption implementation pointer");
        RPC::IStringIterator* output = implementation->Systems();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<RPC::IStringIterator*>(output);
    },

    // virtual RPC::IStringIterator* Designators(const string&) const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        const IContentDecryption* implementation = input.Implementation<IContentDecryption>();
        ASSERT((implementation != nullptr) && "Null IContentDecryption implementation pointer");
        RPC::IStringIterator* output = implementation->Designators(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<RPC::IStringIterator*>(output);
    },

    // virtual RPC::IStringIterator* Sessions(const string&) const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        const IContentDecryption* implementation = input.Implementation<IContentDecryption>();
        ASSERT((implementation != nullptr) && "Null IContentDecryption implementation pointer");
        RPC::IStringIterator* output = implementation->Sessions(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<RPC::IStringIterator*>(output);
    },

    nullptr
}; // ContentDecryptionStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// IContentDecryption interface proxy definitions
//
// Methods:
//  (0) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (1) virtual uint32_t Reset() = 0
//  (2) virtual RPC::IStringIterator* Systems() const = 0
//  (3) virtual RPC::IStringIterator* Designators(const string&) const = 0
//  (4) virtual RPC::IStringIterator* Sessions(const string&) const = 0
//

class ContentDecryptionProxy final : public ProxyStub::UnknownProxyType<IContentDecryption> {
public:
    ContentDecryptionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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
        }

        return output;
    }

    uint32_t Reset() override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        uint32_t output{};
        if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
            // read return value
            output = Number<uint32_t>(newMessage->Response());
        }

        return output;
    }

    RPC::IStringIterator* Systems() const override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        RPC::IStringIterator* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<RPC::IStringIterator*>(Interface(newMessage->Response(), RPC::IStringIterator::ID));
        }

        return output_proxy;
    }

    RPC::IStringIterator* Designators(const string& param0) const override
    {
        IPCMessage newMessage(BaseClass::Message(3));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        RPC::IStringIterator* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<RPC::IStringIterator*>(Interface(newMessage->Response(), RPC::IStringIterator::ID));
        }

        return output_proxy;
    }

    RPC::IStringIterator* Sessions(const string& param0) const override
    {
        IPCMessage newMessage(BaseClass::Message(4));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        RPC::IStringIterator* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<RPC::IStringIterator*>(Interface(newMessage->Response(), RPC::IStringIterator::ID));
        }

        return output_proxy;
    }
}; // class ContentDecryptionProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::UnknownStubType<IContentDecryption, ContentDecryptionStubMethods> ContentDecryptionStub;

static class Instantiation {
public:
    Instantiation()
    {
        RPC::Administrator::Instance().Announce<IContentDecryption, ContentDecryptionProxy, ContentDecryptionStub>();
    }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


//
// generated automatically from "IComposition.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IComposition
//   - class ::WPEFramework::Exchange::IComposition::IClient
//   - class ::WPEFramework::Exchange::IComposition::INotification
//

#include "IComposition.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// IComposition interface stub definitions
//
// Methods:
//  (0) virtual void Register(IComposition::INotification*) = 0
//  (1) virtual void Unregister(IComposition::INotification*) = 0
//  (2) virtual IComposition::IClient* Client(const uint8_t) = 0
//  (3) virtual IComposition::IClient* Client(const string&) = 0
//  (4) virtual uint32_t Geometry(const string&, const IComposition::Rectangle&) = 0
//  (5) virtual IComposition::Rectangle Geometry(const string&) const = 0
//  (6) virtual uint32_t ToTop(const string&) = 0
//  (7) virtual uint32_t PutBelow(const string&, const string&) = 0
//  (8) virtual RPC::IStringIterator* ClientsInZorder() const = 0
//  (9) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (10) virtual void Resolution(const IComposition::ScreenResolution) = 0
//  (11) virtual IComposition::ScreenResolution Resolution() const = 0
//

ProxyStub::MethodHandler CompositionStubMethods[] = {
    // virtual void Register(IComposition::INotification*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        IComposition::INotification* param0 = reader.Number<IComposition::INotification*>();
        IComposition::INotification* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<IComposition::INotification>(channel, param0);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of IComposition::INotification proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of IComposition::INotification proxy");
            }
        }

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IComposition* implementation = input.Implementation<IComposition>();
            ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
            implementation->Register(param0_proxy);
        }

        if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
            TRACE_L1("Warning: IComposition::INotification proxy destroyed");
        }
    },

    // virtual void Unregister(IComposition::INotification*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        IComposition::INotification* param0 = reader.Number<IComposition::INotification*>();
        IComposition::INotification* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<IComposition::INotification>(channel, param0);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of IComposition::INotification proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of IComposition::INotification proxy");
            }
        }

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IComposition* implementation = input.Implementation<IComposition>();
            ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
            implementation->Unregister(param0_proxy);
        }

        if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
            TRACE_L1("Warning: IComposition::INotification proxy destroyed");
        }
    },

    // virtual IComposition::IClient* Client(const uint8_t) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const uint8_t param0 = reader.Number<uint8_t>();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        IComposition::IClient* output = implementation->Client(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<IComposition::IClient*>(output);
    },

    // virtual IComposition::IClient* Client(const string&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        IComposition::IClient* output = implementation->Client(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<IComposition::IClient*>(output);
    },

    // virtual uint32_t Geometry(const string&, const IComposition::Rectangle&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();
        // (decompose IComposition::Rectangle)
        IComposition::Rectangle param1;
        param1.x = reader.Number<uint32_t>();
        param1.y = reader.Number<uint32_t>();
        param1.width = reader.Number<uint32_t>();
        param1.height = reader.Number<uint32_t>();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        const uint32_t output = implementation->Geometry(param0, param1);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<const uint32_t>(output);
    },

    // virtual IComposition::Rectangle Geometry(const string&) const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        const IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        const IComposition::Rectangle output = implementation->Geometry(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        // (decompose IComposition::Rectangle)
        writer.Number<uint32_t>(output.x);
        writer.Number<uint32_t>(output.y);
        writer.Number<uint32_t>(output.width);
        writer.Number<uint32_t>(output.height);
    },

    // virtual uint32_t ToTop(const string&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        const uint32_t output = implementation->ToTop(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<const uint32_t>(output);
    },

    // virtual uint32_t PutBelow(const string&, const string&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();
        const string param1 = reader.Text();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        const uint32_t output = implementation->PutBelow(param0, param1);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<const uint32_t>(output);
    },

    // virtual RPC::IStringIterator* ClientsInZorder() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        const IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        RPC::IStringIterator* output = implementation->ClientsInZorder();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<RPC::IStringIterator*>(output);
    },

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
            IComposition* implementation = input.Implementation<IComposition>();
            ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
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

    // virtual void Resolution(const IComposition::ScreenResolution) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const IComposition::ScreenResolution param0 = reader.Number<IComposition::ScreenResolution>();

        // call implementation
        IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        implementation->Resolution(param0);
    },

    // virtual IComposition::ScreenResolution Resolution() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        const IComposition* implementation = input.Implementation<IComposition>();
        ASSERT((implementation != nullptr) && "Null IComposition implementation pointer");
        const IComposition::ScreenResolution output = implementation->Resolution();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<const IComposition::ScreenResolution>(output);
    },

    nullptr
}; // CompositionStubMethods[]

//
// IComposition::IClient interface stub definitions
//
// Methods:
//  (0) virtual string Name() const = 0
//  (1) virtual void Kill() = 0
//  (2) virtual void Opacity(const uint32_t) = 0
//  (3) virtual void ChangedGeometry(const IComposition::Rectangle&) = 0
//  (4) virtual void ChangedZOrder(const uint8_t) = 0
//

ProxyStub::MethodHandler CompositionClientStubMethods[] = {
    // virtual string Name() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        const IComposition::IClient* implementation = input.Implementation<IComposition::IClient>();
        ASSERT((implementation != nullptr) && "Null IComposition::IClient implementation pointer");
        const string output = implementation->Name();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Text(output);
    },

    // virtual void Kill() = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // call implementation
        IComposition::IClient* implementation = input.Implementation<IComposition::IClient>();
        ASSERT((implementation != nullptr) && "Null IComposition::IClient implementation pointer");
        implementation->Kill();
    },

    // virtual void Opacity(const uint32_t) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const uint32_t param0 = reader.Number<uint32_t>();

        // call implementation
        IComposition::IClient* implementation = input.Implementation<IComposition::IClient>();
        ASSERT((implementation != nullptr) && "Null IComposition::IClient implementation pointer");
        implementation->Opacity(param0);
    },

    // virtual void ChangedGeometry(const IComposition::Rectangle&) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        // (decompose IComposition::Rectangle)
        IComposition::Rectangle param0;
        param0.x = reader.Number<uint32_t>();
        param0.y = reader.Number<uint32_t>();
        param0.width = reader.Number<uint32_t>();
        param0.height = reader.Number<uint32_t>();

        // call implementation
        IComposition::IClient* implementation = input.Implementation<IComposition::IClient>();
        ASSERT((implementation != nullptr) && "Null IComposition::IClient implementation pointer");
        implementation->ChangedGeometry(param0);
    },

    // virtual void ChangedZOrder(const uint8_t) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const uint8_t param0 = reader.Number<uint8_t>();

        // call implementation
        IComposition::IClient* implementation = input.Implementation<IComposition::IClient>();
        ASSERT((implementation != nullptr) && "Null IComposition::IClient implementation pointer");
        implementation->ChangedZOrder(param0);
    },

    nullptr
}; // CompositionClientStubMethods[]

//
// IComposition::INotification interface stub definitions
//
// Methods:
//  (0) virtual void Attached(IComposition::IClient*) = 0
//  (1) virtual void Detached(IComposition::IClient*) = 0
//

ProxyStub::MethodHandler CompositionNotificationStubMethods[] = {
    // virtual void Attached(IComposition::IClient*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        IComposition::IClient* param0 = reader.Number<IComposition::IClient*>();
        IComposition::IClient* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<IComposition::IClient>(channel, param0);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of IComposition::IClient proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of IComposition::IClient proxy");
            }
        }

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IComposition::INotification* implementation = input.Implementation<IComposition::INotification>();
            ASSERT((implementation != nullptr) && "Null IComposition::INotification implementation pointer");
            implementation->Attached(param0_proxy);
        }

        if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
            TRACE_L1("Warning: IComposition::IClient proxy destroyed");
        }
    },

    // virtual void Detached(IComposition::IClient*) = 0
    //
    [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {

        RPC::Data::Input& input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        IComposition::IClient* param0 = reader.Number<IComposition::IClient*>();
        IComposition::IClient* param0_proxy = nullptr;
        if (param0 != nullptr) {
            param0_proxy = RPC::Administrator::Instance().ProxyInstance<IComposition::IClient>(channel, param0);
            ASSERT((param0_proxy != nullptr) && "Failed to get instance of IComposition::IClient proxy");
            if (param0_proxy == nullptr) {
                TRACE_L1("Failed to get instance of IComposition::IClient proxy");
            }
        }

        if ((param0 == nullptr) || (param0_proxy != nullptr)) {
            // call implementation
            IComposition::INotification* implementation = input.Implementation<IComposition::INotification>();
            ASSERT((implementation != nullptr) && "Null IComposition::INotification implementation pointer");
            implementation->Detached(param0_proxy);
        }

        if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
            TRACE_L1("Warning: IComposition::IClient proxy destroyed");
        }
    },

    nullptr
}; // CompositionNotificationStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// IComposition interface proxy definitions
//
// Methods:
//  (0) virtual void Register(IComposition::INotification*) = 0
//  (1) virtual void Unregister(IComposition::INotification*) = 0
//  (2) virtual IComposition::IClient* Client(const uint8_t) = 0
//  (3) virtual IComposition::IClient* Client(const string&) = 0
//  (4) virtual uint32_t Geometry(const string&, const IComposition::Rectangle&) = 0
//  (5) virtual IComposition::Rectangle Geometry(const string&) const = 0
//  (6) virtual uint32_t ToTop(const string&) = 0
//  (7) virtual uint32_t PutBelow(const string&, const string&) = 0
//  (8) virtual RPC::IStringIterator* ClientsInZorder() const = 0
//  (9) virtual uint32_t Configure(PluginHost::IShell*) = 0
//  (10) virtual void Resolution(const IComposition::ScreenResolution) = 0
//  (11) virtual IComposition::ScreenResolution Resolution() const = 0
//

class CompositionProxy final : public ProxyStub::UnknownProxyType<IComposition> {
public:
    CompositionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    void Register(IComposition::INotification* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<IComposition::INotification*>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    void Unregister(IComposition::INotification* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<IComposition::INotification*>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    IComposition::IClient* Client(const uint8_t param0) override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<const uint8_t>(param0);

        IComposition::IClient* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<IComposition::IClient*>(Interface(newMessage->Response(), IComposition::IClient::ID));
        }

        return output_proxy;
    }

    IComposition::IClient* Client(const string& param0) override
    {
        IPCMessage newMessage(BaseClass::Message(3));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        IComposition::IClient* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<IComposition::IClient*>(Interface(newMessage->Response(), IComposition::IClient::ID));
        }

        return output_proxy;
    }

    uint32_t Geometry(const string& param0, const IComposition::Rectangle& param1) override
    {
        IPCMessage newMessage(BaseClass::Message(4));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);
        // (decompose IComposition::Rectangle)
        writer.Number<uint32_t>(param1.x);
        writer.Number<uint32_t>(param1.y);
        writer.Number<uint32_t>(param1.width);
        writer.Number<uint32_t>(param1.height);

        uint32_t output{};
        if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
            // read return value
            output = Number<uint32_t>(newMessage->Response());
        }

        return output;
    }

    IComposition::Rectangle Geometry(const string& param0) const override
    {
        IPCMessage newMessage(BaseClass::Message(5));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        IComposition::Rectangle output{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            // (decompose IComposition::Rectangle)
            output.x = Number<uint32_t>(newMessage->Response());
            output.y = Number<uint32_t>(newMessage->Response());
            output.width = Number<uint32_t>(newMessage->Response());
            output.height = Number<uint32_t>(newMessage->Response());
        }

        return output;
    }

    uint32_t ToTop(const string& param0) override
    {
        IPCMessage newMessage(BaseClass::Message(6));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        uint32_t output{};
        if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
            // read return value
            output = Number<uint32_t>(newMessage->Response());
        }

        return output;
    }

    uint32_t PutBelow(const string& param0, const string& param1) override
    {
        IPCMessage newMessage(BaseClass::Message(7));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);
        writer.Text(param1);

        uint32_t output{};
        if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
            // read return value
            output = Number<uint32_t>(newMessage->Response());
        }

        return output;
    }

    RPC::IStringIterator* ClientsInZorder() const override
    {
        IPCMessage newMessage(BaseClass::Message(8));

        RPC::IStringIterator* output_proxy{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output_proxy = reinterpret_cast<RPC::IStringIterator*>(Interface(newMessage->Response(), RPC::IStringIterator::ID));
        }

        return output_proxy;
    }

    uint32_t Configure(PluginHost::IShell* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(9));

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

    void Resolution(const IComposition::ScreenResolution param0) override
    {
        IPCMessage newMessage(BaseClass::Message(10));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<const IComposition::ScreenResolution>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    IComposition::ScreenResolution Resolution() const override
    {
        IPCMessage newMessage(BaseClass::Message(11));

        IComposition::ScreenResolution output = static_cast<IComposition::ScreenResolution>(~0);
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output = Number<IComposition::ScreenResolution>(newMessage->Response());
        }

        return output;
    }
}; // class CompositionProxy

//
// IComposition::IClient interface proxy definitions
//
// Methods:
//  (0) virtual string Name() const = 0
//  (1) virtual void Kill() = 0
//  (2) virtual void Opacity(const uint32_t) = 0
//  (3) virtual void ChangedGeometry(const IComposition::Rectangle&) = 0
//  (4) virtual void ChangedZOrder(const uint8_t) = 0
//

class CompositionClientProxy final : public ProxyStub::UnknownProxyType<IComposition::IClient> {
public:
    CompositionClientProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    string Name() const override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        string output{};
        if (Invoke(newMessage) == Core::ERROR_NONE) {
            // read return value
            output = Text(newMessage->Response());
        }

        return output;
    }

    void Kill() override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    void Opacity(const uint32_t param0) override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<const uint32_t>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    void ChangedGeometry(const IComposition::Rectangle& param0) override
    {
        IPCMessage newMessage(BaseClass::Message(3));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        // (decompose IComposition::Rectangle)
        writer.Number<uint32_t>(param0.x);
        writer.Number<uint32_t>(param0.y);
        writer.Number<uint32_t>(param0.width);
        writer.Number<uint32_t>(param0.height);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    void ChangedZOrder(const uint8_t param0) override
    {
        IPCMessage newMessage(BaseClass::Message(4));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<const uint8_t>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }
}; // class CompositionClientProxy

//
// IComposition::INotification interface proxy definitions
//
// Methods:
//  (0) virtual void Attached(IComposition::IClient*) = 0
//  (1) virtual void Detached(IComposition::IClient*) = 0
//

class CompositionNotificationProxy final : public ProxyStub::UnknownProxyType<IComposition::INotification> {
public:
    CompositionNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    void Attached(IComposition::IClient* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<IComposition::IClient*>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }

    void Detached(IComposition::IClient* param0) override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Number<IComposition::IClient*>(param0);

        Invoke(newMessage);

        Complete(newMessage->Response());
    }
}; // class CompositionNotificationProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::UnknownStubType<IComposition::IClient, CompositionClientStubMethods> CompositionClientStub;
typedef ProxyStub::UnknownStubType<IComposition::INotification, CompositionNotificationStubMethods> CompositionNotificationStub;
typedef ProxyStub::UnknownStubType<IComposition, CompositionStubMethods> CompositionStub;

static class Instantiation {
public:
    Instantiation()
    {
        RPC::Administrator::Instance().Announce<IComposition::IClient, CompositionClientProxy, CompositionClientStub>();
        RPC::Administrator::Instance().Announce<IComposition::INotification, CompositionNotificationProxy, CompositionNotificationStub>();
        RPC::Administrator::Instance().Announce<IComposition, CompositionProxy, CompositionStub>();
    }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


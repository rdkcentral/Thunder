//
// generated automatically from "ITestUtility.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::ITestUtility
//   - class ::WPEFramework::Exchange::ITestUtility::ICommand
//   - class ::WPEFramework::Exchange::ITestUtility::ICommand::IIterator
//

#include "ITestUtility.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// ITestUtility interface stub definitions
//
// Methods:
//  (0) virtual ITestUtility::ICommand::IIterator * Commands() const = 0
//  (1) virtual ITestUtility::ICommand * Command(const string &) const = 0
//

ProxyStub::MethodHandler TestUtilityStubMethods[] = {
    // virtual ITestUtility::ICommand::IIterator * Commands() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility * implementation = input.Implementation<ITestUtility>();
        ASSERT((implementation != nullptr) && "Null ITestUtility implementation pointer (ITestUtility::Commands() stub)");
        ITestUtility::ICommand::IIterator * output = implementation->Commands();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<ITestUtility::ICommand::IIterator *>(output);
    },

    // virtual ITestUtility::ICommand * Command(const string &) const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        const ITestUtility * implementation = input.Implementation<ITestUtility>();
        ASSERT((implementation != nullptr) && "Null ITestUtility implementation pointer (ITestUtility::Command() stub)");
        ITestUtility::ICommand * output = implementation->Command(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<ITestUtility::ICommand *>(output);
    },

    nullptr
}; // TestUtilityStubMethods[]

//
// ITestUtility::ICommand interface stub definitions
//
// Methods:
//  (0) virtual string Execute(const string &) = 0
//  (1) virtual string Description() const = 0
//  (2) virtual string Signature() const = 0
//  (3) virtual string Name() const = 0
//

ProxyStub::MethodHandler TestUtilityCommandStubMethods[] = {
    // virtual string Execute(const string &) = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // read parameters
        RPC::Data::Frame::Reader reader(input.Reader());
        const string param0 = reader.Text();

        // call implementation
        ITestUtility::ICommand * implementation = input.Implementation<ITestUtility::ICommand>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer (ITestUtility::ICommand::Execute() stub)");
        const string output = implementation->Execute(param0);

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Text(output);
    },

    // virtual string Description() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility::ICommand * implementation = input.Implementation<ITestUtility::ICommand>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer (ITestUtility::ICommand::Description() stub)");
        const string output = implementation->Description();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Text(output);
    },

    // virtual string Signature() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility::ICommand * implementation = input.Implementation<ITestUtility::ICommand>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer (ITestUtility::ICommand::Signature() stub)");
        const string output = implementation->Signature();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Text(output);
    },

    // virtual string Name() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility::ICommand * implementation = input.Implementation<ITestUtility::ICommand>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer (ITestUtility::ICommand::Name() stub)");
        const string output = implementation->Name();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Text(output);
    },

    nullptr
}; // TestUtilityCommandStubMethods[]

//
// ITestUtility::ICommand::IIterator interface stub definitions
//
// Methods:
//  (0) virtual void Reset() = 0
//  (1) virtual bool IsValid() const = 0
//  (2) virtual bool Next() = 0
//  (3) virtual ITestUtility::ICommand * Command() const = 0
//

ProxyStub::MethodHandler TestUtilityCommandIteratorStubMethods[] = {
    // virtual void Reset() = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        ITestUtility::ICommand::IIterator * implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer (ITestUtility::ICommand::IIterator::Reset() stub)");
        implementation->Reset();
    },

    // virtual bool IsValid() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility::ICommand::IIterator * implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer (ITestUtility::ICommand::IIterator::IsValid() stub)");
        const bool output = implementation->IsValid();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Boolean(output);
    },

    // virtual bool Next() = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        ITestUtility::ICommand::IIterator * implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer (ITestUtility::ICommand::IIterator::Next() stub)");
        const bool output = implementation->Next();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Boolean(output);
    },

    // virtual ITestUtility::ICommand * Command() const = 0
    //
    [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

        RPC::Data::Input & input(message->Parameters());

        // call implementation
        const ITestUtility::ICommand::IIterator * implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
        ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer (ITestUtility::ICommand::IIterator::Command() stub)");
        ITestUtility::ICommand * output = implementation->Command();

        // write return value
        RPC::Data::Frame::Writer writer(message->Response().Writer());
        writer.Number<ITestUtility::ICommand *>(output);
    },

    nullptr
}; // TestUtilityCommandIteratorStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// ITestUtility interface proxy definitions
//
// Methods:
//  (0) virtual ITestUtility::ICommand::IIterator * Commands() const = 0
//  (1) virtual ITestUtility::ICommand * Command(const string &) const = 0
//

class TestUtilityProxy final : public ProxyStub::UnknownProxyType<ITestUtility> {
public:
    TestUtilityProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    ITestUtility::ICommand::IIterator * Commands() const override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        ITestUtility::ICommand::IIterator * output = reader.Number<ITestUtility::ICommand::IIterator *>();
        ITestUtility::ICommand::IIterator * output_proxy = nullptr;
        if (output != nullptr) {
            output_proxy = const_cast<TestUtilityProxy &>(*this).CreateProxy<ITestUtility::ICommand::IIterator>(output);
            ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestUtility::ICommand::IIterator proxy (TestUtilityProxy::Commands() proxy stub)");
            if (output_proxy == nullptr) {
                TRACE_L1("Failed to get instance of ITestUtility::ICommand::IIterator proxy (ITestUtility::Commands() proxy stub)");
            }
        }

        return output_proxy;
    }

    ITestUtility::ICommand * Command(const string & param0) const override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        ITestUtility::ICommand * output = reader.Number<ITestUtility::ICommand *>();
        ITestUtility::ICommand * output_proxy = nullptr;
        if (output != nullptr) {
            output_proxy = const_cast<TestUtilityProxy &>(*this).CreateProxy<ITestUtility::ICommand>(output);
            ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestUtility::ICommand proxy (TestUtilityProxy::Command() proxy stub)");
            if (output_proxy == nullptr) {
                TRACE_L1("Failed to get instance of ITestUtility::ICommand proxy (ITestUtility::Command() proxy stub)");
            }
        }

        return output_proxy;
    }
}; // class TestUtilityProxy

//
// ITestUtility::ICommand interface proxy definitions
//
// Methods:
//  (0) virtual string Execute(const string &) = 0
//  (1) virtual string Description() const = 0
//  (2) virtual string Signature() const = 0
//  (3) virtual string Name() const = 0
//

class TestUtilityCommandProxy final : public ProxyStub::UnknownProxyType<ITestUtility::ICommand> {
public:
    TestUtilityCommandProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    string Execute(const string & param0) override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        // write parameters
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
        writer.Text(param0);

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        string output = reader.Text();

        return output;
    }

    string Description() const override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        string output = reader.Text();

        return output;
    }

    string Signature() const override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        string output = reader.Text();

        return output;
    }

    string Name() const override
    {
        IPCMessage newMessage(BaseClass::Message(3));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        string output = reader.Text();

        return output;
    }
}; // class TestUtilityCommandProxy

//
// ITestUtility::ICommand::IIterator interface proxy definitions
//
// Methods:
//  (0) virtual void Reset() = 0
//  (1) virtual bool IsValid() const = 0
//  (2) virtual bool Next() = 0
//  (3) virtual ITestUtility::ICommand * Command() const = 0
//

class TestUtilityCommandIteratorProxy final : public ProxyStub::UnknownProxyType<ITestUtility::ICommand::IIterator> {
public:
    TestUtilityCommandIteratorProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }

    void Reset() override
    {
        IPCMessage newMessage(BaseClass::Message(0));

        Invoke(newMessage);
    }

    bool IsValid() const override
    {
        IPCMessage newMessage(BaseClass::Message(1));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        bool output = reader.Boolean();

        return output;
    }

    bool Next() override
    {
        IPCMessage newMessage(BaseClass::Message(2));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        bool output = reader.Boolean();

        return output;
    }

    ITestUtility::ICommand * Command() const override
    {
        IPCMessage newMessage(BaseClass::Message(3));

        Invoke(newMessage);

        // read return value
        RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
        ITestUtility::ICommand * output = reader.Number<ITestUtility::ICommand *>();
        ITestUtility::ICommand * output_proxy = nullptr;
        if (output != nullptr) {
            output_proxy = const_cast<TestUtilityCommandIteratorProxy &>(*this).CreateProxy<ITestUtility::ICommand>(output);
            ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestUtility::ICommand proxy (TestUtilityCommandIteratorProxy::Command() proxy stub)");
            if (output_proxy == nullptr) {
                TRACE_L1("Failed to get instance of ITestUtility::ICommand proxy (ITestUtility::ICommand::IIterator::Command() proxy stub)");
            }
        }

        return output_proxy;
    }
}; // class TestUtilityCommandIteratorProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::StubType<ITestUtility::ICommand, TestUtilityCommandStubMethods, ProxyStub::UnknownStub> TestUtilityCommandStub;
typedef ProxyStub::StubType<ITestUtility, TestUtilityStubMethods, ProxyStub::UnknownStub> TestUtilityStub;
typedef ProxyStub::StubType<ITestUtility::ICommand::IIterator, TestUtilityCommandIteratorStubMethods, ProxyStub::UnknownStub> TestUtilityCommandIteratorStub;

static class Instantiation {
public:
    Instantiation()
    {
        RPC::Administrator::Instance().Announce<ITestUtility::ICommand, TestUtilityCommandProxy, TestUtilityCommandStub>();
        RPC::Administrator::Instance().Announce<ITestUtility, TestUtilityProxy, TestUtilityStub>();
        RPC::Administrator::Instance().Announce<ITestUtility::ICommand::IIterator, TestUtilityCommandIteratorProxy, TestUtilityCommandIteratorStub>();
    }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


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
    //  (0) virtual ITestUtility::ICommand::IIterator* Commands() const = 0
    //  (1) virtual ITestUtility::ICommand* Command(const string&) const = 0
    //

    ProxyStub::MethodHandler TestUtilityStubMethods[] = {
        // virtual ITestUtility::ICommand::IIterator* Commands() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility* implementation = input.Implementation<ITestUtility>();
            ASSERT((implementation != nullptr) && "Null ITestUtility implementation pointer");
            ITestUtility::ICommand::IIterator* output = implementation->Commands();

            // write return value
            writer.Number<ITestUtility::ICommand::IIterator*>(output);
        },

        // virtual ITestUtility::ICommand* Command(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility* implementation = input.Implementation<ITestUtility>();
            ASSERT((implementation != nullptr) && "Null ITestUtility implementation pointer");
            ITestUtility::ICommand* output = implementation->Command(param0);

            // write return value
            writer.Number<ITestUtility::ICommand*>(output);
        },

        nullptr
    }; // TestUtilityStubMethods[]

    //
    // ITestUtility::ICommand interface stub definitions
    //
    // Methods:
    //  (0) virtual string Execute(const string&) = 0
    //  (1) virtual string Description() const = 0
    //  (2) virtual string Signature() const = 0
    //  (3) virtual string Name() const = 0
    //

    ProxyStub::MethodHandler TestUtilityCommandStubMethods[] = {
        // virtual string Execute(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            ITestUtility::ICommand* implementation = input.Implementation<ITestUtility::ICommand>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer");
            const string output = implementation->Execute(param0);

            // write return value
            writer.Text(output);
        },

        // virtual string Description() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility::ICommand* implementation = input.Implementation<ITestUtility::ICommand>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer");
            const string output = implementation->Description();

            // write return value
            writer.Text(output);
        },

        // virtual string Signature() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility::ICommand* implementation = input.Implementation<ITestUtility::ICommand>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer");
            const string output = implementation->Signature();

            // write return value
            writer.Text(output);
        },

        // virtual string Name() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility::ICommand* implementation = input.Implementation<ITestUtility::ICommand>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand implementation pointer");
            const string output = implementation->Name();

            // write return value
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
    //  (3) virtual ITestUtility::ICommand* Command() const = 0
    //

    ProxyStub::MethodHandler TestUtilityCommandIteratorStubMethods[] = {
        // virtual void Reset() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestUtility::ICommand::IIterator* implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer");
            implementation->Reset();
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility::ICommand::IIterator* implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer");
            const bool output = implementation->IsValid();

            // write return value
            writer.Boolean(output);
        },

        // virtual bool Next() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            ITestUtility::ICommand::IIterator* implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer");
            const bool output = implementation->Next();

            // write return value
            writer.Boolean(output);
        },

        // virtual ITestUtility::ICommand* Command() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestUtility::ICommand::IIterator* implementation = input.Implementation<ITestUtility::ICommand::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestUtility::ICommand::IIterator implementation pointer");
            ITestUtility::ICommand* output = implementation->Command();

            // write return value
            writer.Number<ITestUtility::ICommand*>(output);
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
    //  (0) virtual ITestUtility::ICommand::IIterator* Commands() const = 0
    //  (1) virtual ITestUtility::ICommand* Command(const string&) const = 0
    //

    class TestUtilityProxy final : public ProxyStub::UnknownProxyType<ITestUtility> {
    public:
        TestUtilityProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        ITestUtility::ICommand::IIterator* Commands() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            ITestUtility::ICommand::IIterator* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestUtility::ICommand::IIterator*>(Interface(reader.Number<void*>(), ITestUtility::ICommand::IIterator::ID));
            }

            return output_proxy;
        }

        ITestUtility::ICommand* Command(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            ITestUtility::ICommand* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestUtility::ICommand*>(Interface(reader.Number<void*>(), ITestUtility::ICommand::ID));
            }

            return output_proxy;
        }
    }; // class TestUtilityProxy

    //
    // ITestUtility::ICommand interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Execute(const string&) = 0
    //  (1) virtual string Description() const = 0
    //  (2) virtual string Signature() const = 0
    //  (3) virtual string Name() const = 0
    //

    class TestUtilityCommandProxy final : public ProxyStub::UnknownProxyType<ITestUtility::ICommand> {
    public:
        TestUtilityCommandProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Execute(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string Description() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string Signature() const override
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

        string Name() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

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
    //  (3) virtual ITestUtility::ICommand* Command() const = 0
    //

    class TestUtilityCommandIteratorProxy final : public ProxyStub::UnknownProxyType<ITestUtility::ICommand::IIterator> {
    public:
        TestUtilityCommandIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Reset() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            Invoke(newMessage);
        }

        bool IsValid() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        bool Next() override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        ITestUtility::ICommand* Command() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            ITestUtility::ICommand* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestUtility::ICommand*>(Interface(reader.Number<void*>(), ITestUtility::ICommand::ID));
            }

            return output_proxy;
        }
    }; // class TestUtilityCommandIteratorProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<ITestUtility::ICommand, TestUtilityCommandStubMethods> TestUtilityCommandStub;
        typedef ProxyStub::UnknownStubType<ITestUtility, TestUtilityStubMethods> TestUtilityStub;
        typedef ProxyStub::UnknownStubType<ITestUtility::ICommand::IIterator, TestUtilityCommandIteratorStubMethods> TestUtilityCommandIteratorStub;

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

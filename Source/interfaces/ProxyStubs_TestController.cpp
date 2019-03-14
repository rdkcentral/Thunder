//
// generated automatically from "ITestController.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::ITestController
//   - class ::WPEFramework::Exchange::ITestController::ITest
//   - class ::WPEFramework::Exchange::ITestController::ITest::IIterator
//   - class ::WPEFramework::Exchange::ITestController::ICategory
//   - class ::WPEFramework::Exchange::ITestController::ICategory::IIterator
//

#include "ITestController.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // ITestController interface stub definitions
    //
    // Methods:
    //  (0) virtual void Setup() = 0
    //  (1) virtual void TearDown() = 0
    //  (2) virtual ITestController::ICategory::IIterator* Categories() const = 0
    //  (3) virtual ITestController::ICategory* Category(const string&) const = 0
    //

    ProxyStub::MethodHandler TestControllerStubMethods[] = {
        // virtual void Setup() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController* implementation = input.Implementation<ITestController>();
            ASSERT((implementation != nullptr) && "Null ITestController implementation pointer");
            implementation->Setup();
        },

        // virtual void TearDown() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController* implementation = input.Implementation<ITestController>();
            ASSERT((implementation != nullptr) && "Null ITestController implementation pointer");
            implementation->TearDown();
        },

        // virtual ITestController::ICategory::IIterator* Categories() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController* implementation = input.Implementation<ITestController>();
            ASSERT((implementation != nullptr) && "Null ITestController implementation pointer");
            ITestController::ICategory::IIterator* output = implementation->Categories();

            // write return value
            writer.Number<ITestController::ICategory::IIterator*>(output);
        },

        // virtual ITestController::ICategory* Category(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController* implementation = input.Implementation<ITestController>();
            ASSERT((implementation != nullptr) && "Null ITestController implementation pointer");
            ITestController::ICategory* output = implementation->Category(param0);

            // write return value
            writer.Number<ITestController::ICategory*>(output);
        },

        nullptr
    }; // TestControllerStubMethods[]

    //
    // ITestController::ITest interface stub definitions
    //
    // Methods:
    //  (0) virtual string Execute(const string&) = 0
    //  (1) virtual string Description() const = 0
    //  (2) virtual string Name() const = 0
    //

    ProxyStub::MethodHandler TestControllerTestStubMethods[] = {
        // virtual string Execute(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            ITestController::ITest* implementation = input.Implementation<ITestController::ITest>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer");
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
            const ITestController::ITest* implementation = input.Implementation<ITestController::ITest>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer");
            const string output = implementation->Description();

            // write return value
            writer.Text(output);
        },

        // virtual string Name() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ITest* implementation = input.Implementation<ITestController::ITest>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer");
            const string output = implementation->Name();

            // write return value
            writer.Text(output);
        },

        nullptr
    }; // TestControllerTestStubMethods[]

    //
    // ITestController::ITest::IIterator interface stub definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool IsValid() const = 0
    //  (2) virtual bool Next() = 0
    //  (3) virtual ITestController::ITest* Test() const = 0
    //

    ProxyStub::MethodHandler TestControllerTestIteratorStubMethods[] = {
        // virtual void Reset() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController::ITest::IIterator* implementation = input.Implementation<ITestController::ITest::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer");
            implementation->Reset();
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ITest::IIterator* implementation = input.Implementation<ITestController::ITest::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer");
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
            ITestController::ITest::IIterator* implementation = input.Implementation<ITestController::ITest::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer");
            const bool output = implementation->Next();

            // write return value
            writer.Boolean(output);
        },

        // virtual ITestController::ITest* Test() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ITest::IIterator* implementation = input.Implementation<ITestController::ITest::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer");
            ITestController::ITest* output = implementation->Test();

            // write return value
            writer.Number<ITestController::ITest*>(output);
        },

        nullptr
    }; // TestControllerTestIteratorStubMethods[]

    //
    // ITestController::ICategory interface stub definitions
    //
    // Methods:
    //  (0) virtual string Name() const = 0
    //  (1) virtual void Setup() = 0
    //  (2) virtual void TearDown() = 0
    //  (3) virtual void Register(ITestController::ITest*) = 0
    //  (4) virtual void Unregister(ITestController::ITest*) = 0
    //  (5) virtual ITestController::ITest::IIterator* Tests() const = 0
    //  (6) virtual ITestController::ITest* Test(const string&) const = 0
    //

    ProxyStub::MethodHandler TestControllerCategoryStubMethods[] = {
        // virtual string Name() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
            const string output = implementation->Name();

            // write return value
            writer.Text(output);
        },

        // virtual void Setup() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
            implementation->Setup();
        },

        // virtual void TearDown() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
            implementation->TearDown();
        },

        // virtual void Register(ITestController::ITest*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            ITestController::ITest* param0 = reader.Number<ITestController::ITest*>();
            ITestController::ITest* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, ITestController::ITest::ID, false, ITestController::ITest::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<ITestController::ITest>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ITestController::ITest proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
                ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(ITestController::ITest*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            ITestController::ITest* param0 = reader.Number<ITestController::ITest*>();
            ITestController::ITest* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, ITestController::ITest::ID, false, ITestController::ITest::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<ITestController::ITest>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ITestController::ITest proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
                ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual ITestController::ITest::IIterator* Tests() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
            ITestController::ITest::IIterator* output = implementation->Tests();

            // write return value
            writer.Number<ITestController::ITest::IIterator*>(output);
        },

        // virtual ITestController::ITest* Test(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ICategory* implementation = input.Implementation<ITestController::ICategory>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer");
            ITestController::ITest* output = implementation->Test(param0);

            // write return value
            writer.Number<ITestController::ITest*>(output);
        },

        nullptr
    }; // TestControllerCategoryStubMethods[]

    //
    // ITestController::ICategory::IIterator interface stub definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool IsValid() const = 0
    //  (2) virtual bool Next() = 0
    //  (3) virtual ITestController::ICategory* Category() const = 0
    //

    ProxyStub::MethodHandler TestControllerCategoryIteratorStubMethods[] = {
        // virtual void Reset() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITestController::ICategory::IIterator* implementation = input.Implementation<ITestController::ICategory::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer");
            implementation->Reset();
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ICategory::IIterator* implementation = input.Implementation<ITestController::ICategory::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer");
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
            ITestController::ICategory::IIterator* implementation = input.Implementation<ITestController::ICategory::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer");
            const bool output = implementation->Next();

            // write return value
            writer.Boolean(output);
        },

        // virtual ITestController::ICategory* Category() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const ITestController::ICategory::IIterator* implementation = input.Implementation<ITestController::ICategory::IIterator>();
            ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer");
            ITestController::ICategory* output = implementation->Category();

            // write return value
            writer.Number<ITestController::ICategory*>(output);
        },

        nullptr
    }; // TestControllerCategoryIteratorStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // ITestController interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Setup() = 0
    //  (1) virtual void TearDown() = 0
    //  (2) virtual ITestController::ICategory::IIterator* Categories() const = 0
    //  (3) virtual ITestController::ICategory* Category(const string&) const = 0
    //

    class TestControllerProxy final : public ProxyStub::UnknownProxyType<ITestController> {
    public:
        TestControllerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Setup() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            Invoke(newMessage);
        }

        void TearDown() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            Invoke(newMessage);
        }

        ITestController::ICategory::IIterator* Categories() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            ITestController::ICategory::IIterator* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ICategory::IIterator*>(Interface(reader.Number<void*>(), ITestController::ICategory::IIterator::ID));
            }

            return output_proxy;
        }

        ITestController::ICategory* Category(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            ITestController::ICategory* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ICategory*>(Interface(reader.Number<void*>(), ITestController::ICategory::ID));
            }

            return output_proxy;
        }
    }; // class TestControllerProxy

    //
    // ITestController::ITest interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Execute(const string&) = 0
    //  (1) virtual string Description() const = 0
    //  (2) virtual string Name() const = 0
    //

    class TestControllerTestProxy final : public ProxyStub::UnknownProxyType<ITestController::ITest> {
    public:
        TestControllerTestProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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

        string Name() const override
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
    }; // class TestControllerTestProxy

    //
    // ITestController::ITest::IIterator interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool IsValid() const = 0
    //  (2) virtual bool Next() = 0
    //  (3) virtual ITestController::ITest* Test() const = 0
    //

    class TestControllerTestIteratorProxy final : public ProxyStub::UnknownProxyType<ITestController::ITest::IIterator> {
    public:
        TestControllerTestIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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

        ITestController::ITest* Test() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            ITestController::ITest* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ITest*>(Interface(reader.Number<void*>(), ITestController::ITest::ID));
            }

            return output_proxy;
        }
    }; // class TestControllerTestIteratorProxy

    //
    // ITestController::ICategory interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Name() const = 0
    //  (1) virtual void Setup() = 0
    //  (2) virtual void TearDown() = 0
    //  (3) virtual void Register(ITestController::ITest*) = 0
    //  (4) virtual void Unregister(ITestController::ITest*) = 0
    //  (5) virtual ITestController::ITest::IIterator* Tests() const = 0
    //  (6) virtual ITestController::ITest* Test(const string&) const = 0
    //

    class TestControllerCategoryProxy final : public ProxyStub::UnknownProxyType<ITestController::ICategory> {
    public:
        TestControllerCategoryProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Name() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        void Setup() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            Invoke(newMessage);
        }

        void TearDown() override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            Invoke(newMessage);
        }

        void Register(ITestController::ITest* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ITestController::ITest*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(ITestController::ITest* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ITestController::ITest*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        ITestController::ITest::IIterator* Tests() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            ITestController::ITest::IIterator* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ITest::IIterator*>(Interface(reader.Number<void*>(), ITestController::ITest::IIterator::ID));
            }

            return output_proxy;
        }

        ITestController::ITest* Test(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            ITestController::ITest* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ITest*>(Interface(reader.Number<void*>(), ITestController::ITest::ID));
            }

            return output_proxy;
        }
    }; // class TestControllerCategoryProxy

    //
    // ITestController::ICategory::IIterator interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool IsValid() const = 0
    //  (2) virtual bool Next() = 0
    //  (3) virtual ITestController::ICategory* Category() const = 0
    //

    class TestControllerCategoryIteratorProxy final : public ProxyStub::UnknownProxyType<ITestController::ICategory::IIterator> {
    public:
        TestControllerCategoryIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
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

        ITestController::ICategory* Category() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            ITestController::ICategory* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ITestController::ICategory*>(Interface(reader.Number<void*>(), ITestController::ICategory::ID));
            }

            return output_proxy;
        }
    }; // class TestControllerCategoryIteratorProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<ITestController::ITest::IIterator, TestControllerTestIteratorStubMethods> TestControllerTestIteratorStub;
        typedef ProxyStub::UnknownStubType<ITestController::ICategory, TestControllerCategoryStubMethods> TestControllerCategoryStub;
        typedef ProxyStub::UnknownStubType<ITestController::ICategory::IIterator, TestControllerCategoryIteratorStubMethods> TestControllerCategoryIteratorStub;
        typedef ProxyStub::UnknownStubType<ITestController::ITest, TestControllerTestStubMethods> TestControllerTestStub;
        typedef ProxyStub::UnknownStubType<ITestController, TestControllerStubMethods> TestControllerStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<ITestController::ITest::IIterator, TestControllerTestIteratorProxy, TestControllerTestIteratorStub>();
                RPC::Administrator::Instance().Announce<ITestController::ICategory, TestControllerCategoryProxy, TestControllerCategoryStub>();
                RPC::Administrator::Instance().Announce<ITestController::ICategory::IIterator, TestControllerCategoryIteratorProxy, TestControllerCategoryIteratorStub>();
                RPC::Administrator::Instance().Announce<ITestController::ITest, TestControllerTestProxy, TestControllerTestStub>();
                RPC::Administrator::Instance().Announce<ITestController, TestControllerProxy, TestControllerStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace WPEFramework

} // namespace ProxyStubs

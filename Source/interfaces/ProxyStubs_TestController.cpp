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
//  (2) virtual ITestController::ICategory::IIterator * Categories() const = 0
//  (3) virtual ITestController::ICategory * Category(const string &) const = 0
//

ProxyStub::MethodHandler TestControllerStubMethods[] = {
  // virtual void Setup() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController * implementation = input.Implementation<ITestController>();
    ASSERT((implementation != nullptr) && "Null ITestController implementation pointer (ITestController::Setup() stub)");
    implementation->Setup();
  },

  // virtual void TearDown() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController * implementation = input.Implementation<ITestController>();
    ASSERT((implementation != nullptr) && "Null ITestController implementation pointer (ITestController::TearDown() stub)");
    implementation->TearDown();
  },

  // virtual ITestController::ICategory::IIterator * Categories() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController * implementation = input.Implementation<ITestController>();
    ASSERT((implementation != nullptr) && "Null ITestController implementation pointer (ITestController::Categories() stub)");
    ITestController::ICategory::IIterator * output = implementation->Categories();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ICategory::IIterator *>(output);
  },

  // virtual ITestController::ICategory * Category(const string &) const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    const string param0 = reader.Text();

    // call implementation
    const ITestController * implementation = input.Implementation<ITestController>();
    ASSERT((implementation != nullptr) && "Null ITestController implementation pointer (ITestController::Category() stub)");
    ITestController::ICategory * output = implementation->Category(param0);

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ICategory *>(output);
  },

  nullptr
}; // TestControllerStubMethods[]

//
// ITestController::ITest interface stub definitions
//
// Methods:
//  (0) virtual string Execute(const string &) = 0
//  (1) virtual string Description() const = 0
//  (2) virtual string Name() const = 0
//

ProxyStub::MethodHandler TestControllerTestStubMethods[] = {
  // virtual string Execute(const string &) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    const string param0 = reader.Text();

    // call implementation
    ITestController::ITest * implementation = input.Implementation<ITestController::ITest>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer (ITestController::ITest::Execute() stub)");
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
    const ITestController::ITest * implementation = input.Implementation<ITestController::ITest>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer (ITestController::ITest::Description() stub)");
    const string output = implementation->Description();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Text(output);
  },

  // virtual string Name() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ITest * implementation = input.Implementation<ITestController::ITest>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest implementation pointer (ITestController::ITest::Name() stub)");
    const string output = implementation->Name();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
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
//  (3) virtual ITestController::ITest * Test() const = 0
//

ProxyStub::MethodHandler TestControllerTestIteratorStubMethods[] = {
  // virtual void Reset() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController::ITest::IIterator * implementation = input.Implementation<ITestController::ITest::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer (ITestController::ITest::IIterator::Reset() stub)");
    implementation->Reset();
  },

  // virtual bool IsValid() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ITest::IIterator * implementation = input.Implementation<ITestController::ITest::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer (ITestController::ITest::IIterator::IsValid() stub)");
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
    ITestController::ITest::IIterator * implementation = input.Implementation<ITestController::ITest::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer (ITestController::ITest::IIterator::Next() stub)");
    const bool output = implementation->Next();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Boolean(output);
  },

  // virtual ITestController::ITest * Test() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ITest::IIterator * implementation = input.Implementation<ITestController::ITest::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ITest::IIterator implementation pointer (ITestController::ITest::IIterator::Test() stub)");
    ITestController::ITest * output = implementation->Test();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ITest *>(output);
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
//  (3) virtual void Register(ITestController::ITest *) = 0
//  (4) virtual void Unregister(ITestController::ITest *) = 0
//  (5) virtual ITestController::ITest::IIterator * Tests() const = 0
//  (6) virtual ITestController::ITest * Test(const string &) const = 0
//

ProxyStub::MethodHandler TestControllerCategoryStubMethods[] = {
  // virtual string Name() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Name() stub)");
    const string output = implementation->Name();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Text(output);
  },

  // virtual void Setup() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Setup() stub)");
    implementation->Setup();
  },

  // virtual void TearDown() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::TearDown() stub)");
    implementation->TearDown();
  },

  // virtual void Register(ITestController::ITest *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    bool proxy_failed = false;
    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    ITestController::ITest * param0 = reader.Number<ITestController::ITest *>();
    ITestController::ITest * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().CreateProxy<ITestController::ITest>(channel, param0, true, false);
      ASSERT((param0_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy (ITestController::ICategory::Register() stub)");
      if (param0_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ITest proxy (ITestController::ICategory::Register() stub)");
        proxy_failed = true;
      }
    }

    if (!proxy_failed) {
      // call implementation
      ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
      ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Register() stub)");
      implementation->Register(param0_proxy);
    }

    if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("ITestController::ITest::Release() failed (ITestController::ICategory::Register() stub)");
    }
  },

  // virtual void Unregister(ITestController::ITest *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    bool proxy_failed = false;
    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    ITestController::ITest * param0 = reader.Number<ITestController::ITest *>();
    ITestController::ITest * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().CreateProxy<ITestController::ITest>(channel, param0, true, false);
      ASSERT((param0_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy (ITestController::ICategory::Unregister() stub)");
      if (param0_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ITest proxy (ITestController::ICategory::Unregister() stub)");
        proxy_failed = true;
      }
    }

    if (!proxy_failed) {
      // call implementation
      ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
      ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Unregister() stub)");
      implementation->Unregister(param0_proxy);
    }

    if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("ITestController::ITest::Release() failed (ITestController::ICategory::Unregister() stub)");
    }
  },

  // virtual ITestController::ITest::IIterator * Tests() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Tests() stub)");
    ITestController::ITest::IIterator * output = implementation->Tests();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ITest::IIterator *>(output);
  },

  // virtual ITestController::ITest * Test(const string &) const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    const string param0 = reader.Text();

    // call implementation
    const ITestController::ICategory * implementation = input.Implementation<ITestController::ICategory>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory implementation pointer (ITestController::ICategory::Test() stub)");
    ITestController::ITest * output = implementation->Test(param0);

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ITest *>(output);
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
//  (3) virtual ITestController::ICategory * Category() const = 0
//

ProxyStub::MethodHandler TestControllerCategoryIteratorStubMethods[] = {
  // virtual void Reset() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    ITestController::ICategory::IIterator * implementation = input.Implementation<ITestController::ICategory::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer (ITestController::ICategory::IIterator::Reset() stub)");
    implementation->Reset();
  },

  // virtual bool IsValid() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ICategory::IIterator * implementation = input.Implementation<ITestController::ICategory::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer (ITestController::ICategory::IIterator::IsValid() stub)");
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
    ITestController::ICategory::IIterator * implementation = input.Implementation<ITestController::ICategory::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer (ITestController::ICategory::IIterator::Next() stub)");
    const bool output = implementation->Next();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Boolean(output);
  },

  // virtual ITestController::ICategory * Category() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const ITestController::ICategory::IIterator * implementation = input.Implementation<ITestController::ICategory::IIterator>();
    ASSERT((implementation != nullptr) && "Null ITestController::ICategory::IIterator implementation pointer (ITestController::ICategory::IIterator::Category() stub)");
    ITestController::ICategory * output = implementation->Category();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<ITestController::ICategory *>(output);
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
//  (2) virtual ITestController::ICategory::IIterator * Categories() const = 0
//  (3) virtual ITestController::ICategory * Category(const string &) const = 0
//

class TestControllerProxy final : public ProxyStub::UnknownProxyType<ITestController> {
public:
  TestControllerProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
      : BaseClass(channel, implementation, otherSideInformed)
  {
  }

  void Setup() override
  {
    IPCMessage newMessage(BaseClass::Message(0));

    Invoke(newMessage);
  }

  void TearDown() override
  {
    IPCMessage newMessage(BaseClass::Message(1));

    Invoke(newMessage);
  }

  ITestController::ICategory::IIterator * Categories() const override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ICategory::IIterator * output = reader.Number<ITestController::ICategory::IIterator *>();
    ITestController::ICategory::IIterator * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerProxy &>(*this).CreateProxy<ITestController::ICategory::IIterator>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ICategory::IIterator proxy (TestControllerProxy::Categories() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ICategory::IIterator proxy (ITestController::Categories() proxy stub)");
      }
    }

    return output_proxy;
  }

  ITestController::ICategory * Category(const string & param0) const override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Text(param0);

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ICategory * output = reader.Number<ITestController::ICategory *>();
    ITestController::ICategory * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerProxy &>(*this).CreateProxy<ITestController::ICategory>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ICategory proxy (TestControllerProxy::Category() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ICategory proxy (ITestController::Category() proxy stub)");
      }
    }

    return output_proxy;
  }
}; // class TestControllerProxy

//
// ITestController::ITest interface proxy definitions
//
// Methods:
//  (0) virtual string Execute(const string &) = 0
//  (1) virtual string Description() const = 0
//  (2) virtual string Name() const = 0
//

class TestControllerTestProxy final : public ProxyStub::UnknownProxyType<ITestController::ITest> {
public:
  TestControllerTestProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
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

  string Name() const override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    string output = reader.Text();

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
//  (3) virtual ITestController::ITest * Test() const = 0
//

class TestControllerTestIteratorProxy final : public ProxyStub::UnknownProxyType<ITestController::ITest::IIterator> {
public:
  TestControllerTestIteratorProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
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

  ITestController::ITest * Test() const override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ITest * output = reader.Number<ITestController::ITest *>();
    ITestController::ITest * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerTestIteratorProxy &>(*this).CreateProxy<ITestController::ITest>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy (TestControllerTestIteratorProxy::Test() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ITest proxy (ITestController::ITest::IIterator::Test() proxy stub)");
      }
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
//  (3) virtual void Register(ITestController::ITest *) = 0
//  (4) virtual void Unregister(ITestController::ITest *) = 0
//  (5) virtual ITestController::ITest::IIterator * Tests() const = 0
//  (6) virtual ITestController::ITest * Test(const string &) const = 0
//

class TestControllerCategoryProxy final : public ProxyStub::UnknownProxyType<ITestController::ICategory> {
public:
  TestControllerCategoryProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
      : BaseClass(channel, implementation, otherSideInformed)
  {
  }

  string Name() const override
  {
    IPCMessage newMessage(BaseClass::Message(0));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    string output = reader.Text();

    return output;
  }

  void Setup() override
  {
    IPCMessage newMessage(BaseClass::Message(1));

    Invoke(newMessage);
  }

  void TearDown() override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    Invoke(newMessage);
  }

  void Register(ITestController::ITest * param0) override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<ITestController::ITest *>(param0);

    Invoke(newMessage);
  }

  void Unregister(ITestController::ITest * param0) override
  {
    IPCMessage newMessage(BaseClass::Message(4));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<ITestController::ITest *>(param0);

    Invoke(newMessage);
  }

  ITestController::ITest::IIterator * Tests() const override
  {
    IPCMessage newMessage(BaseClass::Message(5));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ITest::IIterator * output = reader.Number<ITestController::ITest::IIterator *>();
    ITestController::ITest::IIterator * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerCategoryProxy &>(*this).CreateProxy<ITestController::ITest::IIterator>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ITest::IIterator proxy (TestControllerCategoryProxy::Tests() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ITest::IIterator proxy (ITestController::ICategory::Tests() proxy stub)");
      }
    }

    return output_proxy;
  }

  ITestController::ITest * Test(const string & param0) const override
  {
    IPCMessage newMessage(BaseClass::Message(6));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Text(param0);

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ITest * output = reader.Number<ITestController::ITest *>();
    ITestController::ITest * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerCategoryProxy &>(*this).CreateProxy<ITestController::ITest>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ITest proxy (TestControllerCategoryProxy::Test() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ITest proxy (ITestController::ICategory::Test() proxy stub)");
      }
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
//  (3) virtual ITestController::ICategory * Category() const = 0
//

class TestControllerCategoryIteratorProxy final : public ProxyStub::UnknownProxyType<ITestController::ICategory::IIterator> {
public:
  TestControllerCategoryIteratorProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
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

  ITestController::ICategory * Category() const override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    ITestController::ICategory * output = reader.Number<ITestController::ICategory *>();
    ITestController::ICategory * output_proxy = nullptr;
    if (output != nullptr) {
      output_proxy = const_cast<TestControllerCategoryIteratorProxy &>(*this).CreateProxy<ITestController::ICategory>(output);
      ASSERT((output_proxy != nullptr) && "Failed to get instance of ITestController::ICategory proxy (TestControllerCategoryIteratorProxy::Category() proxy stub)");
      if (output_proxy == nullptr) {
        TRACE_L1("Failed to get instance of ITestController::ICategory proxy (ITestController::ICategory::IIterator::Category() proxy stub)");
      }
    }

    return output_proxy;
  }
}; // class TestControllerCategoryIteratorProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::StubType<ITestController::ITest::IIterator, TestControllerTestIteratorStubMethods, ProxyStub::UnknownStub> TestControllerTestIteratorStub;
typedef ProxyStub::StubType<ITestController::ICategory, TestControllerCategoryStubMethods, ProxyStub::UnknownStub> TestControllerCategoryStub;
typedef ProxyStub::StubType<ITestController::ICategory::IIterator, TestControllerCategoryIteratorStubMethods, ProxyStub::UnknownStub> TestControllerCategoryIteratorStub;
typedef ProxyStub::StubType<ITestController::ITest, TestControllerTestStubMethods, ProxyStub::UnknownStub> TestControllerTestStub;
typedef ProxyStub::StubType<ITestController, TestControllerStubMethods, ProxyStub::UnknownStub> TestControllerStub;

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


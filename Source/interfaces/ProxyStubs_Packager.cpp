//
// generated automatically from "IPackager.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IPackager
//   - class ::WPEFramework::Exchange::IPackager::IInstallationInfo
//   - class ::WPEFramework::Exchange::IPackager::IPackageInfo
//   - class ::WPEFramework::Exchange::IPackager::INotification
//

#include "IPackager.h"

namespace WPEFramework {

namespace ProxyStubs {

using namespace Exchange;

// -----------------------------------------------------------------
// STUB
// -----------------------------------------------------------------

//
// IPackager interface stub definitions
//
// Methods:
//  (0) virtual void Register(IPackager::INotification *) = 0
//  (1) virtual void Unregister(const IPackager::INotification *) = 0
//  (2) virtual uint32_t Configure(PluginHost::IShell *) = 0
//  (3) virtual uint32_t Install(const string &, const string &, const string &) = 0
//

ProxyStub::MethodHandler PackagerStubMethods[] = {
  // virtual void Register(IPackager::INotification *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    bool proxy_failed = false;
    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    IPackager::INotification * param0 = reader.Number<IPackager::INotification *>();
    IPackager::INotification * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().CreateProxy<IPackager::INotification>(channel, param0, true, false);
      ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPackager::INotification proxy (IPackager::Register() stub)");
      if (param0_proxy == nullptr) {
        TRACE_L1("Failed to get instance of IPackager::INotification proxy (IPackager::Register() stub)");
        proxy_failed = true;
      }
    }

    if (!proxy_failed) {
      // call implementation
      IPackager * implementation = input.Implementation<IPackager>();
      ASSERT((implementation != nullptr) && "Null IPackager implementation pointer (IPackager::Register() stub)");
      implementation->Register(param0_proxy);
    }

    if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("IPackager::INotification::Release() failed (IPackager::Register() stub)");
    }
  },

  // virtual void Unregister(const IPackager::INotification *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    const IPackager::INotification * param0 = reader.Number<IPackager::INotification *>();
    IPackager::INotification * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().FindProxy<IPackager::INotification>(channel.operator->(), const_cast<IPackager::INotification *>(param0));
    }

    // call implementation
    IPackager * implementation = input.Implementation<IPackager>();
    ASSERT((implementation != nullptr) && "Null IPackager implementation pointer (IPackager::Unregister() stub)");
    implementation->Unregister(param0_proxy);
  },

  // virtual uint32_t Configure(PluginHost::IShell *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    bool proxy_failed = false;
    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    PluginHost::IShell * param0 = reader.Number<PluginHost::IShell *>();
    PluginHost::IShell * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().CreateProxy<PluginHost::IShell>(channel, param0, true, false);
      ASSERT((param0_proxy != nullptr) && "Failed to get instance of PluginHost::IShell proxy (IPackager::Configure() stub)");
      if (param0_proxy == nullptr) {
        TRACE_L1("Failed to get instance of PluginHost::IShell proxy (IPackager::Configure() stub)");
        message->Response().Writer().Number<uint32_t>(Core::ERROR_RPC_CALL_FAILED);
        proxy_failed = true;
      }
    }

    if (!proxy_failed) {
      // call implementation
      IPackager * implementation = input.Implementation<IPackager>();
      ASSERT((implementation != nullptr) && "Null IPackager implementation pointer (IPackager::Configure() stub)");
      const uint32_t output = implementation->Configure(param0_proxy);

      // write return value
      RPC::Data::Frame::Writer writer(message->Response().Writer());
      writer.Number<const uint32_t>(output);
    }

    if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("PluginHost::IShell::Release() failed (IPackager::Configure() stub)");
    }
  },

  // virtual uint32_t Install(const string &, const string &, const string &) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    const string param0 = reader.Text();
    const string param1 = reader.Text();
    const string param2 = reader.Text();

    // call implementation
    IPackager * implementation = input.Implementation<IPackager>();
    ASSERT((implementation != nullptr) && "Null IPackager implementation pointer (IPackager::Install() stub)");
    const uint32_t output = implementation->Install(param0, param1, param2);

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<const uint32_t>(output);
  },

  nullptr
}; // PackagerStubMethods[]

//
// IPackager::IInstallationInfo interface stub definitions
//
// Methods:
//  (0) virtual IPackager::state State() const = 0
//  (1) virtual uint8_t Progress() const = 0
//  (2) virtual uint32_t ErrorCode() const = 0
//  (3) virtual uint32_t Abort() = 0
//

ProxyStub::MethodHandler PackagerInstallationInfoStubMethods[] = {
  // virtual IPackager::state State() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IInstallationInfo * implementation = input.Implementation<IPackager::IInstallationInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer (IPackager::IInstallationInfo::State() stub)");
    const IPackager::state output = implementation->State();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<const IPackager::state>(output);
  },

  // virtual uint8_t Progress() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IInstallationInfo * implementation = input.Implementation<IPackager::IInstallationInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer (IPackager::IInstallationInfo::Progress() stub)");
    const uint8_t output = implementation->Progress();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<const uint8_t>(output);
  },

  // virtual uint32_t ErrorCode() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IInstallationInfo * implementation = input.Implementation<IPackager::IInstallationInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer (IPackager::IInstallationInfo::ErrorCode() stub)");
    const uint32_t output = implementation->ErrorCode();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<const uint32_t>(output);
  },

  // virtual uint32_t Abort() = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    IPackager::IInstallationInfo * implementation = input.Implementation<IPackager::IInstallationInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IInstallationInfo implementation pointer (IPackager::IInstallationInfo::Abort() stub)");
    const uint32_t output = implementation->Abort();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Number<const uint32_t>(output);
  },

  nullptr
}; // PackagerInstallationInfoStubMethods[]

//
// IPackager::IPackageInfo interface stub definitions
//
// Methods:
//  (0) virtual string Name() const = 0
//  (1) virtual string Version() const = 0
//  (2) virtual string Architecture() const = 0
//

ProxyStub::MethodHandler PackagerPackageInfoStubMethods[] = {
  // virtual string Name() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IPackageInfo * implementation = input.Implementation<IPackager::IPackageInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer (IPackager::IPackageInfo::Name() stub)");
    const string output = implementation->Name();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Text(output);
  },

  // virtual string Version() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IPackageInfo * implementation = input.Implementation<IPackager::IPackageInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer (IPackager::IPackageInfo::Version() stub)");
    const string output = implementation->Version();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Text(output);
  },

  // virtual string Architecture() const = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage> & message) {

    RPC::Data::Input & input(message->Parameters());

    // call implementation
    const IPackager::IPackageInfo * implementation = input.Implementation<IPackager::IPackageInfo>();
    ASSERT((implementation != nullptr) && "Null IPackager::IPackageInfo implementation pointer (IPackager::IPackageInfo::Architecture() stub)");
    const string output = implementation->Architecture();

    // write return value
    RPC::Data::Frame::Writer writer(message->Response().Writer());
    writer.Text(output);
  },

  nullptr
}; // PackagerPackageInfoStubMethods[]

//
// IPackager::INotification interface stub definitions
//
// Methods:
//  (0) virtual void StateChange(IPackager::IPackageInfo *, IPackager::IInstallationInfo *) = 0
//

ProxyStub::MethodHandler PackagerNotificationStubMethods[] = {
  // virtual void StateChange(IPackager::IPackageInfo *, IPackager::IInstallationInfo *) = 0
  //
  [](Core::ProxyType<Core::IPCChannel> & channel, Core::ProxyType<RPC::InvokeMessage> & message) {

    bool proxy_failed = false;
    RPC::Data::Input & input(message->Parameters());

    // read parameters
    RPC::Data::Frame::Reader reader(input.Reader());
    IPackager::IPackageInfo * param0 = reader.Number<IPackager::IPackageInfo *>();
    IPackager::IInstallationInfo * param1 = reader.Number<IPackager::IInstallationInfo *>();
    IPackager::IPackageInfo * param0_proxy = nullptr;
    if (param0 != nullptr) {
      param0_proxy = RPC::Administrator::Instance().CreateProxy<IPackager::IPackageInfo>(channel, param0, true, false);
      ASSERT((param0_proxy != nullptr) && "Failed to get instance of IPackager::IPackageInfo proxy (IPackager::INotification::StateChange() stub)");
      if (param0_proxy == nullptr) {
        TRACE_L1("Failed to get instance of IPackager::IPackageInfo proxy (IPackager::INotification::StateChange() stub)");
        proxy_failed = true;
      }
    }
    IPackager::IInstallationInfo * param1_proxy = nullptr;
    if (param1 != nullptr) {
      param1_proxy = RPC::Administrator::Instance().CreateProxy<IPackager::IInstallationInfo>(channel, param1, true, false);
      ASSERT((param1_proxy != nullptr) && "Failed to get instance of IPackager::IInstallationInfo proxy (IPackager::INotification::StateChange() stub)");
      if (param1_proxy == nullptr) {
        TRACE_L1("Failed to get instance of IPackager::IInstallationInfo proxy (IPackager::INotification::StateChange() stub)");
        proxy_failed = true;
      }
    }

    if (!proxy_failed) {
      // call implementation
      IPackager::INotification * implementation = input.Implementation<IPackager::INotification>();
      ASSERT((implementation != nullptr) && "Null IPackager::INotification implementation pointer (IPackager::INotification::StateChange() stub)");
      implementation->StateChange(param0_proxy, param1_proxy);
    }

    if ((param0_proxy != nullptr) && (param0_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("IPackager::IPackageInfo::Release() failed (IPackager::INotification::StateChange() stub)");
    }
    if ((param1_proxy != nullptr) && (param1_proxy->Release() != Core::ERROR_NONE)) {
      TRACE_L1("IPackager::IInstallationInfo::Release() failed (IPackager::INotification::StateChange() stub)");
    }
  },

  nullptr
}; // PackagerNotificationStubMethods[]


// -----------------------------------------------------------------
// PROXY
// -----------------------------------------------------------------

//
// IPackager interface proxy definitions
//
// Methods:
//  (0) virtual void Register(IPackager::INotification *) = 0
//  (1) virtual void Unregister(const IPackager::INotification *) = 0
//  (2) virtual uint32_t Configure(PluginHost::IShell *) = 0
//  (3) virtual uint32_t Install(const string &, const string &, const string &) = 0
//

class PackagerProxy final : public ProxyStub::UnknownProxyType<IPackager> {
public:
  PackagerProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
      : BaseClass(channel, implementation, otherSideInformed)
  {
  }

  void Register(IPackager::INotification * param0) override
  {
    IPCMessage newMessage(BaseClass::Message(0));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<IPackager::INotification *>(param0);

    Invoke(newMessage);
  }

  void Unregister(const IPackager::INotification * param0) override
  {
    IPCMessage newMessage(BaseClass::Message(1));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<const IPackager::INotification *>(param0);

    Invoke(newMessage);
  }

  uint32_t Configure(PluginHost::IShell * param0) override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<PluginHost::IShell *>(param0);

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    uint32_t output = reader.Number<uint32_t>();

    return output;
  }

  uint32_t Install(const string & param0, const string & param1, const string & param2) override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Text(param0);
    writer.Text(param1);
    writer.Text(param2);

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    uint32_t output = reader.Number<uint32_t>();

    return output;
  }
}; // class PackagerProxy

//
// IPackager::IInstallationInfo interface proxy definitions
//
// Methods:
//  (0) virtual IPackager::state State() const = 0
//  (1) virtual uint8_t Progress() const = 0
//  (2) virtual uint32_t ErrorCode() const = 0
//  (3) virtual uint32_t Abort() = 0
//

class PackagerInstallationInfoProxy final : public ProxyStub::UnknownProxyType<IPackager::IInstallationInfo> {
public:
  PackagerInstallationInfoProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
      : BaseClass(channel, implementation, otherSideInformed)
  {
  }

  IPackager::state State() const override
  {
    IPCMessage newMessage(BaseClass::Message(0));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    IPackager::state output = reader.Number<IPackager::state>();

    return output;
  }

  uint8_t Progress() const override
  {
    IPCMessage newMessage(BaseClass::Message(1));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    uint8_t output = reader.Number<uint8_t>();

    return output;
  }

  uint32_t ErrorCode() const override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    uint32_t output = reader.Number<uint32_t>();

    return output;
  }

  uint32_t Abort() override
  {
    IPCMessage newMessage(BaseClass::Message(3));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    uint32_t output = reader.Number<uint32_t>();

    return output;
  }
}; // class PackagerInstallationInfoProxy

//
// IPackager::IPackageInfo interface proxy definitions
//
// Methods:
//  (0) virtual string Name() const = 0
//  (1) virtual string Version() const = 0
//  (2) virtual string Architecture() const = 0
//

class PackagerPackageInfoProxy final : public ProxyStub::UnknownProxyType<IPackager::IPackageInfo> {
public:
  PackagerPackageInfoProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
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

  string Version() const override
  {
    IPCMessage newMessage(BaseClass::Message(1));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    string output = reader.Text();

    return output;
  }

  string Architecture() const override
  {
    IPCMessage newMessage(BaseClass::Message(2));

    Invoke(newMessage);

    // read return value
    RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
    string output = reader.Text();

    return output;
  }
}; // class PackagerPackageInfoProxy

//
// IPackager::INotification interface proxy definitions
//
// Methods:
//  (0) virtual void StateChange(IPackager::IPackageInfo *, IPackager::IInstallationInfo *) = 0
//

class PackagerNotificationProxy final : public ProxyStub::UnknownProxyType<IPackager::INotification> {
public:
  PackagerNotificationProxy(Core::ProxyType<Core::IPCChannel> & channel, void * implementation, const bool otherSideInformed)
      : BaseClass(channel, implementation, otherSideInformed)
  {
  }

  void StateChange(IPackager::IPackageInfo * param0, IPackager::IInstallationInfo * param1) override
  {
    IPCMessage newMessage(BaseClass::Message(0));

    // write parameters
    RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
    writer.Number<IPackager::IPackageInfo *>(param0);
    writer.Number<IPackager::IInstallationInfo *>(param1);

    Invoke(newMessage);
  }
}; // class PackagerNotificationProxy


// -----------------------------------------------------------------
// REGISTRATION
// -----------------------------------------------------------------

namespace {

typedef ProxyStub::StubType<IPackager::IInstallationInfo, PackagerInstallationInfoStubMethods, ProxyStub::UnknownStub> PackagerInstallationInfoStub;
typedef ProxyStub::StubType<IPackager, PackagerStubMethods, ProxyStub::UnknownStub> PackagerStub;
typedef ProxyStub::StubType<IPackager::IPackageInfo, PackagerPackageInfoStubMethods, ProxyStub::UnknownStub> PackagerPackageInfoStub;
typedef ProxyStub::StubType<IPackager::INotification, PackagerNotificationStubMethods, ProxyStub::UnknownStub> PackagerNotificationStub;

static class Instantiation {
public:
  Instantiation()
  {
    RPC::Administrator::Instance().Announce<IPackager::IInstallationInfo, PackagerInstallationInfoProxy, PackagerInstallationInfoStub>();
    RPC::Administrator::Instance().Announce<IPackager, PackagerProxy, PackagerStub>();
    RPC::Administrator::Instance().Announce<IPackager::IPackageInfo, PackagerPackageInfoProxy, PackagerPackageInfoStub>();
    RPC::Administrator::Instance().Announce<IPackager::INotification, PackagerNotificationProxy, PackagerNotificationStub>();
  }
} ProxyStubRegistration;

} // namespace

} // namespace WPEFramework

} // namespace ProxyStubs


#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>

#include <core/core.h>
#include <com/com.h>
#include <core/Portability.h>

string g_connectorName = _T("/tmp/wperpc01");

namespace WPEFramework {
namespace Exchange {
    struct IAdder : virtual public Core::IUnknown {
        enum { ID = 0x80000001 };
        virtual uint32_t GetValue() = 0;
        virtual void Add(uint32_t value) = 0;
        virtual pid_t GetPid() = 0;
    };
}
}

using namespace WPEFramework;
using namespace std;

class Adder : public Exchange::IAdder
{
public:
    Adder()
        : m_value(0)
    {
    }

    uint32_t GetValue()
    {
        return m_value;
    }

    void Add(uint32_t value)
    {
        m_value += value;
    }

    pid_t GetPid()
    {
        return getpid();
    }

    BEGIN_INTERFACE_MAP(Adder)
        INTERFACE_ENTRY(Exchange::IAdder)
    END_INTERFACE_MAP

private:
    uint32_t m_value;
};

// Proxystubs.
namespace WPEFramework {
    ProxyStub::MethodHandler AdderStubMethods[] = {
          [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
              //
              // virtual uint32_t GetValue = 0;
              //
              RPC::Data::Frame::Writer response(message->Response().Writer());

              response.Number(message->Parameters().Implementation<Exchange::IAdder>()->GetValue());
          },
          [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
              //
              // virtual void Add(uint32_t value) = 0;
              //
              RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

              uint32_t value = parameters.Number<uint32_t>();

              message->Parameters().Implementation<Exchange::IAdder>()->Add(value);
          },
          [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
              //
              // virtual pid_t GetPid() = 0;
              //
              RPC::Data::Frame::Writer response(message->Response().Writer());

              response.Number(message->Parameters().Implementation<Exchange::IAdder>()->GetPid());
          },
    };

    typedef ProxyStub::StubType<Exchange::IAdder, AdderStubMethods, ProxyStub::UnknownStub> AdderStub;

    class AdderProxy : public ProxyStub::UnknownProxyType<Exchange::IAdder> {
    public:
        AdderProxy(Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~AdderProxy()
        {
        }

        virtual uint32_t GetValue()
        {
            IPCMessage newMessage(BaseClass::Message(0));

            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            return (reader.Number<uint32_t>());
        }

        virtual void Add(uint32_t value)
        {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(value);

            Invoke(newMessage);
        }

        virtual pid_t GetPid()
        {
            IPCMessage newMessage(BaseClass::Message(2));

            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            return (reader.Number<pid_t>());
        }
    };

    namespace {
        class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<Exchange::IAdder, AdderProxy, AdderStub>();
            }

            ~Instantiation()
            {
            }

        } instantiation;
    }
}

class ExternalAccess : public RPC::Communicator
{
private:
    ExternalAccess() = delete;
    ExternalAccess(const ExternalAccess &) = delete;
    ExternalAccess & operator=(const ExternalAccess &) = delete;

public:
    ExternalAccess(const Core::NodeId & source)
        : RPC::Communicator(source, Core::ProxyType< RPC::InvokeServerType<4, 1> >::Create(), _T(""))
    {
        Open(Core::infinite);
    }

    ~ExternalAccess()
    {
        Close(Core::infinite);
    }

private:
    virtual void* Aquire(const string& className, const uint32_t interfaceId, const uint32_t versionId)
    {
        void* result = nullptr;

        if (interfaceId == Exchange::IAdder::ID) {
            Exchange::IAdder * newAdder = Core::Service<Adder>::Create<Exchange::IAdder>();
            result = newAdder;
        }

        return result;
    }
};

TEST(Core_RPC, adder)
{
   IPTestAdministrator::OtherSideMain otherSide = [](IPTestAdministrator & testAdmin) {
      Core::NodeId remoteNode(g_connectorName.c_str());

      ExternalAccess communicator(remoteNode);

      testAdmin.Sync("setup server");

      testAdmin.Sync("done testing");

      communicator.Close(Core::infinite);
   };

   IPTestAdministrator testAdmin(otherSide);

   testAdmin.Sync("setup server");

   {
      // Setup handler.
      Core::ProxyType<RPC::IHandler> handler(Core::ProxyType<RPC::InvokeServerType<4,1> >::Create(Core::Thread::DefaultStackSize()));

      // Setup client.
      Core::NodeId remoteNode(g_connectorName.c_str());
      Core::ProxyType<RPC::CommunicatorClient> client(Core::ProxyType<RPC::CommunicatorClient>::Create(remoteNode, handler));

      // Create remote instance of "IAdder".
      Exchange::IAdder * adder = client->Open<Exchange::IAdder>(_T("Adder"));

      // Perform some arithmatic.
      EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(0));
      adder->Add(20);
      EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(20));
      adder->Add(22);
      EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(42));

      // Make sure other side is indeed running in other process.
      EXPECT_NE(adder->GetPid(), getpid());

      adder->Release();

      client->Close(Core::infinite);
      WPEFramework::Core::Singleton::Dispose();
   }

   testAdmin.Sync("done testing");
}

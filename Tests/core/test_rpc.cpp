#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>

#include <core/core.h>
#include <com/com.h>
#include <core/Portability.h>

static string g_connectorName = _T("/tmp/wperpc01");

namespace WPEFramework {
namespace Exchange {
    struct IAdder : virtual public Core::IUnknown {
        enum { ID = 0x80000001 };
        virtual uint32_t GetValue() = 0;
        virtual void Add(uint32_t value) = 0;
        virtual uint32_t GetPid() = 0;
    };
}

namespace Tests {

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

        uint32_t GetPid()
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
    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IAdder interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t GetValue() = 0
    //  (1) virtual void Add(uint32_t) = 0
    //  (2) virtual uint32_t GetPid() = 0
    //

    ProxyStub::MethodHandler AdderStubMethods[] = {
        // virtual uint32_t GetValue() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IAdder* implementation = input.Implementation<IAdder>();
            ASSERT((implementation != nullptr) && "Null IAdder implementation pointer");
            const uint32_t output = implementation->GetValue();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual void Add(uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IAdder* implementation = input.Implementation<IAdder>();
            ASSERT((implementation != nullptr) && "Null IAdder implementation pointer");
            implementation->Add(param0);
        },

        // virtual uint32_t GetPid() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IAdder* implementation = input.Implementation<IAdder>();
            ASSERT((implementation != nullptr) && "Null IAdder implementation pointer");
            const uint32_t output = implementation->GetPid();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // AdderStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IAdder interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t GetValue() = 0
    //  (1) virtual void Add(uint32_t) = 0
    //  (2) virtual uint32_t GetPid() = 0
    //

    class AdderProxy final : public ProxyStub::UnknownProxyType<IAdder> {
    public:
        AdderProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t GetValue() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void Add(uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        uint32_t GetPid() override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }
    }; // class AdderProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IAdder, AdderStubMethods> AdderStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IAdder, AdderProxy, AdderStub>();
            }
        } ProxyStubRegistration;

    } // namespace

    namespace {
    class ExternalAccess : public RPC::Communicator
    {
    private:
        ExternalAccess() = delete;
        ExternalAccess(const ExternalAccess &) = delete;
        ExternalAccess & operator=(const ExternalAccess &) = delete;

    public:
        ExternalAccess(const Core::NodeId & source)
            : RPC::Communicator(source, _T(""))
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
    }

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
          Core::NodeId remoteNode(g_connectorName.c_str());

          Core::ProxyType<RPC::InvokeServerType<4, 1>> engine(Core::ProxyType<RPC::InvokeServerType<4, 1>>::Create(Core::Thread::DefaultStackSize()));
          Core::ProxyType<RPC::CommunicatorClient> client(
               Core::ProxyType<RPC::CommunicatorClient>::Create(
                   remoteNode,
                   Core::ProxyType<Core::IIPCServer>(engine)
               ));
          engine->Announcements(client->Announcement());

          // Create remote instance of "IAdder".
          Exchange::IAdder * adder = client->Open<Exchange::IAdder>(_T("Adder"));

          // Perform some arithmatic.
          EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(0));
          adder->Add(20);
          EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(20));
          adder->Add(22);
          EXPECT_EQ(adder->GetValue(), static_cast<uint32_t>(42));

          // Make sure other side is indeed running in other process.
          EXPECT_NE(adder->GetPid(), (uint32_t)getpid());

          adder->Release();

          client->Close(Core::infinite);
       }

       testAdmin.Sync("done testing");
       Core::Singleton::Dispose();
    }
}
}

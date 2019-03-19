#include "IUnknown.h"
#include "Administrator.h"

namespace WPEFramework {
namespace ProxyStub {
    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------
    UnknownStub::UnknownStub()
    {
    }

    /* virtual */ UnknownStub::~UnknownStub()
    {
    }

    /* virtual */ Core::IUnknown* UnknownStub::Convert(void* incomingData) const
    {
        Core::IUnknown* result = reinterpret_cast<Core::IUnknown*>(incomingData);

        return (result);
    }

    /* virtual */ void UnknownStub::Handle(const uint16_t index,
        Core::ProxyType<Core::IPCChannel>& /* channel */,
        Core::ProxyType<RPC::InvokeMessage>& message)
    {
        Core::IUnknown* implementation(Convert(message->Parameters().Implementation<void*>()));

        ASSERT(implementation != nullptr);

        if (implementation != nullptr) {
            switch (index) {
            case 0: {
                // AddRef
                implementation->AddRef();
                break;
            }
            case 1: {
                // Release
                RPC::Data::Frame::Writer response(message->Response().Writer());

                response.Number<uint32_t>(implementation->Release());
                break;
            }
            case 2: {
                // QueryInterface
                RPC::Data::Frame::Reader reader(message->Parameters().Reader());
                RPC::Data::Frame::Writer response(message->Response().Writer());
                uint32_t newInterfaceId(reader.Number<uint32_t>());

                void* newInterface = implementation->QueryInterface(newInterfaceId);
                response.Number<void*>(newInterface);

                break;
            }
            default: {
                TRACE_L1("Method ID [%d] not existing.\n", index);
                break;
            }
            }
        }
    }

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    static class UnknownInstantiation {
    public:
        UnknownInstantiation()
        {
            RPC::Administrator::Instance().Announce<Core::IUnknown, UnknownProxyType<Core::IUnknown>, UnknownStub>();
        }
        ~UnknownInstantiation()
        {
        }

    } UnknownRegistration;
}
}

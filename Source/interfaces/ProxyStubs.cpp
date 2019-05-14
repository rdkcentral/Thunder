#include "IPerformance.h"
#include "Module.h"

MODULE_NAME_DECLARATION(BUILDREF_WEBBRIDGE)

namespace WPEFramework {

// -------------------------------------------------------------------------------------------
// STUB
// -------------------------------------------------------------------------------------------

//
// Exchange::IPerformance interface stub definitions
//
ProxyStub::MethodHandler PerformanceStubMethods[] = {

    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
        Core::ProxyType<RPC::InvokeMessage>& message) {
        RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
        RPC::Data::Frame::Writer response(message->Response().Writer());

        const uint8_t* buffer;
        uint16_t bufferLength = parameters.LockBuffer<uint16_t>(buffer);
        parameters.UnlockBuffer(bufferLength);

        response.Number(message->Parameters()
                            .Implementation<Exchange::IPerformance>()
                            ->Send(bufferLength, buffer));
    },
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
        Core::ProxyType<RPC::InvokeMessage>& message) {
        RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
        RPC::Data::Frame::Writer response(message->Response().Writer());

        uint16_t bufferLength;
        bufferLength = parameters.Number<uint16_t>();
        uint8_t* buffer = static_cast<uint8_t*>(ALLOCA(bufferLength));

        response.Number(message->Parameters()
                            .Implementation<Exchange::IPerformance>()
                            ->Receive(bufferLength, buffer));

        response.Buffer(bufferLength, buffer);
    },
    [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED,
        Core::ProxyType<RPC::InvokeMessage>& message) {
        RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
        RPC::Data::Frame::Writer response(message->Response().Writer());

        const uint8_t* buffer;
        uint16_t bufferLength = parameters.LockBuffer<uint16_t>(buffer);
        parameters.UnlockBuffer(bufferLength);

        uint16_t receiveLength;
        receiveLength = parameters.Number<uint16_t>();
        uint8_t* storage = static_cast<uint8_t*>(ALLOCA(receiveLength > bufferLength ? receiveLength : bufferLength));

        ::memcpy(storage, buffer, bufferLength);
        response.Number(message->Parameters()
                                .Implementation<Exchange::IPerformance>()
                            ->Exchange(bufferLength, storage, receiveLength));

        response.Buffer(bufferLength, storage);
    },
    nullptr
};

typedef ProxyStub::UnknownStubType<Exchange::IPerformance, PerformanceStubMethods> PerformanceStub;

// -------------------------------------------------------------------------------------------
// PROXY
// -------------------------------------------------------------------------------------------
class PerformanceProxy : public ProxyStub::UnknownProxyType<Exchange::IPerformance> {
public:
    PerformanceProxy(const Core::ProxyType<Core::IPCChannel>& channel,
        void* implementation, const bool otherSideInformed)
        : BaseClass(channel, implementation, otherSideInformed)
    {
    }
    virtual ~PerformanceProxy() {}

public:
    virtual uint32_t Send(const uint16_t bufferSize, const uint8_t buffer[]) override
    {
        IPCMessage newMessage(BaseClass::Message(0));
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

        uint32_t result = ~0;
        writer.Buffer(bufferSize, buffer);

        if (Invoke(newMessage) == Core::ERROR_NONE) {

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            result = reader.Number<uint32_t>();
        }

        return result;
    }

    virtual uint32_t Receive(uint16_t& bufferSize, uint8_t buffer[]) const override
    {
        IPCMessage newMessage(BaseClass::Message(1));
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

        uint32_t result = ~0;
        writer.Number(bufferSize);

        if (Invoke(newMessage) == Core::ERROR_NONE) {

            const uint8_t* receiveBuffer;
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            result = reader.Number<uint32_t>();
            uint16_t receivedBufferSize = reader.LockBuffer<uint16_t>(receiveBuffer);
            ::memcpy(buffer, receiveBuffer, (receivedBufferSize <= bufferSize ? receivedBufferSize : bufferSize));
            reader.UnlockBuffer(receivedBufferSize);
            bufferSize = receivedBufferSize;
        }

        return result;
    }
    virtual uint32_t Exchange(uint16_t& bufferSize, uint8_t buffer[], const uint16_t maxBufferSize) override
    {
        IPCMessage newMessage(BaseClass::Message(2));
        RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());

        uint32_t result = ~0;
        writer.Buffer(bufferSize, buffer);
        writer.Number(maxBufferSize);

        if (Invoke(newMessage) == Core::ERROR_NONE) {
            const uint8_t* receiveBuffer;
            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            result = reader.Number<uint32_t>();
            bufferSize = reader.LockBuffer<uint16_t>(receiveBuffer);
            ::memcpy(buffer, receiveBuffer, (bufferSize <= maxBufferSize ? bufferSize : maxBufferSize));
            reader.UnlockBuffer(bufferSize);
        }

        return result;
    }
};

namespace {
    static class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<Exchange::IPerformance, PerformanceProxy, PerformanceStub>();
        }

        ~Instantiation() {}

    } PerformanceProxyStubRegistration;
}

} // namespace WPEFramework

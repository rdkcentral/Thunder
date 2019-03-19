#include "IStringIterator.h"
#include "Communicator.h"
#include "IUnknown.h"

namespace WPEFramework {
namespace ProxyStub {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    ProxyStub::MethodHandler StringIteratorStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            string result;

            // virtual bool Next(string& text) = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = message->Parameters().Implementation<RPC::IStringIterator>()->Next(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Text(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            string result;

            // virtual bool Previous() = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            bool valid = message->Parameters().Implementation<RPC::IStringIterator>()->Previous(result);

            response.Boolean(valid);

            if (valid == true) {
                response.Text(result);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual void Reset(const uint32_t position)
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            uint32_t position(parameters.Number<uint32_t>());

            message->Parameters().Implementation<RPC::IStringIterator>()->Reset(position);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual bool IsValid() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Boolean(message->Parameters().Implementation<RPC::IStringIterator>()->IsValid());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual uint32_t Count() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Number<uint32_t>(message->Parameters().Implementation<RPC::IStringIterator>()->Count());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            // virtual string Current() const = 0;
            RPC::Data::Frame::Writer response(message->Response().Writer());

            response.Text(message->Parameters().Implementation<RPC::IStringIterator>()->Current());
        },
        nullptr
    };

    typedef ProxyStub::UnknownStubType<RPC::IStringIterator, StringIteratorStubMethods> StringIteratorStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class StringIteratorProxy : public UnknownProxyType<RPC::IStringIterator> {
    public:
        StringIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
            TRACE_L1("Constructed StringIteratorProxy: %p", this);
        }
        virtual ~StringIteratorProxy()
        {
            TRACE_L1("Destructed StringIteratorProxy: %p", this);
        }

    public:
        virtual bool Next(string& result) override
        {
            bool valid = false;

            IPCMessage newMessage(BaseClass::Message(0));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Text();
                }
            }

            return (valid);
        }
        virtual bool Previous(string& result) override
        {
            bool valid = false;
            IPCMessage newMessage(BaseClass::Message(1));

            if (Invoke(newMessage) == Core::ERROR_NONE) {

                RPC::Data::Frame::Reader reader = newMessage->Response().Reader();
                valid = reader.Boolean();

                if (valid == true) {
                    result = reader.Text();
                }
            }

            return (valid);
        }
        virtual void Reset(const uint32_t position) override
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(position);

            Invoke(newMessage);
        }
        virtual bool IsValid() const override
        {
            bool result = false;
            IPCMessage newMessage(BaseClass::Message(3));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Boolean();
            }

            return (result);
        }
        virtual uint32_t Count() const override
        {
            uint32_t result = 0;
            IPCMessage newMessage(BaseClass::Message(4));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Number<uint32_t>();
            }

            return (result);
        }
        virtual string Current() const override
        {
            string result;
            IPCMessage newMessage(BaseClass::Message(5));

            if (Invoke(newMessage) == Core::ERROR_NONE) {
                result = newMessage->Response().Reader().Text();
            }

            return (result);
        }
    };

    // -------------------------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------------------------

    namespace {

        class RPCInstantiation {
        public:
            RPCInstantiation()
            {
                RPC::Administrator::Instance().Announce<RPC::IStringIterator, StringIteratorProxy, StringIteratorStub>();
            }
            ~RPCInstantiation()
            {
            }

        } RPCRegistration;
    }
}
}

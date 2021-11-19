//
// generated automatically from "ITrace.h"
//
// implements RPC proxy stubs for:
//   - class ITraceIterator
//   - class ITraceController
//

#include "Module.h"
#include "ITrace.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Trace;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // ITraceIterator interface stub definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool Info(bool&, string&, string&) const = 0
    //

    ProxyStub::MethodHandler TraceIteratorStubMethods[] = {
        // virtual void Reset() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ITraceIterator* implementation = reinterpret_cast<ITraceIterator*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ITraceIterator implementation pointer");
            implementation->Reset();
        },

        // virtual bool Info(bool&, string&, string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            bool param0{}; // storage
            string param1{}; // storage
            string param2{}; // storage

            // call implementation
            const ITraceIterator* implementation = reinterpret_cast<const ITraceIterator*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ITraceIterator implementation pointer");
            const bool output = implementation->Info(param0, param1, param2);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
            writer.Boolean(param0);
            writer.Text(param1);
            writer.Text(param2);
        },

        nullptr
    }; // TraceIteratorStubMethods[]

    //
    // ITraceController interface stub definitions
    //
    // Methods:
    //  (0) virtual void Enable(const bool, const string&, const string&) = 0
    //

    ProxyStub::MethodHandler TraceControllerStubMethods[] = {
        // virtual void Enable(const bool, const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const bool param0 = reader.Boolean();
            const string param1 = reader.Text();
            const string param2 = reader.Text();

            // call implementation
            ITraceController* implementation = reinterpret_cast<ITraceController*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ITraceController implementation pointer");
            implementation->Enable(param0, param1, param2);
        },

        nullptr
    }; // TraceControllerStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // ITraceIterator interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Reset() = 0
    //  (1) virtual bool Info(bool&, string&, string&) const = 0
    //

    class TraceIteratorProxy final : public ProxyStub::UnknownProxyType<ITraceIterator> {
    public:
        TraceIteratorProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Reset() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            Invoke(newMessage);
        }

        bool Info(bool& /* out */ param0, string& /* out */ param1, string& /* out */ param2) const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
                param0 = reader.Boolean();
                param1 = reader.Text();
                param2 = reader.Text();
            }

            return output;
        }
    }; // class TraceIteratorProxy

    //
    // ITraceController interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Enable(const bool, const string&, const string&) = 0
    //

    class TraceControllerProxy final : public ProxyStub::UnknownProxyType<ITraceController> {
    public:
        TraceControllerProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Enable(const bool param0, const string& param1, const string& param2) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Boolean(param0);
            writer.Text(param1);
            writer.Text(param2);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class TraceControllerProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<ITraceIterator, TraceIteratorStubMethods> TraceIteratorStub;
        typedef ProxyStub::UnknownStubType<ITraceController, TraceControllerStubMethods> TraceControllerStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<ITraceIterator, TraceIteratorProxy, TraceIteratorStub>();
                RPC::Administrator::Instance().Announce<ITraceController, TraceControllerProxy, TraceControllerStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<ITraceIterator>();
                RPC::Administrator::Instance().Recall<ITraceController>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

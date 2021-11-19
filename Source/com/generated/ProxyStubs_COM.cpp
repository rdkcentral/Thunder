//
// generated automatically from "ICOM.h"
//
// implements RPC proxy stubs for:
//   - template class IIteratorType<typename ELEMENT, const uint32_t INTERFACE_ID> [with ELEMENT = string, INTERFACE_ID = ID_STRINGITERATOR]
//   - template class IIteratorType<typename ELEMENT, const uint32_t INTERFACE_ID> [with ELEMENT = uint32_t, INTERFACE_ID = ID_VALUEITERATOR]
//   - class IRemoteConnection
//   - class IRemoteConnection::INotification
//

#include "Module.h"
#include "ICOM.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace RPC;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  interface stub definitions
    //
    // Methods:
    //  (0) virtual bool Next(string&) = 0
    //  (1) virtual bool Previous(string&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual string Current() const = 0
    //

    ProxyStub::MethodHandler IteratorTypeInstance251D949FStubMethods[] = {
        // virtual bool Next(string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            string param0{}; // storage

            // call implementation
            IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->Next(param0);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
            writer.Text(param0);
        },

        // virtual bool Previous(string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            string param0{}; // storage

            // call implementation
            IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->Previous(param0);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
            writer.Text(param0);
        },

        // virtual void Reset(const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            implementation->Reset(param0);
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->IsValid();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual uint32_t Count() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            const uint32_t output = implementation->Count();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual string Current() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  implementation pointer");
            const string output = implementation->Current();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // IteratorTypeInstance251D949FStubMethods[]

    //
    // IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  interface stub definitions
    //
    // Methods:
    //  (0) virtual bool Next(uint32_t&) = 0
    //  (1) virtual bool Previous(uint32_t&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual uint32_t Current() const = 0
    //

    ProxyStub::MethodHandler IteratorTypeInstance3971735AStubMethods[] = {
        // virtual bool Next(uint32_t&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            uint32_t param0{}; // storage

            // call implementation
            IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->Next(param0);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
            writer.Number<uint32_t>(param0);
        },

        // virtual bool Previous(uint32_t&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            uint32_t param0{}; // storage

            // call implementation
            IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->Previous(param0);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
            writer.Number<uint32_t>(param0);
        },

        // virtual void Reset(const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            implementation->Reset(param0);
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            const bool output = implementation->IsValid();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual uint32_t Count() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            const uint32_t output = implementation->Count();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Current() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */* implementation = reinterpret_cast<const IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  implementation pointer");
            const uint32_t output = implementation->Current();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // IteratorTypeInstance3971735AStubMethods[]

    //
    // IRemoteConnection interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t Id() const = 0
    //  (1) virtual uint32_t RemoteId() const = 0
    //  (2) virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
    //  (3) virtual void Terminate() = 0
    //  (4) virtual uint32_t Launch() = 0
    //  (5) virtual void PostMortem() = 0
    //

    ProxyStub::MethodHandler RemoteConnectionStubMethods[] = {
        // virtual uint32_t Id() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IRemoteConnection* implementation = reinterpret_cast<const IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            const uint32_t output = implementation->Id();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t RemoteId() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IRemoteConnection* implementation = reinterpret_cast<const IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            const uint32_t output = implementation->RemoteId();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();
            const string param1 = reader.Text();
            const uint32_t output_interfaceid = reader.Number<uint32_t>();
            const uint32_t param3 = reader.Number<uint32_t>();

            // call implementation
            IRemoteConnection* implementation = reinterpret_cast<IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            void* output = implementation->Aquire(param0, param1, output_interfaceid, param3);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<void*>(output));
            if (output != nullptr) {
                RPC::Administrator::Instance().RegisterInterface(channel, output, output_interfaceid);
            }
        },

        // virtual void Terminate() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IRemoteConnection* implementation = reinterpret_cast<IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            implementation->Terminate();
        },

        // virtual uint32_t Launch() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IRemoteConnection* implementation = reinterpret_cast<IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            const uint32_t output = implementation->Launch();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual void PostMortem() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IRemoteConnection* implementation = reinterpret_cast<IRemoteConnection*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection implementation pointer");
            implementation->PostMortem();
        },

        nullptr
    }; // RemoteConnectionStubMethods[]

    //
    // IRemoteConnection::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Activated(IRemoteConnection*) = 0
    //  (1) virtual void Deactivated(IRemoteConnection*) = 0
    //

    ProxyStub::MethodHandler RemoteConnectionNotificationStubMethods[] = {
        // virtual void Activated(IRemoteConnection*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IRemoteConnection* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IRemoteConnection proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IRemoteConnection proxy");
                }
            }

            // call implementation
            IRemoteConnection::INotification* implementation = reinterpret_cast<IRemoteConnection::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection::INotification implementation pointer");
            implementation->Activated(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Deactivated(IRemoteConnection*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IRemoteConnection* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IRemoteConnection proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IRemoteConnection proxy");
                }
            }

            // call implementation
            IRemoteConnection::INotification* implementation = reinterpret_cast<IRemoteConnection::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IRemoteConnection::INotification implementation pointer");
            implementation->Deactivated(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // RemoteConnectionNotificationStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */  interface proxy definitions
    //
    // Methods:
    //  (0) virtual bool Next(string&) = 0
    //  (1) virtual bool Previous(string&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual string Current() const = 0
    //

    class IteratorTypeInstance251D949FProxy final : public ProxyStub::UnknownProxyType<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */ > {
    public:
        IteratorTypeInstance251D949FProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        bool Next(string& /* out */ param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
                param0 = reader.Text();
            }

            return output;
        }

        bool Previous(string& /* out */ param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
                param0 = reader.Text();
            }

            return output;
        }

        void Reset(const uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        bool IsValid() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        uint32_t Count() const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        string Current() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class IteratorTypeInstance251D949FProxy

    //
    // IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */  interface proxy definitions
    //
    // Methods:
    //  (0) virtual bool Next(uint32_t&) = 0
    //  (1) virtual bool Previous(uint32_t&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual uint32_t Current() const = 0
    //

    class IteratorTypeInstance3971735AProxy final : public ProxyStub::UnknownProxyType<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */ > {
    public:
        IteratorTypeInstance3971735AProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        bool Next(uint32_t& /* out */ param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
                param0 = reader.Number<uint32_t>();
            }

            return output;
        }

        bool Previous(uint32_t& /* out */ param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
                param0 = reader.Number<uint32_t>();
            }

            return output;
        }

        void Reset(const uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        bool IsValid() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        uint32_t Count() const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Current() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }
    }; // class IteratorTypeInstance3971735AProxy

    //
    // IRemoteConnection interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t Id() const = 0
    //  (1) virtual uint32_t RemoteId() const = 0
    //  (2) virtual void* Aquire(const uint32_t, const string&, const uint32_t, const uint32_t) = 0
    //  (3) virtual void Terminate() = 0
    //  (4) virtual uint32_t Launch() = 0
    //  (5) virtual void PostMortem() = 0
    //

    class RemoteConnectionProxy final : public ProxyStub::UnknownProxyType<IRemoteConnection> {
    public:
        RemoteConnectionProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t Id() const override
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

        uint32_t RemoteId() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void* Aquire(const uint32_t param0, const string& param1, const uint32_t param2, const uint32_t param3) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);
            writer.Text(param1);
            writer.Number<const uint32_t>(param2);
            writer.Number<const uint32_t>(param3);

            // invoke the method handler
            void* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = Interface(reader.Number<RPC::instance_id>(), param2);
            }

            return output_proxy;
        }

        void Terminate() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            Invoke(newMessage);
        }

        uint32_t Launch() override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        void PostMortem() override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            Invoke(newMessage);
        }

    }; // class RemoteConnectionProxy

    //
    // IRemoteConnection::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Activated(IRemoteConnection*) = 0
    //  (1) virtual void Deactivated(IRemoteConnection*) = 0
    //

    class RemoteConnectionNotificationProxy final : public ProxyStub::UnknownProxyType<IRemoteConnection::INotification> {
    public:
        RemoteConnectionNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Activated(IRemoteConnection* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IRemoteConnection*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Deactivated(IRemoteConnection* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IRemoteConnection*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class RemoteConnectionNotificationProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */ , IteratorTypeInstance251D949FStubMethods> IteratorTypeInstance251D949FStub;
        typedef ProxyStub::UnknownStubType<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */ , IteratorTypeInstance3971735AStubMethods> IteratorTypeInstance3971735AStub;
        typedef ProxyStub::UnknownStubType<IRemoteConnection, RemoteConnectionStubMethods> RemoteConnectionStub;
        typedef ProxyStub::UnknownStubType<IRemoteConnection::INotification, RemoteConnectionNotificationStubMethods> RemoteConnectionNotificationStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */ , IteratorTypeInstance251D949FProxy, IteratorTypeInstance251D949FStub>();
                RPC::Administrator::Instance().Announce<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */ , IteratorTypeInstance3971735AProxy, IteratorTypeInstance3971735AStub>();
                RPC::Administrator::Instance().Announce<IRemoteConnection, RemoteConnectionProxy, RemoteConnectionStub>();
                RPC::Administrator::Instance().Announce<IRemoteConnection::INotification, RemoteConnectionNotificationProxy, RemoteConnectionNotificationStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<IIteratorType<string, ID_STRINGITERATOR> /* instatiated template class */ >();
                RPC::Administrator::Instance().Recall<IIteratorType<uint32_t, ID_VALUEITERATOR> /* instatiated template class */ >();
                RPC::Administrator::Instance().Recall<IRemoteConnection>();
                RPC::Administrator::Instance().Recall<IRemoteConnection::INotification>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

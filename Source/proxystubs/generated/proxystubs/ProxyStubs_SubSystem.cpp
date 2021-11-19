//
// generated automatically from "ISubSystem.h"
//
// implements RPC proxy stubs for:
//   - class ISubSystem
//   - class ISubSystem::INotification
//   - class ISubSystem::ISecurity
//   - class ISubSystem::IInternet
//   - class ISubSystem::ILocation
//   - class ISubSystem::IIdentifier
//   - class ISubSystem::ITime
//   - class ISubSystem::IProvisioning
//   - class ISubSystem::IDecryption
//

#include "Module.h"
#include "ISubSystem.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace PluginHost;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // ISubSystem interface stub definitions
    //
    // Methods:
    //  (0) virtual void Register(ISubSystem::INotification*) = 0
    //  (1) virtual void Unregister(ISubSystem::INotification*) = 0
    //  (2) virtual string BuildTreeHash() const = 0
    //  (3) virtual void Set(const ISubSystem::subsystem, Core::IUnknown*) = 0
    //  (4) virtual const Core::IUnknown* Get(const ISubSystem::subsystem) const = 0
    //  (5) virtual bool IsActive(const ISubSystem::subsystem) const = 0
    //

    ProxyStub::MethodHandler SubSystemStubMethods[] = {
        // virtual void Register(ISubSystem::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            ISubSystem::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of ISubSystem::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of ISubSystem::INotification proxy");
                }
            }

            // call implementation
            ISubSystem* implementation = reinterpret_cast<ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            implementation->Register(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(ISubSystem::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            ISubSystem::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of ISubSystem::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of ISubSystem::INotification proxy");
                }
            }

            // call implementation
            ISubSystem* implementation = reinterpret_cast<ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            implementation->Unregister(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual string BuildTreeHash() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem* implementation = reinterpret_cast<const ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            const string output = implementation->BuildTreeHash();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual void Set(const ISubSystem::subsystem, Core::IUnknown*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const ISubSystem::subsystem param0 = reader.Number<ISubSystem::subsystem>();
            RPC::instance_id param1 = reader.Number<RPC::instance_id>();
            Core::IUnknown* param1_proxy = nullptr;
            ProxyStub::UnknownProxy* param1_proxy_inst = nullptr;
            if (param1 != 0) {
                param1_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param1, false, param1_proxy);
                ASSERT((param1_proxy_inst != nullptr) && (param1_proxy != nullptr) && "Failed to get instance of Core::IUnknown proxy");

                if ((param1_proxy_inst == nullptr) || (param1_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of Core::IUnknown proxy");
                }
            }

            // call implementation
            ISubSystem* implementation = reinterpret_cast<ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            implementation->Set(param0, param1_proxy);

            if (param1_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param1_proxy_inst, message->Response());
            }
        },

        // virtual const Core::IUnknown* Get(const ISubSystem::subsystem) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const ISubSystem::subsystem param0 = reader.Number<ISubSystem::subsystem>();

            // call implementation
            const ISubSystem* implementation = reinterpret_cast<const ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            const Core::IUnknown* output = implementation->Get(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<const Core::IUnknown*>(output));
            if (output != nullptr) {
                RPC::Administrator::Instance().RegisterInterface(channel, output);
            }
        },

        // virtual bool IsActive(const ISubSystem::subsystem) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const ISubSystem::subsystem param0 = reader.Number<ISubSystem::subsystem>();

            // call implementation
            const ISubSystem* implementation = reinterpret_cast<const ISubSystem*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem implementation pointer");
            const bool output = implementation->IsActive(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        nullptr
    }; // SubSystemStubMethods[]

    //
    // ISubSystem::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Updated() = 0
    //

    ProxyStub::MethodHandler SubSystemNotificationStubMethods[] = {
        // virtual void Updated() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISubSystem::INotification* implementation = reinterpret_cast<ISubSystem::INotification*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::INotification implementation pointer");
            implementation->Updated();
        },

        nullptr
    }; // SubSystemNotificationStubMethods[]

    //
    // ISubSystem::ISecurity interface stub definitions
    //
    // Methods:
    //  (0) virtual string Callsign() const = 0
    //

    ProxyStub::MethodHandler SubSystemSecurityStubMethods[] = {
        // virtual string Callsign() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ISecurity* implementation = reinterpret_cast<const ISubSystem::ISecurity*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ISecurity implementation pointer");
            const string output = implementation->Callsign();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // SubSystemSecurityStubMethods[]

    //
    // ISubSystem::IInternet interface stub definitions
    //
    // Methods:
    //  (0) virtual string PublicIPAddress() const = 0
    //  (1) virtual ISubSystem::IInternet::network_type NetworkType() const = 0
    //

    ProxyStub::MethodHandler SubSystemInternetStubMethods[] = {
        // virtual string PublicIPAddress() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IInternet* implementation = reinterpret_cast<const ISubSystem::IInternet*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IInternet implementation pointer");
            const string output = implementation->PublicIPAddress();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual ISubSystem::IInternet::network_type NetworkType() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IInternet* implementation = reinterpret_cast<const ISubSystem::IInternet*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IInternet implementation pointer");
            const ISubSystem::IInternet::network_type output = implementation->NetworkType();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const ISubSystem::IInternet::network_type>(output);
        },

        nullptr
    }; // SubSystemInternetStubMethods[]

    //
    // ISubSystem::ILocation interface stub definitions
    //
    // Methods:
    //  (0) virtual string TimeZone() const = 0
    //  (1) virtual string Country() const = 0
    //  (2) virtual string Region() const = 0
    //  (3) virtual string City() const = 0
    //

    ProxyStub::MethodHandler SubSystemLocationStubMethods[] = {
        // virtual string TimeZone() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ILocation* implementation = reinterpret_cast<const ISubSystem::ILocation*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ILocation implementation pointer");
            const string output = implementation->TimeZone();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Country() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ILocation* implementation = reinterpret_cast<const ISubSystem::ILocation*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ILocation implementation pointer");
            const string output = implementation->Country();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Region() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ILocation* implementation = reinterpret_cast<const ISubSystem::ILocation*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ILocation implementation pointer");
            const string output = implementation->Region();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string City() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ILocation* implementation = reinterpret_cast<const ISubSystem::ILocation*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ILocation implementation pointer");
            const string output = implementation->City();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // SubSystemLocationStubMethods[]

    //
    // ISubSystem::IIdentifier interface stub definitions
    //
    // Methods:
    //  (0) virtual uint8_t Identifier(const uint8_t, uint8_t*) const = 0
    //  (1) virtual string Architecture() const = 0
    //  (2) virtual string Chipset() const = 0
    //  (3) virtual string FirmwareVersion() const = 0
    //

    ProxyStub::MethodHandler SubSystemIdentifierStubMethods[] = {
        // virtual uint8_t Identifier(const uint8_t, uint8_t*) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t param1_length = reader.Number<uint8_t>();

            // allocate receive buffer
            uint8_t* param1{};
            if (param1_length != 0) {
                param1 = static_cast<uint8_t*>(ALLOCA(param1_length));
                ASSERT(param1 != nullptr);
            }

            // call implementation
            const ISubSystem::IIdentifier* implementation = reinterpret_cast<const ISubSystem::IIdentifier*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IIdentifier implementation pointer");
            const uint8_t output = implementation->Identifier(param1_length, param1);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint8_t>(output);
            if ((param1 != nullptr) && (param1_length != 0)) {
                writer.Buffer<uint8_t>(param1_length, param1);
            }
        },

        // virtual string Architecture() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IIdentifier* implementation = reinterpret_cast<const ISubSystem::IIdentifier*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IIdentifier implementation pointer");
            const string output = implementation->Architecture();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Chipset() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IIdentifier* implementation = reinterpret_cast<const ISubSystem::IIdentifier*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IIdentifier implementation pointer");
            const string output = implementation->Chipset();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string FirmwareVersion() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IIdentifier* implementation = reinterpret_cast<const ISubSystem::IIdentifier*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IIdentifier implementation pointer");
            const string output = implementation->FirmwareVersion();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // SubSystemIdentifierStubMethods[]

    //
    // ISubSystem::ITime interface stub definitions
    //
    // Methods:
    //  (0) virtual uint64_t TimeSync() const = 0
    //

    ProxyStub::MethodHandler SubSystemTimeStubMethods[] = {
        // virtual uint64_t TimeSync() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::ITime* implementation = reinterpret_cast<const ISubSystem::ITime*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::ITime implementation pointer");
            const uint64_t output = implementation->TimeSync();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint64_t>(output);
        },

        nullptr
    }; // SubSystemTimeStubMethods[]

    //
    // ISubSystem::IProvisioning interface stub definitions
    //
    // Methods:
    //  (0) virtual string Storage() const = 0
    //  (1) virtual bool Next(string&) = 0
    //  (2) virtual bool Previous(string&) = 0
    //  (3) virtual void Reset(const uint32_t) = 0
    //  (4) virtual bool IsValid() const = 0
    //  (5) virtual uint32_t Count() const = 0
    //  (6) virtual string Current() const = 0
    //

    ProxyStub::MethodHandler SubSystemProvisioningStubMethods[] = {
        // virtual string Storage() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IProvisioning* implementation = reinterpret_cast<const ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
            const string output = implementation->Storage();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual bool Next(string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            string param0{}; // storage

            // call implementation
            ISubSystem::IProvisioning* implementation = reinterpret_cast<ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
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
            ISubSystem::IProvisioning* implementation = reinterpret_cast<ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
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
            ISubSystem::IProvisioning* implementation = reinterpret_cast<ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
            implementation->Reset(param0);
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IProvisioning* implementation = reinterpret_cast<const ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
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
            const ISubSystem::IProvisioning* implementation = reinterpret_cast<const ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
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
            const ISubSystem::IProvisioning* implementation = reinterpret_cast<const ISubSystem::IProvisioning*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IProvisioning implementation pointer");
            const string output = implementation->Current();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // SubSystemProvisioningStubMethods[]

    //
    // ISubSystem::IDecryption interface stub definitions
    //
    // Methods:
    //  (0) virtual bool Next(string&) = 0
    //  (1) virtual bool Previous(string&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual string Current() const = 0
    //

    ProxyStub::MethodHandler SubSystemDecryptionStubMethods[] = {
        // virtual bool Next(string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            string param0{}; // storage

            // call implementation
            ISubSystem::IDecryption* implementation = reinterpret_cast<ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
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
            ISubSystem::IDecryption* implementation = reinterpret_cast<ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
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
            ISubSystem::IDecryption* implementation = reinterpret_cast<ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
            implementation->Reset(param0);
        },

        // virtual bool IsValid() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISubSystem::IDecryption* implementation = reinterpret_cast<const ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
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
            const ISubSystem::IDecryption* implementation = reinterpret_cast<const ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
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
            const ISubSystem::IDecryption* implementation = reinterpret_cast<const ISubSystem::IDecryption*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null ISubSystem::IDecryption implementation pointer");
            const string output = implementation->Current();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        nullptr
    }; // SubSystemDecryptionStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // ISubSystem interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Register(ISubSystem::INotification*) = 0
    //  (1) virtual void Unregister(ISubSystem::INotification*) = 0
    //  (2) virtual string BuildTreeHash() const = 0
    //  (3) virtual void Set(const ISubSystem::subsystem, Core::IUnknown*) = 0
    //  (4) virtual const Core::IUnknown* Get(const ISubSystem::subsystem) const = 0
    //  (5) virtual bool IsActive(const ISubSystem::subsystem) const = 0
    //

    class SubSystemProxy final : public ProxyStub::UnknownProxyType<ISubSystem> {
    public:
        SubSystemProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Register(ISubSystem::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<ISubSystem::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(ISubSystem::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<ISubSystem::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        string BuildTreeHash() const override
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

        void Set(const ISubSystem::subsystem param0, Core::IUnknown* param1) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const ISubSystem::subsystem>(param0);
            writer.Number<RPC::instance_id>(RPC::instance_cast<Core::IUnknown*>(param1));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        const Core::IUnknown* Get(const ISubSystem::subsystem param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const ISubSystem::subsystem>(param0);

            // invoke the method handler
            Core::IUnknown* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<Core::IUnknown*>(Interface(reader.Number<RPC::instance_id>(), Core::IUnknown::ID));
            }

            return output_proxy;
        }

        bool IsActive(const ISubSystem::subsystem param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const ISubSystem::subsystem>(param0);

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

    }; // class SubSystemProxy

    //
    // ISubSystem::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Updated() = 0
    //

    class SubSystemNotificationProxy final : public ProxyStub::UnknownProxyType<ISubSystem::INotification> {
    public:
        SubSystemNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Updated() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class SubSystemNotificationProxy

    //
    // ISubSystem::ISecurity interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Callsign() const = 0
    //

    class SubSystemSecurityProxy final : public ProxyStub::UnknownProxyType<ISubSystem::ISecurity> {
    public:
        SubSystemSecurityProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Callsign() const override
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
    }; // class SubSystemSecurityProxy

    //
    // ISubSystem::IInternet interface proxy definitions
    //
    // Methods:
    //  (0) virtual string PublicIPAddress() const = 0
    //  (1) virtual ISubSystem::IInternet::network_type NetworkType() const = 0
    //

    class SubSystemInternetProxy final : public ProxyStub::UnknownProxyType<ISubSystem::IInternet> {
    public:
        SubSystemInternetProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string PublicIPAddress() const override
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

        ISubSystem::IInternet::network_type NetworkType() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            ISubSystem::IInternet::network_type output = static_cast<ISubSystem::IInternet::network_type>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<ISubSystem::IInternet::network_type>();
            }

            return output;
        }

    }; // class SubSystemInternetProxy

    //
    // ISubSystem::ILocation interface proxy definitions
    //
    // Methods:
    //  (0) virtual string TimeZone() const = 0
    //  (1) virtual string Country() const = 0
    //  (2) virtual string Region() const = 0
    //  (3) virtual string City() const = 0
    //

    class SubSystemLocationProxy final : public ProxyStub::UnknownProxyType<ISubSystem::ILocation> {
    public:
        SubSystemLocationProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string TimeZone() const override
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

        string Country() const override
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

        string Region() const override
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

        string City() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

    }; // class SubSystemLocationProxy

    //
    // ISubSystem::IIdentifier interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint8_t Identifier(const uint8_t, uint8_t*) const = 0
    //  (1) virtual string Architecture() const = 0
    //  (2) virtual string Chipset() const = 0
    //  (3) virtual string FirmwareVersion() const = 0
    //

    class SubSystemIdentifierProxy final : public ProxyStub::UnknownProxyType<ISubSystem::IIdentifier> {
    public:
        SubSystemIdentifierProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint8_t Identifier(const uint8_t param0, uint8_t* /* out */ param1) const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint8_t>(param0);

            // invoke the method handler
            uint8_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint8_t>();
                if ((param1 != 0) && (param0 != 0)) {
                    reader.Buffer<uint8_t>(param0, param1);
                }
            }

            return output;
        }

        string Architecture() const override
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

        string Chipset() const override
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

        string FirmwareVersion() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class SubSystemIdentifierProxy

    //
    // ISubSystem::ITime interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint64_t TimeSync() const = 0
    //

    class SubSystemTimeProxy final : public ProxyStub::UnknownProxyType<ISubSystem::ITime> {
    public:
        SubSystemTimeProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint64_t TimeSync() const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            uint64_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint64_t>();
            }

            return output;
        }
    }; // class SubSystemTimeProxy

    //
    // ISubSystem::IProvisioning interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Storage() const = 0
    //  (1) virtual bool Next(string&) = 0
    //  (2) virtual bool Previous(string&) = 0
    //  (3) virtual void Reset(const uint32_t) = 0
    //  (4) virtual bool IsValid() const = 0
    //  (5) virtual uint32_t Count() const = 0
    //  (6) virtual string Current() const = 0
    //

    class SubSystemProvisioningProxy final : public ProxyStub::UnknownProxyType<ISubSystem::IProvisioning> {
    public:
        SubSystemProvisioningProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Storage() const override
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

        bool Next(string& /* out */ param0) override
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

        bool Previous(string& /* out */ param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

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
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        bool IsValid() const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

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

        string Current() const override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }
    }; // class SubSystemProvisioningProxy

    //
    // ISubSystem::IDecryption interface proxy definitions
    //
    // Methods:
    //  (0) virtual bool Next(string&) = 0
    //  (1) virtual bool Previous(string&) = 0
    //  (2) virtual void Reset(const uint32_t) = 0
    //  (3) virtual bool IsValid() const = 0
    //  (4) virtual uint32_t Count() const = 0
    //  (5) virtual string Current() const = 0
    //

    class SubSystemDecryptionProxy final : public ProxyStub::UnknownProxyType<ISubSystem::IDecryption> {
    public:
        SubSystemDecryptionProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
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
    }; // class SubSystemDecryptionProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<ISubSystem, SubSystemStubMethods> SubSystemStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::INotification, SubSystemNotificationStubMethods> SubSystemNotificationStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::ISecurity, SubSystemSecurityStubMethods> SubSystemSecurityStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::IInternet, SubSystemInternetStubMethods> SubSystemInternetStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::ILocation, SubSystemLocationStubMethods> SubSystemLocationStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::IIdentifier, SubSystemIdentifierStubMethods> SubSystemIdentifierStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::ITime, SubSystemTimeStubMethods> SubSystemTimeStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::IProvisioning, SubSystemProvisioningStubMethods> SubSystemProvisioningStub;
        typedef ProxyStub::UnknownStubType<ISubSystem::IDecryption, SubSystemDecryptionStubMethods> SubSystemDecryptionStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<ISubSystem, SubSystemProxy, SubSystemStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::INotification, SubSystemNotificationProxy, SubSystemNotificationStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::ISecurity, SubSystemSecurityProxy, SubSystemSecurityStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::IInternet, SubSystemInternetProxy, SubSystemInternetStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::ILocation, SubSystemLocationProxy, SubSystemLocationStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::IIdentifier, SubSystemIdentifierProxy, SubSystemIdentifierStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::ITime, SubSystemTimeProxy, SubSystemTimeStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::IProvisioning, SubSystemProvisioningProxy, SubSystemProvisioningStub>();
                RPC::Administrator::Instance().Announce<ISubSystem::IDecryption, SubSystemDecryptionProxy, SubSystemDecryptionStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<ISubSystem>();
                RPC::Administrator::Instance().Recall<ISubSystem::INotification>();
                RPC::Administrator::Instance().Recall<ISubSystem::ISecurity>();
                RPC::Administrator::Instance().Recall<ISubSystem::IInternet>();
                RPC::Administrator::Instance().Recall<ISubSystem::ILocation>();
                RPC::Administrator::Instance().Recall<ISubSystem::IIdentifier>();
                RPC::Administrator::Instance().Recall<ISubSystem::ITime>();
                RPC::Administrator::Instance().Recall<ISubSystem::IProvisioning>();
                RPC::Administrator::Instance().Recall<ISubSystem::IDecryption>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

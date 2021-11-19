//
// generated automatically from "IShell.h"
//
// implements RPC proxy stubs for:
//   - class IShell
//

#include "Module.h"
#include "IShell.h"

#include <com/com.h>

namespace WPEFramework {

namespace ProxyStubs {

    using namespace PluginHost;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IShell interface stub definitions
    //
    // Methods:
    //  (0) virtual void EnableWebServer(const string&, const string&) = 0
    //  (1) virtual void DisableWebServer() = 0
    //  (2) virtual string Version() const = 0
    //  (3) virtual string Model() const = 0
    //  (4) virtual bool Background() const = 0
    //  (5) virtual string Accessor() const = 0
    //  (6) virtual string WebPrefix() const = 0
    //  (7) virtual string Locator() const = 0
    //  (8) virtual string ClassName() const = 0
    //  (9) virtual string Versions() const = 0
    //  (10) virtual string Callsign() const = 0
    //  (11) virtual string PersistentPath() const = 0
    //  (12) virtual string VolatilePath() const = 0
    //  (13) virtual string DataPath() const = 0
    //  (14) virtual string ProxyStubPath() const = 0
    //  (15) virtual string Substitute(const string&) const = 0
    //  (16) virtual bool AutoStart() const = 0
    //  (17) virtual bool Resumed() const = 0
    //  (18) virtual string HashKey() const = 0
    //  (19) virtual string ConfigLine() const = 0
    //  (20) virtual bool IsSupported(const uint8_t) const = 0
    //  (21) virtual ISubSystem* SubSystems() = 0
    //  (22) virtual void Notify(const string&) = 0
    //  (23) virtual void Register(IPlugin::INotification*) = 0
    //  (24) virtual void Unregister(IPlugin::INotification*) = 0
    //  (25) virtual IShell::state State() const = 0
    //  (26) virtual void* QueryInterfaceByCallsign(const uint32_t, const string&) = 0
    //  (27) virtual uint32_t Activate(const IShell::reason) = 0
    //  (28) virtual uint32_t Deactivate(const IShell::reason) = 0
    //  (29) virtual uint32_t Unavailable(const IShell::reason) = 0
    //  (30) virtual IShell::reason Reason() const = 0
    //  (31) virtual uint32_t Submit(const uint32_t, /* undefined */ const Core::ProxyType<Core::JSON::IElement>&) = 0
    //  (32) virtual IShell::ICOMLink* COMLink() = 0
    //

    ProxyStub::MethodHandler ShellStubMethods[] = {
        // virtual void EnableWebServer(const string&, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            implementation->EnableWebServer(param0, param1);
        },

        // virtual void DisableWebServer() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            implementation->DisableWebServer();
        },

        // virtual string Version() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Version();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Model() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Model();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual bool Background() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const bool output = implementation->Background();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual string Accessor() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Accessor();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string WebPrefix() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->WebPrefix();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Locator() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Locator();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string ClassName() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->ClassName();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Versions() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Versions();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Callsign() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Callsign();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string PersistentPath() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->PersistentPath();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string VolatilePath() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->VolatilePath();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string DataPath() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->DataPath();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string ProxyStubPath() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->ProxyStubPath();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string Substitute(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->Substitute(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual bool AutoStart() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const bool output = implementation->AutoStart();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual bool Resumed() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const bool output = implementation->Resumed();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual string HashKey() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->HashKey();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string ConfigLine() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const string output = implementation->ConfigLine();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual bool IsSupported(const uint8_t) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t param0 = reader.Number<uint8_t>();

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const bool output = implementation->IsSupported(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual ISubSystem* SubSystems() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            ISubSystem* output = implementation->SubSystems();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<ISubSystem*>(output));
            if (output != nullptr) {
                RPC::Administrator::Instance().RegisterInterface(channel, output);
            }
        },

        // virtual void Notify(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            implementation->Notify(param0);
        },

        // virtual void Register(IPlugin::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IPlugin::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IPlugin::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IPlugin::INotification proxy");
                }
            }

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            implementation->Register(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IPlugin::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            RPC::instance_id param0 = reader.Number<RPC::instance_id>();
            IPlugin::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != 0) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, false, param0_proxy);
                ASSERT((param0_proxy_inst != nullptr) && (param0_proxy != nullptr) && "Failed to get instance of IPlugin::INotification proxy");

                if ((param0_proxy_inst == nullptr) || (param0_proxy == nullptr)) {
                    TRACE_L1("Failed to get instance of IPlugin::INotification proxy");
                }
            }

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            implementation->Unregister(param0_proxy);

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual IShell::state State() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const IShell::state output = implementation->State();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const IShell::state>(output);
        },

        // virtual void* QueryInterfaceByCallsign(const uint32_t, const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t output_interfaceid = reader.Number<uint32_t>();
            const string param1 = reader.Text();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            void* output = implementation->QueryInterfaceByCallsign(output_interfaceid, param1);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<void*>(output));
            if (output != nullptr) {
                RPC::Administrator::Instance().RegisterInterface(channel, output, output_interfaceid);
            }
        },

        // virtual uint32_t Activate(const IShell::reason) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IShell::reason param0 = reader.Number<IShell::reason>();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const uint32_t output = implementation->Activate(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Deactivate(const IShell::reason) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IShell::reason param0 = reader.Number<IShell::reason>();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const uint32_t output = implementation->Deactivate(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Unavailable(const IShell::reason) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IShell::reason param0 = reader.Number<IShell::reason>();

            // call implementation
            IShell* implementation = reinterpret_cast<IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const uint32_t output = implementation->Unavailable(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual IShell::reason Reason() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const IShell* implementation = reinterpret_cast<const IShell*>(input.Implementation());
            ASSERT((implementation != nullptr) && "Null IShell implementation pointer");
            const IShell::reason output = implementation->Reason();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const IShell::reason>(output);
        },

        // virtual uint32_t Submit(const uint32_t, /* undefined */ const Core::ProxyType<Core::JSON::IElement>&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message VARIABLE_IS_NOT_USED) {
            // RPC::Data::Input& input(message->Parameters());

            // RPC::Data::Frame::Reader reader(input.Reader());
            // RPC::Data::Frame::Writer writer(message->Response().Writer());
            // TODO
        },

        // virtual IShell::ICOMLink* COMLink() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message VARIABLE_IS_NOT_USED) {
            // RPC::Data::Input& input(message->Parameters());

            // RPC::Data::Frame::Writer writer(message->Response().Writer());
            // TODO
        },

        nullptr
    }; // ShellStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IShell interface proxy definitions
    //
    // Methods:
    //  (0) virtual void EnableWebServer(const string&, const string&) = 0
    //  (1) virtual void DisableWebServer() = 0
    //  (2) virtual string Version() const = 0
    //  (3) virtual string Model() const = 0
    //  (4) virtual bool Background() const = 0
    //  (5) virtual string Accessor() const = 0
    //  (6) virtual string WebPrefix() const = 0
    //  (7) virtual string Locator() const = 0
    //  (8) virtual string ClassName() const = 0
    //  (9) virtual string Versions() const = 0
    //  (10) virtual string Callsign() const = 0
    //  (11) virtual string PersistentPath() const = 0
    //  (12) virtual string VolatilePath() const = 0
    //  (13) virtual string DataPath() const = 0
    //  (14) virtual string ProxyStubPath() const = 0
    //  (15) virtual string Substitute(const string&) const = 0
    //  (16) virtual bool AutoStart() const = 0
    //  (17) virtual bool Resumed() const = 0
    //  (18) virtual string HashKey() const = 0
    //  (19) virtual string ConfigLine() const = 0
    //  (20) virtual bool IsSupported(const uint8_t) const = 0
    //  (21) virtual ISubSystem* SubSystems() = 0
    //  (22) virtual void Notify(const string&) = 0
    //  (23) virtual void Register(IPlugin::INotification*) = 0
    //  (24) virtual void Unregister(IPlugin::INotification*) = 0
    //  (25) virtual IShell::state State() const = 0
    //  (26) virtual void* QueryInterfaceByCallsign(const uint32_t, const string&) = 0
    //  (27) virtual uint32_t Activate(const IShell::reason) = 0
    //  (28) virtual uint32_t Deactivate(const IShell::reason) = 0
    //  (29) virtual uint32_t Unavailable(const IShell::reason) = 0
    //  (30) virtual IShell::reason Reason() const = 0
    //  (31) virtual uint32_t Submit(const uint32_t, /* undefined */ const Core::ProxyType<Core::JSON::IElement>&) = 0
    //  (32) virtual IShell::ICOMLink* COMLink() = 0
    //

    class ShellProxy final : public ProxyStub::UnknownProxyType<IShell> {
    public:
        ShellProxy(const Core::ProxyType<Core::IPCChannel>& channel, RPC::instance_id implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void EnableWebServer(const string& param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);

            // invoke the method handler
            Invoke(newMessage);
        }

        void DisableWebServer() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            Invoke(newMessage);
        }

        string Version() const override
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

        string Model() const override
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

        bool Background() const override
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

        string Accessor() const override
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

        string WebPrefix() const override
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

        string Locator() const override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string ClassName() const override
        {
            IPCMessage newMessage(BaseClass::Message(8));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string Versions() const override
        {
            IPCMessage newMessage(BaseClass::Message(9));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string Callsign() const override
        {
            IPCMessage newMessage(BaseClass::Message(10));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string PersistentPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(11));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string VolatilePath() const override
        {
            IPCMessage newMessage(BaseClass::Message(12));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string DataPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(13));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string ProxyStubPath() const override
        {
            IPCMessage newMessage(BaseClass::Message(14));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string Substitute(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(15));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        bool AutoStart() const override
        {
            IPCMessage newMessage(BaseClass::Message(16));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        bool Resumed() const override
        {
            IPCMessage newMessage(BaseClass::Message(17));

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        string HashKey() const override
        {
            IPCMessage newMessage(BaseClass::Message(18));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string ConfigLine() const override
        {
            IPCMessage newMessage(BaseClass::Message(19));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        bool IsSupported(const uint8_t param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(20));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint8_t>(param0);

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        ISubSystem* SubSystems() override
        {
            IPCMessage newMessage(BaseClass::Message(21));

            // invoke the method handler
            ISubSystem* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ISubSystem*>(Interface(reader.Number<RPC::instance_id>(), ISubSystem::ID));
            }

            return output_proxy;
        }

        void Notify(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(22));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void Register(IPlugin::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(23));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IPlugin::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IPlugin::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(24));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<RPC::instance_id>(RPC::instance_cast<IPlugin::INotification*>(param0));

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        IShell::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(25));

            // invoke the method handler
            IShell::state output = static_cast<IShell::state>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IShell::state>();
            }

            return output;
        }

        void* QueryInterfaceByCallsign(const uint32_t param0, const string& param1) override
        {
            IPCMessage newMessage(BaseClass::Message(26));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);
            writer.Text(param1);

            // invoke the method handler
            void* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = Interface(reader.Number<RPC::instance_id>(), param0);
            }

            return output_proxy;
        }

        uint32_t Activate(const IShell::reason param0) override
        {
            IPCMessage newMessage(BaseClass::Message(27));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IShell::reason>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Deactivate(const IShell::reason param0) override
        {
            IPCMessage newMessage(BaseClass::Message(28));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IShell::reason>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Unavailable(const IShell::reason param0) override
        {
            IPCMessage newMessage(BaseClass::Message(29));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IShell::reason>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        IShell::reason Reason() const override
        {
            IPCMessage newMessage(BaseClass::Message(30));

            // invoke the method handler
            IShell::reason output = static_cast<IShell::reason>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IShell::reason>();
            }

            return output;
        }

        uint32_t Submit(const uint32_t param0 VARIABLE_IS_NOT_USED, const Core::ProxyType<Core::JSON::IElement>& param1 VARIABLE_IS_NOT_USED) override
        {
            // IPCMessage newMessage(BaseClass::Message(31));

            // RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            // TODO

            // RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            // TODO

            // Complete(newMessage->Response());
            // TODO

            uint32_t output{Core::ERROR_UNAVAILABLE};
            return (output);
        }

        IShell::ICOMLink* COMLink() override
        {
            // IPCMessage newMessage(BaseClass::Message(32));

            // RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
            // TODO

            // Complete(newMessage->Response());
            // TODO

            IShell::ICOMLink* output{};
            return (output);
        }

    }; // class ShellProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IShell, ShellStubMethods> ShellStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IShell, ShellProxy, ShellStub>();
            }
            ~Instantiation()
            {
                RPC::Administrator::Instance().Recall<IShell>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

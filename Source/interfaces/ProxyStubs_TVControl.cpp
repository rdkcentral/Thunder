//
// generated automatically from "ITVControl.h"
//
// implements RPC proxy stubs for:
//   - class ::WPEFramework::Exchange::IStream
//   - class ::WPEFramework::Exchange::IStream::IControl
//   - class ::WPEFramework::Exchange::IStream::IControl::IGeometry
//   - class ::WPEFramework::Exchange::IStream::IControl::ICallback
//   - class ::WPEFramework::Exchange::IStream::ICallback
//   - class ::WPEFramework::Exchange::IPlayer
//

#include "ITVControl.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace Exchange;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // IStream interface stub definitions
    //
    // Methods:
    //  (0) virtual string Metadata() const = 0
    //  (1) virtual IStream::streamtype Type() const = 0
    //  (2) virtual IStream::drmtype DRM() const = 0
    //  (3) virtual IStream::IControl* Control() = 0
    //  (4) virtual void Callback(IStream::ICallback*) = 0
    //  (5) virtual IStream::state State() const = 0
    //  (6) virtual uint32_t Load(string) = 0
    //

    ProxyStub::MethodHandler StreamStubMethods[] = {
        // virtual string Metadata() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            const string output = implementation->Metadata();

            // write return value
            writer.Text(output);
        },

        // virtual IStream::streamtype Type() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            const IStream::streamtype output = implementation->Type();

            // write return value
            writer.Number<const IStream::streamtype>(output);
        },

        // virtual IStream::drmtype DRM() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            const IStream::drmtype output = implementation->DRM();

            // write return value
            writer.Number<const IStream::drmtype>(output);
        },

        // virtual IStream::IControl* Control() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            IStream::IControl* output = implementation->Control();

            // write return value
            writer.Number<IStream::IControl*>(output);
        },

        // virtual void Callback(IStream::ICallback*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IStream::ICallback* param0 = reader.Number<IStream::ICallback*>();
            IStream::ICallback* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IStream::ICallback::ID, false, IStream::ICallback::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IStream::ICallback>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IStream::ICallback proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IStream::ICallback proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IStream* implementation = input.Implementation<IStream>();
                ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
                implementation->Callback(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual IStream::state State() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            const IStream::state output = implementation->State();

            // write return value
            writer.Number<const IStream::state>(output);
        },

        // virtual uint32_t Load(string) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IStream* implementation = input.Implementation<IStream>();
            ASSERT((implementation != nullptr) && "Null IStream implementation pointer");
            const uint32_t output = implementation->Load(param0);

            // write return value
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // StreamStubMethods[]

    //
    // IStream::IControl interface stub definitions
    //
    // Methods:
    //  (0) virtual void Speed(const int32_t) = 0
    //  (1) virtual int32_t Speed() const = 0
    //  (2) virtual void Position(const uint64_t) = 0
    //  (3) virtual uint64_t Position() const = 0
    //  (4) virtual void TimeRange(uint64_t&, uint64_t&) const = 0
    //  (5) virtual IStream::IControl::IGeometry* Geometry() const = 0
    //  (6) virtual void Geometry(const IStream::IControl::IGeometry*) = 0
    //  (7) virtual void Callback(IStream::IControl::ICallback*) = 0
    //

    ProxyStub::MethodHandler StreamControlStubMethods[] = {
        // virtual void Speed(const int32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const int32_t param0 = reader.Number<int32_t>();

            // call implementation
            IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            implementation->Speed(param0);
        },

        // virtual int32_t Speed() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            const int32_t output = implementation->Speed();

            // write return value
            writer.Number<const int32_t>(output);
        },

        // virtual void Position(const uint64_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint64_t param0 = reader.Number<uint64_t>();

            // call implementation
            IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            implementation->Position(param0);
        },

        // virtual uint64_t Position() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            const uint64_t output = implementation->Position();

            // write return value
            writer.Number<const uint64_t>(output);
        },

        // virtual void TimeRange(uint64_t&, uint64_t&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            uint64_t param0 = reader.Number<uint64_t>();
            uint64_t param1 = reader.Number<uint64_t>();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            implementation->TimeRange(param0, param1);

            // write return values
            writer.Number<uint64_t>(param0);
            writer.Number<uint64_t>(param1);
        },

        // virtual IStream::IControl::IGeometry* Geometry() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl* implementation = input.Implementation<IStream::IControl>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
            IStream::IControl::IGeometry* output = implementation->Geometry();

            // write return value
            writer.Number<IStream::IControl::IGeometry*>(output);
        },

        // virtual void Geometry(const IStream::IControl::IGeometry*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IStream::IControl::IGeometry* param0 = reader.Number<IStream::IControl::IGeometry*>();
            IStream::IControl::IGeometry* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IStream::IControl::IGeometry::ID, false, IStream::IControl::IGeometry::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IStream::IControl::IGeometry>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IStream::IControl::IGeometry proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IStream::IControl::IGeometry proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IStream::IControl* implementation = input.Implementation<IStream::IControl>();
                ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
                implementation->Geometry(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Callback(IStream::IControl::ICallback*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IStream::IControl::ICallback* param0 = reader.Number<IStream::IControl::ICallback*>();
            IStream::IControl::ICallback* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IStream::IControl::ICallback::ID, false, IStream::IControl::ICallback::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IStream::IControl::ICallback>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IStream::IControl::ICallback proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IStream::IControl::ICallback proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IStream::IControl* implementation = input.Implementation<IStream::IControl>();
                ASSERT((implementation != nullptr) && "Null IStream::IControl implementation pointer");
                implementation->Callback(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // StreamControlStubMethods[]

    //
    // IStream::IControl::IGeometry interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t X() const = 0
    //  (1) virtual uint32_t Y() const = 0
    //  (2) virtual uint32_t Z() const = 0
    //  (3) virtual uint32_t Width() const = 0
    //  (4) virtual uint32_t Height() const = 0
    //

    ProxyStub::MethodHandler StreamControlGeometryStubMethods[] = {
        // virtual uint32_t X() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl::IGeometry* implementation = input.Implementation<IStream::IControl::IGeometry>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::IGeometry implementation pointer");
            const uint32_t output = implementation->X();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Y() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl::IGeometry* implementation = input.Implementation<IStream::IControl::IGeometry>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::IGeometry implementation pointer");
            const uint32_t output = implementation->Y();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Z() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl::IGeometry* implementation = input.Implementation<IStream::IControl::IGeometry>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::IGeometry implementation pointer");
            const uint32_t output = implementation->Z();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Width() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl::IGeometry* implementation = input.Implementation<IStream::IControl::IGeometry>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::IGeometry implementation pointer");
            const uint32_t output = implementation->Width();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        // virtual uint32_t Height() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            const IStream::IControl::IGeometry* implementation = input.Implementation<IStream::IControl::IGeometry>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::IGeometry implementation pointer");
            const uint32_t output = implementation->Height();

            // write return value
            writer.Number<const uint32_t>(output);
        },

        nullptr
    }; // StreamControlGeometryStubMethods[]

    //
    // IStream::IControl::ICallback interface stub definitions
    //
    // Methods:
    //  (0) virtual void TimeUpdate(uint64_t) = 0
    //

    ProxyStub::MethodHandler StreamControlCallbackStubMethods[] = {
        // virtual void TimeUpdate(uint64_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint64_t param0 = reader.Number<uint64_t>();

            // call implementation
            IStream::IControl::ICallback* implementation = input.Implementation<IStream::IControl::ICallback>();
            ASSERT((implementation != nullptr) && "Null IStream::IControl::ICallback implementation pointer");
            implementation->TimeUpdate(param0);
        },

        nullptr
    }; // StreamControlCallbackStubMethods[]

    //
    // IStream::ICallback interface stub definitions
    //
    // Methods:
    //  (0) virtual void DRM(uint32_t) = 0
    //  (1) virtual void StateChange(IStream::state) = 0
    //

    ProxyStub::MethodHandler StreamCallbackStubMethods[] = {
        // virtual void DRM(uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint32_t param0 = reader.Number<uint32_t>();

            // call implementation
            IStream::ICallback* implementation = input.Implementation<IStream::ICallback>();
            ASSERT((implementation != nullptr) && "Null IStream::ICallback implementation pointer");
            implementation->DRM(param0);
        },

        // virtual void StateChange(IStream::state) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IStream::state param0 = reader.Number<IStream::state>();

            // call implementation
            IStream::ICallback* implementation = input.Implementation<IStream::ICallback>();
            ASSERT((implementation != nullptr) && "Null IStream::ICallback implementation pointer");
            implementation->StateChange(param0);
        },

        nullptr
    }; // StreamCallbackStubMethods[]

    //
    // IPlayer interface stub definitions
    //
    // Methods:
    //  (0) virtual IStream* CreateStream(IStream::streamtype) = 0
    //  (1) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //

    ProxyStub::MethodHandler PlayerStubMethods[] = {
        // virtual IStream* CreateStream(IStream::streamtype) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const IStream::streamtype param0 = reader.Number<IStream::streamtype>();

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            // call implementation
            IPlayer* implementation = input.Implementation<IPlayer>();
            ASSERT((implementation != nullptr) && "Null IPlayer implementation pointer");
            IStream* output = implementation->CreateStream(param0);

            // write return value
            writer.Number<IStream*>(output);
        },

        // virtual uint32_t Configure(PluginHost::IShell*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            PluginHost::IShell* param0 = reader.Number<PluginHost::IShell*>();
            PluginHost::IShell* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, PluginHost::IShell::ID, false, PluginHost::IShell::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<PluginHost::IShell>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of PluginHost::IShell proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of PluginHost::IShell proxy");
                }
            }

            RPC::Data::Frame::Writer writer(message->Response().Writer());

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IPlayer* implementation = input.Implementation<IPlayer>();
                ASSERT((implementation != nullptr) && "Null IPlayer implementation pointer");
                const uint32_t output = implementation->Configure(param0_proxy);

                // write return value
                writer.Number<const uint32_t>(output);
            } else {
                // return error code
                writer.Number<const uint32_t>(Core::ERROR_RPC_CALL_FAILED);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // PlayerStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // IStream interface proxy definitions
    //
    // Methods:
    //  (0) virtual string Metadata() const = 0
    //  (1) virtual IStream::streamtype Type() const = 0
    //  (2) virtual IStream::drmtype DRM() const = 0
    //  (3) virtual IStream::IControl* Control() = 0
    //  (4) virtual void Callback(IStream::ICallback*) = 0
    //  (5) virtual IStream::state State() const = 0
    //  (6) virtual uint32_t Load(string) = 0
    //

    class StreamProxy final : public ProxyStub::UnknownProxyType<IStream> {
    public:
        StreamProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        string Metadata() const override
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

        IStream::streamtype Type() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            IStream::streamtype output = static_cast<IStream::streamtype>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IStream::streamtype>();
            }

            return output;
        }

        IStream::drmtype DRM() const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            IStream::drmtype output = static_cast<IStream::drmtype>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IStream::drmtype>();
            }

            return output;
        }

        IStream::IControl* Control() override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            IStream::IControl* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<IStream::IControl*>(Interface(reader.Number<void*>(), IStream::IControl::ID));
            }

            return output_proxy;
        }

        void Callback(IStream::ICallback* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStream::ICallback*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        IStream::state State() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            IStream::state output = static_cast<IStream::state>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<IStream::state>();
            }

            return output;
        }

        uint32_t Load(string param0) override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }
    }; // class StreamProxy

    //
    // IStream::IControl interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Speed(const int32_t) = 0
    //  (1) virtual int32_t Speed() const = 0
    //  (2) virtual void Position(const uint64_t) = 0
    //  (3) virtual uint64_t Position() const = 0
    //  (4) virtual void TimeRange(uint64_t&, uint64_t&) const = 0
    //  (5) virtual IStream::IControl::IGeometry* Geometry() const = 0
    //  (6) virtual void Geometry(const IStream::IControl::IGeometry*) = 0
    //  (7) virtual void Callback(IStream::IControl::ICallback*) = 0
    //

    class StreamControlProxy final : public ProxyStub::UnknownProxyType<IStream::IControl> {
    public:
        StreamControlProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Speed(const int32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const int32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        int32_t Speed() const override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            int32_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<int32_t>();
            }

            return output;
        }

        void Position(const uint64_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint64_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        uint64_t Position() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            uint64_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint64_t>();
            }

            return output;
        }

        void TimeRange(uint64_t& param0, uint64_t& param1) const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<uint64_t>(param0);
            writer.Number<uint64_t>(param1);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                param0 = reader.Number<uint64_t>();
                param1 = reader.Number<uint64_t>();
            }
        }

        IStream::IControl::IGeometry* Geometry() const override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // invoke the method handler
            IStream::IControl::IGeometry* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<IStream::IControl::IGeometry*>(Interface(reader.Number<void*>(), IStream::IControl::IGeometry::ID));
            }

            return output_proxy;
        }

        void Geometry(const IStream::IControl::IGeometry* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IStream::IControl::IGeometry*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Callback(IStream::IControl::ICallback* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IStream::IControl::ICallback*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class StreamControlProxy

    //
    // IStream::IControl::IGeometry interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t X() const = 0
    //  (1) virtual uint32_t Y() const = 0
    //  (2) virtual uint32_t Z() const = 0
    //  (3) virtual uint32_t Width() const = 0
    //  (4) virtual uint32_t Height() const = 0
    //

    class StreamControlGeometryProxy final : public ProxyStub::UnknownProxyType<IStream::IControl::IGeometry> {
    public:
        StreamControlGeometryProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t X() const override
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

        uint32_t Y() const override
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

        uint32_t Z() const override
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

        uint32_t Width() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();
            }

            return output;
        }

        uint32_t Height() const override
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
    }; // class StreamControlGeometryProxy

    //
    // IStream::IControl::ICallback interface proxy definitions
    //
    // Methods:
    //  (0) virtual void TimeUpdate(uint64_t) = 0
    //

    class StreamControlCallbackProxy final : public ProxyStub::UnknownProxyType<IStream::IControl::ICallback> {
    public:
        StreamControlCallbackProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void TimeUpdate(uint64_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint64_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class StreamControlCallbackProxy

    //
    // IStream::ICallback interface proxy definitions
    //
    // Methods:
    //  (0) virtual void DRM(uint32_t) = 0
    //  (1) virtual void StateChange(IStream::state) = 0
    //

    class StreamCallbackProxy final : public ProxyStub::UnknownProxyType<IStream::ICallback> {
    public:
        StreamCallbackProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void DRM(uint32_t param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const uint32_t>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void StateChange(IStream::state param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IStream::state>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class StreamCallbackProxy

    //
    // IPlayer interface proxy definitions
    //
    // Methods:
    //  (0) virtual IStream* CreateStream(IStream::streamtype) = 0
    //  (1) virtual uint32_t Configure(PluginHost::IShell*) = 0
    //

    class PlayerProxy final : public ProxyStub::UnknownProxyType<IPlayer> {
    public:
        PlayerProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        IStream* CreateStream(IStream::streamtype param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const IStream::streamtype>(param0);

            // invoke the method handler
            IStream* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<IStream*>(Interface(reader.Number<void*>(), IStream::ID));
            }

            return output_proxy;
        }

        uint32_t Configure(PluginHost::IShell* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<PluginHost::IShell*>(param0);

            // invoke the method handler
            uint32_t output{};
            if ((output = Invoke(newMessage)) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<uint32_t>();

                Complete(reader);
            }

            return output;
        }
    }; // class PlayerProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IStream::ICallback, StreamCallbackStubMethods> StreamCallbackStub;
        typedef ProxyStub::UnknownStubType<IStream::IControl, StreamControlStubMethods> StreamControlStub;
        typedef ProxyStub::UnknownStubType<IStream, StreamStubMethods> StreamStub;
        typedef ProxyStub::UnknownStubType<IStream::IControl::ICallback, StreamControlCallbackStubMethods> StreamControlCallbackStub;
        typedef ProxyStub::UnknownStubType<IPlayer, PlayerStubMethods> PlayerStub;
        typedef ProxyStub::UnknownStubType<IStream::IControl::IGeometry, StreamControlGeometryStubMethods> StreamControlGeometryStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IStream::ICallback, StreamCallbackProxy, StreamCallbackStub>();
                RPC::Administrator::Instance().Announce<IStream::IControl, StreamControlProxy, StreamControlStub>();
                RPC::Administrator::Instance().Announce<IStream, StreamProxy, StreamStub>();
                RPC::Administrator::Instance().Announce<IStream::IControl::ICallback, StreamControlCallbackProxy, StreamControlCallbackStub>();
                RPC::Administrator::Instance().Announce<IPlayer, PlayerProxy, PlayerStub>();
                RPC::Administrator::Instance().Announce<IStream::IControl::IGeometry, StreamControlGeometryProxy, StreamControlGeometryStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace WPEFramework

} // namespace ProxyStubs

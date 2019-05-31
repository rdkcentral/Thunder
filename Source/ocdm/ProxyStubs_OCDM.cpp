//
// generated automatically from "IOCDM.h"
//
// implements RPC proxy stubs for:
//   - class ISession
//   - class ISession::ICallback
//   - class ISessionExt
//   - class IAccessorOCDM
//   - class IAccessorOCDM::INotification
//   - class IAccessorOCDMExt
//

#include "IOCDM.h"

namespace WPEFramework {

namespace ProxyStubs {

    using namespace OCDM;

    // -----------------------------------------------------------------
    // STUB
    // -----------------------------------------------------------------

    //
    // ISession interface stub definitions
    //
    // Methods:
    //  (0) virtual OCDM_RESULT Load() = 0
    //  (1) virtual void Update(const uint8_t*, const uint16_t) = 0
    //  (2) virtual OCDM_RESULT Remove() = 0
    //  (3) virtual ISession::KeyStatus Status() const = 0
    //  (4) virtual string BufferId() const = 0
    //  (5) virtual string SessionId() const = 0
    //  (6) virtual void Close() = 0
    //  (7) virtual void Revoke(ISession::ICallback*) = 0
    //

    ProxyStub::MethodHandler SessionStubMethods[] = {
        // virtual OCDM_RESULT Load() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            const OCDM_RESULT output = implementation->Load();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual void Update(const uint8_t*, const uint16_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint16_t param0_length = reader.LockBuffer<uint16_t>(param0);
            reader.UnlockBuffer(param0_length);

            // call implementation
            ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            implementation->Update(param0, param0_length);
        },

        // virtual OCDM_RESULT Remove() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            const OCDM_RESULT output = implementation->Remove();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual ISession::KeyStatus Status() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            const ISession::KeyStatus output = implementation->Status();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const ISession::KeyStatus>(output);
        },

        // virtual string BufferId() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            const string output = implementation->BufferId();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual string SessionId() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            const string output = implementation->SessionId();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual void Close() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISession* implementation = input.Implementation<ISession>();
            ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
            implementation->Close();
        },

        // virtual void Revoke(ISession::ICallback*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            ISession::ICallback* param0 = reader.Number<ISession::ICallback*>();
            ISession::ICallback* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, ISession::ICallback::ID, false, ISession::ICallback::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<ISession::ICallback>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of ISession::ICallback proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ISession::ICallback proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                ISession* implementation = input.Implementation<ISession>();
                ASSERT((implementation != nullptr) && "Null ISession implementation pointer");
                implementation->Revoke(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        nullptr
    }; // SessionStubMethods[]

    //
    // ISession::ICallback interface stub definitions
    //
    // Methods:
    //  (0) virtual void OnKeyMessage(const uint8_t*, const uint16_t, const string) = 0
    //  (1) virtual void OnKeyReady() = 0
    //  (2) virtual void OnKeyError(const int16_t, const OCDM_RESULT, const string) = 0
    //  (3) virtual void OnKeyStatusUpdate(const ISession::KeyStatus) = 0
    //

    ProxyStub::MethodHandler SessionCallbackStubMethods[] = {
        // virtual void OnKeyMessage(const uint8_t*, const uint16_t, const string) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint16_t param0_length = reader.LockBuffer<uint16_t>(param0);
            reader.UnlockBuffer(param0_length);
            const string param2 = reader.Text();

            // call implementation
            ISession::ICallback* implementation = input.Implementation<ISession::ICallback>();
            ASSERT((implementation != nullptr) && "Null ISession::ICallback implementation pointer");
            implementation->OnKeyMessage(param0, param0_length, param2);
        },

        // virtual void OnKeyReady() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISession::ICallback* implementation = input.Implementation<ISession::ICallback>();
            ASSERT((implementation != nullptr) && "Null ISession::ICallback implementation pointer");
            implementation->OnKeyReady();
        },

        // virtual void OnKeyError(const int16_t, const OCDM_RESULT, const string) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const int16_t param0 = reader.Number<int16_t>();
            const OCDM_RESULT param1 = reader.Number<OCDM_RESULT>();
            const string param2 = reader.Text();

            // call implementation
            ISession::ICallback* implementation = input.Implementation<ISession::ICallback>();
            ASSERT((implementation != nullptr) && "Null ISession::ICallback implementation pointer");
            implementation->OnKeyError(param0, param1, param2);
        },

        // virtual void OnKeyStatusUpdate(const ISession::KeyStatus) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const ISession::KeyStatus param0 = reader.Number<ISession::KeyStatus>();

            // call implementation
            ISession::ICallback* implementation = input.Implementation<ISession::ICallback>();
            ASSERT((implementation != nullptr) && "Null ISession::ICallback implementation pointer");
            implementation->OnKeyStatusUpdate(param0);
        },

        nullptr
    }; // SessionCallbackStubMethods[]

    //
    // ISessionExt interface stub definitions
    //
    // Methods:
    //  (0) virtual uint32_t SessionIdExt() const = 0
    //  (1) virtual string BufferIdExt() const = 0
    //  (2) virtual OCDM_RESULT SetDrmHeader(const uint8_t*, uint32_t) = 0
    //  (3) virtual OCDM_RESULT GetChallengeDataExt(uint8_t*, uint32_t&, uint32_t) = 0
    //  (4) virtual OCDM_RESULT CancelChallengeDataExt() = 0
    //  (5) virtual OCDM_RESULT StoreLicenseData(const uint8_t*, uint32_t, uint8_t*) = 0
    //  (6) virtual OCDM_RESULT InitDecryptContextByKid() = 0
    //  (7) virtual OCDM_RESULT CleanDecryptContext() = 0
    //

    ProxyStub::MethodHandler SessionExtStubMethods[] = {
        // virtual uint32_t SessionIdExt() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const uint32_t output = implementation->SessionIdExt();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual string BufferIdExt() const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            const ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const string output = implementation->BufferIdExt();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual OCDM_RESULT SetDrmHeader(const uint8_t*, uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint32_t param0_length = reader.LockBuffer<uint32_t>(param0);
            reader.UnlockBuffer(param0_length);

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->SetDrmHeader(param0, param0_length);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT GetChallengeDataExt(uint8_t* [inout], uint32_t& [inout], uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint32_t param0_length = reader.LockBuffer<uint32_t>(param0);
            reader.UnlockBuffer(param0_length);
            const uint32_t param2 = reader.Number<uint32_t>();

            uint8_t* param0_buffer{};
            param0_buffer = const_cast<uint8_t*>(param0); // reuse the input buffer

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->GetChallengeDataExt(param0_buffer, param0_length, param2);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint32_t>(param0_length, param0_buffer);
        },

        // virtual OCDM_RESULT CancelChallengeDataExt() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->CancelChallengeDataExt();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT StoreLicenseData(const uint8_t*, uint32_t, uint8_t* [out]) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint32_t param0_length = reader.LockBuffer<uint32_t>(param0);
            reader.UnlockBuffer(param0_length);

            // allocate receive buffer
            uint8_t* param2{};
            const uint8_t param2_length = 16;
            param2 = static_cast<uint8_t*>(ALLOCA(param2_length));
            ASSERT(param2 != nullptr);

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->StoreLicenseData(param0, param0_length, param2);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint8_t>(param2_length, param2);
        },

        // virtual OCDM_RESULT InitDecryptContextByKid() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->InitDecryptContextByKid();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT CleanDecryptContext() = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // call implementation
            ISessionExt* implementation = input.Implementation<ISessionExt>();
            ASSERT((implementation != nullptr) && "Null ISessionExt implementation pointer");
            const OCDM_RESULT output = implementation->CleanDecryptContext();

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        nullptr
    }; // SessionExtStubMethods[]

    //
    // IAccessorOCDM interface stub definitions
    //
    // Methods:
    //  (0) virtual bool IsTypeSupported(const string, const string) const = 0
    //  (1) virtual OCDM_RESULT CreateSession(const string, const int32_t, const string, const uint8_t*, const uint16_t, const uint8_t*, const uint16_t, ISession::ICallback*, string&, ISession*&) = 0
    //  (2) virtual OCDM_RESULT SetServerCertificate(const string, const uint8_t*, const uint16_t) = 0
    //  (3) virtual void Register(IAccessorOCDM::INotification*) = 0
    //  (4) virtual void Unregister(IAccessorOCDM::INotification*) = 0
    //  (5) virtual ISession* Session(const string) = 0
    //  (6) virtual ISession* Session(const uint8_t*, const uint8_t) = 0
    //

    ProxyStub::MethodHandler AccessorOCDMStubMethods[] = {
        // virtual bool IsTypeSupported(const string, const string) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const string param1 = reader.Text();

            // call implementation
            const IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
            const bool output = implementation->IsTypeSupported(param0, param1);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual OCDM_RESULT CreateSession(const string, const int32_t, const string, const uint8_t*, const uint16_t, const uint8_t*, const uint16_t, ISession::ICallback*, string& [out], ISession*& [out]) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const int32_t param1 = reader.Number<int32_t>();
            const string param2 = reader.Text();
            const uint8_t* param3 = nullptr;
            uint16_t param3_length = reader.LockBuffer<uint16_t>(param3);
            reader.UnlockBuffer(param3_length);
            const uint8_t* param5 = nullptr;
            uint16_t param5_length = reader.LockBuffer<uint16_t>(param5);
            reader.UnlockBuffer(param5_length);
            ISession::ICallback* param7 = reader.Number<ISession::ICallback*>();
            string param8{}; // storage
            ISession* param9{}; // storage
            ISession::ICallback* param7_proxy = nullptr;
            ProxyStub::UnknownProxy* param7_proxy_inst = nullptr;
            if (param7 != nullptr) {
                param7_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param7, ISession::ICallback::ID, false, ISession::ICallback::ID, true);
                if (param7_proxy_inst != nullptr) {
                    param7_proxy = param7_proxy_inst->QueryInterface<ISession::ICallback>();
                }

                ASSERT((param7_proxy != nullptr) && "Failed to get instance of ISession::ICallback proxy");
                if (param7_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ISession::ICallback proxy");
                }
            }

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            if ((param7 == nullptr) || (param7_proxy != nullptr)) {
                // call implementation
                IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
                ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
                const OCDM_RESULT output = implementation->CreateSession(param0, param1, param2, param3, param3_length, param5, param5_length, param7_proxy, param8, param9);
                writer.Number<const OCDM_RESULT>(output);
                writer.Text(param8);
                writer.Number<ISession*>(param9);
            }

            if (param7_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param7_proxy_inst, message->Response());
            }
        },

        // virtual OCDM_RESULT SetServerCertificate(const string, const uint8_t*, const uint16_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t* param1 = nullptr;
            uint16_t param1_length = reader.LockBuffer<uint16_t>(param1);
            reader.UnlockBuffer(param1_length);

            // call implementation
            IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
            const OCDM_RESULT output = implementation->SetServerCertificate(param0, param1, param1_length);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual void Register(IAccessorOCDM::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IAccessorOCDM::INotification* param0 = reader.Number<IAccessorOCDM::INotification*>();
            IAccessorOCDM::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IAccessorOCDM::INotification::ID, false, IAccessorOCDM::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IAccessorOCDM::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IAccessorOCDM::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IAccessorOCDM::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
                ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
                implementation->Register(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual void Unregister(IAccessorOCDM::INotification*) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            IAccessorOCDM::INotification* param0 = reader.Number<IAccessorOCDM::INotification*>();
            IAccessorOCDM::INotification* param0_proxy = nullptr;
            ProxyStub::UnknownProxy* param0_proxy_inst = nullptr;
            if (param0 != nullptr) {
                param0_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param0, IAccessorOCDM::INotification::ID, false, IAccessorOCDM::INotification::ID, true);
                if (param0_proxy_inst != nullptr) {
                    param0_proxy = param0_proxy_inst->QueryInterface<IAccessorOCDM::INotification>();
                }

                ASSERT((param0_proxy != nullptr) && "Failed to get instance of IAccessorOCDM::INotification proxy");
                if (param0_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of IAccessorOCDM::INotification proxy");
                }
            }

            if ((param0 == nullptr) || (param0_proxy != nullptr)) {
                // call implementation
                IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
                ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
                implementation->Unregister(param0_proxy);
            }

            if (param0_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param0_proxy_inst, message->Response());
            }
        },

        // virtual ISession* Session(const string) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
            ISession* output = implementation->Session(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<ISession*>(output);
        },

        // virtual ISession* Session(const uint8_t*, const uint8_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const uint8_t* param0 = nullptr;
            uint8_t param0_length = reader.LockBuffer<uint8_t>(param0);
            reader.UnlockBuffer(param0_length);

            // call implementation
            IAccessorOCDM* implementation = input.Implementation<IAccessorOCDM>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM implementation pointer");
            ISession* output = implementation->Session(param0, param0_length);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<ISession*>(output);
        },

        nullptr
    }; // AccessorOCDMStubMethods[]

    //
    // IAccessorOCDM::INotification interface stub definitions
    //
    // Methods:
    //  (0) virtual void Create(const string&) = 0
    //  (1) virtual void Destroy(const string&) = 0
    //  (2) virtual void KeyChange(const string&, const uint8_t*, const uint8_t, const ISession::KeyStatus) = 0
    //

    ProxyStub::MethodHandler AccessorOCDMNotificationStubMethods[] = {
        // virtual void Create(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDM::INotification* implementation = input.Implementation<IAccessorOCDM::INotification>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM::INotification implementation pointer");
            implementation->Create(param0);
        },

        // virtual void Destroy(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDM::INotification* implementation = input.Implementation<IAccessorOCDM::INotification>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM::INotification implementation pointer");
            implementation->Destroy(param0);
        },

        // virtual void KeyChange(const string&, const uint8_t*, const uint8_t, const ISession::KeyStatus) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t* param1 = nullptr;
            uint8_t param1_length = reader.LockBuffer<uint8_t>(param1);
            reader.UnlockBuffer(param1_length);
            const ISession::KeyStatus param3 = reader.Number<ISession::KeyStatus>();

            // call implementation
            IAccessorOCDM::INotification* implementation = input.Implementation<IAccessorOCDM::INotification>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDM::INotification implementation pointer");
            implementation->KeyChange(param0, param1, param1_length, param3);
        },

        nullptr
    }; // AccessorOCDMNotificationStubMethods[]

    //
    // IAccessorOCDMExt interface stub definitions
    //
    // Methods:
    //  (0) virtual time_t GetDrmSystemTime(const string&) const = 0
    //  (1) virtual OCDM_RESULT CreateSessionExt(const string, const uint8_t*, uint32_t, ISession::ICallback*, string&, ISessionExt*&) = 0
    //  (2) virtual string GetVersionExt(const string&) const = 0
    //  (3) virtual uint32_t GetLdlSessionLimit(const string&) const = 0
    //  (4) virtual bool IsSecureStopEnabled(const string&) = 0
    //  (5) virtual OCDM_RESULT EnableSecureStop(const string&, bool) = 0
    //  (6) virtual uint32_t ResetSecureStops(const string&) = 0
    //  (7) virtual OCDM_RESULT GetSecureStopIds(const string&, uint8_t*, uint8_t, uint32_t&) = 0
    //  (8) virtual OCDM_RESULT GetSecureStop(const string&, const uint8_t*, uint32_t, uint8_t*, uint16_t&) = 0
    //  (9) virtual OCDM_RESULT CommitSecureStop(const string&, const uint8_t*, uint32_t, const uint8_t*, uint32_t) = 0
    //  (10) virtual OCDM_RESULT CreateSystemExt(const string&) = 0
    //  (11) virtual OCDM_RESULT InitSystemExt(const string&) = 0
    //  (12) virtual OCDM_RESULT TeardownSystemExt(const string&) = 0
    //  (13) virtual OCDM_RESULT DeleteKeyStore(const string&) = 0
    //  (14) virtual OCDM_RESULT DeleteSecureStore(const string&) = 0
    //  (15) virtual OCDM_RESULT GetKeyStoreHash(const string&, uint8_t*, uint32_t) = 0
    //  (16) virtual OCDM_RESULT GetSecureStoreHash(const string&, uint8_t*, uint32_t) = 0
    //

    ProxyStub::MethodHandler AccessorOCDMExtStubMethods[] = {
        // virtual time_t GetDrmSystemTime(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            const IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const time_t output = implementation->GetDrmSystemTime(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const time_t>(output);
        },

        // virtual OCDM_RESULT CreateSessionExt(const string, const uint8_t*, uint32_t, ISession::ICallback*, string& [out], ISessionExt*& [out]) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t* param1 = nullptr;
            uint32_t param1_length = reader.LockBuffer<uint32_t>(param1);
            reader.UnlockBuffer(param1_length);
            ISession::ICallback* param3 = reader.Number<ISession::ICallback*>();
            string param4{}; // storage
            ISessionExt* param5{}; // storage
            ISession::ICallback* param3_proxy = nullptr;
            ProxyStub::UnknownProxy* param3_proxy_inst = nullptr;
            if (param3 != nullptr) {
                param3_proxy_inst = RPC::Administrator::Instance().ProxyInstance(channel, param3, ISession::ICallback::ID, false, ISession::ICallback::ID, true);
                if (param3_proxy_inst != nullptr) {
                    param3_proxy = param3_proxy_inst->QueryInterface<ISession::ICallback>();
                }

                ASSERT((param3_proxy != nullptr) && "Failed to get instance of ISession::ICallback proxy");
                if (param3_proxy == nullptr) {
                    TRACE_L1("Failed to get instance of ISession::ICallback proxy");
                }
            }

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());

            if ((param3 == nullptr) || (param3_proxy != nullptr)) {
                // call implementation
                IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
                ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
                const OCDM_RESULT output = implementation->CreateSessionExt(param0, param1, param1_length, param3_proxy, param4, param5);
                writer.Number<const OCDM_RESULT>(output);
                writer.Text(param4);
                writer.Number<ISessionExt*>(param5);
            }

            if (param3_proxy_inst != nullptr) {
                RPC::Administrator::Instance().Release(param3_proxy_inst, message->Response());
            }
        },

        // virtual string GetVersionExt(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            const IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const string output = implementation->GetVersionExt(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Text(output);
        },

        // virtual uint32_t GetLdlSessionLimit(const string&) const = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            const IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const uint32_t output = implementation->GetLdlSessionLimit(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual bool IsSecureStopEnabled(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const bool output = implementation->IsSecureStopEnabled(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Boolean(output);
        },

        // virtual OCDM_RESULT EnableSecureStop(const string&, bool) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const bool param1 = reader.Boolean();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->EnableSecureStop(param0, param1);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual uint32_t ResetSecureStops(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const uint32_t output = implementation->ResetSecureStops(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const uint32_t>(output);
        },

        // virtual OCDM_RESULT GetSecureStopIds(const string&, uint8_t* [out], uint8_t, uint32_t& [inout]) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t param1_length = reader.Number<uint8_t>();
            uint32_t param3 = reader.Number<uint32_t>();

            // allocate receive buffer
            uint8_t* param1{};
            if (param1_length != 0) {
                param1 = static_cast<uint8_t*>(ALLOCA(param1_length));
                ASSERT(param1 != nullptr);
            }

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->GetSecureStopIds(param0, param1, param1_length, param3);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint8_t>(param1_length, param1);
            writer.Number<uint32_t>(param3);
        },

        // virtual OCDM_RESULT GetSecureStop(const string&, const uint8_t*, uint32_t, uint8_t* [out], uint16_t& [inout]) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t* param1 = nullptr;
            uint32_t param1_length = reader.LockBuffer<uint32_t>(param1);
            reader.UnlockBuffer(param1_length);
            uint16_t param3_length = reader.Number<uint16_t>();

            // allocate receive buffer
            uint8_t* param3{};
            if (param3_length != 0) {
                param3 = static_cast<uint8_t*>(ALLOCA(param3_length));
                ASSERT(param3 != nullptr);
            }

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->GetSecureStop(param0, param1, param1_length, param3, param3_length);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint16_t>(param3_length, param3);
        },

        // virtual OCDM_RESULT CommitSecureStop(const string&, const uint8_t*, uint32_t, const uint8_t*, uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint8_t* param1 = nullptr;
            uint32_t param1_length = reader.LockBuffer<uint32_t>(param1);
            reader.UnlockBuffer(param1_length);
            const uint8_t* param3 = nullptr;
            uint32_t param3_length = reader.LockBuffer<uint32_t>(param3);
            reader.UnlockBuffer(param3_length);

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->CommitSecureStop(param0, param1, param1_length, param3, param3_length);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT CreateSystemExt(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->CreateSystemExt(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT InitSystemExt(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->InitSystemExt(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT TeardownSystemExt(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->TeardownSystemExt(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT DeleteKeyStore(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->DeleteKeyStore(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT DeleteSecureStore(const string&) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->DeleteSecureStore(param0);

            // write return value
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
        },

        // virtual OCDM_RESULT GetKeyStoreHash(const string&, uint8_t* [out], uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint32_t param1_length = reader.Number<uint32_t>();

            // allocate receive buffer
            uint8_t* param1{};
            if (param1_length != 0) {
                ASSERT((param1_length < 0x10000) && "Buffer length too big");
                param1 = static_cast<uint8_t*>(ALLOCA(param1_length));
                ASSERT(param1 != nullptr);
            }

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->GetKeyStoreHash(param0, param1, param1_length);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint32_t>(param1_length, param1);
        },

        // virtual OCDM_RESULT GetSecureStoreHash(const string&, uint8_t* [out], uint32_t) = 0
        //
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            RPC::Data::Input& input(message->Parameters());

            // read parameters
            RPC::Data::Frame::Reader reader(input.Reader());
            const string param0 = reader.Text();
            const uint32_t param1_length = reader.Number<uint32_t>();

            // allocate receive buffer
            uint8_t* param1{};
            if (param1_length != 0) {
                ASSERT((param1_length < 0x10000) && "Buffer length too big");
                param1 = static_cast<uint8_t*>(ALLOCA(param1_length));
                ASSERT(param1 != nullptr);
            }

            // call implementation
            IAccessorOCDMExt* implementation = input.Implementation<IAccessorOCDMExt>();
            ASSERT((implementation != nullptr) && "Null IAccessorOCDMExt implementation pointer");
            const OCDM_RESULT output = implementation->GetSecureStoreHash(param0, param1, param1_length);

            // write return values
            RPC::Data::Frame::Writer writer(message->Response().Writer());
            writer.Number<const OCDM_RESULT>(output);
            writer.Buffer<uint32_t>(param1_length, param1);
        },

        nullptr
    }; // AccessorOCDMExtStubMethods[]

    // -----------------------------------------------------------------
    // PROXY
    // -----------------------------------------------------------------

    //
    // ISession interface proxy definitions
    //
    // Methods:
    //  (0) virtual OCDM_RESULT Load() = 0
    //  (1) virtual void Update(const uint8_t*, const uint16_t) = 0
    //  (2) virtual OCDM_RESULT Remove() = 0
    //  (3) virtual ISession::KeyStatus Status() const = 0
    //  (4) virtual string BufferId() const = 0
    //  (5) virtual string SessionId() const = 0
    //  (6) virtual void Close() = 0
    //  (7) virtual void Revoke(ISession::ICallback*) = 0
    //

    class SessionProxy final : public ProxyStub::UnknownProxyType<ISession> {
    public:
        SessionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        OCDM_RESULT Load() override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        void Update(const uint8_t* param0, const uint16_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint16_t>(param1, param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        OCDM_RESULT Remove() override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        ISession::KeyStatus Status() const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // invoke the method handler
            ISession::KeyStatus output = static_cast<ISession::KeyStatus>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<ISession::KeyStatus>();
            }

            return output;
        }

        string BufferId() const override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            string output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Text();
            }

            return output;
        }

        string SessionId() const override
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

        void Close() override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // invoke the method handler
            Invoke(newMessage);
        }

        void Revoke(ISession::ICallback* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<ISession::ICallback*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }
    }; // class SessionProxy

    //
    // ISession::ICallback interface proxy definitions
    //
    // Methods:
    //  (0) virtual void OnKeyMessage(const uint8_t*, const uint16_t, const string) = 0
    //  (1) virtual void OnKeyReady() = 0
    //  (2) virtual void OnKeyError(const int16_t, const OCDM_RESULT, const string) = 0
    //  (3) virtual void OnKeyStatusUpdate(const ISession::KeyStatus) = 0
    //

    class SessionCallbackProxy final : public ProxyStub::UnknownProxyType<ISession::ICallback> {
    public:
        SessionCallbackProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void OnKeyMessage(const uint8_t* param0, const uint16_t param1, const string param2) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint16_t>(param1, param0);
            writer.Text(param2);

            // invoke the method handler
            Invoke(newMessage);
        }

        void OnKeyReady() override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // invoke the method handler
            Invoke(newMessage);
        }

        void OnKeyError(const int16_t param0, const OCDM_RESULT param1, const string param2) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const int16_t>(param0);
            writer.Number<const OCDM_RESULT>(param1);
            writer.Text(param2);

            // invoke the method handler
            Invoke(newMessage);
        }

        void OnKeyStatusUpdate(const ISession::KeyStatus param0) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<const ISession::KeyStatus>(param0);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class SessionCallbackProxy

    //
    // ISessionExt interface proxy definitions
    //
    // Methods:
    //  (0) virtual uint32_t SessionIdExt() const = 0
    //  (1) virtual string BufferIdExt() const = 0
    //  (2) virtual OCDM_RESULT SetDrmHeader(const uint8_t*, uint32_t) = 0
    //  (3) virtual OCDM_RESULT GetChallengeDataExt(uint8_t*, uint32_t&, uint32_t) = 0
    //  (4) virtual OCDM_RESULT CancelChallengeDataExt() = 0
    //  (5) virtual OCDM_RESULT StoreLicenseData(const uint8_t*, uint32_t, uint8_t*) = 0
    //  (6) virtual OCDM_RESULT InitDecryptContextByKid() = 0
    //  (7) virtual OCDM_RESULT CleanDecryptContext() = 0
    //

    class SessionExtProxy final : public ProxyStub::UnknownProxyType<ISessionExt> {
    public:
        SessionExtProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        uint32_t SessionIdExt() const override
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

        string BufferIdExt() const override
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

        OCDM_RESULT SetDrmHeader(const uint8_t* param0, uint32_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint32_t>(param1, param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT GetChallengeDataExt(uint8_t* /* inout */ param0, uint32_t& /* inout */ param1, uint32_t param2) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint32_t>(param1, param0);
            writer.Number<const uint32_t>(param2);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint32_t>(param1, param0);
            }

            return output;
        }

        OCDM_RESULT CancelChallengeDataExt() override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT StoreLicenseData(const uint8_t* param0, uint32_t param1, uint8_t* /* out */ param2) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint32_t>(param1, param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint8_t>(16, param2);
            }

            return output;
        }

        OCDM_RESULT InitDecryptContextByKid() override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT CleanDecryptContext() override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }
    }; // class SessionExtProxy

    //
    // IAccessorOCDM interface proxy definitions
    //
    // Methods:
    //  (0) virtual bool IsTypeSupported(const string, const string) const = 0
    //  (1) virtual OCDM_RESULT CreateSession(const string, const int32_t, const string, const uint8_t*, const uint16_t, const uint8_t*, const uint16_t, ISession::ICallback*, string&, ISession*&) = 0
    //  (2) virtual OCDM_RESULT SetServerCertificate(const string, const uint8_t*, const uint16_t) = 0
    //  (3) virtual void Register(IAccessorOCDM::INotification*) = 0
    //  (4) virtual void Unregister(IAccessorOCDM::INotification*) = 0
    //  (5) virtual ISession* Session(const string) = 0
    //  (6) virtual ISession* Session(const uint8_t*, const uint8_t) = 0
    //

    class AccessorOCDMProxy final : public ProxyStub::UnknownProxyType<IAccessorOCDM> {
    public:
        AccessorOCDMProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        bool IsTypeSupported(const string param0, const string param1) const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Text(param1);

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        OCDM_RESULT CreateSession(const string param0, const int32_t param1, const string param2, const uint8_t* param3, const uint16_t param4, const uint8_t* param5, const uint16_t param6, ISession::ICallback* param7, string& /* out */ param8, ISession*& /* out */ param9) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<const int32_t>(param1);
            writer.Text(param2);
            writer.Buffer<uint16_t>(param4, param3);
            writer.Buffer<uint16_t>(param6, param5);
            writer.Number<ISession::ICallback*>(param7);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                param8 = reader.Text();
                param9 = reinterpret_cast<ISession*>(Interface(reader.Number<void*>(), ISession::ID));

                Complete(reader);
            }

            return output;
        }

        OCDM_RESULT SetServerCertificate(const string param0, const uint8_t* param1, const uint16_t param2) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Buffer<uint16_t>(param2, param1);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        void Register(IAccessorOCDM::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(3));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IAccessorOCDM::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        void Unregister(IAccessorOCDM::INotification* param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number<IAccessorOCDM::INotification*>(param0);

            // invoke the method handler
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                Complete(reader);
            }
        }

        ISession* Session(const string param0) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            ISession* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ISession*>(Interface(reader.Number<void*>(), ISession::ID));
            }

            return output_proxy;
        }

        ISession* Session(const uint8_t* param0, const uint8_t param1) override
        {
            IPCMessage newMessage(BaseClass::Message(6));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer<uint8_t>(param1, param0);

            // invoke the method handler
            ISession* output_proxy{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output_proxy = reinterpret_cast<ISession*>(Interface(reader.Number<void*>(), ISession::ID));
            }

            return output_proxy;
        }
    }; // class AccessorOCDMProxy

    //
    // IAccessorOCDM::INotification interface proxy definitions
    //
    // Methods:
    //  (0) virtual void Create(const string&) = 0
    //  (1) virtual void Destroy(const string&) = 0
    //  (2) virtual void KeyChange(const string&, const uint8_t*, const uint8_t, const ISession::KeyStatus) = 0
    //

    class AccessorOCDMNotificationProxy final : public ProxyStub::UnknownProxyType<IAccessorOCDM::INotification> {
    public:
        AccessorOCDMNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        void Create(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void Destroy(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            Invoke(newMessage);
        }

        void KeyChange(const string& param0, const uint8_t* param1, const uint8_t param2, const ISession::KeyStatus param3) override
        {
            IPCMessage newMessage(BaseClass::Message(2));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Buffer<uint8_t>(param2, param1);
            writer.Number<const ISession::KeyStatus>(param3);

            // invoke the method handler
            Invoke(newMessage);
        }
    }; // class AccessorOCDMNotificationProxy

    //
    // IAccessorOCDMExt interface proxy definitions
    //
    // Methods:
    //  (0) virtual time_t GetDrmSystemTime(const string&) const = 0
    //  (1) virtual OCDM_RESULT CreateSessionExt(const string, const uint8_t*, uint32_t, ISession::ICallback*, string&, ISessionExt*&) = 0
    //  (2) virtual string GetVersionExt(const string&) const = 0
    //  (3) virtual uint32_t GetLdlSessionLimit(const string&) const = 0
    //  (4) virtual bool IsSecureStopEnabled(const string&) = 0
    //  (5) virtual OCDM_RESULT EnableSecureStop(const string&, bool) = 0
    //  (6) virtual uint32_t ResetSecureStops(const string&) = 0
    //  (7) virtual OCDM_RESULT GetSecureStopIds(const string&, uint8_t*, uint8_t, uint32_t&) = 0
    //  (8) virtual OCDM_RESULT GetSecureStop(const string&, const uint8_t*, uint32_t, uint8_t*, uint16_t&) = 0
    //  (9) virtual OCDM_RESULT CommitSecureStop(const string&, const uint8_t*, uint32_t, const uint8_t*, uint32_t) = 0
    //  (10) virtual OCDM_RESULT CreateSystemExt(const string&) = 0
    //  (11) virtual OCDM_RESULT InitSystemExt(const string&) = 0
    //  (12) virtual OCDM_RESULT TeardownSystemExt(const string&) = 0
    //  (13) virtual OCDM_RESULT DeleteKeyStore(const string&) = 0
    //  (14) virtual OCDM_RESULT DeleteSecureStore(const string&) = 0
    //  (15) virtual OCDM_RESULT GetKeyStoreHash(const string&, uint8_t*, uint32_t) = 0
    //  (16) virtual OCDM_RESULT GetSecureStoreHash(const string&, uint8_t*, uint32_t) = 0
    //

    class AccessorOCDMExtProxy final : public ProxyStub::UnknownProxyType<IAccessorOCDMExt> {
    public:
        AccessorOCDMExtProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        time_t GetDrmSystemTime(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(0));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            time_t output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<time_t>();
            }

            return output;
        }

        OCDM_RESULT CreateSessionExt(const string param0, const uint8_t* param1, uint32_t param2, ISession::ICallback* param3, string& /* out */ param4, ISessionExt*& /* out */ param5) override
        {
            IPCMessage newMessage(BaseClass::Message(1));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Buffer<uint32_t>(param2, param1);
            writer.Number<ISession::ICallback*>(param3);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                param4 = reader.Text();
                param5 = reinterpret_cast<ISessionExt*>(Interface(reader.Number<void*>(), ISessionExt::ID));

                Complete(reader);
            }

            return output;
        }

        string GetVersionExt(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(2));

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

        uint32_t GetLdlSessionLimit(const string& param0) const override
        {
            IPCMessage newMessage(BaseClass::Message(3));

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

        bool IsSecureStopEnabled(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(4));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            bool output{};
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Boolean();
            }

            return output;
        }

        OCDM_RESULT EnableSecureStop(const string& param0, bool param1) override
        {
            IPCMessage newMessage(BaseClass::Message(5));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Boolean(param1);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        uint32_t ResetSecureStops(const string& param0) override
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

        OCDM_RESULT GetSecureStopIds(const string& param0, uint8_t* /* out */ param1, uint8_t param2, uint32_t& /* inout */ param3) override
        {
            IPCMessage newMessage(BaseClass::Message(7));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<const uint8_t>(param2);
            writer.Number<uint32_t>(param3);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint8_t>(param2, param1);
                param3 = reader.Number<uint32_t>();
            }

            return output;
        }

        OCDM_RESULT GetSecureStop(const string& param0, const uint8_t* param1, uint32_t param2, uint8_t* /* out */ param3, uint16_t& /* inout */ param4) override
        {
            IPCMessage newMessage(BaseClass::Message(8));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Buffer<uint32_t>(param2, param1);
            writer.Number<uint16_t>(param4);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint16_t>(param4, param3);
            }

            return output;
        }

        OCDM_RESULT CommitSecureStop(const string& param0, const uint8_t* param1, uint32_t param2, const uint8_t* param3, uint32_t param4) override
        {
            IPCMessage newMessage(BaseClass::Message(9));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Buffer<uint32_t>(param2, param1);
            writer.Buffer<uint32_t>(param4, param3);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT CreateSystemExt(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(10));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT InitSystemExt(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(11));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT TeardownSystemExt(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(12));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT DeleteKeyStore(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(13));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT DeleteSecureStore(const string& param0) override
        {
            IPCMessage newMessage(BaseClass::Message(14));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return value
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
            }

            return output;
        }

        OCDM_RESULT GetKeyStoreHash(const string& param0, uint8_t* /* out */ param1, uint32_t param2) override
        {
            IPCMessage newMessage(BaseClass::Message(15));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<const uint32_t>(param2);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint32_t>(param2, param1);
            }

            return output;
        }

        OCDM_RESULT GetSecureStoreHash(const string& param0, uint8_t* /* out */ param1, uint32_t param2) override
        {
            IPCMessage newMessage(BaseClass::Message(16));

            // write parameters
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(param0);
            writer.Number<const uint32_t>(param2);

            // invoke the method handler
            OCDM_RESULT output = static_cast<OCDM_RESULT>(~0);
            if (Invoke(newMessage) == Core::ERROR_NONE) {
                // read return values
                RPC::Data::Frame::Reader reader(newMessage->Response().Reader());
                output = reader.Number<OCDM_RESULT>();
                reader.Buffer<uint32_t>(param2, param1);
            }

            return output;
        }
    }; // class AccessorOCDMExtProxy

    // -----------------------------------------------------------------
    // REGISTRATION
    // -----------------------------------------------------------------

    namespace {

        typedef ProxyStub::UnknownStubType<IAccessorOCDM, AccessorOCDMStubMethods> AccessorOCDMStub;
        typedef ProxyStub::UnknownStubType<IAccessorOCDMExt, AccessorOCDMExtStubMethods> AccessorOCDMExtStub;
        typedef ProxyStub::UnknownStubType<ISession::ICallback, SessionCallbackStubMethods> SessionCallbackStub;
        typedef ProxyStub::UnknownStubType<ISessionExt, SessionExtStubMethods> SessionExtStub;
        typedef ProxyStub::UnknownStubType<IAccessorOCDM::INotification, AccessorOCDMNotificationStubMethods> AccessorOCDMNotificationStub;
        typedef ProxyStub::UnknownStubType<ISession, SessionStubMethods> SessionStub;

        static class Instantiation {
        public:
            Instantiation()
            {
                RPC::Administrator::Instance().Announce<IAccessorOCDM, AccessorOCDMProxy, AccessorOCDMStub>();
                RPC::Administrator::Instance().Announce<IAccessorOCDMExt, AccessorOCDMExtProxy, AccessorOCDMExtStub>();
                RPC::Administrator::Instance().Announce<ISession::ICallback, SessionCallbackProxy, SessionCallbackStub>();
                RPC::Administrator::Instance().Announce<ISessionExt, SessionExtProxy, SessionExtStub>();
                RPC::Administrator::Instance().Announce<IAccessorOCDM::INotification, AccessorOCDMNotificationProxy, AccessorOCDMNotificationStub>();
                RPC::Administrator::Instance().Announce<ISession, SessionProxy, SessionStub>();
            }
        } ProxyStubRegistration;

    } // namespace

} // namespace ProxyStubs

}

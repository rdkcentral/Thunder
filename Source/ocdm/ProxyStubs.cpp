#include "Module.h"
#include "IOCDM.h"
#include "open_cdm.h"

namespace WPEFramework {

    // -------------------------------------------------------------------------------------------
    // STUB
    // -------------------------------------------------------------------------------------------

    //
    // OCDM::IAccessorOCDM interface stub definitions (interface/ICDM.h)
    //
    ProxyStub::MethodHandler AccesorOCDMStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual OCDM_RESULT IsTypeSupported(
            //     const std::string keySystem,
            //     const std::string mimeType) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            std::string keySystem = parameters.Text();
            std::string mimeType = parameters.Text();

            response.Number(message->Parameters().Implementation<OCDM::IAccessorOCDM>()->IsTypeSupported(keySystem, mimeType));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // static OCDM::ISession* Session(
            //     const std::string sessionId) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            std::string sessionId = parameters.Text();

            response.Number<OCDM::ISession*> (message->Parameters().Implementation<OCDM::IAccessorOCDM>()->Session(sessionId));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // static OCDM::ISession* Session(
            //     const uint8 keyId[], const uint8_t length) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            const uint8_t* keyData;
            uint8_t keyDataLength = parameters.LockBuffer<uint8_t>(keyData);

            response.Number<OCDM::ISession*> (message->Parameters().Implementation<OCDM::IAccessorOCDM>()->Session(keyData, keyDataLength));

            parameters.UnlockBuffer(keyDataLength);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Create a MediaKeySession using the supplied init data and CDM data.
            // virtual OCDM_RESULT CreateSession(
            //   const string keySystem,
            //   const int32_t licenseType,
            //   const string initDataType,
            //   const uint8_t* initData,
            //   const uint16_t initDataLength,
            //   const uint8_t* CDMData,
            //   const uint16_t CDMDataLength,
            //   ISession::ICallback* callback,
            //   std::string& sessionId,
            //   std::string& bufferId,
            //   ISession*& session) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            string keySystem = parameters.Text();
            int32_t licenseType = parameters.Number<int32_t>();
            string initDataType = parameters.Text();

            const uint8_t* initData;
            uint16_t initDataLength = parameters.LockBuffer<uint16_t>(initData);
            parameters.UnlockBuffer(initDataLength);

            const uint8_t* CDMData;
            uint16_t CDMDataLength = parameters.LockBuffer<uint16_t>(CDMData);
            parameters.UnlockBuffer(CDMDataLength);

            OCDM::ISession::ICallback* implementation = parameters.Number<OCDM::ISession::ICallback*>();
            OCDM::ISession::ICallback* proxy = nullptr;

            if (implementation != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance<OCDM::ISession::ICallback>(channel, implementation);

                ASSERT((proxy != nullptr) && "Failed to create proxy");
            }

            string sessionId;
            OCDM::ISession* result = nullptr;

            response.Number(message->Parameters().Implementation<OCDM::IAccessorOCDM>()->CreateSession(
                keySystem,
                licenseType, 
                initDataType,
                initData,
                initDataLength,
                CDMData,
                CDMDataLength,
                proxy,
                sessionId,
                result));

            response.Text(sessionId);
            response.Number<OCDM::ISession*>(result);

            if ( (proxy != nullptr) && (proxy->Release() != Core::ERROR_NONE)) {
                TRACE_L1("Oops seems like we did not maintain a reference to this sink. %d", __LINE__);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Set Server Certificate
            // virtual OCDM_RESULT SetServerCertificate(
            //     const string keySystem,
            //     const uint8_t* serverCertificate,
            //     const uint16_t serverCertificateLength) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());

            string keySystem = parameters.Text();
            const uint8_t* serverCertificate;
            uint16_t serverCertificateLength = parameters.LockBuffer<uint16_t>(serverCertificate);

            response.Number(message->Parameters().Implementation<OCDM::IAccessorOCDM>()->SetServerCertificate(
                keySystem,
                serverCertificate,
                serverCertificateLength));
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual void Register(OCDM::IAccessorOCDM::INotification* callback) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            OCDM::IAccessorOCDM::INotification* implementation = parameters.Number<OCDM::IAccessorOCDM::INotification*>();
            OCDM::IAccessorOCDM::INotification* proxy = nullptr;

            if (implementation != nullptr) {
                proxy = RPC::Administrator::Instance().ProxyInstance<OCDM::IAccessorOCDM::INotification>(channel, implementation);

                ASSERT((proxy != nullptr) && "Failed to create proxy");
            }

            message->Parameters().Implementation<OCDM::IAccessorOCDM>()->Register(proxy);

            if (proxy != nullptr) {
                proxy->Release();
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual void Unregister(OCDM::IAccessorOCDM::INotificationISession::IKeyCallback* callback) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());

            // Need to find the proxy that goes with the given implementation..
            OCDM::IAccessorOCDM::INotification* stub = reader.Number<OCDM::IAccessorOCDM::INotification*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            OCDM::IAccessorOCDM::INotification* proxy = RPC::Administrator::Instance().ProxyFind<OCDM::IAccessorOCDM::INotification>(channel, stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Coud not find stub for OCDM::IAccessorOCDM::INotification: %p"), stub);
            } else {
                message->Parameters().Implementation<OCDM::IAccessorOCDM>()->Unregister(proxy);
            }
        },
 
        nullptr
    };

    //
    // OCDM::IAccessorOCDM::INotification interface stub definitions (interface/ICDM.h)
    //
    ProxyStub::MethodHandler AccesorOCDMNotificationStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Create (
            //     const std::string& sessionId) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            std::string sessionId = parameters.Text();

            message->Parameters().Implementation<OCDM::IAccessorOCDM::INotification>()->Create(sessionId);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void Destroy (
            //     const std::string& sessionId) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            std::string sessionId = parameters.Text();

            message->Parameters().Implementation<OCDM::IAccessorOCDM::INotification>()->Destroy(sessionId);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // static void KeyChange(
            //     const string& sessionId, const uint8 keyId[], const uint8_t length) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());

            const uint8_t* keyData;
            std::string sessionId = parameters.Text();
            uint8_t keyDataLength = parameters.LockBuffer<uint8_t>(keyData);
            parameters.UnlockBuffer(keyDataLength);
            OCDM::ISession::KeyStatus status = parameters.Number<OCDM::ISession::KeyStatus>();

            message->Parameters().Implementation<OCDM::IAccessorOCDM::INotification>()->KeyChange(sessionId, keyData, keyDataLength, status);


        },
        nullptr
    };

    //
    // OCDM::ISession::ICallback interface stub definitions (interface/ICDM.h)
    //
    ProxyStub::MethodHandler CallbackStubMethods[] = {
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Event fired when a key message is successfully created.
            // virtual void OnKeyMessage(
            //    const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            //    const uint16_t keyLength, //__in
            //    const string URL) = 0; //__in_z_opt
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            const uint8_t* buffer;
            uint16_t length = parameters.LockBuffer<uint16_t>(buffer);
            parameters.UnlockBuffer(length);
            string URL = parameters.Text();

            message->Parameters().Implementation<OCDM::ISession::ICallback>()->OnKeyMessage(buffer, length, URL);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Event fired when MediaKeySession has found a usable key.
            // virtual void OnKeyReady() = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
 
            message->Parameters().Implementation<OCDM::ISession::ICallback>()->OnKeyReady();
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Event fired when MediaKeySession encounters an error.
            // virtual void OnKeyError(
            //     const int16 error,
            //     const OCDM_RESULT sysError,
            //     const string errorMessage) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            uint16_t error = parameters.Number<uint16_t>();
            OCDM::OCDM_RESULT sysError = parameters.Number<OCDM::OCDM_RESULT>();
            string data = parameters.Text();
 
            message->Parameters().Implementation<OCDM::ISession::ICallback>()->OnKeyError(error, sysError, data);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // virtual void OnKeyStatusUpdate(const string keyMessage) = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            OCDM::ISession::KeyStatus status = parameters.Number<OCDM::ISession::KeyStatus>();
 
            message->Parameters().Implementation<OCDM::ISession::ICallback>()->OnKeyStatusUpdate(status);
        },
        nullptr
    };

    //
    // OCDM::ISession interface stub definitions (interface/ICDM.h)
    //
    ProxyStub::MethodHandler SessionStubMethods[] = {

       [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Loads the data stored for the specified session into the cdm object
            // virtual OCDM_RESULT Load() = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
 
            response.Number(message->Parameters().Implementation<OCDM::ISession>()->Load());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Process a key message response.
            // virtual void Update(
            //    const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessageResponse)
            //    const uint16_t keyLength) = 0; //__in
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            const uint8_t* buffer;
            uint16_t length = parameters.LockBuffer<uint16_t>(buffer);
 
            message->Parameters().Implementation<OCDM::ISession>()->Update(buffer, length);
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Removes all license(s) and key(s) associated with the session
            // virtual OCDM_RESULT Remove() = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
 
            response.Number(message->Parameters().Implementation<OCDM::ISession>()->Remove());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // Returns the current Key Status in this Session.
            // virtual ISession::KeyStatus Status() const = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
 
            response.Number(message->Parameters().Implementation<OCDM::ISession>()->Status());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual std::string BufferId() const = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
 
            response.Text(message->Parameters().Implementation<OCDM::ISession>()->BufferId());
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual void Close() = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
 
            message->Parameters().Implementation<OCDM::ISession>()->Close();
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual void Revoke (OCDM::ISession::ICallback* callback) = 0;
            //
            RPC::Data::Frame::Reader reader(message->Parameters().Reader());

            // Need to find the proxy that goes with the given implementation..
            OCDM::ISession::ICallback* stub = reader.Number<OCDM::ISession::ICallback*>();

            // NOTE: FindProxy does *NOT* AddRef the result. Do not release what is obtained via FindProxy..
            OCDM::ISession::ICallback* proxy = RPC::Administrator::Instance().ProxyFind<OCDM::ISession::ICallback>(channel, stub);

            if (proxy == nullptr) {
                TRACE_L1(_T("Coud not find stub for OCDM::ISession::ICallback: %p"), stub);
            } else {
                message->Parameters().Implementation<OCDM::ISession>()->Revoke(proxy);
            }
        },
        [](Core::ProxyType<Core::IPCChannel>& channel VARIABLE_IS_NOT_USED, Core::ProxyType<RPC::InvokeMessage>& message) {
            //
            // 
            // virtual std::string BufferId() const = 0;
            //
            RPC::Data::Frame::Reader parameters(message->Parameters().Reader());
            RPC::Data::Frame::Writer response(message->Response().Writer());
 
            response.Text(message->Parameters().Implementation<OCDM::ISession>()->SessionId());
        },
 
        nullptr
    };

    typedef ProxyStub::UnknownStubType<OCDM::IAccessorOCDM, AccesorOCDMStubMethods> AccessorOCDMStub;
    typedef ProxyStub::UnknownStubType<OCDM::IAccessorOCDM::INotification, AccesorOCDMNotificationStubMethods> AccessorOCDMNotificationStub;
    typedef ProxyStub::UnknownStubType<OCDM::ISession::ICallback, CallbackStubMethods> CallbackStub;
    typedef ProxyStub::UnknownStubType<OCDM::ISession, SessionStubMethods> SessionStub;

    // -------------------------------------------------------------------------------------------
    // PROXY
    // -------------------------------------------------------------------------------------------
    class AccessorOCDMProxy : public ProxyStub::UnknownProxyType<OCDM::IAccessorOCDM> {
    public:
        AccessorOCDMProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~AccessorOCDMProxy()
        {
        }

    public:
        virtual OCDM::OCDM_RESULT IsTypeSupported(
            const std::string keySystem,
            const std::string mimeType) const override {

            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(keySystem);
            writer.Text(mimeType);
 
            Invoke(newMessage);

            return (Number<OCDM::OCDM_RESULT>(newMessage->Response()));
        } 
        virtual OCDM::ISession* Session (
            const std::string sessionId) {

            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(sessionId);
 
            Invoke(newMessage);

            return (reinterpret_cast<OCDM::ISession*>(Interface(newMessage->Response(), OCDM::ISession::ID)));
        }
        virtual OCDM::ISession* Session (
            const uint8_t keyId[],
            const uint8_t keyIdLength) {

            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer(keyIdLength, keyId);
 
            Invoke(newMessage);

            return (reinterpret_cast<OCDM::ISession*>(Interface(newMessage->Response(), OCDM::ISession::ID)));
        }
 
        //
        // Create a MediaKeySession using the supplied init data and CDM data.
        virtual OCDM::OCDM_RESULT CreateSession(
            const string keySystem,
            const int32_t licenseType,
            const string initDataType,
            const uint8_t* initData,
            const uint16_t initDataLength,
            const uint8_t* CDMData,
            const uint16_t CDMDataLength,
            OCDM::ISession::ICallback* callback,
            std::string& sessionId,
            OCDM::ISession*& session) override {

            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(keySystem);
            writer.Number(licenseType);
            writer.Text(initDataType);
            writer.Buffer(initDataLength, initData);
            writer.Buffer(CDMDataLength, CDMData);
            writer.Number(callback);

            Invoke(newMessage);

            RPC::Data::Frame::Reader reader(newMessage->Response().Reader());

            OCDM::OCDM_RESULT result = reader.Number<OCDM::OCDM_RESULT>();

            sessionId = reader.Text(); 
            session = reinterpret_cast<OCDM::ISession*>(ProxyInstance(reader.Number<void*>(), OCDM::ISession::ID));

            Complete(reader);

            return (result);
        }
        //
        // Set Server Certificate
        virtual OCDM::OCDM_RESULT SetServerCertificate(
            const string keySystem,
            const uint8_t* serverCertificate,
            const uint16_t serverCertificateLength) override {

            IPCMessage newMessage(BaseClass::Message(4));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(keySystem);
            writer.Buffer(serverCertificateLength, serverCertificate);

            Invoke(newMessage);

            return (Number<OCDM::OCDM_RESULT>(newMessage->Response()));
        }
        //
        // Register for a KeyId change notification
        //
        virtual void Register (OCDM::IAccessorOCDM::INotification* callback) {

            IPCMessage newMessage(BaseClass::Message(5));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(callback);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        //
        // Unregister for a KeyId change notification
        //
        virtual void Unregister (OCDM::IAccessorOCDM::INotification* callback) {

            IPCMessage newMessage(BaseClass::Message(6));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(callback);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
    };
 
    class AccessorOCDMNotificationProxy : public ProxyStub::UnknownProxyType<OCDM::IAccessorOCDM::INotification> {
    public:
        AccessorOCDMNotificationProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~AccessorOCDMNotificationProxy()
        {
        }

    public:
        virtual void Create(const string& sessionId) override {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(sessionId);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual void Destroy(const string& sessionId) override {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(sessionId);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        virtual void KeyChange(const string& sessionId, const uint8_t keyId[], const uint8_t length, const OCDM::ISession::KeyStatus status) override {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Text(sessionId);
            writer.Buffer(length, keyId);
            writer.Number(status);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }
    };

    class CallbackProxy : public ProxyStub::UnknownProxyType<OCDM::ISession::ICallback> {
    public:
        CallbackProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation, const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }

        virtual ~CallbackProxy()
        {
        }

    public:
        //
        // Event fired when a key message is successfully created.
        //
        virtual void OnKeyMessage (const uint8_t* keyMessage, const uint16_t length, const string URL)
        {
            IPCMessage newMessage(BaseClass::Message(0));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer(length, keyMessage);
            writer.Text(URL);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }

        //
        // Event fired when MediaKeySession has found a usable key.
        //
        virtual void OnKeyReady()
        {
            IPCMessage newMessage(BaseClass::Message(1));
            Invoke(newMessage);

            Complete(newMessage->Response());
        }

        //
        // Event fired when MediaKeySession encounters an error.
        //
        virtual void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError, const string message)
        {
            IPCMessage newMessage(BaseClass::Message(2));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(error);
            writer.Number(sysError);
            writer.Text(message);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }

        virtual void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status)
        {
            IPCMessage newMessage(BaseClass::Message(3));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(status);
            Invoke(newMessage);

            Complete(newMessage->Response());
        }
    };


    class SessionProxy : public ProxyStub::UnknownProxyType<OCDM::ISession> {
    public:
        SessionProxy(const Core::ProxyType<Core::IPCChannel>& channel, void* implementation,
            const bool otherSideInformed)
            : BaseClass(channel, implementation, otherSideInformed)
        {
        }
        virtual ~SessionProxy()
        {
        }

    public:
        //
        // Loads the data stored for the specified session into the cdm object
        //
        virtual OCDM::OCDM_RESULT Load() override {
            IPCMessage newMessage(BaseClass::Message(0));

            Invoke(newMessage);

            return (Number<OCDM::OCDM_RESULT>(newMessage->Response()));
        }
        //
        // Process a key message response.
        //
        virtual void Update(const uint8_t* keyMessage, const uint16_t keyLength) override {
            IPCMessage newMessage(BaseClass::Message(1));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Buffer(keyLength, keyMessage);

            Invoke(newMessage);

            Complete(newMessage->Response());
        } 
        //
        // Removes all license(s) and key(s) associated with the session
        //
        virtual OCDM::OCDM_RESULT Remove() override {
            IPCMessage newMessage(BaseClass::Message(2));

            Invoke(newMessage);

            return (Number<OCDM::OCDM_RESULT>(newMessage->Response()));
        }
        //
        // report the current status of the Session with respect to the KeyExchange.
        //
        virtual OCDM::ISession::KeyStatus Status() const {

            IPCMessage newMessage(BaseClass::Message(3));
 
            Invoke(newMessage);

            return (Number<OCDM::ISession::KeyStatus>(newMessage->Response()));
        }
        //
        // Report the name to be used for the Shared Memory for exchanging the Encrypted fragements.
        //
        virtual std::string BufferId() const {

            IPCMessage newMessage(BaseClass::Message(4));
 
            Invoke(newMessage);

            return (Text (newMessage->Response()));
        }
        //
        // We are done with the Sesison, close it.
        //
        virtual void Close () {

            IPCMessage newMessage(BaseClass::Message(5));
 
            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        //
        // Revoke the Session Callback for change notifications
        //
        virtual void Revoke (OCDM::ISession::ICallback* callback) {

            IPCMessage newMessage(BaseClass::Message(6));
            RPC::Data::Frame::Writer writer(newMessage->Parameters().Writer());
            writer.Number(callback);

            Invoke(newMessage);

            Complete(newMessage->Response());
        }
        //
        // Revoke the Session Callback for change notifications
        //
        virtual std::string SessionId () const {

            IPCMessage newMessage(BaseClass::Message(7));
 
            Invoke(newMessage);

            return (Text(newMessage->Response()));
        }
 
    };
 
    // -------------------------------------------------------------------------------------------
    // These proxy stubs are "loaded" by the next method, which needs to be explicitely called
    // since the interface is a dedicated interface and needs loading, if required by a 
    // wrapper aroundthe interface. 
    // -------------------------------------------------------------------------------------------
    static class Instantiation {
    public:
        Instantiation()
        {
            RPC::Administrator::Instance().Announce<OCDM::ISession::ICallback, CallbackProxy, CallbackStub>();
            RPC::Administrator::Instance().Announce<OCDM::ISession, SessionProxy, SessionStub>();
            RPC::Administrator::Instance().Announce<OCDM::IAccessorOCDM, AccessorOCDMProxy, AccessorOCDMStub>();
            RPC::Administrator::Instance().Announce<OCDM::IAccessorOCDM::INotification, AccessorOCDMNotificationProxy, AccessorOCDMNotificationStub>();
        }

        ~Instantiation()
        {
        }

    } OCDMProxyStubRegistration;


} // namespace WPEFramework

extern "C" {
	void* opencdm_announce_proxy_stubs() {
		return (&WPEFramework::OCDMProxyStubRegistration);
	}
}

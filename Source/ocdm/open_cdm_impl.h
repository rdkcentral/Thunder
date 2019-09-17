#pragma once

#include "DataExchange.h"
#include "IOCDM.h"
#include "Module.h"
#include "open_cdm.h"

using namespace WPEFramework;

extern Core::CriticalSection _systemLock;

struct OpenCDMSystem {
    std::string m_keySystem;
};

struct OpenCDMAccessor : public OCDM::IAccessorOCDM,
                         public OCDM::IAccessorOCDMExt {
private:
    OpenCDMAccessor() = delete;
    OpenCDMAccessor(const OpenCDMAccessor&) = delete;
    OpenCDMAccessor& operator=(const OpenCDMAccessor&) = delete;

private:
    class KeyId {
    private:
        KeyId() = delete;
        KeyId& operator=(const KeyId& rhs) = delete;

    public:
        inline KeyId(const KeyId& copy)
            : _status(copy._status)
        {
            ::memcpy(_kid, copy._kid, sizeof(_kid));
        }
        inline KeyId(const uint8_t kid[], const uint8_t length,
            const ::OCDM::ISession::KeyStatus status = OCDM::ISession::StatusPending)
            : _status(status)
        {
            uint8_t copyLength(length > sizeof(_kid) ? sizeof(_kid) : length);

            ::memcpy(_kid, kid, copyLength);

            if (copyLength < sizeof(_kid)) {
                ::memset(&(_kid[copyLength]), 0, sizeof(_kid) - copyLength);
            }
        }
        inline ~KeyId() {}

    public:
        inline bool operator==(const KeyId& rhs) const
        {
            // Hack, in case of PlayReady, the key offered on the interface might be
            // ordered incorrectly, cater for this situation, by silenty comparing
            // with this incorrect value.
            bool equal = false;

            // Regardless of the order, the last 8 bytes should be equal
            if (memcmp(&_kid[8], &(rhs._kid[8]), 8) == 0) {

                // Lets first try the non swapped byte order.
                if (memcmp(_kid, rhs._kid, 8) == 0) {
                    // this is a match :-)
                    equal = true;
                } else {
                    // Let do the byte order alignment as suggested in the spec and see if
                    // it matches than :-)
                    // https://msdn.microsoft.com/nl-nl/library/windows/desktop/aa379358(v=vs.85).aspx
                    uint8_t alignedBuffer[8];
                    alignedBuffer[0] = rhs._kid[3];
                    alignedBuffer[1] = rhs._kid[2];
                    alignedBuffer[2] = rhs._kid[1];
                    alignedBuffer[3] = rhs._kid[0];
                    alignedBuffer[4] = rhs._kid[5];
                    alignedBuffer[5] = rhs._kid[4];
                    alignedBuffer[6] = rhs._kid[7];
                    alignedBuffer[7] = rhs._kid[6];
                    equal = (memcmp(_kid, alignedBuffer, 8) == 0);
                }
            }
            return (equal);
        }
        inline bool operator!=(const KeyId& rhs) const
        {
            return !(operator==(rhs));
        }
        inline const uint8_t* Id() const { return (_kid); }
        inline static uint8_t Length() { return (sizeof(_kid)); }
        inline void Status(::OCDM::ISession::KeyStatus status) { _status = status; }
        ::OCDM::ISession::KeyStatus Status() const { return (_status); }
        string ToString() const
        {
            static TCHAR HexArray[] = "0123456789ABCDEF";
            string result(1, HexArray[(_kid[0] >> 4) & 0xF]);
            result += HexArray[_kid[0] & 0xF];

            for (uint8_t index = 1; index < sizeof(_kid); index++) {
                result += ':';
                result += HexArray[(_kid[index] >> 4) & 0xF];
                result += HexArray[_kid[index] & 0xF];
            }
            return (result);
        }

    private:
        uint8_t _kid[16];
        ::OCDM::ISession::KeyStatus _status;
    };
    typedef std::map<string, OpenCDMSession*> KeyMap;

private:
    OpenCDMAccessor(const TCHAR domainName[])
        : _refCount(1)
		, _engine(Core::ProxyType<RPC::InvokeServerType<4, 1>>::Create(Core::Thread::DefaultStackSize()))
        , _client(Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(domainName), Core::ProxyType<Core::IIPCServer>(_engine)))
        , _remote(nullptr)
        , _remoteExt(nullptr)
        , _adminLock()
        , _signal(false, true)
        , _interested(0)
        , _sessionKeys()
    {
        printf("Trying to open an OCDM connection @ %s\n", domainName);

        _remote = _client->Open<OCDM::IAccessorOCDM>(_T("OpenCDMImplementation"));

        printf("Trying to open an OCDM connection result %p\n", _remote);

        ASSERT(_remote != nullptr);

        if (_remote != nullptr) {
            _remoteExt = _remote->QueryInterface<OCDM::IAccessorOCDMExt>();
        } else {
            _client.Release();
        }
    }

public:
    static OpenCDMAccessor* Instance()
    {

        _systemLock.Lock();

        if (_singleton == nullptr) {
            // See if we have an environment variable set.
            string connector;
            if ((Core::SystemInfo::GetEnvironment(_T("OPEN_CDM_SERVER"), connector) == false) || (connector.empty() == true)) {
                connector = _T("/tmp/ocdm");
            }

            OpenCDMAccessor* result = new OpenCDMAccessor(connector.c_str());

            if (result->_remote != nullptr) {
                _singleton = result;
            } else {
                delete result;
            }
        } else {
            _singleton->AddRef();
        }

        _systemLock.Unlock();

        return (_singleton);
    }
    ~OpenCDMAccessor()
    {
        if (_remote != nullptr) {
            _remote->Release();
        }

        if (_client.IsValid()) {
            _client.Release();
        }

        _singleton = nullptr;
        TRACE_L1("Destructed the OpenCDMAccessor %p", this);
    }
    bool WaitForKey(const uint8_t keyLength, const uint8_t keyId[],
        const uint32_t waitTime,
        const OCDM::ISession::KeyStatus status,
        std::string& sessionId, OpenCDMSystem* system = nullptr) const;

public:
    virtual void AddRef() const override
    {
        Core::InterlockedIncrement(_refCount);
    }
    virtual uint32_t Release() const override
    {
        uint32_t result = Core::ERROR_NONE;

        _systemLock.Lock();

        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
            result = Core::ERROR_DESTRUCTION_SUCCEEDED;
        }

        _systemLock.Unlock();

        return (result);
    }
    BEGIN_INTERFACE_MAP(OpenCDMAccessor)
    INTERFACE_ENTRY(OCDM::IAccessorOCDM)
    END_INTERFACE_MAP

    virtual bool IsTypeSupported(const std::string keySystem,
        const std::string mimeType) const override
    {
        return (_remote->IsTypeSupported(keySystem, mimeType));
    }

    // Create a MediaKeySession using the supplied init data and CDM data.
    virtual OCDM::OCDM_RESULT
    CreateSession(const string keySystem, const int32_t licenseType,
        const std::string initDataType, const uint8_t* initData,
        const uint16_t initDataLength, const uint8_t* CDMData,
        const uint16_t CDMDataLength,
        OCDM::ISession::ICallback* callback, std::string& sessionId,
        OCDM::ISession*& session) override
    {
        return (_remote->CreateSession(
            keySystem, licenseType, initDataType, initData, initDataLength, CDMData,
            CDMDataLength, callback, sessionId, session));
    }

    // Set Server Certificate
    virtual OCDM::OCDM_RESULT
    SetServerCertificate(const string keySystem, const uint8_t* serverCertificate,
        const uint16_t serverCertificateLength) override
    {
        return (_remote->SetServerCertificate(keySystem, serverCertificate,
            serverCertificateLength));
    }

    OpenCDMSession* Session(const std::string& sessionId);

    void AddSession(OpenCDMSession* sessionId);
    void RemoveSession(const string& sessionId);
    void KeyUpdate()
    {
        _adminLock.Lock();

        if (_interested != 0) {
            // We need to notify the "other side", they are expecting an update
            _signal.SetEvent();

            while (_interested != 0) {
                ::SleepMs(0);
            }

            _signal.ResetEvent();
        }
        _adminLock.Unlock();
    }

    virtual uint64_t GetDrmSystemTime(const std::string& keySystem) const override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetDrmSystemTime(keySystem);
    }

    virtual std::string
    GetVersionExt(const std::string& keySystem) const override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetVersionExt(keySystem);
    }

    virtual uint32_t GetLdlSessionLimit(const std::string& keySystem) const
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetLdlSessionLimit(keySystem);
    }

    virtual bool IsSecureStopEnabled(const std::string& keySystem) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->IsSecureStopEnabled(keySystem);
    }

    virtual OCDM::OCDM_RESULT EnableSecureStop(const std::string& keySystem,
        bool enable) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->EnableSecureStop(keySystem, enable);
    }

    virtual uint32_t ResetSecureStops(const std::string& keySystem) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->ResetSecureStops(keySystem);
    }

    virtual OCDM::OCDM_RESULT GetSecureStopIds(const std::string& keySystem,
        uint8_t ids[], uint8_t idSize,
        uint32_t& count)
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetSecureStopIds(keySystem, ids, idSize, count);
    }

    virtual OCDM::OCDM_RESULT GetSecureStop(const std::string& keySystem,
        const uint8_t sessionID[],
        uint32_t sessionIDLength,
        uint8_t rawData[],
        uint16_t& rawSize)
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetSecureStop(keySystem, sessionID, sessionIDLength,
            rawData, rawSize);
    }

    virtual OCDM::OCDM_RESULT
    CommitSecureStop(const std::string& keySystem, const uint8_t sessionID[],
        uint32_t sessionIDLength, const uint8_t serverResponse[],
        uint32_t serverResponseLength) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->CommitSecureStop(keySystem, sessionID, sessionIDLength,
            serverResponse, serverResponseLength);
    }

    virtual OCDM::OCDM_RESULT
    DeleteKeyStore(const std::string& keySystem) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->DeleteKeyStore(keySystem);
    }

    virtual OCDM::OCDM_RESULT
    DeleteSecureStore(const std::string& keySystem) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->DeleteSecureStore(keySystem);
    }

    virtual OCDM::OCDM_RESULT
    GetKeyStoreHash(const std::string& keySystem, uint8_t keyStoreHash[],
        uint32_t keyStoreHashLength) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetKeyStoreHash(keySystem, keyStoreHash,
            keyStoreHashLength);
    }

    virtual OCDM::OCDM_RESULT
    GetSecureStoreHash(const std::string& keySystem, uint8_t secureStoreHash[],
        uint32_t secureStoreHashLength) override
    {
        ASSERT(_remoteExt && "This method only works on IAccessorOCDMExt implementations.");
        return _remoteExt->GetSecureStoreHash(keySystem, secureStoreHash,
            secureStoreHashLength);
    }

private:
    mutable uint32_t _refCount;
	Core::ProxyType<RPC::InvokeServerType<4, 1> > _engine;
    Core::ProxyType<RPC::CommunicatorClient> _client;
    OCDM::IAccessorOCDM* _remote;
    OCDM::IAccessorOCDMExt* _remoteExt;
    mutable Core::CriticalSection _adminLock;
    mutable Core::Event _signal;
    mutable volatile uint32_t _interested;
    KeyMap _sessionKeys;
    static OpenCDMAccessor* _singleton;
};

struct OpenCDMSession {
private:
    class Sink : public OCDM::ISession::ICallback {
    //private:
    public:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

    public:
        Sink(OpenCDMSession* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~Sink() {}

    public:
        // Event fired when a key message is successfully created.
        void OnKeyMessage(const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string& URL) override
        {
            _parent.OnKeyMessage(
                std::string(reinterpret_cast<const char*>(keyMessage), keyLength),
                URL);
        }

        void OnError(const int16_t error, const OCDM::OCDM_RESULT sysError, const std::string& errorMessage) override
        {
            _parent.OnError(error, sysError, errorMessage);
        }

        // Event fired on key status update
        void OnKeyStatusUpdate(const uint8_t keyID[], const uint8_t keyIDLength,const OCDM::ISession::KeyStatus keyMessage) override
        {
            _parent.OnKeyStatusUpdate(keyID, keyIDLength, keyMessage);
        }

        void OnKeyStatusesUpdated() const override
        {
            _parent.OnKeyStatusesUpdated();
        }

        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(OCDM::ISession::ICallback)
        END_INTERFACE_MAP

    private:
        OpenCDMSession& _parent;
    };

    class DataExchange : public OCDM::DataExchange {
    private:
        DataExchange() = delete;
        DataExchange(const DataExchange&) = delete;
        DataExchange& operator=(DataExchange&) = delete;

    public:
        DataExchange(const string& bufferName)
            : OCDM::DataExchange(bufferName)
            , _busy(false)
        {

            TRACE_L1("Constructing buffer client side: %p - %s", this,
                bufferName.c_str());
        }
        virtual ~DataExchange()
        {
            if (_busy == true) {
                TRACE_L1("Destructed a DataExchange while still in progress. %p", this);
            }
            TRACE_L1("Destructing buffer client side: %p - %s", this,
                OCDM::DataExchange::Name().c_str());
        }



    public:
        uint32_t Decrypt(uint8_t* encryptedData, uint32_t encryptedDataLength,
            const uint8_t* ivData, uint16_t ivDataLength,
            const uint8_t* keyId, uint16_t keyIdLength,
            uint32_t initWithLast15 /* = 0 */)
        {
            int ret = 0;

            // This works, because we know that the Audio and the Video streams are
            // fed from
            // the same process, so they will use the same critial section and thus
            // will
            // not interfere with each-other. If Audio and video will be located into
            // two
            // different processes, start using the administartion space to share a
            // lock.
            _systemLock.Lock();

            _busy = true;

            if (RequestProduce(WPEFramework::Core::infinite) == WPEFramework::Core::ERROR_NONE) {

                SetIV(static_cast<uint8_t>(ivDataLength), ivData);
                SetSubSampleData(0, nullptr);
                KeyId(static_cast<uint8_t>(keyIdLength), keyId);
                InitWithLast15(initWithLast15);
                Write(encryptedDataLength, encryptedData);

                // This will trigger the OpenCDMIServer to decrypt this memory...
                Produced();

                // Now we should wait till it is decrypted, that happens if the
                // Producer, can run again.
                if (RequestProduce(WPEFramework::Core::infinite) == WPEFramework::Core::ERROR_NONE) {

                    // For nowe we just copy the clear data..
                    Read(encryptedDataLength, encryptedData);

                    // Get the status of the last decrypt.
                    ret = Status();

                    // And free the lock, for the next production Scenario..
                    Consumed();
                }
            }

            _busy = false;

            _systemLock.Unlock();

            return (ret);
        }

    private:
        bool _busy;
    };

public:
    OpenCDMSession(const OpenCDMSession&) = delete;
    OpenCDMSession& operator= (const OpenCDMSession&) = delete;
    OpenCDMSession() = delete;

    OpenCDMSession(OpenCDMSystem* system,
        const string& initDataType,
        const uint8_t* pbInitData, const uint16_t cbInitData,
        const uint8_t* pbCustomData,
        const uint16_t cbCustomData,
        const LicenseType licenseType,
        OpenCDMSessionCallbacks* callbacks,
        void* userData)
        : _sessionId()
        , _decryptSession(nullptr)
        , _session(nullptr)
        , _sessionExt(nullptr)
        , _refCount(1)
        , _sink(this)
        , _URL()
        , _callback(callbacks)
        , _userData(userData)
        , _keyStatuses()
        , _error()
        , _errorCode(~0)
        , _sysError(OCDM::OCDM_RESULT::OCDM_SUCCESS)
        , _system(system)
    {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string bufferId;
        OCDM::ISession* realSession = nullptr;

        accessor->CreateSession(system->m_keySystem, licenseType, initDataType, pbInitData,
            cbInitData, pbCustomData, cbCustomData, &_sink,
            _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        } else {
            Session(realSession);
            realSession->Release();
            accessor->AddSession(this);
        }
    }

    virtual ~OpenCDMSession()
    {
        OpenCDMAccessor* system = OpenCDMAccessor::Instance();

        system->RemoveSession(_sessionId);

        if (IsValid()) {
           _session->Revoke(&_sink);
        }

        if (_session != nullptr) {
            Session(nullptr);
        }

        TRACE_L1("Destructed the Session Client side: %p", this);
    }

public:
    void AddRef() { Core::InterlockedIncrement(_refCount); }
    bool Release()
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {

            delete this;

            return (true);
        }
        return (false);
    }
    inline const string& SessionId() const { return (_sessionId); }
    inline const string& BufferId() const
    {
        static string EmptyString;

        return (_decryptSession != nullptr ? _decryptSession->Name() : EmptyString);
    }
    inline bool IsValid() const { return (_session != nullptr); }
    inline OCDM::ISession::KeyStatus Status(const uint8_t keyIDLength, const uint8_t keyID[]) const
    {
        std::string key(reinterpret_cast<const char*>(keyID), keyIDLength);

        std::map<std::string, OCDM::ISession::KeyStatus>::const_iterator index =  _keyStatuses.find(key); 

        return (index != _keyStatuses.end() ? index->second : OCDM::ISession::StatusPending); 
    }
    inline bool HasKeyId(const uint8_t keyIDLength, const uint8_t keyID[]) const
    {
        std::string key(reinterpret_cast<const char*>(keyID), keyIDLength);

        std::map<std::string, OCDM::ISession::KeyStatus>::const_iterator index =  _keyStatuses.find(key); 

        return (index != _keyStatuses.end());
    }
    inline void Close()
    {
        ASSERT(_session != nullptr);

        _session->Close();
    }
    inline int Remove()
    {

        ASSERT(_session != nullptr);

        return (_session->Remove() == 0);
    }
    inline int Load()
    {

        ASSERT(_session != nullptr);

        return (_session->Load() == 0);
    }
    inline void Update(const uint8_t* pbResponse, const uint16_t cbResponse)
    {

        ASSERT(_session != nullptr);

        _session->Update(pbResponse, cbResponse);
    }
    uint32_t Decrypt(uint8_t* encryptedData, const uint32_t encryptedDataLength,
        const uint8_t* ivData, uint16_t ivDataLength,
        const uint8_t* keyId, const uint16_t keyIdLength,
        uint32_t initWithLast15)
    {
        uint32_t result = OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER;
        if (_decryptSession != nullptr) {
            result = _decryptSession->Decrypt(encryptedData, encryptedDataLength, ivData,
                ivDataLength, keyId, keyIdLength,
                initWithLast15);
            if(result)
            {
                TRACE_L1("Decrypt() failed with return code: %x", result);
                result = OpenCDMError::ERROR_UNKNOWN;
            }
        }
        return (result);
    }

    uint32_t SessionIdExt() const
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->SessionIdExt();
    }

    OCDM::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[],
        uint32_t drmHeaderLength)
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->SetDrmHeader(drmHeader, drmHeaderLength);
    }

    OCDM::OCDM_RESULT GetChallengeDataExt(uint8_t* challenge,
        uint32_t& challengeSize,
        uint32_t isLDL)
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->GetChallengeDataExt(challenge, challengeSize, isLDL);
    }

    OCDM::OCDM_RESULT CancelChallengeDataExt()
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->CancelChallengeDataExt();
    }

    OCDM::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[],
        uint32_t licenseDataSize,
        uint8_t* secureStopId)
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->StoreLicenseData(licenseData, licenseDataSize,
            secureStopId);
    }

    OCDM::OCDM_RESULT SelectKeyId(const uint8_t keyLength, const uint8_t keyId[])
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->SelectKeyId(keyLength, keyId);
    }

    OCDM::OCDM_RESULT CleanDecryptContext()
    {
        ASSERT(_sessionExt && "This method only works on OCDM::ISessionExt implementations.");
        return _sessionExt->CleanDecryptContext();
    }

public:
    inline uint32_t Error() const
    {
        return (_errorCode);
    }
    inline uint32_t Error(const uint8_t keyId[], uint8_t length) const
    {
        return (_sysError);
    }

    bool BelongsTo(OpenCDMSystem* system) { return system == _system; }

protected:
    void Session(OCDM::ISession* session)
    {
        ASSERT((_session == nullptr) ^ (session == nullptr));

        if (session == nullptr) {

            ASSERT (_session != nullptr);
            ASSERT (_decryptSession != nullptr);

            _session->Release();

            delete _decryptSession;
            _decryptSession = nullptr;

            if (_sessionExt != nullptr) {
                _sessionExt->Release();
                _sessionExt = nullptr;
            }
        }

        _session = session;

        if (session != nullptr) {

            ASSERT (_decryptSession == nullptr);

            _session->AddRef();
            _decryptSession = new DataExchange(_session->BufferId());
            _sessionExt = _session->QueryInterface<OCDM::ISessionExt>();
        }
    }
   // Event fired when a key message is successfully created.
    void OnKeyMessage(const std::string& keyMessage, const std::string& URL)
    {
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback != nullptr && _callback->process_challenge_callback != nullptr)
            _callback->process_challenge_callback(this, _userData, _URL.c_str(), reinterpret_cast<const uint8_t*>(keyMessage.c_str()), static_cast<uint16_t>(keyMessage.length()));
    }

    // Event fired when MediaKeySession encounters an error.
    void OnError(const int16_t error, const OCDM::OCDM_RESULT sysError,
        const std::string& errorMessage)
    {
        _errorCode = error;
        _sysError = sysError;

        if (_callback != nullptr && _callback->error_message_callback != nullptr) {
            _callback->error_message_callback(this, _userData, errorMessage.c_str());
        }
    }

    // Event fired on key status update
    void OnKeyStatusUpdate(const uint8_t keyID[], const uint8_t keyIDLength, const OCDM::ISession::KeyStatus status)
    {   
        printf("BRAM2 DEBUG OnKeyStatusUpdate %d %d %p\n", keyIDLength, status, _callback);
        std::string keyId(reinterpret_cast<const char*>(keyID), keyIDLength);
        _keyStatuses[keyId] = status;

        if ((_callback != nullptr) && (_callback->key_update_callback != nullptr)){
            _callback->key_update_callback(this, _userData, keyID, keyIDLength);
        } else {
            printf("BRAM3 DEBUG OnKeyStatusUpdate\n");
        }
    }

    void OnKeyStatusesUpdated() const
    {
        if (_callback->keys_updated_callback)
            _callback->keys_updated_callback(this, _userData);
    }

protected:
    std::string _sessionId;
    DataExchange* _decryptSession;

private:

    OCDM::ISession* _session;
    OCDM::ISessionExt* _sessionExt;
    uint32_t _refCount;
    WPEFramework::Core::Sink<Sink> _sink;
    std::string _URL;
    OpenCDMSessionCallbacks* _callback;
    void* _userData; 
    std::map<std::string, OCDM::ISession::KeyStatus> _keyStatuses;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    OpenCDMSystem* _system;
};


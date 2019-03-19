/*
 * Copyright 2016-2017 TATA ELXSI
 * Copyright 2016-2017 Metrological
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "open_cdm.h"
#include "DataExchange.h"
#include "IOCDM.h"
#include "Module.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

static Core::CriticalSection _systemLock;
static const char EmptyString[] = { '\0' };

static KeyStatus CDMState(const OCDM::ISession::KeyStatus state)
{

    switch (state) {
    case OCDM::ISession::StatusPending:
        return KeyStatus::StatusPending;
    case OCDM::ISession::Usable:
        return KeyStatus::Usable;
    case OCDM::ISession::InternalError:
        return KeyStatus::InternalError;
    case OCDM::ISession::Released:
        return KeyStatus::Released;
    case OCDM::ISession::Expired:
        return KeyStatus::Expired;
    default:
        assert(false);
    }

    return KeyStatus::InternalError;
}

struct OpenCDMSession {
private:
    OpenCDMSession(const OpenCDMSession&) = delete;
    OpenCDMSession& operator=(OpenCDMSession&) = delete;

private:
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

            TRACE_L1("Constructing buffer client side: %p - %s", this, bufferName.c_str());
        }
        virtual ~DataExchange()
        {
            if (_busy == true) {
                TRACE_L1("Destructed a DataExchange while still in progress. %p", this);
            }
            TRACE_L1("Destructing buffer client side: %p - %s", this, OCDM::DataExchange::Name().c_str());
        }

    public:
        uint32_t Decrypt(uint8_t* encryptedData, uint32_t encryptedDataLength, const uint8_t* ivData, uint16_t ivDataLength, const uint8_t* keyId, uint16_t keyIdLength)
        {
            int ret = 0;

            // This works, because we know that the Audio and the Video streams are fed from
            // the same process, so they will use the same critial section and thus will
            // not interfere with each-other. If Audio and video will be located into two
            // different processes, start using the administartion space to share a lock.
            _systemLock.Lock();

            _busy = true;

            if (RequestProduce(Core::infinite) == Core::ERROR_NONE) {

                SetIV(static_cast<uint8_t>(ivDataLength), ivData);
                SetSubSampleData(0, nullptr);
                KeyId(static_cast<uint8_t>(keyIdLength), keyId);
                Write(encryptedDataLength, encryptedData);

                // This will trigger the OpenCDMIServer to decrypt this memory...
                Produced();

                // Now we should wait till it is decrypted, that happens if the Producer, can run again.
                if (RequestProduce(Core::infinite) == Core::ERROR_NONE) {

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
    OpenCDMSession()
        : _sessionId()
        , _session(nullptr)
        , _decryptSession(nullptr)
        , _refCount(1)
    {
        TRACE_L1("Constructing the Session Client side: %p, (nil)", this);
    }
    explicit OpenCDMSession(OCDM::ISession* session)
        : _sessionId(session->SessionId())
        , _session(session)
        , _decryptSession(new DataExchange(_session->BufferId()))
        , _refCount(1)
    {

        ASSERT(session != nullptr);

        if (_session != nullptr) {
            _session->AddRef();
        }
    }
    virtual ~OpenCDMSession()
    {
        if (_session != nullptr) {
            _session->Release();
        }
        if (_decryptSession != nullptr) {
            delete _decryptSession;
        }
        TRACE_L1("Destructed the Session Client side: %p", this);
    }

public:
    virtual bool IsExtended() const
    {
        return (false);
    }
    void AddRef()
    {
        Core::InterlockedIncrement(_refCount);
    }
    bool Release()
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {

            delete this;

            return (true);
        }
        return (false);
    }
    inline const string& SessionId() const
    {
        return (_sessionId);
    }
    inline const string& BufferId() const
    {
        static string EmptyString;

        return (_decryptSession != nullptr ? _decryptSession->Name() : EmptyString);
    }
    inline bool IsValid() const
    {

        return (_session != nullptr);
    }
    inline OCDM::ISession::KeyStatus Status() const
    {

        return (_session != nullptr ? _session->Status() : OCDM::ISession::StatusPending);
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
    uint32_t Decrypt(uint8_t* encryptedData, const uint32_t encryptedDataLength, const uint8_t* ivData, uint16_t ivDataLength, const uint8_t* keyId, const uint8_t keyIdLength)
    {
        uint32_t result = OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER;
        if (_decryptSession != nullptr) {
            result = OpenCDMError::ERROR_NONE;
            _decryptSession->Decrypt(encryptedData, encryptedDataLength, ivData, ivDataLength, keyId, keyIdLength);
        }
        return (result);
    }
    inline void Revoke(OCDM::ISession::ICallback* callback)
    {

        ASSERT(_session != nullptr);
        ASSERT(callback != nullptr);

        return (_session->Revoke(callback));
    }

protected:
    void Session(OCDM::ISession* session)
    {
        ASSERT((_session == nullptr) ^ (session == nullptr));

        if ((session == nullptr) && (_session != nullptr)) {
            _session->Release();
        }
        _session = session;

        if (_session != nullptr) {
            _decryptSession = new DataExchange(_session->BufferId());
        } else {
            delete _decryptSession;
            _decryptSession = nullptr;
        }
    }
    std::string _sessionId;

private:
    OCDM::ISession* _session;
    DataExchange* _decryptSession;
    uint32_t _refCount;
};

struct ExtendedOpenCDMSession : public OpenCDMSession {
private:
    ExtendedOpenCDMSession() = delete;
    ExtendedOpenCDMSession(const ExtendedOpenCDMSession&) = delete;
    ExtendedOpenCDMSession& operator=(ExtendedOpenCDMSession&) = delete;

    enum sessionState {
        // Initialized.
        SESSION_INIT = 0x00,

        // ExtendedOpenCDMSession created, waiting for message callback.
        SESSION_MESSAGE = 0x01,
        SESSION_READY = 0x02,
        SESSION_ERROR = 0x04,
        SESSION_LOADED = 0x08,
        SESSION_UPDATE = 0x10
    };

private:
    class Sink : public OCDM::ISession::ICallback {
    private:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

    public:
        Sink(ExtendedOpenCDMSession* parent)
            : _parent(*parent)
        {
            ASSERT(parent != nullptr);
        }
        virtual ~Sink()
        {
        }

    public:
        // Event fired when a key message is successfully created.
        virtual void OnKeyMessage(
            const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string URL)
        {
            _parent.OnKeyMessage(std::string(reinterpret_cast<const char*>(keyMessage), keyLength), URL);
        }
        // Event fired when MediaKeySession has found a usable key.
        virtual void OnKeyReady()
        {
            _parent.OnKeyReady();
        }
        // Event fired when MediaKeySession encounters an error.
        virtual void OnKeyError(
            const int16_t error,
            const OCDM::OCDM_RESULT sysError,
            const std::string errorMessage)
        {
            _parent.OnKeyError(error, sysError, errorMessage);
        }
        // Event fired on key status update
        virtual void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus keyMessage)
        {
            _parent.OnKeyStatusUpdate(keyMessage);
        }

        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(OCDM::ISession::ICallback)
        END_INTERFACE_MAP

    private:
        ExtendedOpenCDMSession& _parent;
    };

public:
    ExtendedOpenCDMSession(
        OCDM::IAccessorOCDM* system,
        const string keySystem,
        const std::string& initDataType,
        const uint8_t* pbInitData,
        const uint16_t cbInitData,
        const uint8_t* pbCustomData,
        const uint16_t cbCustomData,
        const LicenseType licenseType,
        OpenCDMSessionCallbacks* callbacks)
        : OpenCDMSession()
        , _sink(this)
        , _state(SESSION_INIT)
        , _message()
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(0)
        , _key(OCDM::ISession::StatusPending)
        , _callback(callbacks)
    {

        std::string bufferId;
        OCDM::ISession* realSession = nullptr;

        system->CreateSession(keySystem, licenseType, initDataType, pbInitData, cbInitData, pbCustomData, cbCustomData, &_sink, _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        } else {
            OpenCDMSession::Session(realSession);
        }
    }
    virtual ~ExtendedOpenCDMSession()
    {
        if (OpenCDMSession::IsValid() == true) {
            Revoke(&_sink);
            OpenCDMSession::Session(nullptr);
        }
    }

public:
    virtual bool IsExtended() const override
    {
        return (true);
    }
    inline KeyStatus Status(const uint8_t /* keyId */[], uint8_t /* length */) const
    {
        return (::CDMState(_key));
    }
    inline uint32_t Error() const
    {
        return (_errorCode);
    }
    inline uint32_t Error(const uint8_t keyId[], uint8_t length) const
    {
        return (_sysError);
    }
    void GetKeyMessage(std::string& challenge, uint8_t* licenseURL, uint16_t& urlLength)
    {

        ASSERT(IsValid() == true);

        _state.WaitState(SESSION_MESSAGE | SESSION_READY, Core::infinite);

        if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
            challenge = _message;
            if (urlLength > static_cast<int>(_URL.length())) {
                urlLength = static_cast<uint16_t>(_URL.length());
            }
            memcpy(licenseURL, _URL.c_str(), urlLength);
            TRACE_L1("Returning a KeyMessage, Length: [%d,%d]", urlLength, static_cast<uint32_t>(challenge.length()));
        } else if ((_state & SESSION_READY) == SESSION_READY) {
            challenge.clear();
            *licenseURL = '\0';
            urlLength = 0;
            TRACE_L1("Returning a KeyMessage failed. %d", __LINE__);
        }
    }
    int Load(std::string& response)
    {
        int ret = 1;

        _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE | SESSION_MESSAGE)));

        response.clear();

        if (OpenCDMSession::Load() == 0) {

            _state.WaitState(SESSION_UPDATE, Core::infinite);

            if (_key == OCDM::ISession::Usable) {
                ret = 0;
            } else if (_state == SESSION_MESSAGE) {
                ret = 0;
                response = "message:" + _message;
            }
        }

        return ret;
    }
    KeyStatus Update(const uint8_t* pbResponse, const uint16_t cbResponse, std::string& response)
    {

        _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE | SESSION_MESSAGE)));

        OpenCDMSession::Update(pbResponse, cbResponse);

        _state.WaitState(SESSION_UPDATE | SESSION_MESSAGE, Core::infinite);
        if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
            response = "message:" + _message;
        }

        return CDMState(_key);
    }
    int Remove(std::string& response)
    {
        int ret = 1;

        _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE | SESSION_MESSAGE)));

        if (OpenCDMSession::Remove() == 0) {

            _state.WaitState(SESSION_UPDATE, Core::infinite);

            if (_key == OCDM::ISession::StatusPending) {
                ret = 0;
            } else if (_state == SESSION_MESSAGE) {
                ret = 0;
                response = "message:" + _message;
            }
        }

        return (ret);
    }

private:
    // void (*process_challenge) (void * userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength);
    // void (*key_update)        (void * userData, const uint8_t keyId[], const uint8_t length);
    // void (*message)           (void * userData, const char message[]);

    // Event fired when a key message is successfully created.
    void OnKeyMessage(const std::string& keyMessage, const std::string& URL)
    {
        _message = keyMessage;
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_MESSAGE | SESSION_UPDATE);
        } else {
            _callback->process_challenge(this, _URL.c_str(), reinterpret_cast<const uint8_t*>(_message.c_str()), static_cast<uint16_t>(_message.length()));
        }
    }
    // Event fired when MediaKeySession has found a usable key.
    void OnKeyReady()
    {
        _key = OCDM::ISession::Usable;
        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        } else {
            _callback->key_update(this, nullptr, 0);
        }
    }
    // Event fired when MediaKeySession encounters an error.
    void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError, const std::string& errorMessage)
    {
        _key = OCDM::ISession::InternalError;
        _error = errorMessage;
        _errorCode = error;
        _sysError = sysError;

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_ERROR | SESSION_UPDATE);
        } else {
            _callback->key_update(this, nullptr, 0);
            _callback->message(this, errorMessage.c_str());
        }
    }
    // Event fired on key status update
    void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status)
    {
        _key = status;

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        } else {
            _callback->key_update(this, nullptr, 0);
        }
    }

private:
    Core::Sink<Sink> _sink;
    Core::StateTrigger<sessionState> _state;
    std::string _message;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    OCDM::ISession::KeyStatus _key;
    OpenCDMSessionCallbacks* _callback;
};

struct OpenCDMAccessor : public OCDM::IAccessorOCDM {
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
        inline KeyId(const uint8_t kid[], const uint8_t length, const ::OCDM::ISession::KeyStatus status = OCDM::ISession::StatusPending)
            : _status(status)
        {
            uint8_t copyLength(length > sizeof(_kid) ? sizeof(_kid) : length);

            ::memcpy(_kid, kid, copyLength);

            if (copyLength < sizeof(_kid)) {
                ::memset(&(_kid[copyLength]), 0, sizeof(_kid) - copyLength);
            }
        }
        inline ~KeyId()
        {
        }

    public:
        inline bool operator==(const KeyId& rhs) const
        {
            // Hack, in case of PlayReady, the key offered on the interface might be
            // ordered incorrectly, cater for this situation, by silenty comparing with this incorrect value.
            bool equal = false;

            // Regardless of the order, the last 8 bytes should be equal
            if (memcmp(&_kid[8], &(rhs._kid[8]), 8) == 0) {

                // Lets first try the non swapped byte order.
                if (memcmp(_kid, rhs._kid, 8) == 0) {
                    // this is a match :-)
                    equal = true;
                } else {
                    // Let do the byte order alignment as suggested in the spec and see if it matches than :-)
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
        inline const uint8_t* Id() const
        {
            return (_kid);
        }
        inline static uint8_t Length()
        {
            return (sizeof(_kid));
        }
        inline void Status(::OCDM::ISession::KeyStatus status)
        {
            _status = status;
        }
        ::OCDM::ISession::KeyStatus Status() const
        {
            return (_status);
        }
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
    class Sink : public OCDM::IAccessorOCDM::INotification {
    private:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator=(const Sink&) = delete;

    public:
        Sink(OpenCDMAccessor* parent)
            : _parent(*parent)
        {
        }
        virtual ~Sink()
        {
        }

    private:
        virtual void Create(const string& sessionId) override
        {
            _parent.AddSession(sessionId);
        }
        virtual void Destroy(const string& sessionId) override
        {
            _parent.RemoveSession(sessionId);
        }
        virtual void KeyChange(const string& sessionId, const uint8_t keyId[], const uint8_t length, const OCDM::ISession::KeyStatus status) override
        {
            _parent.KeyUpdate(sessionId, keyId, length, status);
        }

        BEGIN_INTERFACE_MAP(Sink)
        INTERFACE_ENTRY(OCDM::IAccessorOCDM::INotification)
        END_INTERFACE_MAP
    private:
        OpenCDMAccessor& _parent;
    };
    typedef std::list<KeyId> KeyList;
    typedef std::map<string, KeyList> KeyMap;

private:
    OpenCDMAccessor(const TCHAR domainName[])
        : _refCount(1)
        , _client(Core::ProxyType<RPC::CommunicatorClient>::Create(Core::NodeId(domainName), Core::ProxyType<RPC::InvokeServerType<4, 1>>::Create(Core::Thread::DefaultStackSize())))
        , _remote(nullptr)
        , _adminLock()
        , _signal(false, true)
        , _interested(0)
        , _sessionKeys()
        , _sink(this)
    {
        Reconnect();
    }

    void Reconnect(void) const
    {
        if (_client->IsOpen() == false) {
            if (_remote != nullptr) {
                _remote->Release();
            }
            _remote = _client->Open<OCDM::IAccessorOCDM>(_T("OpenCDMImplementation"));

            ASSERT(_remote != nullptr);

            if (_remote != nullptr) {
                _remote->Register(&_sink);
            } else {
                _client.Release();
            }
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
            // Reconnect if server is down
            _singleton->Reconnect();

            _singleton->AddRef();
        }

        _systemLock.Unlock();

        return (_singleton);
    }
    ~OpenCDMAccessor()
    {
        if (_remote != nullptr) {
            Unregister(&_sink);
            _remote->Release();
        }

        if (_client.IsValid()) {
            _client.Release();
        }

        _singleton = nullptr;
        TRACE_L1("Destructed the OpenCDMAccessor %p", this);
    }
    bool WaitForKey(const uint8_t keyLength, const uint8_t keyId[], const uint32_t waitTime, const OCDM::ISession::KeyStatus status) const
    {
        bool result = false;
        KeyId paramKey(keyId, keyLength);
        uint64_t timeOut(Core::Time::Now().Add(waitTime).Ticks());

        do {
            _adminLock.Lock();

            KeyMap::const_iterator session(_sessionKeys.begin());
            const KeyList* container = nullptr;
            KeyList::const_iterator index;

            while ((container == nullptr) && (session != _sessionKeys.end())) {
                index = session->second.begin();

                while ((index != session->second.end()) && (*index != paramKey)) {
                    index++;
                }

                if (index != session->second.end()) {
                    container = &(session->second);
                } else {
                    session++;
                }
            }

            if ((container != nullptr) && (index != container->end())) {
                result = (index->Status() == status);
            }

            if (result == false) {
                _interested++;

                _adminLock.Unlock();

                if ((container != nullptr) && (index != container->end())) {
                    TRACE_L1("Waiting for KeyId: %s, current: %d", paramKey.ToString().c_str(), index->Status());
                } else {
                    TRACE_L1("Waiting for KeyId: %s, current: <Not Found>", paramKey.ToString().c_str());
                }

                uint64_t now(Core::Time::Now().Ticks());

                if (now < timeOut) {
                    _signal.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
                }

                Core::InterlockedDecrement(_interested);
            } else {
                _adminLock.Unlock();
            }
        } while ((result == false) && (timeOut < Core::Time::Now().Ticks()));

        return (result);
    }

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

    virtual OCDM::OCDM_RESULT IsTypeSupported(
        const std::string keySystem,
        const std::string mimeType) const override
    {
        // Do reconnection here again if server is down.
        // This is first call from WebKit when new session is started
        // If ProxyStub return error for this call, there will be not next call from WebKit
        Reconnect();
        return (_remote->IsTypeSupported(keySystem, mimeType));
    }

    // Create a MediaKeySession using the supplied init data and CDM data.
    virtual OCDM::OCDM_RESULT CreateSession(
        const string keySystem,
        const int32_t licenseType,
        const std::string initDataType,
        const uint8_t* initData,
        const uint16_t initDataLength,
        const uint8_t* CDMData,
        const uint16_t CDMDataLength,
        OCDM::ISession::ICallback* callback,
        std::string& sessionId,
        OCDM::ISession*& session) override
    {
        return (_remote->CreateSession(keySystem, licenseType, initDataType, initData, initDataLength, CDMData, CDMDataLength, callback, sessionId, session));
    }

    // Set Server Certificate
    virtual OCDM::OCDM_RESULT SetServerCertificate(
        const string keySystem,
        const uint8_t* serverCertificate,
        const uint16_t serverCertificateLength) override
    {
        return (_remote->SetServerCertificate(keySystem, serverCertificate, serverCertificateLength));
    }

    virtual OCDM::ISession* Session(
        const std::string sessionId) override
    {
        return (_remote->Session(sessionId));
    }

    virtual OCDM::ISession* Session(
        const uint8_t keyId[], const uint8_t length) override
    {
        return (_remote->Session(keyId, length));
    }
    virtual void Register(OCDM::IAccessorOCDM::INotification* notification) override
    {
        _remote->Register(notification);
    }
    virtual void Unregister(OCDM::IAccessorOCDM::INotification* notification) override
    {
        _remote->Unregister(notification);
    }
    void AddSession(const string& sessionId)
    {

        _adminLock.Lock();

        KeyMap::iterator session(_sessionKeys.find(sessionId));

        if (session == _sessionKeys.end()) {
            _sessionKeys.insert(std::pair<string, KeyList>(sessionId, KeyList()));
        } else {
            TRACE_L1("Same session created, again ???? Keep the old one than. [%s]", sessionId.c_str());
        }

        _adminLock.Unlock();
    }
    void RemoveSession(const string& sessionId)
    {

        _adminLock.Lock();

        KeyMap::iterator session(_sessionKeys.find(sessionId));

        if (session != _sessionKeys.end()) {
            _sessionKeys.erase(session);
        } else {
            TRACE_L1("A session is destroyed of which we were not aware [%s]", sessionId.c_str());
        }

        _adminLock.Unlock();
    }
    void KeyUpdate(const string& sessionId, const uint8_t keyId[], const uint8_t keyLength, const OCDM::ISession::KeyStatus status)
    {

        _adminLock.Lock();

        KeyMap::iterator session(_sessionKeys.find(sessionId));

        ASSERT(session != _sessionKeys.end());

        if (session != _sessionKeys.end()) {

            KeyList& container(session->second);
            KeyList::iterator index(container.begin());
            KeyId paramKey(keyId, keyLength, status);

            while ((index != container.end()) && (*index != paramKey)) {
                index++;
            }

            if (index != container.end()) {
                index->Status(status);
            } else {
                container.push_back(paramKey);
            }

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
    }

private:
    mutable uint32_t _refCount;
    mutable Core::ProxyType<RPC::CommunicatorClient> _client;
    mutable OCDM::IAccessorOCDM* _remote;
    mutable Core::CriticalSection _adminLock;
    mutable Core::Event _signal;
    mutable volatile uint32_t _interested;
    std::map<string, std::list<KeyId>> _sessionKeys;
    mutable Core::Sink<Sink> _sink;
    static OpenCDMAccessor* _singleton;
};

/* static */ OpenCDMAccessor* OpenCDMAccessor::_singleton = nullptr;

namespace media {

OpenCdm::OpenCdm()
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{
}

OpenCdm::OpenCdm(const OpenCdm& copy)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(copy._session)
    , _keySystem(copy._keySystem)
{

    if (_session != nullptr) {
        TRACE_L1("Created a copy of OpenCdm instance: %p", this);
        _session->AddRef();
    }
}

OpenCdm::OpenCdm(const std::string& sessionId)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(sessionId);

        if (entry != nullptr) {
            _session = new OpenCDMSession(entry);
            TRACE_L1("Created an OpenCdm instance: %p from session %s, [%p]", this, sessionId.c_str(), entry);
            entry->Release();
        } else {
            TRACE_L1("Failed to create an OpenCdm instance, for session %s", sessionId.c_str());
        }
    } else {
        TRACE_L1("Failed to create an OpenCdm instance: %p for session %s", this, sessionId.c_str());
    }
}

OpenCdm::OpenCdm(const uint8_t keyId[], const uint8_t length)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(keyId, length);

        if (entry != nullptr) {
            _session = new OpenCDMSession(entry);
            // TRACE_L1 ("Created an OpenCdm instance: %p from keyId [%p]", this, entry);
            entry->Release();
        } else {
            TRACE_L1("Failed to create an OpenCdm instance, for keyId [%d]", __LINE__);
        }
    } else {
        TRACE_L1("Failed to create an OpenCdm instance: %p for keyId failed", this);
    }
}

OpenCdm::~OpenCdm()
{
    if (_session != nullptr) {
        _session->Release();
        TRACE_L1("Destructed an OpenCdm instance: %p", this);
    }
    if (_implementation != nullptr) {
        _implementation->Release();
    }
}

/* static */ OpenCdm& OpenCdm::Instance()
{
    return Core::SingletonType<OpenCdm>::Instance();
}

// ---------------------------------------------------------------------------------------------
// INSTANTIATION OPERATIONS:
// ---------------------------------------------------------------------------------------------
// Before instantiating the ROOT DRM OBJECT, Check if it is capable of decrypting the requested
// asset.
bool OpenCdm::GetSession(const uint8_t keyId[], const uint8_t length, const uint32_t waitTime)
{

    if ((_session == nullptr) && (_implementation != nullptr) && (_implementation->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable) == true)) {
        _session = new OpenCDMSession(_implementation->Session(keyId, length));
    }

    return (_session != nullptr);
}

bool OpenCdm::IsTypeSupported(const std::string& keySystem, const std::string& mimeType) const
{
    TRACE_L1("Checking for key system %s", keySystem.c_str());
    return ((_implementation != nullptr) && (_implementation->IsTypeSupported(keySystem, mimeType) == 0));
}

// The next call is the startng point of creating a decryption context. It select the DRM system
// to be used within this OpenCDM object.
void OpenCdm::SelectKeySystem(const std::string& keySystem)
{
    if (_implementation != nullptr) {
        _keySystem = keySystem;
        TRACE_L1("Creation of key system %s succeeded.", _keySystem.c_str());
    } else {
        TRACE_L1("Creation of key system %s failed. No valid remote side", keySystem.c_str());
    }
}

// ---------------------------------------------------------------------------------------------
// ROOT DRM OBJECT OPERATIONS:
// ---------------------------------------------------------------------------------------------
// If required, ServerCertificates can be added to this OpenCdm object (DRM Context).
int OpenCdm::SetServerCertificate(const uint8_t* data, const uint32_t dataLength)
{

    int result = 1;

    if (_keySystem.empty() == false) {

        ASSERT(_implementation != nullptr);

        TRACE_L1("Set server certificate data %d", dataLength);
        result = _implementation->SetServerCertificate(_keySystem, data, dataLength);
    } else {
        TRACE_L1("Setting server certificate failed, there is no key system. %d", __LINE__);
    }

    return result;
}

// Now for every particular stream a session needs to be created. Create a session for all
// encrypted streams that require decryption. (This allows for MultiKey decryption)
std::string OpenCdm::CreateSession(const std::string& dataType, const uint8_t* addData, const uint16_t addDataLength, const uint8_t* cdmData, const uint16_t cdmDataLength, const LicenseType license)
{

    std::string result;

    if (_keySystem.empty() == false) {

        ASSERT(_session == nullptr);

        ExtendedOpenCDMSession* newSession = new ExtendedOpenCDMSession(_implementation, _keySystem, dataType, addData, addDataLength, cdmData, cdmDataLength, static_cast<::LicenseType>(license), nullptr);

        result = newSession->SessionId();

        _session = newSession;

        TRACE_L1("Created an OpenCdm instance: %p for keySystem %s, %p", this, _keySystem.c_str(), newSession);
    } else {
        TRACE_L1("Creating session failed, there is no key system. %d", __LINE__);
    }

    return (result);
}

// ---------------------------------------------------------------------------------------------
// ROOT DRM -> SESSION OBJECT OPERATIONS:
// ---------------------------------------------------------------------------------------------
// The following operations work on a Session. There is no direct access to the session that
// requires the operation, so before executing the session operation, first select it with
// the SelectSession above.
void OpenCdm::GetKeyMessage(std::string& response, uint8_t* data, uint16_t& dataLength)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this session.
    static_cast<ExtendedOpenCDMSession*>(_session)->GetKeyMessage(response, data, dataLength);
}

KeyStatus OpenCdm::Update(const uint8_t* data, const uint16_t dataLength, std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Update(data, dataLength, response));
}

int OpenCdm::Load(std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Load(response));
}

int OpenCdm::Remove(std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Remove(response));
}

KeyStatus OpenCdm::Status() const
{
    KeyStatus result = StatusPending;

    if (_session != nullptr) {
        result = CDMState(_session->Status());
    }

    return (result);
}

int OpenCdm::Close()
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    if (_session != nullptr) {
        _session->Close();
        _session->Release();
        _session = nullptr;
    }

    return (0);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength, const uint8_t* IV, const uint16_t IVLength)
{
    ASSERT(_session != nullptr);

    return (_session != nullptr ? _session->Decrypt(encrypted, encryptedLength, IV, IVLength, nullptr, 0) : 1);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength, const uint8_t* IV, const uint16_t IVLength, const uint8_t keyIdLength, const uint8_t keyId[], const uint32_t waitTime)
{

    if (_implementation->WaitForKey(keyIdLength, keyId, waitTime, OCDM::ISession::Usable) == true) {
        if (_session == nullptr) {
            _session = new OpenCDMSession(_implementation->Session(keyId, keyIdLength));
        }
        return (_session->Decrypt(encrypted, encryptedLength, IV, IVLength, keyId, keyIdLength));
    }

    return (1);
}

} // namespace media

/**
 * \brief Creates DRM system.
 *
 * \param keySystem Name of required key system (See \ref opencdm_is_type_supported)
 * \return \ref OpenCDMAccessor instance, NULL on error.
 */
struct OpenCDMAccessor* opencdm_create_system()
{
    return (OpenCDMAccessor::Instance());
}

/**
 * Destructs an \ref OpenCDMAccessor instance.
 * \param system \ref OpenCDMAccessor instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_destruct_system(struct OpenCDMAccessor* system)
{
    if (system != nullptr) {
        system->Release();
    }
    return (OpenCDMError::ERROR_NONE);
}

/**
 * \brief Checks if a DRM system is supported.
 *
 * \param keySystem Name of required key system (e.g. "com.microsoft.playready").
 * \param mimeType MIME type.
 * \return Zero if supported, Non-zero otherwise.
 * \remark mimeType is currently ignored.
 */
OpenCDMError opencdm_is_type_supported(struct OpenCDMAccessor* system, const char keySystem[], const char mimeType[])
{
    OpenCDMError result(OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED);

    if ((system != nullptr) && (system->IsTypeSupported(std::string(keySystem), std::string(mimeType)) == 0)) {
        result = OpenCDMError::ERROR_NONE;
    }
    return (result);
}

/**
 * \brief Maps key ID to \ref OpenCDMSession instance.
 *
 * In some situations we only have the key ID, but need the specific \ref OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 * REPLACING: void* acquire_session(const uint8_t* keyId, const uint8_t keyLength, const uint32_t waitTime);
 */
struct OpenCDMSession* opencdm_get_session(struct OpenCDMAccessor* system, const uint8_t keyId[], const uint8_t length, const uint32_t waitTime)
{
    struct OpenCDMSession* result = nullptr;

    if ((system != nullptr) && (system->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable) == true)) {
        OCDM::ISession* session(system->Session(keyId, length));

        if (session != nullptr) {
            result = new OpenCDMSession(session);
        }
    }

    return (result);
}

/**
 * \brief Sets server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method will set that certificate. Other DRMs will ignore this call.
 * \param serverCertificate Buffer containing certificate data.
 * \param serverCertificateLength Buffer length of certificate data.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_system_set_server_certificate(struct OpenCDMAccessor* system, const char keySystem[], const uint8_t serverCertificate[], uint16_t serverCertificateLength)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(system->SetServerCertificate(keySystem, serverCertificate, serverCertificateLength));
    }
    return (result);
}

/**
 * \brief Create DRM session (for actual decrypting of data).
 *
 * Creates an instance of \ref OpenCDMSession using initialization data.
 * \param keySystem DRM system to create the session for.
 * \param licenseType DRM specifc signed integer selecting License Type (e.g. "Limited Duration" for PlayReady).
 * \param initDataType Type of data passed in \ref initData.
 * \param initData Initialization data.
 * \param initDataLength Length (in bytes) of initialization data.
 * \param CDMData CDM data.
 * \param CDMDataLength Length (in bytes) of \ref CDMData.
 * \param session Output parameter that will contain pointer to instance of \ref OpenCDMSession.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_create_session(struct OpenCDMAccessor* system, const char keySystem[], const LicenseType licenseType,
    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks,
    struct OpenCDMSession** session)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *session = new ExtendedOpenCDMSession(static_cast<OCDM::IAccessorOCDM*>(system), std::string(keySystem), std::string(initDataType), initData, initDataLength, CDMData, CDMDataLength, licenseType, callbacks);

        result = (*session != nullptr ? OpenCDMError::ERROR_NONE : OpenCDMError::ERROR_INVALID_SESSION);
    }

    return (result);
}

/**
 * Destructs an \ref OpenCDMSession instance.
 * \param system \ref OpenCDMSession instance to desctruct.
 * \return Zero on success, non-zero on error.
 * REPLACING: void release_session(void* session);
 */
OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = OpenCDMError::ERROR_NONE;
        session->Release();
    }

    return (result);
}

/**
 * Loads the data stored for a specified OpenCDM session into the CDM context.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_load(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Load());
    }

    return (result);
}

/**
 * Gets session ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return ExtendedOpenCDMSession ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_id(const struct OpenCDMSession* session)
{
    const char* result = EmptyString;
    if (session != nullptr) {
        result = session->SessionId().c_str();
    }
    return (result);
}

/**
 * Gets buffer ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Buffer ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_buffer_id(const struct OpenCDMSession* session)
{
    const char* result = EmptyString;
    if (session != nullptr) {
        result = session->BufferId().c_str();
    }
    return (result);
}

/**
 * Returns status of a particular key assigned to a session.
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return key status.
 */
KeyStatus opencdm_session_status(const struct OpenCDMSession* session, const uint8_t keyId[], uint8_t length)
{
    KeyStatus result(KeyStatus::InternalError);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<const ExtendedOpenCDMSession*>(session)->Status(keyId, length);
    }

    return (result);
}

/**
 * Returns error for key (if any).
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return Key error (zero if no error, non-zero if error).
 */
uint32_t opencdm_session_error(const struct OpenCDMSession* session, const uint8_t keyId[], uint8_t length)
{
    uint32_t result(~0);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<const ExtendedOpenCDMSession*>(session)->Error(keyId, length);
    }

    return (result);
}

/**
 * Returns system error. This reference general system, instead of specific key.
 * \param session \ref OpenCDMSession instance.
 * \return System error code, zero if no error.
 */
OpenCDMError opencdm_session_system_error(const struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<OpenCDMError>(static_cast<const ExtendedOpenCDMSession*>(session)->Error());
    }

    return (result);
}

/**
 * Process a key message response.
 * \param session \ref OpenCDMSession instance.
 * \param keyMessage Key message to process.
 * \param keyLength Length of key message buffer (in bytes).
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_update(struct OpenCDMSession* session, const uint8_t keyMessage[], uint16_t keyLength)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->Update(keyMessage, keyLength);
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * Removes all keys/licenses related to a session.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_remove(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Remove());
    }

    return (result);
}

/**
 * Closes a session.
 * \param session \ref OpenCDMSession instance.
 * \return zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_close(struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        session->Close();
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

/**
 * \brief Performs decryption.
 *
 * This method accepts encrypted data and will typically decrypt it out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param encrypted Buffer containing encrypted data. If applicable, decrypted data will be stored here after this call returns.
 * \param encryptedLength Length of encrypted data buffer (in bytes).
 * \param IV Initial vector (IV) used during decryption.
 * \param IVLength Length of IV buffer (in bytes).
 * \return Zero on success, non-zero on error.
 * REPLACING: uint32_t decrypt(void* session, uint8_t*, const uint32_t, const uint8_t*, const uint16_t);
 */
OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session, uint8_t encrypted[], const uint32_t encryptedLength, const uint8_t IV[], const uint16_t IVLength)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Decrypt(encrypted, encryptedLength, IV, IVLength, nullptr, 0));
    }

    return (result);
}

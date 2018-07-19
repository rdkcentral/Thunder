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
#include "Module.h"
#include "DataExchange.h"
#include "IOCDM.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

namespace media {

static Core::CriticalSection _systemLock;

static OpenCdm::KeyStatus CDMState(const OCDM::ISession::KeyStatus state) {

    switch(state) {
        case OCDM::ISession::StatusPending:    return OpenCdm::KeyStatus::StatusPending;
        case OCDM::ISession::Usable:           return OpenCdm::KeyStatus::Usable;
        case OCDM::ISession::InternalError:    return OpenCdm::KeyStatus::InternalError;
        case OCDM::ISession::Released:         return OpenCdm::KeyStatus::Released;
        case OCDM::ISession::Expired:          return OpenCdm::KeyStatus::Expired;
        default:           assert(false); 
    }
 
    return OpenCdm::KeyStatus::InternalError;
}

class AccessorOCDM : public OCDM::IAccessorOCDM {
private:
    AccessorOCDM () = delete;
    AccessorOCDM (const AccessorOCDM&) = delete;
    AccessorOCDM& operator= (const AccessorOCDM&) = delete;

private:
    class RPCClient {
    private:
        RPCClient() = delete;
        RPCClient(const RPCClient&) = delete;
        RPCClient& operator=(const RPCClient&) = delete;

        typedef WPEFramework::RPC::InvokeServerType<4, 1> RPCService;

    public:
        RPCClient(const Core::NodeId& nodeId)
            : _client(Core::ProxyType<RPC::CommunicatorClient>::Create(nodeId))
            , _service(Core::ProxyType<RPCService>::Create(Core::Thread::DefaultStackSize())) {

            _client->CreateFactory<RPC::InvokeMessage>(2);
            if (_client->Open(RPC::CommunicationTimeOut) == Core::ERROR_NONE) {

                // TODO, Seems the announce is still progressing, make sure the open blocks, till it completes.
                SleepMs(100);

                // I would not expect that the next line is needed, left it in for reference for testing.
                // If it is neede, it needs to move to the RPC::CommunicatorClient..
                _client->Register(_service);
            }
            else {
                _client.Release();
            }
        }
        ~RPCClient() {
            if (_client.IsValid() == true) {
                _client->Unregister(_service);
                _client.Release();
            }
        }

    public:
        inline bool IsOperational() const {
            return (_client.IsValid());
        }

        template <typename INTERFACE>
        INTERFACE* Create(const string& objectName, const uint32_t version = static_cast<uint32_t>(~0)) {
            INTERFACE* result = nullptr;

            if (_client.IsValid() == true) {
                // Oke we could open the channel, lets get the interface
                result = _client->Create<INTERFACE>(objectName, version);
            }

            return (result);
        }

    private:
        Core::ProxyType<RPC::CommunicatorClient> _client;
        Core::ProxyType<RPCService> _service;
    };
    class KeyId {
    private:
        KeyId() = delete;
        KeyId& operator= (const KeyId& rhs) = delete;

    public:
        inline KeyId(const KeyId& copy) 
            :  _status(copy._status) {
            ::memcpy(_kid, copy._kid, sizeof(_kid));
        }
        inline KeyId(const uint8_t kid[], const uint8_t length, const ::OCDM::ISession::KeyStatus status = OCDM::ISession::StatusPending)
            :  _status(status) {
            uint8_t copyLength (length > sizeof(_kid) ? sizeof(_kid) : length);

            ::memcpy(_kid, kid, copyLength);

            if (copyLength < sizeof(_kid)) {
                ::memset(&(_kid[copyLength]), 0, sizeof(_kid) - copyLength);
            }
        }
        inline ~KeyId() {
        }

    public:
        inline bool operator==(const KeyId& rhs) const {
            return (::memcmp(_kid, rhs._kid, sizeof(_kid)) == 0);
        }
        inline bool operator!=(const KeyId& rhs) const {
            return !(operator==(rhs));
        }
        inline const uint8_t* Id() const {
            return (_kid);
        }
        inline static uint8_t Length() {
            return (sizeof(_kid));
        }
        inline void Status (::OCDM::ISession::KeyStatus status) {
            _status = status;
        }
        ::OCDM::ISession::KeyStatus Status () const {
            return (_status);
        }
        string ToString() const {
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
        Sink& operator= (const Sink&) = delete;

    public:
        Sink(AccessorOCDM& parent) 
            : _parent(parent) {
        }
        virtual ~Sink() {
        }

    private: 
        virtual void Create(const string& sessionId) override {
            _parent.AddSession(sessionId);
        }
        virtual void Destroy(const string& sessionId) override {
            _parent.RemoveSession(sessionId);
        }
        virtual void KeyChange(const string& sessionId, const uint8_t keyId[], const uint8_t length, const OCDM::ISession::KeyStatus status) override {
            _parent.KeyUpdate(sessionId, keyId, length, status);
        }

    private:
        AccessorOCDM& _parent;
    };
    typedef std::list<KeyId> KeyList;
    typedef std::map<string, KeyList > KeyMap;

private:
    AccessorOCDM (const TCHAR domainName[]) 
        : _refCount(1)
        , _client(Core::NodeId(domainName))
        , _remote(nullptr)
        , _adminLock()
        , _signal(false, true)
        , _interested(0)
        , _sessionKeys() {

        if (_client.IsOperational() == true) { 
            _remote = _client.Create<OCDM::IAccessorOCDM>(_T(""));
        }
    }

public:
    static AccessorOCDM* Instance () {

        _systemLock.Lock();

        if (_singleton == nullptr) {
            AccessorOCDM* result = new AccessorOCDM ("/tmp/ocdm"); 

            if (result->_remote != nullptr) {
                _singleton = result;
            }
            else {
                delete result;
            }
        } 
        else {
            _singleton->AddRef();
        }

        _systemLock.Unlock();

        return (_singleton);
    }
    ~AccessorOCDM() {
        if (_remote != nullptr) {
            _remote->Release();
        }
        _singleton = nullptr;
        TRACE_L1("Destructed the AccessorOCDM %p", this);
    }
    bool WaitForKey (const uint8_t keyLength, const uint8_t keyId[], const uint32_t waitTime, const OCDM::ISession::KeyStatus status) const {
        bool result = false;
        KeyId paramKey (keyId, keyLength);
        uint64_t timeOut (Core::Time::Now().Add(waitTime).Ticks());

        do {
            _adminLock.Lock();

            KeyMap::const_iterator session (_sessionKeys.begin());
            const KeyList* container = nullptr;
            KeyList::const_iterator index;

            while ((container == nullptr) && (session != _sessionKeys.end())) {
                index  = session->second.begin();

                while ((index != session->second.end()) && (*index != paramKey)) { index++; }

                if (index != session->second.end()) {
                    container = &(session->second);
                }
                else {
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
                }
                else {
                    TRACE_L1("Waiting for KeyId: %s, current: <Not Found>", paramKey.ToString().c_str());
                }

                uint64_t now (Core::Time::Now().Ticks());

                if (now < timeOut) {
                    _signal.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
                }

                Core::InterlockedDecrement(_interested);
            }
            else {
                _adminLock.Unlock();
            }

        } while ((result == false) && (timeOut < Core::Time::Now().Ticks()));

        return (result);
    }
 
private:
    virtual void AddRef() const override {
        Core::InterlockedIncrement(_refCount);
    }
    virtual uint32_t Release() const override {

        _systemLock.Lock();

        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
            return (Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        _systemLock.Unlock();

        return (Core::ERROR_NONE);
    }
    BEGIN_INTERFACE_MAP(AccessorOCDM)
        INTERFACE_ENTRY(OCDM::IAccessorOCDM)
    END_INTERFACE_MAP

    virtual OCDM::OCDM_RESULT IsTypeSupported(
        const std::string keySystem,
        const std::string mimeType) const override {
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
        OCDM::ISession*& session) override {
        return (_remote->CreateSession(keySystem, licenseType, initDataType, initData, initDataLength, CDMData, CDMDataLength, callback, sessionId, session));
    }

    // Set Server Certificate
    virtual OCDM::OCDM_RESULT SetServerCertificate(
        const string keySystem,
        const uint8_t* serverCertificate,
        const uint16_t serverCertificateLength) override {
        return (_remote->SetServerCertificate(keySystem, serverCertificate, serverCertificateLength));
    }
 
    virtual OCDM::ISession* Session(
        const std::string sessionId) override {
        return (_remote->Session(sessionId));
    }

    virtual OCDM::ISession* Session(
        const uint8_t keyId[], const uint8_t length) override {
        return (_remote->Session(keyId, length));
    }
    virtual void Register(OCDM::IAccessorOCDM::INotification* notification) override {
        _remote->Register(notification);
    }
    virtual void Unregister(OCDM::IAccessorOCDM::INotification* notification) override {
        _remote->Unregister(notification);
    }
    void AddSession(const string& sessionId) {

        _adminLock.Lock();

        KeyMap::iterator session (_sessionKeys.find(sessionId));

        if (session == _sessionKeys.end()) {
            _sessionKeys.insert(std::pair<string, KeyList>(sessionId, KeyList()));
        }
        else {
            TRACE_L1("Same session created, again ???? Keep the old one than. [%s]", sessionId.c_str());
        }

        _adminLock.Unlock();
    }
    void RemoveSession(const string& sessionId) {

        _adminLock.Lock();

        KeyMap::iterator session (_sessionKeys.find(sessionId));

        if (session != _sessionKeys.end()) {
            _sessionKeys.erase(session);
        }
        else {
            TRACE_L1("A session is destroyed of which we were not aware [%s]", sessionId.c_str()); 
        }

        _adminLock.Unlock();
    }
    void KeyUpdate(const string& sessionId, const uint8_t keyId[], const uint8_t keyLength, const OCDM::ISession::KeyStatus status) {

        _adminLock.Lock();

        KeyMap::iterator session (_sessionKeys.find(sessionId));
        
        ASSERT (session != _sessionKeys.end());

        if (session != _sessionKeys.end()) {

            KeyList& container (session->second);
            KeyList::iterator  index(container.begin());
            KeyId paramKey (keyId, keyLength, status);

            while ((index != container.end()) && (*index != paramKey)) { index++; }

            if (index != container.end()) {
                index->Status(status);
            }
            else {
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
    RPCClient _client;
    OCDM::IAccessorOCDM* _remote;
    mutable Core::CriticalSection _adminLock;
    mutable Core::Event _signal;
    mutable volatile uint32_t _interested;
    std::map<string, std::list<KeyId> > _sessionKeys;
    static AccessorOCDM* _singleton;
};

/* static */ AccessorOCDM* AccessorOCDM::_singleton = nullptr;

 
    class OpenCdmSession {
    private:
        OpenCdmSession(const OpenCdmSession&) = delete;
        OpenCdmSession& operator= (OpenCdmSession&) = delete;

    private:
        class DataExchange : public OCDM::DataExchange {
        private:
            DataExchange () = delete;
            DataExchange (const DataExchange&) = delete;
            DataExchange& operator= (DataExchange&) = delete;

       public:
            DataExchange (const string& bufferName)
                : OCDM::DataExchange (bufferName)
                , _busy(false) {

                TRACE_L1("Constructing buffer client side: %p - %s", this, bufferName.c_str());
            }
            virtual ~DataExchange () {
                if (_busy == true) {
                    TRACE_L1("Destructed a DataExchange while still in progress. %p", this);
                }
                TRACE_L1("Destructing buffer client side: %p - %s", this, OCDM::DataExchange::Name().c_str());
            }

        public:
           uint32_t Decrypt(uint8_t* encryptedData, uint32_t encryptedDataLength, const uint8_t* ivData, uint16_t ivDataLength) {
                int ret = 0;

                // This works, because we know that the Audio and the Video streams are fed from
                // the same process, so they will use the same critial section and thus will 
                // not interfere with each-other. If Audio and video will be located into two
                // different processes, start using the administartion space to share a lock.
                _systemLock.Lock();

                _busy = true;

                if (RequestProduce(WPEFramework::Core::infinite) == WPEFramework::Core::ERROR_NONE) {

                    SetIV(ivDataLength, ivData);
                    SetSubSampleData(0, nullptr);
                    Write(encryptedDataLength, encryptedData);

                    // This will trigger the OpenCDMIServer to decrypt this memory...
                    Produced();

                    // Now we should wait till it is decrypted, that happens if the Producer, can run again.
                    if (RequestProduce(WPEFramework::Core::infinite) == WPEFramework::Core::ERROR_NONE) {

                        // For nowe we just copy the clear data..
                        Read(encryptedDataLength, encryptedData);

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
        OpenCdmSession()
            : _session(nullptr)
            , _decryptSession(nullptr)
            , _refCount(1) {
            TRACE_L1("Constructing the Session Client side: %p, (nil)", this);
        }
        explicit OpenCdmSession(OCDM::ISession* session)
            : _session(session)
            , _decryptSession(_session->BufferId())
            , _refCount(1) {

            ASSERT(session != nullptr);

            if (_session != nullptr) {
                _session->AddRef();
            }
        }
        virtual ~OpenCdmSession() {
            if (_session != nullptr) {
                _session->Release();
            }
            TRACE_L1("Destructed the Session Client side: %p", this);
        }

    public:
        void AddRef() {
            Core::InterlockedIncrement(_refCount);
        }
        bool Release() {
            if (Core::InterlockedDecrement(_refCount) == 0) {

                delete this;

                return (true);
            }
            return (false);
        }
        inline bool IsValid() const {

            return (_session != nullptr);
        }
        inline OCDM::ISession::KeyStatus Status () const {

            return (_session != nullptr ? _session->Status() : OCDM::ISession::StatusPending);
        }
        inline void Close() {
            ASSERT (_session != nullptr);

            _session->Close();
        }

        inline int Remove() {

            ASSERT (_session != nullptr);

            return (_session->Remove() == 0);
        }
        inline int Load() {

            ASSERT (_session != nullptr);

            return (_session->Load() == 0);
        }
        inline void Update (const uint8_t* pbResponse, const uint16_t cbResponse) {

            ASSERT (_session != nullptr);

            _session->Update(pbResponse, cbResponse);
        }
        uint32_t Decrypt(uint8_t* encryptedData, const uint32_t encryptedDataLength, const uint8_t* ivData, uint16_t ivDataLength) {
            ASSERT (_decryptSession != nullptr);

            return ( _decryptSession.Decrypt(encryptedData, encryptedDataLength, ivData, ivDataLength) );
        }
        inline void Revoke (OCDM::ISession::ICallback* callback) {

            ASSERT (_session != nullptr);
            ASSERT (callback != nullptr);

            return (_session->Revoke(callback));
        }

    protected:
        void Session(OCDM::ISession* session) {
            ASSERT ((_session == nullptr) ^ (session == nullptr));

            if ( (session == nullptr) && (_session != nullptr) ) {
                _session->Release();
            }
            _session = session;
        }

    private:
        OCDM::ISession* _session;
        DataExchange _decryptSession;
        uint32_t _refCount;
    };

    class Session : public OpenCdmSession {
    private:
        Session() = delete;
        Session(const Session&) = delete;
        Session& operator= (Session&) = delete;

    enum sessionState {
        // Initialized.
        SESSION_INIT    = 0x00,

        // Session created, waiting for message callback.
        SESSION_MESSAGE = 0x01,
        SESSION_READY   = 0x02,
        SESSION_ERROR   = 0x04,
        SESSION_LOADED  = 0x08,
        SESSION_UPDATE  = 0x10
    };

    private:
        class Sink : public OCDM::ISession::ICallback {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;
        public:
            Sink(Session* parent) 
                : _parent(*parent) {
                ASSERT(parent != nullptr);
            }
            virtual ~Sink() {
            }

        public:
            // Event fired when a key message is successfully created.
            virtual void OnKeyMessage(
                const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
                const uint16_t keyLength, //__in
                const std::string URL) {
                _parent.OnKeyMessage(std::string(reinterpret_cast<const char*>(keyMessage), keyLength), URL);
            }
            // Event fired when MediaKeySession has found a usable key.
            virtual void OnKeyReady() {
                _parent.OnKeyReady();
            }
            // Event fired when MediaKeySession encounters an error.
            virtual void OnKeyError(
                const int16_t error,
                const OCDM::OCDM_RESULT sysError,
                const std::string errorMessage) {
                _parent.OnKeyError(error, sysError, errorMessage);
            }
            // Event fired on key status update
            virtual void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus keyMessage) {
                _parent.OnKeyStatusUpdate(keyMessage);
            }

            BEGIN_INTERFACE_MAP(Sink)
                INTERFACE_ENTRY(OCDM::ISession::ICallback)
            END_INTERFACE_MAP
 
        private:
            Session& _parent;
        };

    public:
        Session(
            OCDM::IAccessorOCDM* system,
            const string keySystem, 
            const std::string& initDataType, 
            const uint8_t* pbInitData, 
            const uint16_t cbInitData, 
            const uint8_t* pbCustomData, 
            const uint16_t cbCustomData, 
            const OpenCdm::LicenseType licenseType) 
            : OpenCdmSession()
            , _sink(this)
            , _state(SESSION_INIT)
            , _message()
            , _URL()
            , _error()
            , _key(OCDM::ISession::StatusPending)
            , _sessionId() {

            std::string bufferId;
            OCDM::ISession* realSession = nullptr;

            system->CreateSession(keySystem, licenseType, initDataType, pbInitData, cbInitData, pbCustomData, cbCustomData, &_sink, _sessionId, realSession);

            if (realSession == nullptr) {
                TRACE_L1("Creating a Session failed. %d", __LINE__);
            }
            else {
                OpenCdmSession::Session(realSession);
            }
        }
        virtual ~Session() {
            if (OpenCdmSession::IsValid() == true) {
                Revoke(&_sink);
                OpenCdmSession::Session(nullptr);
            }
        }

    public:
        inline OpenCdm::KeyStatus Status () const {
            return (CDMState(_key));
        }
        inline const std::string& SessionId() const {
            return(_sessionId);
        }
        void GetKeyMessage(std::string& challenge, uint8_t* licenseURL, uint16_t& urlLength) {

            ASSERT (IsValid() == true);

            _state.WaitState(SESSION_MESSAGE|SESSION_READY, WPEFramework::Core::infinite);

            if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
                challenge = _message;
                if (urlLength > static_cast<int>(_URL.length())) {
                    urlLength = _URL.length();
                }
                memcpy(licenseURL, _URL.c_str(), urlLength);
                TRACE_L1("Returning a KeyMessage, Length: [%d,%d]", urlLength, static_cast<uint32_t>(challenge.length()));
            }
            else if ((_state & SESSION_READY) == SESSION_READY) {
                challenge.clear();
                *licenseURL = '\0';
                urlLength = 0;
                TRACE_L1("Returning a KeyMessage failed. %d", __LINE__);
            }
        }
        int Load (std::string& response) { 
           int ret = 1;

           _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE|SESSION_MESSAGE)));

           response.clear();

           if (OpenCdmSession::Load() == 0) {

                _state.WaitState(SESSION_UPDATE, WPEFramework::Core::infinite);

                if (_key == OCDM::ISession::Usable) {
                    ret = 0;
                }
                else if (_state  == SESSION_MESSAGE) {
                    ret = 0;
                    response = "message:" + _message;
                }
            }

            return ret;
        }
        OpenCdm::KeyStatus Update(const uint8_t* pbResponse, const uint16_t cbResponse, std::string& response) {

            _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE|SESSION_MESSAGE)));

            OpenCdmSession::Update(pbResponse, cbResponse);

            _state.WaitState(SESSION_UPDATE | SESSION_MESSAGE, WPEFramework::Core::infinite);
            if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
                response = "message:" + _message;
            }

            return CDMState(_key);
        }
        int Remove(std::string& response) {
            int ret = 1;

            _state =  static_cast<sessionState>(_state & (~(SESSION_UPDATE|SESSION_MESSAGE)));

            if (OpenCdmSession::Remove() == 0) {

                _state.WaitState(SESSION_UPDATE, WPEFramework::Core::infinite);

                if (_key ==  OCDM::ISession::StatusPending) {
                    ret = 0;
                }
                else if (_state  == SESSION_MESSAGE) {
                    ret = 0;
                    response = "message:" + _message;
                }
            }

            return (ret);
        }

   private:
        // Event fired when a key message is successfully created.
        void OnKeyMessage(const std::string& keyMessage, const std::string& URL) {
            _message = keyMessage;
            _URL = URL;
            TRACE_L1("Received URL: [%s]", _URL.c_str());
            _state = static_cast<sessionState>(_state | SESSION_MESSAGE | SESSION_UPDATE);
        }
        // Event fired when MediaKeySession has found a usable key.
        void OnKeyReady() {
            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        }
        // Event fired when MediaKeySession encounters an error.
        void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError, const std::string& errorMessage) {
            _error = errorMessage;
            _state = static_cast<sessionState>(_state | SESSION_ERROR | SESSION_UPDATE);
        }
        // Event fired on key status update
        void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status) {
            _key = status;

            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        }

    private:
        WPEFramework::Core::Sink<Sink> _sink;
        WPEFramework::Core::StateTrigger<sessionState> _state;
        std::string _message;
        std::string _URL;
        std::string _error;
        OCDM::ISession::KeyStatus _key;
        std::string _sessionId;
    };

OpenCdm::OpenCdm() : _implementation (AccessorOCDM::Instance()), _session(nullptr), _keySystem() {
}

OpenCdm::OpenCdm(const OpenCdm& copy) : _implementation (AccessorOCDM::Instance()), _session(copy._session), _keySystem(copy._keySystem) {
    
    if (_session != nullptr) {
        TRACE_L1 ("Created a copy of OpenCdm instance: %p", this);
        _session->AddRef();
    }
}

OpenCdm::OpenCdm(const std::string& sessionId) : _implementation (AccessorOCDM::Instance()), _session(nullptr), _keySystem() {

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(sessionId);

        if (entry != nullptr) {
            _session = new OpenCdmSession(entry);
            TRACE_L1 ("Created an OpenCdm instance: %p from session %s, [%p]", this, sessionId.c_str(), entry);
            entry->Release();
        }
        else {
            TRACE_L1 ("Failed to create an OpenCdm instance, for session %s", sessionId.c_str());
        }
    }
    else {
        TRACE_L1 ("Failed to create an OpenCdm instance: %p for session %s", this, sessionId.c_str());
    }
}

OpenCdm::OpenCdm (const uint8_t keyId[], const uint8_t length)  : _implementation (AccessorOCDM::Instance()), _session(nullptr), _keySystem() {

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(keyId, length);

        if (entry != nullptr) {
            _session = new OpenCdmSession(entry);
            // TRACE_L1 ("Created an OpenCdm instance: %p from keyId [%p]", this, entry);
            entry->Release();
        }
        else {
            TRACE_L1 ("Session not yet available, maybe we need to wait for it [%d]", __LINE__);
        }
    }
    else {
        TRACE_L1 ("Failed to create an OpenCdm instance: %p for keyId failed", this);
    }
}

OpenCdm::~OpenCdm() {
    if (_session != nullptr) {
        _session->Release();
        TRACE_L1 ("Destructed an OpenCdm instance: %p", this);
    }
    if (_implementation != nullptr) {
        _implementation->Release();
    }
}
 
// ---------------------------------------------------------------------------------------------
// INSTANTIATION OPERATIONS:
// ---------------------------------------------------------------------------------------------
// Before instantiating the ROOT DRM OBJECT, Check if it is capable of decrypting the requested
// asset.
bool OpenCdm::IsTypeSupported(const std::string& keySystem, const std::string& mimeType) const {
    TRACE_L1("Checking for key system %s", keySystem.c_str());
    return ( (_implementation != nullptr) && 
             (_implementation->IsTypeSupported(keySystem, mimeType) == 0) ); 
}

// The next call is the startng point of creating a decryption context. It select the DRM system 
// to be used within this OpenCDM object.
void OpenCdm::SelectKeySystem(const std::string& keySystem) {
    if (_implementation != nullptr) {
        _keySystem = keySystem;
        TRACE_L1("Creation of key system %s succeeded.", _keySystem.c_str());
    }
    else {
        TRACE_L1("Creation of key system %s failed. No valid remote side", keySystem.c_str());
    }
}

// ---------------------------------------------------------------------------------------------
// ROOT DRM OBJECT OPERATIONS:
// ---------------------------------------------------------------------------------------------
// If required, ServerCertificates can be added to this OpenCdm object (DRM Context).
int OpenCdm::SetServerCertificate(const uint8_t* data, const uint32_t dataLength) {

    int result = 1;

    if (_keySystem.empty() == false) {

        ASSERT (_implementation != nullptr);

        TRACE_L1("Set server certificate data %d", dataLength);
        result = _implementation->SetServerCertificate(_keySystem, data, dataLength);
    }
    else {
        TRACE_L1("Setting server certificate failed, there is no key system. %d", __LINE__);
    }

    return result;
}
 
// Now for every particular stream a session needs to be created. Create a session for all
// encrypted streams that require decryption. (This allows for MultiKey decryption)
std::string OpenCdm::CreateSession(const std::string& dataType, const uint8_t* addData, const uint16_t addDataLength, const LicenseType license) {

    std::string result;

    if (_keySystem.empty() == false) {

        ASSERT (_session == nullptr);

        Session* newSession = new Session (_implementation, _keySystem, dataType, addData, addDataLength, nullptr, 0, license);

        result = newSession->SessionId();

        _session = newSession;

        TRACE_L1("Created an OpenCdm instance: %p for keySystem %s, %p", this, _keySystem.c_str(), newSession);
    }
    else {
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
void OpenCdm::GetKeyMessage(std::string& response, uint8_t* data, uint16_t& dataLength) {

    ASSERT ( (_session != nullptr) && (dynamic_cast<Session*> (_session) != nullptr) );

    TRACE_L1("%s",  __PRETTY_FUNCTION__);

    // Oke a session has been selected. Operation should take place on this session.
    static_cast<Session*>(_session)->GetKeyMessage(response, data, dataLength);
}

OpenCdm::KeyStatus OpenCdm::Update(const uint8_t* data, const uint16_t dataLength, std::string& response) {

    ASSERT ( (_session != nullptr) && (dynamic_cast<Session*> (_session) != nullptr) );

    TRACE_L1("%s",  __PRETTY_FUNCTION__);

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<Session*>(_session)->Update(data, dataLength, response));
}

int OpenCdm::Load(std::string& response) {

    ASSERT ( (_session != nullptr) && (dynamic_cast<Session*> (_session) != nullptr) );

    TRACE_L1("%s",  __PRETTY_FUNCTION__);

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<Session*>(_session)->Load(response));
}

int OpenCdm::Remove(std::string& response) {

    ASSERT ( (_session != nullptr) && (dynamic_cast<Session*> (_session) != nullptr) );

    TRACE_L1("%s",  __PRETTY_FUNCTION__);

    // Oke a session has been selected. Operation should take place on this session.
    return (static_cast<Session*>(_session)->Remove(response));
}

OpenCdm::KeyStatus OpenCdm::Status() const {
    KeyStatus result = StatusPending;

    if (_session != nullptr) {
        result = CDMState(_session->Status());
    }

    return (result);
}

int OpenCdm::Close() {

    ASSERT ( (_session != nullptr) && (dynamic_cast<Session*> (_session) != nullptr) );

    TRACE_L1("%s",  __PRETTY_FUNCTION__);

    if (_session != nullptr) {
        _session->Close();
        _session->Release();
        _session = nullptr;
    }

    return (0);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength, const uint8_t* IV, const uint16_t IVLength) {
    ASSERT (_session != nullptr);

    return (_session != nullptr ? _session->Decrypt(encrypted, encryptedLength, IV, IVLength) : 1);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength, const uint8_t* IV, const uint16_t IVLength, const uint8_t keyIdLength, const uint8_t keyId[], const uint32_t waitTime) {

    if (_implementation->WaitForKey (keyIdLength, keyId, waitTime, OCDM::ISession::Usable) == true) {
        if (_session == nullptr) {
            _session = new OpenCdmSession(_implementation->Session(keyId, keyIdLength));
        }
        return (_session->Decrypt(encrypted, encryptedLength, IV, IVLength));
    }

    return (1);
}

} // namespace media

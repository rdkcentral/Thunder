#ifndef EXTENDEDOPENCDMSESSION_H
#define EXTENDEDOPENCDMSESSION_H

#include "Module.h"
#include "open_cdm.h"
#include "DataExchange.h"
#include "IOCDM.h"
#include "open_cdm_impl.h"
using namespace WPEFramework;

extern Core::CriticalSection _systemLock;
extern const char EmptyString[];

KeyStatus CDMState(const OCDM::ISession::KeyStatus state);

struct ExtendedOpenCDMSession : public OpenCDMSession {
private:
    ExtendedOpenCDMSession() = delete;
    ExtendedOpenCDMSession(const ExtendedOpenCDMSession&) = delete;
    ExtendedOpenCDMSession& operator= (ExtendedOpenCDMSession&) = delete;

    enum sessionState {
        // Initialized.
        SESSION_INIT    = 0x00,

        // ExtendedOpenCDMSession created, waiting for message callback.
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
        Sink(ExtendedOpenCDMSession* parent) 
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
        OpenCDMSessionCallbacks * callbacks)
        : OpenCDMSession()
        , _sink(this)
        , _state(SESSION_INIT)
        , _message()
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(0)
        , _key(OCDM::ISession::StatusPending)
        , _callback(callbacks) {

        std::string bufferId;
        OCDM::ISession* realSession = nullptr;

        system->CreateSession(keySystem, licenseType, initDataType, pbInitData, cbInitData, pbCustomData, cbCustomData, &_sink, _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        }
        else {
            OpenCDMSession::Session(realSession);
        }
    }
    virtual ~ExtendedOpenCDMSession() {
        if (OpenCDMSession::IsValid() == true) {
            Revoke(&_sink);
            OpenCDMSession::Session(nullptr);
        }
    }

public:
    virtual bool IsExtended() const override {
        return (true);
    }
    inline KeyStatus Status (const uint8_t /* keyId */[], uint8_t /* length */) const {
        return (::CDMState(_key));
    }
    inline uint32_t Error() const {
        return (_errorCode);
    }
    inline uint32_t Error(const uint8_t keyId[], uint8_t length) const {
        return (_sysError);
    }
    void GetKeyMessage(std::string& challenge, uint8_t* licenseURL, uint16_t& urlLength) {

        ASSERT (IsValid() == true);

        _state.WaitState(SESSION_MESSAGE|SESSION_READY, WPEFramework::Core::infinite);

        if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
            challenge = _message;
            if (urlLength > static_cast<int>(_URL.length())) {
                urlLength = static_cast<uint16_t>(_URL.length());
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

        if (OpenCDMSession::Load() == 0) {

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
    KeyStatus Update(const uint8_t* pbResponse, const uint16_t cbResponse, std::string& response) {

        _state = static_cast<sessionState>(_state & (~(SESSION_UPDATE|SESSION_MESSAGE)));

        OpenCDMSession::Update(pbResponse, cbResponse);

        _state.WaitState(SESSION_UPDATE | SESSION_MESSAGE, WPEFramework::Core::infinite);
        if ((_state & SESSION_MESSAGE) == SESSION_MESSAGE) {
            response = "message:" + _message;
        }

        return CDMState(_key);
    }
    int Remove(std::string& response) {
        int ret = 1;

        _state =  static_cast<sessionState>(_state & (~(SESSION_UPDATE|SESSION_MESSAGE)));

        if (OpenCDMSession::Remove() == 0) {

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
    // void (*process_challenge) (void * userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength);
    // void (*key_update)        (void * userData, const uint8_t keyId[], const uint8_t length);
    // void (*message)           (void * userData, const char message[]);

    // Event fired when a key message is successfully created.
    void OnKeyMessage(const std::string& keyMessage, const std::string& URL) {
        _message = keyMessage;
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_MESSAGE | SESSION_UPDATE);
        }
        else {
            _callback->process_challenge(this, _URL.c_str(), reinterpret_cast<const uint8_t*>(_message.c_str()), static_cast<uint16_t>(_message.length()));
        }
    }
    // Event fired when MediaKeySession has found a usable key.
    void OnKeyReady() {
        _key = OCDM::ISession::Usable;
        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        }
        else {
            _callback->key_update(this, nullptr, 0);
        }
    }
    // Event fired when MediaKeySession encounters an error.
    void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError, const std::string& errorMessage) {
        _key = OCDM::ISession::InternalError;
        _error = errorMessage;
        _errorCode = error;
        _sysError = sysError;

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_ERROR | SESSION_UPDATE);
        }
        else {
            _callback->key_update(this, nullptr, 0);
            _callback->message(this, errorMessage.c_str());
        }
    }
    // Event fired on key status update
    void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status) {
        _key = status;

        if (_callback == nullptr) {
            _state = static_cast<sessionState>(_state | SESSION_READY | SESSION_UPDATE);
        }
        else {
            _callback->key_update(this, nullptr, 0);
        }
    }

private:
    WPEFramework::Core::Sink<Sink> _sink;
    WPEFramework::Core::StateTrigger<sessionState> _state;
    std::string _message;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    OCDM::ISession::KeyStatus _key;
    OpenCDMSessionCallbacks* _callback;
};

#endif /* EXTENDEDOPENCDMSESSION_H */

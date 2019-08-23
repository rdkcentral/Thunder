#ifndef EXTENDEDOPENCDMSESSION_H
#define EXTENDEDOPENCDMSESSION_H

#include "DataExchange.h"
#include "IOCDM.h"
#include "Module.h"
#include "open_cdm.h"
#include "open_cdm_impl.h"
using namespace WPEFramework;

// If we build in release, we do not want to "hang" forever, forcefull close after 5S waiting...
#ifdef __DEBUG__
const unsigned int a_Time = WPEFramework::Core::infinite;
#else
const unsigned int a_Time = 5000;// Expect time in MS
#endif

extern Core::CriticalSection _systemLock;
extern const char EmptyString[];

KeyStatus CDMState(const OCDM::ISession::KeyStatus state);

struct ExtendedOpenCDMSession : public OpenCDMSession {
protected:
    ExtendedOpenCDMSession() = delete;
    ExtendedOpenCDMSession(const ExtendedOpenCDMSession&) = delete;
    ExtendedOpenCDMSession& operator=(ExtendedOpenCDMSession&) = delete;

protected:
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
        virtual ~Sink() {}

    public:
        // Event fired when a key message is successfully created.
        virtual void
        OnKeyMessage(const uint8_t* keyMessage, //__in_bcount(f_cbKeyMessage)
            const uint16_t keyLength, //__in
            const std::string URL)
        {
            _parent.OnKeyMessage(
                std::string(reinterpret_cast<const char*>(keyMessage), keyLength),
                URL);
        }
        // Event fired when MediaKeySession has found a usable key.
        virtual void OnKeyReady() { _parent.OnKeyReady(); }
        // Event fired when MediaKeySession encounters an error.
        virtual void OnKeyError(const int16_t error,
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
    ExtendedOpenCDMSession(OpenCDMSessionCallbacks* callbacks)
        : OpenCDMSession()
        , _sink(this)
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(OCDM::OCDM_RESULT::OCDM_SUCCESS)
        , _key(OCDM::ISession::StatusPending)
        , _callback(callbacks)
    {
        TRACE_L1("Constructing the Session Client side - ExtendedOpenCDMSession: "
                 "%p, (nil)",
            this);
    }
    ExtendedOpenCDMSession(OCDM::IAccessorOCDM* system, const string keySystem,
        const std::string& initDataType,
        const uint8_t* pbInitData, const uint16_t cbInitData,
        const uint8_t* pbCustomData,
        const uint16_t cbCustomData,
        const LicenseType licenseType,
        OpenCDMSessionCallbacks* callbacks,
        void* userData)
        : OpenCDMSession()
        , _sink(this)
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(OCDM::OCDM_RESULT::OCDM_SUCCESS)
        , _key(OCDM::ISession::StatusPending)
        , _callback(callbacks)
        , _userData(userData)
    {

        std::string bufferId;
        OCDM::ISession* realSession = nullptr;

        system->CreateSession(keySystem, licenseType, initDataType, pbInitData,
            cbInitData, pbCustomData, cbCustomData, &_sink,
            _sessionId, realSession);

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
        }
    }

public:
    virtual bool IsExtended() const override { return (true); }
    inline KeyStatus Status(const uint8_t /* keyId */[],
        uint8_t /* length */) const
    {
        return (::CDMState(_key));
    }
    inline uint32_t Error() const { return (_errorCode); }
    inline uint32_t Error(const uint8_t keyId[], uint8_t length) const
    {
        return (_sysError);
    }

protected:
    // void (*process_challenge) (void * userData, const char url[], const uint8_t
    // challenge[], const uint16_t challengeLength);
    // void (*key_update)        (void * userData, const uint8_t keyId[], const
    // uint8_t length);
    // void (*message)           (void * userData, const char message[]);

    // Event fired when a key message is successfully created.
    void OnKeyMessage(const std::string& keyMessage, const std::string& URL)
    {
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback != nullptr && _callback->process_challenge_callback != nullptr)
            _callback->process_challenge_callback(this, _userData, _URL.c_str(), reinterpret_cast<const uint8_t*>(keyMessage.c_str()), static_cast<uint16_t>(keyMessage.length()));
    }
    // Event fired when MediaKeySession has found a usable key.
    void OnKeyReady()
    {
        _key = OCDM::ISession::Usable;
        if (_callback != nullptr && _callback->key_update_callback != nullptr)
            _callback->key_update_callback(this, _userData, nullptr, 0);
    }
    // Event fired when MediaKeySession encounters an error.
    void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError,
        const std::string& errorMessage)
    {
        _key = OCDM::ISession::InternalError;
        _error = errorMessage;
        _errorCode = error;
        _sysError = sysError;

        if (_callback != nullptr && _callback->message_callback != nullptr) {
            _callback->key_update_callback(this, _userData, nullptr, 0);
            _callback->message_callback(this, _userData, errorMessage.c_str());
        }
    }
    // Event fired on key status update
    void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status)
    {
        _key = status;

        if (_callback != nullptr && _callback->key_update_callback != nullptr)
            _callback->key_update_callback(this, _userData, nullptr, 0);
    }

protected:
    WPEFramework::Core::Sink<Sink> _sink;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    OCDM::ISession::KeyStatus _key;
    OpenCDMSessionCallbacks* _callback;
    void* _userData;
};

#endif /* EXTENDEDOPENCDMSESSION_H */

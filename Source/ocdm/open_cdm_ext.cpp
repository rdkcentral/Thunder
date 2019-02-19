#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

#include "ExtendedOpenCDMSession.h"

#define ASSERT_NOT_EXECUTED() {                             \
    fprintf(stderr, "Error: didn't expect to use %s (%s:%d)!!\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    abort(); \
}

struct OpenCDMSystemExt {
    OpenCDMAccessor * m_accessor;
    std::string m_keySystem;
};

// TODO: naming: Extended...Ext
// TODO: shares code with ExtendedOpenCDMSession, maybe intro baseclass?
struct ExtendedOpenCDMSessionExt : public OpenCDMSession {
private:
    // TODO: this is copies from "ExtendedOpenCDMSession"
    class Sink : public OCDM::ISession::ICallback {
    private:
        Sink() = delete;
        Sink(const Sink&) = delete;
        Sink& operator= (const Sink&) = delete;
    public:
        Sink(ExtendedOpenCDMSessionExt* parent)
            : _parent(*parent) {
            ASSERT(parent != nullptr);
        }
        virtual ~Sink() {
        }

    public:
        // TODO: forward messages to parent

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
        ExtendedOpenCDMSessionExt& _parent;
    };

private:
    ExtendedOpenCDMSessionExt() = delete;
    ExtendedOpenCDMSessionExt(const ExtendedOpenCDMSessionExt&) = delete;
    ExtendedOpenCDMSessionExt& operator= (ExtendedOpenCDMSessionExt&) = delete;

    enum sessionState {
        // Initialized.
        SESSION_INIT    = 0x00,

        // ExtendedOpenCDMSessionExt created, waiting for message callback.
        SESSION_MESSAGE = 0x01,
        SESSION_READY   = 0x02,
        SESSION_ERROR   = 0x04,
        SESSION_LOADED  = 0x08,
        SESSION_UPDATE  = 0x10
    };

public:
    ExtendedOpenCDMSessionExt(
        OpenCDMAccessor* system, const uint8_t drmHeader[], uint32_t drmHeaderLength,
        OpenCDMSessionCallbacks * callbacks)
        : OpenCDMSession()
        , _sink(this)
        , _state(SESSION_INIT)
        , _message()
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(0)
        , _userData(this)
        , _realSession(nullptr)
        , _callback(callbacks)
     {

        OCDM::ISessionExt* realSession = nullptr;

        // TODO: real conversion between license types
        system->CreateSessionExt(drmHeader, drmHeaderLength, &_sink, _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        }
        else {
            // TODO ?
            OpenCDMSession::SessionExt(realSession);
        }

        _realSession = realSession;
    }

    virtual ~ExtendedOpenCDMSessionExt() {
        // TODO: do we need something like this here as well?
        //if (OpenCDMSession::IsValid() == true) {
        //    OpenCDMSession::Session(nullptr);
        //}
    }

    uint32_t SessionIdExt() const
    {
        return _realSession->SessionIdExt();
    }

    uint16_t PlaylevelCompressedVideo() const
    {
        return _realSession->PlaylevelCompressedVideo();
    }

    uint16_t PlaylevelUncompressedVideo() const
    {
        return _realSession->PlaylevelUncompressedVideo();
    }

    uint16_t PlaylevelAnalogVideo() const
    {
        return _realSession->PlaylevelAnalogVideo();
    }

    uint16_t PlaylevelCompressedAudio() const
    {
        return _realSession->PlaylevelCompressedAudio();
    }

    uint16_t PlaylevelUncompressedAudio() const
    {
        return _realSession->PlaylevelUncompressedAudio();
    }

    std::string GetContentIdExt() const
    {
        return _realSession->GetContentIdExt();
    }

    void SetContentIdExt(const std::string & contentId)
    {
        _realSession->SetContentIdExt(contentId);
    }

    OCDM::ISessionExt::LicenseTypeExt GetLicenseTypeExt() const
    {
        return _realSession->GetLicenseTypeExt();
    }

    void SetLicenseTypeExt(OCDM::ISessionExt::LicenseTypeExt licenseType)
    {
        _realSession->SetLicenseTypeExt(licenseType);
    }

    OCDM::ISessionExt::SessionStateExt GetSessionStateExt() const
    {
        return _realSession->GetSessionStateExt();
    }

    void SetSessionStateExt(OCDM::ISessionExt::SessionStateExt sessionState)
    {
        _realSession->SetSessionStateExt(sessionState);
    }

    OCDM::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[], uint32_t drmHeaderLength)
    {
        return _realSession->SetDrmHeader(drmHeader, drmHeaderLength);
    }

    OCDM::OCDM_RESULT GetChallengeDataNetflix(uint8_t * challenge, uint32_t & challengeSize, uint32_t isLDL)
    {
        return _realSession->GetChallengeDataNetflix(challenge, challengeSize, isLDL);
    }

    OCDM::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[], uint32_t licenseDataSize, unsigned char * secureStopId)
    {
        return _realSession->StoreLicenseData(licenseData, licenseDataSize, secureStopId);
    }

    OCDM::OCDM_RESULT InitDecryptContextByKid()
    {
        return _realSession->InitDecryptContextByKid();
    }

    // TODO: these are copy/pasted from "ExtendedOpenCDMSession", merge
    // Event fired when a key message is successfully created.
    void OnKeyMessage(const std::string& keyMessage, const std::string& URL) {
        _message = keyMessage;
        _URL = URL;
        TRACE_L1("Received URL: [%s]", _URL.c_str());

        if (_callback != nullptr) {
            _callback->process_challenge(this, _URL.c_str(), reinterpret_cast<const uint8_t*>(_message.c_str()), static_cast<uint16_t>(_message.length()));
        }
    }
    // Event fired when MediaKeySession has found a usable key.
    void OnKeyReady() {
        //_key = OCDM::ISession::Usable;
        if (_callback != nullptr) {
            _callback->key_update(this, nullptr, 0);
        }
    }
    // Event fired when MediaKeySession encounters an error.
    void OnKeyError(const int16_t error, const OCDM::OCDM_RESULT sysError, const std::string& errorMessage) {
        //_key = OCDM::ISession::InternalError;
        _error = errorMessage;
        _errorCode = error;
        _sysError = sysError;

        if (_callback != nullptr) {
            _callback->key_update(this, nullptr, 0);
            _callback->message(this, errorMessage.c_str());
        }
    }
    // Event fired on key status update
    void OnKeyStatusUpdate(const OCDM::ISession::KeyStatus status) {
        //_key = status;

        if (_callback != nullptr) {
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
    void* _userData;
    OCDM::ISessionExt* _realSession;
    OpenCDMSessionCallbacks* _callback;
};

struct OpenCDMSystemExt* opencdm_create_system_ext(struct OpenCDMAccessor * system, const char keySystem[])
{
    OpenCDMAccessor* accessor = opencdm_create_system();
    accessor->CreateSystemNetflix(keySystem);
    accessor->InitSystemNetflix(keySystem);

    OpenCDMSystemExt * output = new OpenCDMSystemExt;
    output->m_accessor = accessor;
    output->m_keySystem = keySystem;
    return output;
}

OpenCDMError opencdm_system_get_version(struct OpenCDMAccessor* system, const char keySystem[], char versionStr[])
{
    // TODO: use keySystem
    versionStr[0] = '\0';

    std::string versionStdStr = system->GetVersionExt(keySystem);

    assert(versionStdStr.length() < 64);

    strcpy(versionStr, versionStdStr.c_str());

    return ERROR_NONE;
}

OpenCDMError opencdm_system_ext_get_ldl_session_limit(OpenCDMSystemExt* system, uint32_t * ldlLimit)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    std::string keySystem = system->m_keySystem;
    *ldlLimit = accessor->GetLdlSessionLimit(keySystem);
    return ERROR_NONE;
}

OpenCDMError opencdm_system_ext_enable_secure_stop(struct OpenCDMSystemExt* system, uint32_t use)
{
    // TODO: conversion
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->EnableSecureStop(system->m_keySystem, use != 0);
}

OpenCDMError opencdm_system_ext_commit_secure_stop(OpenCDMSystemExt* system, const unsigned char sessionID[], uint32_t sessionIDLength, const unsigned char serverResponse[], uint32_t serverResponseLength)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->CommitSecureStop(system->m_keySystem, sessionID, sessionIDLength, serverResponse, serverResponseLength);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, const char keySystem[], uint64_t * time)
{
    //OpenCDMAccessor* accessor = system->m_accessor;

    // TODO: use keySystem
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        time_t cTime;
        cTime = system->GetDrmSystemTime(keySystem);
        *time = static_cast<uint64_t>(cTime);
        result = ERROR_NONE;
    }
    return (result);
}

uint32_t opencdm_session_get_session_id_ext(struct OpenCDMSession * opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return sessionExt->SessionIdExt();
}

OpenCDMError opencdm_destroy_session_ext(OpenCDMSession * opencdmSession)
{
    OpenCDMError result (OpenCDMError::ERROR_INVALID_SESSION);

    if (opencdmSession != nullptr) {
        result = OpenCDMError::ERROR_NONE;
        opencdmSession->Release();
    }

    return (result);
}

uint16_t opencdm_session_get_playlevel_compressed_video(OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return sessionExt->PlaylevelCompressedVideo();
}

uint16_t opencdm_session_get_playlevel_uncompressed_video(OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return sessionExt->PlaylevelUncompressedVideo();
}

uint16_t opencdm_session_get_playlevel_analog_video(OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return sessionExt->PlaylevelAnalogVideo();
}

uint16_t opencdm_session_get_playlevel_compressed_audio(OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return sessionExt->PlaylevelCompressedAudio();
}

uint16_t opencdm_session_get_playlevel_uncompressed_audio(OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return sessionExt->PlaylevelUncompressedAudio();
}

OpenCDMError opencdm_session_get_content_id(struct OpenCDMSession * opencdmSession, char * buffer, uint32_t * bufferSize)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    std::string contentIdStr = sessionExt->GetContentIdExt();

    if (*bufferSize == 0) {
        *bufferSize = contentIdStr.length();
    } else {
        assert(contentIdStr.length() <= *bufferSize);

        *bufferSize = contentIdStr.length();
        memcpy(buffer, contentIdStr.c_str(), *bufferSize);
    }

    return ERROR_NONE;
}

OpenCDMError opencdm_session_set_content_id(struct OpenCDMSession * opencdmSession, const char contentId[], uint32_t contentIdLength)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    std::string contentIdStr(contentId, contentIdLength);
    sessionExt->SetContentIdExt(contentIdStr);

    return ERROR_NONE;
}

OpenCDMError opencdm_session_set_drm_header(struct OpenCDMSession * opencdmSession, const uint8_t drmHeader[], uint32_t drmHeaderSize)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    return (OpenCDMError)sessionExt->SetDrmHeader(drmHeader, drmHeaderSize);
}

OpenCDMError opencdm_session_get_challenge_data(struct OpenCDMSession * mOpenCDMSession, uint8_t * challenge, uint32_t * challengeSize, uint32_t isLDL)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    // TODO: real conversion
    return (OpenCDMError)sessionExt->GetChallengeDataNetflix(challenge, *challengeSize, isLDL);
}

OpenCDMError opencdm_session_store_license_data(struct OpenCDMSession * mOpenCDMSession, const uint8_t licenseData[], uint32_t licenseDataSize, unsigned char * secureStopId)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    // TODO: real conversion
    return (OpenCDMError)sessionExt->StoreLicenseData(licenseData, licenseDataSize, secureStopId);
}

OpenCDMError opencdm_session_init_decrypt_context_by_kid(struct OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    // TODO: real conversion
    return (OpenCDMError)sessionExt->InitDecryptContextByKid();
}

/*
OpenCDMError opencdm_init_system_ext(struct OpenCDMAccessor* system)
{
    return (OpenCDMError)system->InitSystemNetflix();
}
*/

OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystemExt* system)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        // TODO: real conversion
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteSecureStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystemExt* system, uint8_t secureStoreHash[], uint32_t secureStoreHashLength)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        // TODO: real conversion
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetSecureStoreHash(keySystem, secureStoreHash, secureStoreHashLength);
    }
    return (result);
}

/*
OpenCDMError opencdm_system_teardown(struct OpenCDMAccessor* system)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        // TODO: real conversion
        result = (OpenCDMError)system->TeardownSystemNetflix();
    }
    return (result);
}
*/

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
                                    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks * callbacks,
                                    struct OpenCDMSession** session) {

    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (strcmp(keySystem, "com.netflix.playready") != 0) {
        if (system != nullptr) {
            *session = new ExtendedOpenCDMSession(static_cast<OCDM::IAccessorOCDM*>(system), std::string(keySystem), std::string(initDataType), initData, initDataLength,CDMData,CDMDataLength, licenseType, callbacks);

            result = (*session != nullptr ? OpenCDMError::ERROR_NONE : OpenCDMError::ERROR_INVALID_SESSION);
        }
    } else {
        if (system != nullptr) {
            *session = new ExtendedOpenCDMSessionExt(system, initData, initDataLength, callbacks);
            result = OpenCDMError::ERROR_NONE;
        }
    }

    return (result);
}

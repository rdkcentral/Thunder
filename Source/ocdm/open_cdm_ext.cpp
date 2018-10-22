#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

#define ASSERT_NOT_EXECUTED() {                             \
    fprintf(stderr, "Error: didn't expect to use %s (%s:%d)!!\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    abort(); \
}

// TODO: naming: Extended...Ext
// TODO: shares code with ExtendedOpenCDMSession, maybe intro baseclass?
struct ExtendedOpenCDMSessionExt : public OpenCDMSession {
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
        OpenCDMAccessor* system, uint32_t sessionId, const char contentId[], uint32_t contentIdLength,
        enum OcdmLicenseType licenseType, const uint8_t drmHeader[], uint32_t drmHeaderLength)
        : OpenCDMSession()
        //, _sink(this)
        , _state(SESSION_INIT)
        , _message()
        , _URL()
        , _error()
        , _errorCode(0)
        , _sysError(0)
        , _userData(this)
        , _realSession(nullptr)
     {

        std::string bufferId;
        OCDM::ISessionExt* realSession = nullptr;

        // TODO: real conversion between license types
        system->CreateSessionExt(sessionId, contentId, contentIdLength, (OCDM::ISessionExt::LicenseTypeExt)(uint32_t)licenseType, drmHeader, drmHeaderLength, realSession);

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
        /*
        if (OpenCDMSession::IsValid() == true) {
            OpenCDMSession::Session(nullptr);
        }
        */
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

    uint32_t DecryptNetflix(const unsigned char* IVData, uint32_t IVDataSize, unsigned long long byteOffset, unsigned char dataBuffer[], uint32_t dataBufferSize) {
        uint32_t result = OpenCDMError::ERROR_INVALID_DECRYPT_BUFFER;
        if (_decryptSession != nullptr) {
            result = OpenCDMError::ERROR_NONE;
            _decryptSession->DecryptNetflix(IVData, IVDataSize, byteOffset, dataBuffer, dataBufferSize);
        }
        return (result);
    }


private:
    //WPEFramework::Core::Sink<Sink> _sink;
    WPEFramework::Core::StateTrigger<sessionState> _state;
    std::string _message;
    std::string _URL;
    std::string _error;
    uint32_t _errorCode;
    OCDM::OCDM_RESULT _sysError;
    void* _userData;
    OCDM::ISessionExt* _realSession;
};

struct OpenCDMAccessor* opencdm_create_system_netflix(const char readDir[], const char storeLocation[])
{
    OpenCDMAccessor* output = opencdm_create_system();

    output->CreateSystemNetflix(readDir, storeLocation);

    return output;
}

OpenCDMError opencdm_system_get_version(OpenCDMAccessor* system, char versionStr[])
{
    versionStr[0] = '\0';

    std::string versionStdStr = system->GetVersionExt();

    assert(versionStdStr.length() < 64);

    strcpy(versionStr, versionStdStr.c_str());

    return ERROR_NONE;
}

OpenCDMError opencdm_system_get_ldl_session_limit(OpenCDMAccessor* system, uint32_t * ldlLimit)
{
    *ldlLimit = system->GetLdlSessionLimit();
    return ERROR_NONE;
}

OpenCDMError opencdm_system_enable_secure_stop(OpenCDMAccessor* system, uint32_t use)
{
    // TODO: conversion
    return (OpenCDMError)system->EnableSecureStop(use != 0);
}

OpenCDMError opencdm_system_commit_secure_stop(OpenCDMAccessor* system, const unsigned char sessionID[], uint32_t sessionIDLength, const unsigned char serverResponse[], uint32_t serverResponseLength)
{
    return (OpenCDMError)system->CommitSecureStop(sessionID, sessionIDLength, serverResponse, serverResponseLength);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, time_t * time) {
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *time = static_cast<OpenCDMError>(system->GetDrmSystemTime());
        result = ERROR_NONE;
    }
    return (result);
}

OpenCDMError opencdm_create_session_netflix(struct OpenCDMAccessor* system, struct OpenCDMSession ** opencdmSession, uint32_t sessionId, const char contentId[], uint32_t contentIdLength,
                                            enum OcdmLicenseType licenseType, const uint8_t drmHeader[], uint32_t drmHeaderLength)
{

    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *opencdmSession = new ExtendedOpenCDMSessionExt(system, sessionId, contentId, contentIdLength, licenseType, drmHeader, drmHeaderLength);
        result = OpenCDMError::ERROR_NONE;
    }

    return (result);
}

uint32_t opencdm_session_get_session_id_netflix(struct OpenCDMSession * opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return sessionExt->SessionIdExt();
}

OpenCDMError opencdm_destroy_session_netflix(OpenCDMSession * opencdmSession)
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
        *bufferSize = contentIdStr.length() + 1;
    } else {
        *bufferSize = contentIdStr.length() + 1;
        strcpy(buffer, contentIdStr.c_str());
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

enum OcdmLicenseType opencdm_session_get_license_type(struct OpenCDMSession * opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    return (OcdmLicenseType)sessionExt->GetLicenseTypeExt();
}

OpenCDMError opencdm_session_set_license_type(struct OpenCDMSession * opencdmSession, enum OcdmLicenseType licenseType)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    sessionExt->SetLicenseTypeExt((OCDM::ISessionExt::LicenseTypeExt)licenseType);

    return ERROR_NONE;
}

enum OcdmSessionState opencdm_session_get_session_state(struct OpenCDMSession * opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    return (OcdmSessionState)sessionExt->GetSessionStateExt();
}

OpenCDMError opencdm_session_set_session_state(struct OpenCDMSession * opencdmSession, enum OcdmSessionState sessionState)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    sessionExt->SetSessionStateExt((OCDM::ISessionExt::SessionStateExt)sessionState);

    return ERROR_NONE;
}

OpenCDMError opencdm_session_set_drm_header(struct OpenCDMSession * opencdmSession, const uint8_t drmHeader[], uint32_t drmHeaderSize)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    // TODO: real conversion
    return (OpenCDMError)sessionExt->SetDrmHeader(drmHeader, drmHeaderSize);
}

OpenCDMError opencdm_session_get_challenge_data_netflix(struct OpenCDMSession * mOpenCDMSession, uint8_t * challenge, uint32_t * challengeSize, uint32_t isLDL)
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

OpenCDMError opencdm_init_system_netflix(struct OpenCDMAccessor* system)
{
    return (OpenCDMError)system->InitSystemNetflix();
}

OpenCDMError opencdm_session_decrypt_netflix(OpenCDMSession * mOpenCDMSession, const unsigned char* IVData, uint32_t IVDataSize, unsigned long long byteOffset, unsigned char dataBuffer[], uint32_t dataBufferSize)
{
    OpenCDMError result (ERROR_INVALID_SESSION);

    if (mOpenCDMSession != nullptr) {
        result  = static_cast<OpenCDMError>(mOpenCDMSession->DecryptNetflix(IVData, IVDataSize, byteOffset, dataBuffer, dataBufferSize));
    }

    return (result);
}

OpenCDMError opencdm_system_teardown()
{
    ASSERT_NOT_EXECUTED();

    return ERROR_NONE;
}

OpenCDMError opencdm_delete_secure_store(struct OpenCDMAccessor* system)
{
    ASSERT_NOT_EXECUTED();

    return ERROR_NONE;
}

OpenCDMError opencdm_get_secure_store_hash(struct OpenCDMAccessor* system, uint8_t secureStoreHash[], uint32_t secureStoreHashLength)
{
    ASSERT_NOT_EXECUTED();

    return ERROR_NONE;
}



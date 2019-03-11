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

struct ExtendedOpenCDMSessionExt : public ExtendedOpenCDMSession {
public:
    ExtendedOpenCDMSessionExt() = delete;
    ExtendedOpenCDMSessionExt(const ExtendedOpenCDMSessionExt&) = delete;
    ExtendedOpenCDMSessionExt& operator= (ExtendedOpenCDMSessionExt&) = delete;

public:
    ExtendedOpenCDMSessionExt(
        OpenCDMAccessor* system, const uint8_t drmHeader[], uint32_t drmHeaderLength,
        OpenCDMSessionCallbacks * callbacks)
        : ExtendedOpenCDMSession(callbacks)
        , _userData(this)
        , _realSession(nullptr)
     {

        OCDM::ISessionExt* realSession = nullptr;

        system->CreateSessionExt(drmHeader, drmHeaderLength, &_sink, _sessionId, realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        }
        else {
            OpenCDMSession::SessionExt(realSession);
        }

        _realSession = realSession;
    }

    virtual ~ExtendedOpenCDMSessionExt() {
        OpenCDMSession::SessionExt(nullptr);
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

    OCDM::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[], uint32_t drmHeaderLength)
    {
        return _realSession->SetDrmHeader(drmHeader, drmHeaderLength);
    }

    OCDM::OCDM_RESULT GetChallengeDataExt(uint8_t * challenge, uint32_t & challengeSize, uint32_t isLDL)
    {
        return _realSession->GetChallengeDataExt(challenge, challengeSize, isLDL);
    }


    OCDM::OCDM_RESULT CancelChallengeDataExt()
    {
        return _realSession->CancelChallengeDataExt();
    }

    OCDM::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[], uint32_t licenseDataSize, uint8_t * secureStopId)
    {
        return _realSession->StoreLicenseData(licenseData, licenseDataSize, secureStopId);
    }

    OCDM::OCDM_RESULT InitDecryptContextByKid()
    {
        return _realSession->InitDecryptContextByKid();
    }

    OCDM::OCDM_RESULT CleanDecryptContext()
    {
        return _realSession->CleanDecryptContext();
    }

private:
    void* _userData;
    OCDM::ISessionExt* _realSession;
};

struct OpenCDMSystemExt* opencdm_create_system_ext(struct OpenCDMAccessor * system, const char keySystem[])
{
    ASSERT(system != nullptr);

    OpenCDMAccessor* accessor = system;
    accessor->CreateSystemExt(keySystem);
    accessor->InitSystemExt(keySystem);

    OpenCDMSystemExt * output = new OpenCDMSystemExt;
    output->m_accessor = accessor;
    output->m_keySystem = keySystem;
    return output;
}

OpenCDMError opencdm_destruct_system_ext(struct OpenCDMSystemExt * system)
{
    if (system != nullptr) {
       delete system;
    }
    return ERROR_NONE;
}

OpenCDMError opencdm_system_get_version(struct OpenCDMAccessor* system, const char keySystem[], char versionStr[])
{
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

bool opencdm_system_ext_is_secure_stop_enabled(struct OpenCDMSystemExt* system)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->IsSecureStopEnabled(system->m_keySystem);
}

OpenCDMError opencdm_system_ext_enable_secure_stop(struct OpenCDMSystemExt* system, uint32_t use)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->EnableSecureStop(system->m_keySystem, use != 0);
}

uint32_t opencdm_system_ext_reset_secure_stop(struct OpenCDMSystemExt* system)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->ResetSecureStops(system->m_keySystem);
}

OpenCDMError opencdm_system_ext_get_secure_stop_ids(OpenCDMSystemExt* system, uint8_t ids[], uint8_t idSize, uint32_t * count)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->GetSecureStopIds(system->m_keySystem, ids, idSize, *count);
}

OpenCDMError opencdm_system_ext_get_secure_stop(OpenCDMSystemExt* system, const uint8_t sessionID[], uint32_t sessionIDLength, uint8_t rawData[], uint16_t * rawSize)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->GetSecureStop(system->m_keySystem, sessionID, sessionIDLength, rawData, *rawSize);
}

OpenCDMError opencdm_system_ext_commit_secure_stop(OpenCDMSystemExt* system, const uint8_t sessionID[], uint32_t sessionIDLength, const uint8_t serverResponse[], uint32_t serverResponseLength)
{
    OpenCDMAccessor* accessor = system->m_accessor;
    return (OpenCDMError)accessor->CommitSecureStop(system->m_keySystem, sessionID, sessionIDLength, serverResponse, serverResponseLength);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, const char keySystem[], uint64_t * time)
{
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

OpenCDMError opencdm_destruct_session_ext(OpenCDMSession * opencdmSession)
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

OpenCDMError opencdm_session_set_drm_header(struct OpenCDMSession * opencdmSession, const uint8_t drmHeader[], uint32_t drmHeaderSize)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return (OpenCDMError)sessionExt->SetDrmHeader(drmHeader, drmHeaderSize);
}

OpenCDMError opencdm_session_get_challenge_data(struct OpenCDMSession * mOpenCDMSession, uint8_t * challenge, uint32_t * challengeSize, uint32_t isLDL)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->GetChallengeDataExt(challenge, *challengeSize, isLDL);
}

OpenCDMError opencdm_session_cancel_challenge_data(struct OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->CancelChallengeDataExt();
}

OpenCDMError opencdm_session_store_license_data(struct OpenCDMSession * mOpenCDMSession, const uint8_t licenseData[], uint32_t licenseDataSize, uint8_t * secureStopId)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->StoreLicenseData(licenseData, licenseDataSize, secureStopId);
}

OpenCDMError opencdm_session_init_decrypt_context_by_kid(struct OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->InitDecryptContextByKid();
}

OpenCDMError opencdm_session_clean_decrypt_context(struct OpenCDMSession * mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->CleanDecryptContext();
}

OpenCDMError opencdm_delete_key_store(struct OpenCDMSystemExt* system)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteKeyStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystemExt* system)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteSecureStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystemExt* system, uint8_t keyStoreHash[], uint32_t keyStoreHashLength)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetKeyStoreHash(keySystem, keyStoreHash, keyStoreHashLength);
    }
    return (result);
}

OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystemExt* system, uint8_t secureStoreHash[], uint32_t secureStoreHashLength)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetSecureStoreHash(keySystem, secureStoreHash, secureStoreHashLength);
    }
    return (result);
}

OpenCDMError opencdm_system_teardown(struct OpenCDMSystemExt* system)
{
    OpenCDMError result (ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor * accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->TeardownSystemExt(keySystem);
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

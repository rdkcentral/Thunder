#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

#include "ExtendedOpenCDMSession.h"

#define ASSERT_NOT_EXECUTED()                                         \
    {                                                                 \
        fprintf(stderr, "Error: didn't expect to use %s (%s:%d)!!\n", \
            __PRETTY_FUNCTION__, __FILE__, __LINE__);                 \
        abort();                                                      \
    }

struct ExtendedOpenCDMSessionExt : public ExtendedOpenCDMSession {
public:
    ExtendedOpenCDMSessionExt() = delete;
    ExtendedOpenCDMSessionExt(const ExtendedOpenCDMSessionExt&) = delete;
    ExtendedOpenCDMSessionExt& operator=(ExtendedOpenCDMSessionExt&) = delete;

public:
    ExtendedOpenCDMSessionExt(const string keySystem, OpenCDMAccessor* system, const uint8_t drmHeader[],
        uint32_t drmHeaderLength,
        OpenCDMSessionCallbacks* callbacks)
        : ExtendedOpenCDMSession(callbacks)
        , _userData(nullptr)
        , _realSession(nullptr)
    {

        OCDM::ISessionExt* realSession = nullptr;

        system->CreateSessionExt(keySystem, drmHeader, drmHeaderLength, &_sink, _sessionId,
            realSession);

        if (realSession == nullptr) {
            TRACE_L1("Creating a Session failed. %d", __LINE__);
        } else {
            OpenCDMSession::SessionExt(realSession);
        }

        _realSession = realSession;
    }

    virtual ~ExtendedOpenCDMSessionExt() { OpenCDMSession::SessionExt(nullptr); }

    uint32_t SessionIdExt() const { return _realSession->SessionIdExt(); }

    OCDM::OCDM_RESULT SetDrmHeader(const uint8_t drmHeader[],
        uint32_t drmHeaderLength)
    {
        return _realSession->SetDrmHeader(drmHeader, drmHeaderLength);
    }

    OCDM::OCDM_RESULT GetChallengeDataExt(uint8_t* challenge,
        uint32_t& challengeSize,
        uint32_t isLDL)
    {
        return _realSession->GetChallengeDataExt(challenge, challengeSize, isLDL);
    }

    OCDM::OCDM_RESULT CancelChallengeDataExt()
    {
        return _realSession->CancelChallengeDataExt();
    }

    OCDM::OCDM_RESULT StoreLicenseData(const uint8_t licenseData[],
        uint32_t licenseDataSize,
        uint8_t* secureStopId)
    {
        return _realSession->StoreLicenseData(licenseData, licenseDataSize,
            secureStopId);
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

/*
struct OpenCDMSystemExt*
opencdm_create_system_ext(const char keySystem[])
{
    //OpenCDMAccessor* accessor = system;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    // TODO: can these two be put together?
    accessor->CreateSystemExt(keySystem);
    accessor->InitSystemExt(keySystem);

    OpenCDMSystemExt* output = new OpenCDMSystemExt;
    output->m_accessor = accessor;
    output->m_keySystem = keySystem;
    return output;
}
*/

struct OpenCDMSystem* opencdm_create_system(const char keySystem[])
{
/*
    OpenCDMSystem * output = new OpenCDMSystem;
    output->m_keySystem = keySystem;
    return output;
*/    

    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();

    // TODO: can these two be put together?
    accessor->CreateSystemExt(keySystem);
    accessor->InitSystemExt(keySystem);

    OpenCDMSystemExt* output = new OpenCDMSystemExt;
    output->m_accessor = accessor;
    output->m_keySystem = keySystem;
    //output->m_keySystem = keySystem;
    
    // HACK
    return (OpenCDMSystem*)output;
}

/*
OpenCDMError opencdm_destruct_system_ext(struct OpenCDMSystemExt* system)
{
    if (system != nullptr) {
        delete system;
    }
    return ERROR_NONE;
}
*/

OpenCDMError opencdm_system_get_version(struct OpenCDMSystem* system2,
    char versionStr[])
{
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    versionStr[0] = '\0';

    std::string versionStdStr = accessor->GetVersionExt(system->m_keySystem);

    assert(versionStdStr.length() < 64);

    strcpy(versionStr, versionStdStr.c_str());

    return ERROR_NONE;
}

OpenCDMError opencdm_system_ext_get_ldl_session_limit(OpenCDMSystem* system2,
    uint32_t* ldlLimit)
{
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    std::string keySystem = system->m_keySystem;
    *ldlLimit = accessor->GetLdlSessionLimit(keySystem);
    return ERROR_NONE;
}

bool opencdm_system_ext_is_secure_stop_enabled(
    struct OpenCDMSystem* system2)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->IsSecureStopEnabled(system->m_keySystem);
}

OpenCDMError
opencdm_system_ext_enable_secure_stop(struct OpenCDMSystem* system2,
    uint32_t use)
{
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    return (OpenCDMError)accessor->EnableSecureStop(system->m_keySystem,
        use != 0);
}

uint32_t opencdm_system_ext_reset_secure_stop(struct OpenCDMSystem* system2)
{
   OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->ResetSecureStops(system->m_keySystem);
}

OpenCDMError opencdm_system_ext_get_secure_stop_ids(OpenCDMSystem* system2,
    uint8_t ids[],
    uint8_t idSize,
    uint32_t* count)
{
   OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->GetSecureStopIds(system->m_keySystem, ids,
        idSize, *count);
}

OpenCDMError opencdm_system_ext_get_secure_stop(OpenCDMSystem* system2,
    const uint8_t sessionID[],
    uint32_t sessionIDLength,
    uint8_t rawData[],
    uint16_t* rawSize)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->GetSecureStop(
        system->m_keySystem, sessionID, sessionIDLength, rawData, *rawSize);
}

OpenCDMError opencdm_system_ext_commit_secure_stop(
    OpenCDMSystem* system2, const uint8_t sessionID[],
    uint32_t sessionIDLength, const uint8_t serverResponse[],
    uint32_t serverResponseLength)
{
   OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->CommitSecureStop(
        system->m_keySystem, sessionID, sessionIDLength, serverResponse,
        serverResponseLength);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMSystem* system2,
    uint64_t* time)
{
   OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        time_t cTime;
        cTime = accessor->GetDrmSystemTime(system->m_keySystem);
        *time = static_cast<uint64_t>(cTime);
        result = ERROR_NONE;
    }
    return (result);
}

uint32_t
opencdm_session_get_session_id_ext(struct OpenCDMSession* opencdmSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return sessionExt->SessionIdExt();
}

OpenCDMError opencdm_destruct_session_ext(OpenCDMSession* opencdmSession)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);

    if (opencdmSession != nullptr) {
        result = OpenCDMError::ERROR_NONE;
        opencdmSession->Release();
    }

    return (result);
}

OpenCDMError
opencdm_session_set_drm_header(struct OpenCDMSession* opencdmSession,
    const uint8_t drmHeader[],
    uint32_t drmHeaderSize)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(opencdmSession);

    return (OpenCDMError)sessionExt->SetDrmHeader(drmHeader, drmHeaderSize);
}

OpenCDMError
opencdm_session_get_challenge_data(struct OpenCDMSession* mOpenCDMSession,
    uint8_t* challenge, uint32_t* challengeSize,
    uint32_t isLDL)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->GetChallengeDataExt(challenge,
        *challengeSize, isLDL);
}

OpenCDMError
opencdm_session_cancel_challenge_data(struct OpenCDMSession* mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->CancelChallengeDataExt();
}

OpenCDMError opencdm_session_store_license_data(
    struct OpenCDMSession* mOpenCDMSession, const uint8_t licenseData[],
    uint32_t licenseDataSize, uint8_t* secureStopId)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->StoreLicenseData(
        licenseData, licenseDataSize, secureStopId);
}

OpenCDMError opencdm_session_init_decrypt_context_by_kid(
    struct OpenCDMSession* mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);
    OpenCDMError output = (OpenCDMError)sessionExt->InitDecryptContextByKid();
    return output;
}

OpenCDMError
opencdm_session_clean_decrypt_context(struct OpenCDMSession* mOpenCDMSession)
{
    ExtendedOpenCDMSessionExt* sessionExt = static_cast<ExtendedOpenCDMSessionExt*>(mOpenCDMSession);

    return (OpenCDMError)sessionExt->CleanDecryptContext();
}

OpenCDMError opencdm_delete_key_store(struct OpenCDMSystem* system2)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteKeyStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystem* system2)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteSecureStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystem* system2,
    uint8_t keyStoreHash[],
    uint32_t keyStoreHashLength)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetKeyStoreHash(keySystem, keyStoreHash,
            keyStoreHashLength);
    }
    return (result);
}

OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystem* system2,
    uint8_t secureStoreHash[],
    uint32_t secureStoreHashLength)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetSecureStoreHash(
            keySystem, secureStoreHash, secureStoreHashLength);
    }
    return (result);
}

/*
OpenCDMError opencdm_system_teardown(struct OpenCDMSystemExt* system)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = system->m_accessor;
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->TeardownSystemExt(keySystem);
    }
    return (result);
}
*/

/**
 * \brief Create DRM session (for actual decrypting of data).
 *
 * Creates an instance of \ref OpenCDMSession using initialization data.
 * \param keySystem DRM system to create the session for.
 * \param licenseType DRM specifc signed integer selecting License Type (e.g.
 * "Limited Duration" for PlayReady).
 * \param initDataType Type of data passed in \ref initData.
 * \param initData Initialization data.
 * \param initDataLength Length (in bytes) of initialization data.
 * \param CDMData CDM data.
 * \param CDMDataLength Length (in bytes) of \ref CDMData.
 * \param session Output parameter that will contain pointer to instance of \ref
 * OpenCDMSession.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError
opencdm_construct_session(struct OpenCDMSystem* system2,
    const LicenseType licenseType, const char initDataType[],
    const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength,
    OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session)
{
    OpenCDMSystemExt * system = (OpenCDMSystemExt*)system2;
    OpenCDMError result(ERROR_INVALID_ACCESSOR);
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();

    TRACE_L1("Creating a Session for %s", system->m_keySystem.c_str());

    // TODO: Since we are passing key system name anyway, not need for if here.
    if (system->m_keySystem != "com.netflix.playready") {
        if (system != nullptr) {
            *session = new ExtendedOpenCDMSession(
                static_cast<OCDM::IAccessorOCDM*>(accessor), system->m_keySystem,
                std::string(initDataType), initData, initDataLength, CDMData,
                CDMDataLength, licenseType, callbacks, userData);

            result = (*session != nullptr ? OpenCDMError::ERROR_NONE
                                          : OpenCDMError::ERROR_INVALID_SESSION);
        }
    } else {
        if (system != nullptr) {
            *session = new ExtendedOpenCDMSessionExt(system->m_keySystem, accessor, initData, initDataLength,
                callbacks);
            result = OpenCDMError::ERROR_NONE;
        }
    }

    TRACE_L1("Created a Session, result %p, %d", *session, result);
    return (result);
}

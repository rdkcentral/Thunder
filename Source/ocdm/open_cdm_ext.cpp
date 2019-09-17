#include "open_cdm_ext.h"

#include "open_cdm_impl.h"

#define ASSERT_NOT_EXECUTED()                                         \
    {                                                                 \
        fprintf(stderr, "Error: didn't expect to use %s (%s:%d)!!\n", \
            __PRETTY_FUNCTION__, __FILE__, __LINE__);                 \
        abort();                                                      \
    }

struct OpenCDMSystem* opencdm_create_system(const char keySystem[])
{
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();

     
    OpenCDMSystem* output = new OpenCDMSystem;
    output->m_keySystem = keySystem;
   
    return output;
}

OpenCDMError opencdm_system_get_version(struct OpenCDMSystem* system,
    char versionStr[])
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    versionStr[0] = '\0';

    std::string versionStdStr = accessor->GetVersionExt(system->m_keySystem);

    assert(versionStdStr.length() < 64);

    strcpy(versionStr, versionStdStr.c_str());

    return ERROR_NONE;
}

OpenCDMError opencdm_system_ext_get_ldl_session_limit(OpenCDMSystem* system,
    uint32_t* ldlLimit)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    std::string keySystem = system->m_keySystem;
    *ldlLimit = accessor->GetLdlSessionLimit(keySystem);
    return ERROR_NONE;
}

uint32_t opencdm_system_ext_is_secure_stop_enabled(
    struct OpenCDMSystem* system)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->IsSecureStopEnabled(system->m_keySystem);
}

OpenCDMError
opencdm_system_ext_enable_secure_stop(struct OpenCDMSystem* system,
    uint32_t use)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->EnableSecureStop(system->m_keySystem,
        use != 0);
}

uint32_t opencdm_system_ext_reset_secure_stop(struct OpenCDMSystem* system)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->ResetSecureStops(system->m_keySystem);
}

OpenCDMError opencdm_system_ext_get_secure_stop_ids(OpenCDMSystem* system,
    uint8_t ids[],
    uint8_t idSize,
    uint32_t* count)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->GetSecureStopIds(system->m_keySystem, ids,
        idSize, *count);
}

OpenCDMError opencdm_system_ext_get_secure_stop(OpenCDMSystem* system,
    const uint8_t sessionID[],
    uint32_t sessionIDLength,
    uint8_t rawData[],
    uint16_t* rawSize)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->GetSecureStop(
        system->m_keySystem, sessionID, sessionIDLength, rawData, *rawSize);
}

OpenCDMError opencdm_system_ext_commit_secure_stop(
    OpenCDMSystem* system, const uint8_t sessionID[],
    uint32_t sessionIDLength, const uint8_t serverResponse[],
    uint32_t serverResponseLength)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    return (OpenCDMError)accessor->CommitSecureStop(
        system->m_keySystem, sessionID, sessionIDLength, serverResponse,
        serverResponseLength);
}

OpenCDMError opencdm_system_get_drm_time(struct OpenCDMSystem* system,
    uint64_t* time)
{
    ASSERT(system != nullptr);
    OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        *time = accessor->GetDrmSystemTime(system->m_keySystem);
        result = ERROR_NONE;
    }
    return (result);
}

uint32_t
opencdm_session_get_session_id_ext(struct OpenCDMSession* opencdmSession)
{
    uint32_t result = OpenCDMError::ERROR_INVALID_SESSION;
    ASSERT(opencdmSession != nullptr);

    if (opencdmSession != nullptr) {
       result = opencdmSession->SessionIdExt();
    }

    return result;
}

OpenCDMError opencdm_destruct_session_ext(OpenCDMSession* opencdmSession)
{
    OpenCDMError result(OpenCDMError::ERROR_INVALID_SESSION);
    ASSERT(opencdmSession != nullptr);

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
    ASSERT(opencdmSession != nullptr);
    return (OpenCDMError)opencdmSession->SetDrmHeader(drmHeader, drmHeaderSize);
}

OpenCDMError
opencdm_session_get_challenge_data(struct OpenCDMSession* mOpenCDMSession,
    uint8_t* challenge, uint32_t* challengeSize,
    uint32_t isLDL)
{
    ASSERT(mOpenCDMSession != nullptr);
    return (OpenCDMError)mOpenCDMSession->GetChallengeDataExt(challenge,
        *challengeSize, isLDL);
}

OpenCDMError
opencdm_session_cancel_challenge_data(struct OpenCDMSession* mOpenCDMSession)
{
    ASSERT(mOpenCDMSession != nullptr);
    return (OpenCDMError)mOpenCDMSession->CancelChallengeDataExt();
}

OpenCDMError opencdm_session_store_license_data(
    struct OpenCDMSession* mOpenCDMSession, const uint8_t licenseData[],
    uint32_t licenseDataSize, uint8_t* secureStopId)
{
    ASSERT(mOpenCDMSession != nullptr);
    return (OpenCDMError)mOpenCDMSession->StoreLicenseData(
        licenseData, licenseDataSize, secureStopId);
}

OpenCDMError opencdm_session_select_key_id(
    struct OpenCDMSession* mOpenCDMSession, uint8_t keyLenght, const uint8_t keyId[])
{
    ASSERT(mOpenCDMSession != nullptr);
    OpenCDMError output = (OpenCDMError)mOpenCDMSession->SelectKeyId(keyLenght, keyId);
    return output;
}

OpenCDMError opencdm_session_clean_decrypt_context(struct OpenCDMSession* mOpenCDMSession)
{
    ASSERT(mOpenCDMSession != nullptr);
    return (OpenCDMError)mOpenCDMSession->CleanDecryptContext();
}

OpenCDMError opencdm_delete_key_store(struct OpenCDMSystem* system)
{
    ASSERT(system != nullptr);
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteKeyStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystem* system)
{
    ASSERT(system != nullptr);
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->DeleteSecureStore(keySystem);
    }
    return (result);
}

OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t keyStoreHash[],
    uint32_t keyStoreHashLength)
{
    ASSERT(system != nullptr);
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetKeyStoreHash(keySystem, keyStoreHash,
            keyStoreHashLength);
    }
    return (result);
}

OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t secureStoreHash[],
    uint32_t secureStoreHashLength)
{
    ASSERT(system != nullptr);
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        OpenCDMAccessor* accessor = OpenCDMAccessor::Instance();
        std::string keySystem = system->m_keySystem;
        result = (OpenCDMError)accessor->GetSecureStoreHash(
            keySystem, secureStoreHash, secureStoreHashLength);
    }
    return (result);
}

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
opencdm_construct_session(struct OpenCDMSystem* system,
    const LicenseType licenseType, const char initDataType[],
    const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength,
    OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session)
{
    ASSERT(system != nullptr);
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    TRACE_L1("Creating a Session for %s", system->m_keySystem.c_str());

    if (system != nullptr) {
        *session = new OpenCDMSession(system, std::string(initDataType),
                            initData, initDataLength, CDMData,
                            CDMDataLength, licenseType, callbacks, userData);
        result = (*session != nullptr ? OpenCDMError::ERROR_NONE
                                      : OpenCDMError::ERROR_INVALID_SESSION);
    }

    TRACE_L1("Created a Session, result %p, %d", *session, result);
    return (result);
}

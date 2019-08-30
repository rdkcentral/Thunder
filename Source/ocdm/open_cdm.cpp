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
#include "open_cdm_impl.h"

#include "ExtendedOpenCDMSession.h"

MODULE_NAME_DECLARATION(BUILD_REFERENCE)

using namespace WPEFramework;

Core::CriticalSection _systemLock;
const char EmptyString[] = { '\0' };

#ifdef _MSVC_LANG
extern "C" {
	void ForceLinkingOfOpenCDM() 
	{
        printf("Forcefully linked in the OCDM library for the ProxyStubs in this library!!\n");
	}
}
#endif

KeyStatus CDMState(const OCDM::ISession::KeyStatus state)
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

/* static */ OpenCDMAccessor* OpenCDMAccessor::_singleton = nullptr;

/**
 * \brief Creates DRM system.
 *
 * \param keySystem Name of required key system (See \ref
 * opencdm_is_type_supported)
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
 * \param keySystem Name of required key system (e.g.
 * "com.microsoft.playready").
 * \param mimeType MIME type.
 * \return Zero if supported, Non-zero otherwise.
 * \remark mimeType is currently ignored.
 */
OpenCDMError opencdm_is_type_supported(struct OpenCDMAccessor* system,
    const char keySystem[],
    const char mimeType[])
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
 * In some situations we only have the key ID, but need the specific \ref
 * OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or
 * timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 * REPLACING: void* acquire_session(const uint8_t* keyId, const uint8_t
 * keyLength, const uint32_t waitTime);
 */
struct OpenCDMSession* opencdm_get_session(struct OpenCDMAccessor* system,
    const uint8_t keyId[],
    const uint8_t length,
    const uint32_t waitTime)
{
    struct OpenCDMSession* result = nullptr;

    std::string sessionId;
    if ((system != nullptr) && (system->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable, sessionId) == true)) {
        OCDM::ISession* session(system->Session(sessionId));

        if (session != nullptr) {
            result = new OpenCDMSession(session);
            session->Release();
        }
    }

    return (result);
}

/**
 * \brief Sets server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method
 * will set that certificate. Other DRMs will ignore this call.
 * \param serverCertificate Buffer containing certificate data.
 * \param serverCertificateLength Buffer length of certificate data.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_system_set_server_certificate(
    struct OpenCDMAccessor* system, const char keySystem[],
    const uint8_t serverCertificate[], uint16_t serverCertificateLength)
{
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(system->SetServerCertificate(
            keySystem, serverCertificate, serverCertificateLength));
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
        delete session;
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
 * \return session ID, valid as long as \ref session is valid.
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
KeyStatus opencdm_session_status(const struct OpenCDMSession* session,
    const uint8_t keyId[], uint8_t length)
{
    KeyStatus result(KeyStatus::InternalError);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<const ExtendedOpenCDMSession*>(session)->Status(
            keyId, length);
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
uint32_t opencdm_session_error(const struct OpenCDMSession* session,
    const uint8_t keyId[], uint8_t length)
{
    uint32_t result(~0);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<const ExtendedOpenCDMSession*>(session)->Error(
            keyId, length);
    }

    return (result);
}

/**
 * Returns system error. This reference general system, instead of specific key.
 * \param session \ref OpenCDMSession instance.
 * \return System error code, zero if no error.
 */
OpenCDMError
opencdm_session_system_error(const struct OpenCDMSession* session)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if ((session != nullptr) && (session->IsExtended() == true)) {
        result = static_cast<OpenCDMError>(
            static_cast<const ExtendedOpenCDMSession*>(session)->Error());
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
OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
    const uint8_t keyMessage[],
    uint16_t keyLength)
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
 * This method accepts encrypted data and will typically decrypt it
 * out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system
 * allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param encrypted Buffer containing encrypted data. If applicable, decrypted
 * data will be stored here after this call returns.
 * \param encryptedLength Length of encrypted data buffer (in bytes).
 * \param IV Initial vector (IV) used during decryption.
 * \param IVLength Length of IV buffer (in bytes).
 * \return Zero on success, non-zero on error.
 * REPLACING: uint32_t decrypt(void* session, uint8_t*, const uint32_t, const
 * uint8_t*, const uint16_t);
 */
OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session,
    uint8_t encrypted[],
    const uint32_t encryptedLength,
    const uint8_t* IV, const uint16_t IVLength,
    const uint8_t* keyId, const uint16_t keyIdLength,
    uint32_t initWithLast15 /* = 0 */)
{
    OpenCDMError result(ERROR_INVALID_SESSION);

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Decrypt(
            encrypted, encryptedLength, IV, IVLength, keyId, keyIdLength, initWithLast15));
    }

    return (result);
}


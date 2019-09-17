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
 * Destructs an \ref OpenCDMAccessor instance.
 * \param system \ref OpenCDMAccessor instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system)
{
    // FIXME: Assure all sessions belonging to this system are destructed as well if any.
    assert(system != nullptr);
    if (system != nullptr) {
       delete system;
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
OpenCDMError opencdm_is_type_supported(const char keySystem[],
    const char mimeType[])
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(OpenCDMError::ERROR_KEYSYSTEM_NOT_SUPPORTED);

    if ((accessor != nullptr) && (accessor->IsTypeSupported(std::string(keySystem), std::string(mimeType)) == 0)) {
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
struct OpenCDMSession* opencdm_get_session(const uint8_t keyId[],
    const uint8_t length,
    const uint32_t waitTime)
{
    return opencdm_get_system_session(nullptr, keyId, length, waitTime);
}

struct OpenCDMSession* opencdm_get_system_session(struct OpenCDMSystem* system, const uint8_t keyId[],
    const uint8_t length, const uint32_t waitTime)
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    struct OpenCDMSession* result = nullptr;

    std::string sessionId;
    if ((accessor != nullptr) && (accessor->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable, sessionId, system) == true)) {
        result = accessor->Session(sessionId);
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
OpenCDMError opencdm_system_set_server_certificate(struct OpenCDMSystem* system,
    const uint8_t serverCertificate[], const uint16_t serverCertificateLength)
{
    OpenCDMAccessor * accessor = OpenCDMAccessor::Instance();
    OpenCDMError result(ERROR_INVALID_ACCESSOR);

    if (system != nullptr) {
        result = static_cast<OpenCDMError>(accessor->SetServerCertificate(
            system->m_keySystem, serverCertificate, serverCertificateLength));
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
 * Checks if a session has a specific keyid. Will check both BE/LE
 * \param session \ref OpenCDMSession instance.
 * \param length Length of key ID buffer (in bytes).
 * \param keyId Key ID.
 * \return 1 if keyID found else 0.
 */
uint32_t opencdm_session_has_key_id(struct OpenCDMSession* session, 
    const uint8_t length, const uint8_t keyId[])
{
    bool result = false;
    if (session != nullptr) {
        result = session->HasKeyId(length, keyId);
    }
    
    return result ? 1 : 0;
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

    if (session != nullptr) {
        result = CDMState(session->Status(length, keyId));
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

    if (session != nullptr) {
        result = session->Error(keyId, length);
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

    if (session != nullptr) {
        result = static_cast<OpenCDMError>(session->Error());
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
        result = encryptedLength > 0 ? static_cast<OpenCDMError>(session->Decrypt(
            encrypted, encryptedLength, IV, IVLength, keyId, keyIdLength, initWithLast15)) : ERROR_NONE;
    }

    return (result);
}


bool OpenCDMAccessor::WaitForKey(const uint8_t keyLength, const uint8_t keyId[],
        const uint32_t waitTime,
        const OCDM::ISession::KeyStatus status,
        std::string& sessionId, OpenCDMSystem* system) const
    {
        bool result = false;
        KeyId paramKey(keyId, keyLength);
        uint64_t timeOut(Core::Time::Now().Add(waitTime).Ticks());

        do {
            _adminLock.Lock();

            KeyMap::const_iterator session(_sessionKeys.begin());

            for  (; session != _sessionKeys.end(); ++session) {
                if (!system || session->second->BelongsTo(system) == true) {
                    if (session->second->HasKeyId(keyLength, keyId) == true)
                        break;
                }
            }

            if (session != _sessionKeys.end()) {
                result = (session->second->Status(keyLength, keyId) == status);
            }

            if (result == false) {
                _interested++;

                _adminLock.Unlock();

                TRACE_L1("Waiting for KeyId: %s", paramKey.ToString().c_str());

                uint64_t now(Core::Time::Now().Ticks());

                if (now < timeOut) {
                    _signal.Lock(static_cast<uint32_t>((timeOut - now) / Core::Time::TicksPerMillisecond));
                }

                Core::InterlockedDecrement(_interested);
            } else {
                sessionId = session->first;
                _adminLock.Unlock();
            }
        } while ((result == false) && (timeOut > Core::Time::Now().Ticks()));

        return (result);
    }
    OpenCDMSession* OpenCDMAccessor::Session(const std::string sessionId)
    {
        OpenCDMSession* result = nullptr;
        KeyMap::iterator index = _sessionKeys.find(sessionId);

        if(index != _sessionKeys.end()){
            result = index->second;
            result->AddRef();
        }

        return (result);
    }

    void OpenCDMAccessor::AddSession(OpenCDMSession* session)
    {
        string sessionId = session->SessionId();

        _adminLock.Lock();

        KeyMap::iterator index(_sessionKeys.find(sessionId));

        if (index == _sessionKeys.end()) {
            _sessionKeys.insert(std::pair<string, OpenCDMSession*>(sessionId, session));
        } else {
            TRACE_L1("Same session created, again ???? Keep the old one than. [%s]",
                sessionId.c_str());
        }

        _adminLock.Unlock();
    }
    void OpenCDMAccessor::RemoveSession(const string& sessionId)
    {

        _adminLock.Lock();

        KeyMap::iterator index(_sessionKeys.find(sessionId));

        if (index != _sessionKeys.end()) {
            _sessionKeys.erase(index);
        } else {
            TRACE_L1("A session is destroyed of which we were not aware [%s]",
                sessionId.c_str());
        }

        _adminLock.Unlock();
    }

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

// TODO: figure out how to force linking of libocdm.so
void ForceLinkingOfOpenCDM() {}

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

namespace media {

OpenCdm::OpenCdm()
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{
}

OpenCdm::OpenCdm(const OpenCdm& copy)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(copy._session)
    , _keySystem(copy._keySystem)
{

    if (_session != nullptr) {
        TRACE_L1("Created a copy of OpenCdm instance: %p", this);
        _session->AddRef();
    }
}

OpenCdm::OpenCdm(const std::string& sessionId)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(sessionId);

        if (entry != nullptr) {
            _session = new OpenCDMSession(entry);
            TRACE_L1("Created an OpenCdm instance: %p from session %s, [%p]", this,
                sessionId.c_str(), entry);
            entry->Release();
        } else {
            TRACE_L1("Failed to create an OpenCdm instance, for session %s",
                sessionId.c_str());
        }
    } else {
        TRACE_L1("Failed to create an OpenCdm instance: %p for session %s", this,
            sessionId.c_str());
    }
}

OpenCdm::OpenCdm(const uint8_t keyId[], const uint8_t length)
    : _implementation(OpenCDMAccessor::Instance())
    , _session(nullptr)
    , _keySystem()
{

    if (_implementation != nullptr) {

        OCDM::ISession* entry = _implementation->Session(keyId, length);

        if (entry != nullptr) {
            _session = new OpenCDMSession(entry);
            // TRACE_L1 ("Created an OpenCdm instance: %p from keyId [%p]", this,
            // entry);
            entry->Release();
        } else {
            TRACE_L1("Failed to create an OpenCdm instance, for keyId [%d]",
                __LINE__);
        }
    } else {
        TRACE_L1("Failed to create an OpenCdm instance: %p for keyId failed", this);
    }
}

OpenCdm::~OpenCdm()
{
    if (_session != nullptr) {
        _session->Release();
        TRACE_L1("Destructed an OpenCdm instance: %p", this);
    }
    if (_implementation != nullptr) {
        _implementation->Release();
    }
}

/* static */ OpenCdm& OpenCdm::Instance()
{
    return Core::SingletonType<OpenCdm>::Instance();
}

// ---------------------------------------------------------------------------------------------
// INSTANTIATION OPERATIONS:
// ---------------------------------------------------------------------------------------------
// Before instantiating the ROOT DRM OBJECT, Check if it is capable of
// decrypting the requested
// asset.
bool OpenCdm::GetSession(const uint8_t keyId[], const uint8_t length,
    const uint32_t waitTime)
{

    if ((_session == nullptr) && (_implementation != nullptr) && (_implementation->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable) == true)) {
        _session = new OpenCDMSession(_implementation->Session(keyId, length));
    }

    return (_session != nullptr);
}

bool OpenCdm::IsTypeSupported(const std::string& keySystem,
    const std::string& mimeType) const
{
    TRACE_L1("Checking for key system %s", keySystem.c_str());
    return ((_implementation != nullptr) && (_implementation->IsTypeSupported(keySystem, mimeType) == 0));
}

// The next call is the startng point of creating a decryption context. It
// select the DRM system
// to be used within this OpenCDM object.
void OpenCdm::SelectKeySystem(const std::string& keySystem)
{
    if (_implementation != nullptr) {
        _keySystem = keySystem;
        TRACE_L1("Creation of key system %s succeeded.", _keySystem.c_str());
    } else {
        TRACE_L1("Creation of key system %s failed. No valid remote side",
            keySystem.c_str());
    }
}

// ---------------------------------------------------------------------------------------------
// ROOT DRM OBJECT OPERATIONS:
// ---------------------------------------------------------------------------------------------
// If required, ServerCertificates can be added to this OpenCdm object (DRM
// Context).
int OpenCdm::SetServerCertificate(const uint8_t* data,
    const uint32_t dataLength)
{

    int result = 1;

    if (_keySystem.empty() == false) {

        ASSERT(_implementation != nullptr);

        TRACE_L1("Set server certificate data %d", dataLength);
        result = _implementation->SetServerCertificate(_keySystem, data, dataLength);
    } else {
        TRACE_L1("Setting server certificate failed, there is no key system. %d",
            __LINE__);
    }

    return result;
}

// Now for every particular stream a session needs to be created. Create a
// session for all
// encrypted streams that require decryption. (This allows for MultiKey
// decryption)
std::string OpenCdm::CreateSession(const std::string& dataType,
    const uint8_t* addData,
    const uint16_t addDataLength,
    const uint8_t* cdmData,
    const uint16_t cdmDataLength,
    const LicenseType license)
{

    std::string result;

    if (_keySystem.empty() == false) {

        ASSERT(_session == nullptr);

        ExtendedOpenCDMSession* newSession = new ExtendedOpenCDMSession(_implementation, _keySystem, dataType, addData, addDataLength, cdmData, cdmDataLength, static_cast<::LicenseType>(license), nullptr, nullptr);
        result = newSession->SessionId();

        _session = newSession;

        TRACE_L1("Created an OpenCdm instance: %p for keySystem %s, %p", this,
            _keySystem.c_str(), newSession);
    } else {
        TRACE_L1("Creating session failed, there is no key system. %d", __LINE__);
    }

    return (result);
}

// ---------------------------------------------------------------------------------------------
// ROOT DRM -> SESSION OBJECT OPERATIONS:
// ---------------------------------------------------------------------------------------------
// The following operations work on a Session. There is no direct access to the
// session that
// requires the operation, so before executing the session operation, first
// select it with
// the SelectSession above.
void OpenCdm::GetKeyMessage(std::string& response, uint8_t* data,
    uint16_t& dataLength)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this
    // session.
    static_cast<ExtendedOpenCDMSession*>(_session)->GetKeyMessage(response, data,
        dataLength);
}

KeyStatus OpenCdm::Update(const uint8_t* data, const uint16_t dataLength,
    std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this
    // session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Update(
        data, dataLength, response));
}

int OpenCdm::Load(std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this
    // session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Load(response));
}

int OpenCdm::Remove(std::string& response)
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    // Oke a session has been selected. Operation should take place on this
    // session.
    return (static_cast<ExtendedOpenCDMSession*>(_session)->Remove(response));
}

KeyStatus OpenCdm::Status() const
{
    KeyStatus result = StatusPending;

    if (_session != nullptr) {
        result = CDMState(_session->Status());
    }

    return (result);
}

int OpenCdm::Close()
{

    ASSERT((_session != nullptr) && (_session->IsExtended() == true));

    if (_session != nullptr) {
        _session->Close();
        _session->Release();
        _session = nullptr;
    }

    return (0);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength,
    const uint8_t* IV, const uint16_t IVLength,
    uint32_t initWithLast15)
{
    ASSERT(_session != nullptr);

    return (_session != nullptr
            ? _session->Decrypt(encrypted, encryptedLength, IV, IVLength,
                  nullptr, 0, initWithLast15)
            : 1);
}

uint32_t OpenCdm::Decrypt(uint8_t* encrypted, const uint32_t encryptedLength,
    const uint8_t* IV, const uint16_t IVLength,
    const uint8_t keyIdLength, const uint8_t keyId[],
    uint32_t initWithLast15, const uint32_t waitTime)
{

    if (_implementation->WaitForKey(keyIdLength, keyId, waitTime,
            OCDM::ISession::Usable)
        == true) {
        if (_session == nullptr) {
            _session = new OpenCDMSession(_implementation->Session(keyId, keyIdLength));
        }
        return (_session->Decrypt(encrypted, encryptedLength, IV, IVLength, keyId,
            keyIdLength, initWithLast15));
    }

    return (1);
}

} // namespace media

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

    if ((system != nullptr) && (system->WaitForKey(length, keyId, waitTime, OCDM::ISession::Usable) == true)) {
        OCDM::ISession* session(system->Session(keyId, length));

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
 * \return ExtendedOpenCDMSession ID, valid as long as \ref session is valid.
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


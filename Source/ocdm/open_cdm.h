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

#ifndef __OCDM_WRAPPER_H_
#define __OCDM_WRAPPER_H_

// WPEWebkit implementation is using the following header file to integrate their solution with
// DRM systems (PlayReady/WideVine/ClearKey).
// The implementation behind this class/interface exists in two flavors.
// 1) Fraunhofers adapted reference implementation, based on SUNRPC
// 2) Metrologicals framework, based on their proprietary RPC mechanism.
//
// The second option exists because during testing the reference/adapted implementation of Frauenhofer
// it was observed:
// - Older implementations of ucLibc had different dynamic characteristics that caused deadlocks
// - Message exchange over the SUNRPC mechanism is using continous heap memory allocations/deallocactions,
//   leading to a higher risk of memory fragmentation.
// - SUNRPC only works on UDP/TCP, given the nature and size of the messages, UDP was not an option so TCP
//   is used. There is no domain socket implementation for the SUNRPC mechanism. Domain sockets transfer
//   data, as an average, twice as fast as TCP sockets.
// - SUNRPC requires an external process (bind) to do program number lookup to TCP port connecting. Currently
//   the Frauenhofer OpenCDMi reference implementation is the only implementation requiring this service on
//   most deplyments with the WPEWebkit.
// - Common Vulnerabilities and Exposures (CVE's) have been reported with the SUNRPC that have not been resolved
//   on most platforms where the solution is deployed.
// - The Metrological RPC mechanism allows for a configurable in or out of process deplyment of the OpenCDMi
//   deployment without rebuilding.
//
// So due to performance and security exploits it was decided to offer a second implementation of the OpenCDMi
// specification that did notrequire to change the WPEWebKit

#include <stdint.h>
#include <string.h>

/**
 * Represents an OCDM system
 */
struct OpenCDMAccessor;

/**
 * Represents a OpenCDM session, use this one to decrypt.
 */
struct OpenCDMSession;

typedef enum {
    Temporary = 0,
    PersistentUsageRecord,
    PersistentLicense
} LicenseType;

/**
 * Key status.
 */
typedef enum {
    Usable = 0,
    Expired,
    Released,
    OutputRestricted,
    OutputDownscaled,
    StatusPending,
    InternalError
} KeyStatus;

#ifdef _MSVC_LANG
#undef EXTERNAL
#ifdef OCDM_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#pragma comment(lib, "ocdm.lib")
#endif
#else
#define EXTERNAL
#endif

#ifdef __cplusplus

#include <list>
#include <string>

namespace media {

class EXTERNAL OpenCdm {
private:
    OpenCdm& operator=(const OpenCdm&) = delete;

public:
    typedef ::KeyStatus KeyStatus;
    enum LicenseType {
        Temporary = ::LicenseType::Temporary,
        PersistentUsageRecord = ::LicenseType::PersistentUsageRecord,
        PersistentLicense = ::LicenseType::PersistentLicense,
    };

public:
    OpenCdm();
    OpenCdm(const OpenCdm& copy);
    explicit OpenCdm(const std::string& sessionId);
    OpenCdm(const uint8_t keyId[], const uint8_t length);
    ~OpenCdm();
    static OpenCdm& Instance();

public:
    bool GetSession(const uint8_t keyId[], const uint8_t length, const uint32_t waitTime);

    // ---------------------------------------------------------------------------------------------
    // INSTANTIATION OPERATIONS:
    // ---------------------------------------------------------------------------------------------
    // Before instantiating the ROOT DRM OBJECT, Check if it is capable of decrypting the requested
    // asset.
    bool IsTypeSupported(const std::string& keySystem, const std::string& mimeType) const;

    // The next call is the startng point of creating a decryption context. It select the DRM system
    // to be used within this OpenCDM object.
    void SelectKeySystem(const std::string& keySystem);

    // ---------------------------------------------------------------------------------------------
    // ROOT DRM OBJECT OPERATIONS:
    // ---------------------------------------------------------------------------------------------
    // If required, ServerCertificates can be added to this OpenCdm object (DRM Context).
    int SetServerCertificate(const uint8_t*, const uint32_t);

    // Now for every particular stream a session needs to be created. Create a session for all
    // encrypted streams that require decryption. (This allows for MultiKey decryption)
    std::string CreateSession(const std::string&, const uint8_t* addData, const uint16_t addDataLength, const uint8_t* cdmData, const uint16_t cdmDataLength, const LicenseType license);

    // ---------------------------------------------------------------------------------------------
    // ROOT DRM -> SESSION OBJECT OPERATIONS:
    // ---------------------------------------------------------------------------------------------
    void GetKeyMessage(std::string&, uint8_t*, uint16_t&);
    KeyStatus Update(const uint8_t*, const uint16_t, std::string&);
    int Load(std::string&);
    int Remove(std::string&);
    int Close();
    KeyStatus Status() const;

    uint32_t Decrypt(uint8_t*, const uint32_t, const uint8_t*, const uint16_t);
    uint32_t Decrypt(uint8_t*, const uint32_t, const uint8_t*, const uint16_t, const uint8_t, const uint8_t[], const uint32_t waitTime = 6000);

    inline const std::string& KeySystem() const
    {
        return (_keySystem);
    }

private:
    OpenCDMAccessor* _implementation;
    OpenCDMSession* _session;
    std::string _keySystem;
};

} // namespace media

extern "C" {

#endif

/**
 * Sometimes the compiler would like to be smart, if we do not reference anything here
 * and you enable the rightflags, the linker drops the dependency. Than Proxy/Stubs do
 * not get loaded, so lets make the instantiation of the ProxyStubs explicit !!!
 */
EXTERNAL void* opencdm_announce_proxy_stubs();

/**
 * OpenCDM error code. Zero always means success.
 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_INVALID_ACCESSOR = 0x80000001,
    ERROR_KEYSYSTEM_NOT_SUPPORTED = 0x80000002,
    ERROR_INVALID_SESSION = 0x80000003,
    ERROR_INVALID_DECRYPT_BUFFER = 0x80000004
} OpenCDMError;

/**
 * Registered callbacks with OCDM sessions.
 */
typedef struct
{
    /**
    * Request of process of DRM challenge data. Server is indicated by \ref url. The response of the server
    * needs to be send to \ref opencdm_session_update.
    *
    * \param userData Pointer passed along when \ref opencdm_session_callback was called.
    * \param url Target URL to send challenge to.
    * \param challenge Buffer containing challenge.
    * \param challengeLength Length of challenge (in bytes).
    */
    void (*process_challenge)(struct OpenCDMSession* session, const char url[], const uint8_t challenge[], const uint16_t challengeLength);

    /**
    * Called when status of a key changes. Use \ref opencdm_session_status to find out new key status.
    *
    * \param userData Pointer passed along when \ref opencdm_session_callback was issued.
    * \param keyId Buffer containing key ID.
    * \param length Length of key ID buffer.
    */
    void (*key_update)(struct OpenCDMSession* session, const uint8_t keyId[], const uint8_t length);

    /**
    * Called when a message is received from the DRM system
    *
    * \param message Text string, null terminated, from the DRM session.
    */
    void (*message)(struct OpenCDMSession* userData, const char message[]);
} OpenCDMSessionCallbacks;

/**
 * \brief Creates DRM system.
 *
 * \param keySystem Name of required key system (See \ref opencdm_is_type_supported)
 * \return \ref OpenCDMAccessor instance, NULL on error.
 */
struct OpenCDMAccessor* opencdm_create_system();

/**
 * Destructs an \ref OpenCDMAccessor instance.
 * \param system \ref OpenCDMAccessor instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_destruct_system(struct OpenCDMAccessor* system);

/**
 * \brief Checks if a DRM system is supported.
 *
 * \param keySystem Name of required key system (e.g. "com.microsoft.playready").
 * \param mimeType MIME type.
 * \return Zero if supported, Non-zero otherwise.
 * \remark mimeType is currently ignored.
 */
OpenCDMError opencdm_is_type_supported(struct OpenCDMAccessor* system, const char keySystem[], const char mimeType[]);

/**
 * \brief Maps key ID to \ref OpenCDMSession instance.
 *
 * In some situations we only have the key ID, but need the specific \ref OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 * REPLACING: void* acquire_session(const uint8_t* keyId, const uint8_t keyLength, const uint32_t waitTime);
 */
struct OpenCDMSession* opencdm_get_session(struct OpenCDmAccessor* system, const uint8_t keyId[], const uint8_t length, const uint32_t waitTime);

/**
 * \brief Sets server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method will set that certificate. Other DRMs will ignore this call.
 * \param serverCertificate Buffer containing certificate data.
 * \param serverCertificateLength Buffer length of certificate data.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_system_set_server_certificate(struct OpenCDMAccessor* system, const char keySystem[], const uint8_t serverCertificate[], const uint16_t serverCertificateLength);

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
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks,
    struct OpenCDMSession** session);

/**
 * Destructs an \ref OpenCDMSession instance.
 * \param system \ref OpenCDMSession instance to desctruct.
 * \return Zero on success, non-zero on error.
 * REPLACING: void release_session(void* session);
 */
OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session);

/**
 * Loads the data stored for a specified OpenCDM session into the CDM context.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_load(struct OpenCDMSession* session);

/**
 * Process a key message response.
 * \param session \ref OpenCDMSession instance.
 * \param keyMessage Key message to process.
 * \param keyLength Length of key message buffer (in bytes).
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_update(struct OpenCDMSession* session, const uint8_t keyMessage[], const uint16_t keyLength);

/**
 * Removes all keys/licenses related to a session.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_remove(struct OpenCDMSession* session);

/**
 * Gets Session ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Session ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_id(const struct OpenCDMSession* session);

/**
 * Returns status of a particular key assigned to a session.
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return key status.
 */
KeyStatus opencdm_session_status(const struct OpenCDMSession* session, const uint8_t keyId[], const uint8_t length);

/**
 * Returns error for key (if any).
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return Key error (zero if no error, non-zero if error).
 */
uint32_t opencdm_session_error(const struct OpenCDMSession* session, const uint8_t keyId[], const uint8_t length);

/**
 * Returns system error. This reference general system, instead of specific key.
 * \param session \ref OpenCDMSession instance.
 * \return System error code, zero if no error.
 */
OpenCDMError opencdm_session_system_error(const struct OpenCDMSession* session);

/**
 * Gets buffer ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Buffer ID, valid as long as \ref session is valid.
 */
const char* opencdm_session_buffer_id(const struct OpenCDMSession* session);

/**
 * Closes a session.
 * \param session \ref OpenCDMSession instance.
 * \return zero on success, non-zero on error.
 */
OpenCDMError opencdm_session_close(struct OpenCDMSession* session);

/**
 * \brief Performs decryption.
 *
 * This method accepts encrypted data and will typically decrypt it out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param encrypted Buffer containing encrypted data. If applicable, decrypted data will be stored here after this call returns.
 * \param encryptedLength Length of encrypted data buffer (in bytes).
 * \param IV Initial vector (IV) used during decryption.
 * \param IVLength Length of IV buffer (in bytes).
 * \return Zero on success, non-zero on error.
 * REPLACING: uint32_t decrypt(void* session, uint8_t*, const uint32_t, const uint8_t*, const uint16_t);
 */
OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session, uint8_t encrypted[], const uint32_t encryptedLength, const uint8_t IV[], uint16_t IVLength);

#ifdef __cplusplus
}
#endif

#endif // __OCDM_WRAPPER_H_

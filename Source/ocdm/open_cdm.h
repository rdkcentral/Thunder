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

// WPEWebkit implementation is using the following header file to integrate
// their solution with
// DRM systems (PlayReady/WideVine/ClearKey).
// The implementation behind this class/interface exists in two flavors.
// 1) Fraunhofers adapted reference implementation, based on SUNRPC
// 2) Metrologicals framework, based on their proprietary RPC mechanism.
//
// The second option exists because during testing the reference/adapted
// implementation of Frauenhofer
// it was observed:
// - Older implementations of ucLibc had different dynamic characteristics that
// caused deadlocks
// - Message exchange over the SUNRPC mechanism is using continous heap memory
// allocations/deallocactions,
//   leading to a higher risk of memory fragmentation.
// - SUNRPC only works on UDP/TCP, given the nature and size of the messages,
// UDP was not an option so TCP
//   is used. There is no domain socket implementation for the SUNRPC mechanism.
//   Domain sockets transfer
//   data, as an average, twice as fast as TCP sockets.
// - SUNRPC requires an external process (bind) to do program number lookup to
// TCP port connecting. Currently
//   the Frauenhofer OpenCDMi reference implementation is the only
//   implementation requiring this service on
//   most deplyments with the WPEWebkit.
// - Common Vulnerabilities and Exposures (CVE's) have been reported with the
// SUNRPC that have not been resolved
//   on most platforms where the solution is deployed.
// - The Metrological RPC mechanism allows for a configurable in or out of
// process deplyment of the OpenCDMi
//   deployment without rebuilding.
//
// So due to performance and security exploits it was decided to offer a second
// implementation of the OpenCDMi
// specification that did notrequire to change the WPEWebKit

#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <list>

#ifndef EXTERNAL
#ifdef _MSVC_LANG
#ifdef OCDM_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#endif
#else
#define EXTERNAL __attribute__ ((visibility ("default")))
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WINDOWS) 
    #if !defined(OCDM_EXPORTS)
    #pragma comment(lib, "ocdm.lib")
    #endif

    /**
     * Sometimes the compiler would like to be smart, if we do not reference
     * anything here
     * and you enable the rightflags, the linker drops the dependency. Than
     * Proxy/Stubs do
     * not get loaded, so lets make the instantiation of the ProxyStubs explicit !!!
     */
    EXTERNAL void ForceLinkingOfOpenCDM();
#endif

#define SESSION_ID_LEN 16
#define MAX_NUM_SECURE_STOPS 8

/**
 * Represents an OCDM system
 */
struct OpenCDMSystem;

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
    OutputRestrictedHDCP22,
    OutputDownscaled,
    StatusPending,
    InternalError,
    HWError
} KeyStatus;
  
/**
 * OpenCDM error code. Zero always means success.
 */
typedef enum {
    ERROR_NONE = 0,
    ERROR_UNKNOWN = 1,
    ERROR_MORE_DATA_AVAILBALE=2,
    ERROR_INVALID_ACCESSOR = 0x80000001,
    ERROR_KEYSYSTEM_NOT_SUPPORTED = 0x80000002,
    ERROR_INVALID_SESSION = 0x80000003,
    ERROR_INVALID_DECRYPT_BUFFER = 0x80000004,
    ERROR_OUT_OF_MEMORY = 0x80000005,
    ERROR_FAIL = 0x80004005,
    ERROR_INVALID_ARG = 0x80070057,
    ERROR_SERVER_INTERNAL_ERROR = 0x8004C600,
    ERROR_SERVER_INVALID_MESSAGE = 0x8004C601,
    ERROR_SERVER_SERVICE_SPECIFIC = 0x8004C604,
} OpenCDMError;

/**
 * Registered callbacks with OCDM sessions.
 */
typedef struct {
    /**
    * Request of process of DRM challenge data. Server is indicated by \ref url. The response of the server
    * needs to be send to \ref opencdm_session_update.
    *
    * \param session The session the notification applies to.
    * \param userData Pointer passed along when \ref opencdm_construct_session was issued.
    * \param url Target URL to send challenge to.
    * \param challenge Buffer containing challenge.
    * \param challengeLength Length of challenge (in bytes).
    */
    void (*process_challenge_callback)(struct OpenCDMSession* session, void* userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength);

    /**
    * Called when status of a key changes. Use \ref opencdm_session_status to find out new key status.
    *
    * \param session The session the notification applies to.
    * \param userData Pointer passed along when \ref opencdm_construct_session was issued.
    * \param keyId Buffer containing key ID.
    * \param length Length of key ID buffer.
    */
    void (*key_update_callback)(struct OpenCDMSession* session, void* userData, const uint8_t keyId[], const uint8_t length);

    /**
    * Called when an error message is received from the DRM system
    *
    * \param session The session the notification applies to.
    * \param userData Pointer passed along when \ref opencdm_construct_session was issued.
    * \param message Text string, null terminated, from the DRM session.
    */
    void (*error_message_callback)(struct OpenCDMSession* session, void* userData, const char message[]);

    /**
    * Called after all known key status changes were reported.
    *
    * \param session The session the notification applies to.
    * \param userData Pointer passed along when \ref opencdm_construct_session was issued.
    */
    void (*keys_updated_callback)(const struct OpenCDMSession* session, void* userData);
} OpenCDMSessionCallbacks;

EXTERNAL OpenCDMError opencdm_init();

EXTERNAL OpenCDMError opencdm_deinit();

/**
 * \brief Creates DRM system.
 *
 * \return \ref OpenCDMAccessor instance, NULL on error.
 */
EXTERNAL struct OpenCDMSystem* opencdm_create_system(const char keySystem[]);

/**
 * \brief Creates DRM system.
 *
 * \param system Output parameter that will contain pointer to instance of \ref OpenCDMSystem.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_create_system_extended(const char keySystem[], struct OpenCDMSystem** system);

/**
 * Destructs an \ref OpenCDMAccessor instance.
 * \param system \ref OpenCDMAccessor instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_destruct_system(struct OpenCDMSystem* system);

/**
 * \brief Checks if a DRM system is supported.
 *
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem Name of required key system (e.g.
 * "com.microsoft.playready").
 * \param mimeType MIME type.
 * \return Zero if supported, Non-zero otherwise.
 * \remark mimeType is currently ignored.
 */
EXTERNAL OpenCDMError opencdm_is_type_supported(const char keySystem[],
    const char mimeType[]);

/**
 * \brief Retrieves DRM system specific metadata.
 *
 * \param system Instance of \ref OpenCDMAccessor.
 * \param metadata, buffer to write metadata into, always 0 terminated (also when not large enough to hold all data) except when metadata is
 *     Null of course. Null allowed to retrieve required size needed for this buffer in metadataSize to be able to allocate required buffer 
 *     for subsequent call to opencdm_is_type_supported
 * \param metadataSize, in: size of metadata buffer, out: required size to hold all data available when return value is ERROR_MORE_DATA_AVAILBALE,
 *     , number of characters written into metadata (incl 0 terminator) otherwise. Note in case metadata could not hold all data but was not of zero
 *     length it is filled up to the maximum size (still zero terminated) but also ERROR_MORE_DATA_AVAILBALE is returned with the required size needed
 *     to hold all data
 * \return Zero on success, non-zero on error. ERROR_MORE_DATA_AVAILBALE when the buffer was not large enough to hold all the data available. 
 */
EXTERNAL OpenCDMError opencdm_system_get_metadata(struct OpenCDMSystem* system, 
    char metadata[], 
    uint16_t* metadataSize);

/**
 * \brief Returns string describing version of DRM system.
 *
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem Name of queried  key system (e.g.
 * "com.microsoft.playready").
 * \param versionStr Char buffer to receive NULL-terminated version string.
 * (Should as least be 64 chars long.)
 * \return Zero if successful, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_system_get_version(struct OpenCDMSystem* system,
    char versionStr[]);

/**
 * \brief Returns time according to DRM system.
 * Some systems (e.g. PlayReady) keep their own clocks, for example to prevent
 * rollback. Systems
 * not implementing their own clock can return the system time.
 *
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem Name of queried key system (e.g. "com.microsoft.playready").
 * \param time Output variable that will contain DRM system time.
 * \return Zero if successful, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_system_get_drm_time(struct OpenCDMSystem* system,
    uint64_t* time);

/**
 * \brief Maps key ID to \ref OpenCDMSession instance.
 *
 * In some situations we only have the key ID, but need the specific \ref
 * OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or
 * timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 */
EXTERNAL struct OpenCDMSession* opencdm_get_session(const uint8_t keyId[],
    const uint8_t length,
    const uint32_t waitTime);

/**
 * \brief Maps key ID to \ref OpenCDMSession instance within the given system instance.
 *
 * In some situations we only have the key ID, but need the specific \ref
 * OpenCDMSession instance that
 * belongs to this key ID. This method facilitates this requirement.
 * \param system Instance of \ref OpenCDMSystem.
 * \param keyId Array containing key ID.
 * \param length Length of keyId array.
 * \param maxWaitTime Maximum allowed time to block (in miliseconds).
 * \return \ref OpenCDMSession belonging to key ID, or NULL when not found or
 * timed out. This instance
 *         also needs to be destructed using \ref opencdm_session_destruct.
 */
EXTERNAL struct OpenCDMSession* opencdm_get_system_session(struct OpenCDMSystem* system, const uint8_t keyId[],
    const uint8_t length, const uint32_t waitTime);


/**
 * \brief Sets server certificate.
 *
 * Some DRMs (e.g. WideVine) use a system-wide server certificate. This method
 * will set that certificate. Other DRMs will ignore this call.
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem Name of key system to set server certificate for.
 * \param serverCertificate Buffer containing certificate data.
 * \param serverCertificateLength Buffer length of certificate data.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_system_set_server_certificate(
    struct OpenCDMSystem* system,
    const uint8_t serverCertificate[], const uint16_t serverCertificateLength);

/**
 * \brief Create DRM session (for actual decrypting of data).
 *
 * Creates an instance of \ref OpenCDMSession using initialization data.
 * \param system Instance of \ref OpenCDMAccessor.
 * \param keySystem DRM system to create the session for.
 * \param licenseType DRM specifc signed integer selecting License Type (e.g.
 * "Limited Duration" for PlayReady).
 * \param initDataType Type of data passed in \ref initData.
 * \param initData Initialization data.
 * \param initDataLength Length (in bytes) of initialization data.
 * \param CDMData CDM data.
 * \param CDMDataLength Length (in bytes) of \ref CDMData.
 * \param callbacks the instance of \ref OpenCDMSessionCallbacks with callbacks to be called on events.
 * \param userData the user data to be passed back to the \ref OpenCDMSessionCallbacks callbacks.
 * \param session Output parameter that will contain pointer to instance of \ref OpenCDMSession.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_construct_session(struct OpenCDMSystem* system, const LicenseType licenseType,
    const char initDataType[], const uint8_t initData[], const uint16_t initDataLength,
    const uint8_t CDMData[], const uint16_t CDMDataLength, OpenCDMSessionCallbacks* callbacks, void* userData,
    struct OpenCDMSession** session);

/**
 * Destructs an \ref OpenCDMSession instance.
 * \param system \ref OpenCDMSession instance to desctruct.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_destruct_session(struct OpenCDMSession* session);

/**
 * Loads the data stored for a specified OpenCDM session into the CDM context.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_session_load(struct OpenCDMSession* session);

/**
 * Process a key message response.
 * \param session \ref OpenCDMSession instance.
 * \param keyMessage Key message to process.
 * \param keyLength Length of key message buffer (in bytes).
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_session_update(struct OpenCDMSession* session,
    const uint8_t keyMessage[],
    const uint16_t keyLength);

/**
 * Removes all keys/licenses related to a session.
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_session_remove(struct OpenCDMSession* session);

/**
 * Retrieves DRM session specific metadata of a session.
 * \param session \ref OpenCDMSession instance.
* \param metadata, buffer to write metadata into, always 0 terminated (also when not large enough to hold all data) except when metadata is
 *     Null of course. Null allowed to retrieve required size needed for this buffer in metadataSize to be able to allocate required buffer 
 *     for subsequent call to opencdm_session_metadata
 * \param metadataSize, in: size of metadata buffer, out: required size to hold all data available when return value is ERROR_MORE_DATA_AVAILBALE,
 *     , number of characters written into metadata (incl 0 terminator) otherwise. Note in case metadata could not hold all data but was not of zero
 *     length it is filled up to the maximum size (still zero terminated) but also ERROR_MORE_DATA_AVAILBALE is returned with the required size needed
 *     to hold all data
 * \return Zero on success, non-zero on error. ERROR_MORE_DATA_AVAILBALE when the buffer was not large enough to hold all the data available. 

 */
EXTERNAL OpenCDMError opencdm_session_metadata(const struct OpenCDMSession* session, 
    char metadata[], 
    uint16_t* metadataSize);

/**
 * Let CDM know playback stopped and reset output protection
 * \param session \ref OpenCDMSession instance.
 * \return Zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_session_resetoutputprotection(struct OpenCDMSession* session);

/**
 * Gets Session ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Session ID, valid as long as \ref session is valid.
 */
EXTERNAL const char* opencdm_session_id(const struct OpenCDMSession* session);

/**
 * Checks if a session has a specific keyid. Will check both BE/LE
 * \param session \ref OpenCDMSession instance.
 * \param length Length of key ID buffer (in bytes).
 * \param keyId Key ID.
 * \return 1 if keyID found else 0.
 */
EXTERNAL uint32_t opencdm_session_has_key_id(struct OpenCDMSession* session, 
    const uint8_t length, const uint8_t keyId[]);

/**
 * Returns status of a particular key assigned to a session.
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return key status.
 */
EXTERNAL KeyStatus opencdm_session_status(const struct OpenCDMSession* session,
    const uint8_t keyId[], const uint8_t length);

/**
 * Returns error for key (if any).
 * \param session \ref OpenCDMSession instance.
 * \param keyId Key ID.
 * \param length Length of key ID buffer (in bytes).
 * \return Key error (zero if no error, non-zero if error).
 */
EXTERNAL uint32_t opencdm_session_error(const struct OpenCDMSession* session,
    const uint8_t keyId[], const uint8_t length);

/**
 * Returns system error. This reference general system, instead of specific key.
 * \param session \ref OpenCDMSession instance.
 * \return System error code, zero if no error.
 */
EXTERNAL OpenCDMError opencdm_session_system_error(const struct OpenCDMSession* session);

/**
 * Gets buffer ID for a session.
 * \param session \ref OpenCDMSession instance.
 * \return Buffer ID, valid as long as \ref session is valid.
 */
EXTERNAL const char* opencdm_session_buffer_id(const struct OpenCDMSession* session);

/**
 * Closes a session.
 * \param session \ref OpenCDMSession instance.
 * \return zero on success, non-zero on error.
 */
EXTERNAL OpenCDMError opencdm_session_close(struct OpenCDMSession* session);

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
 * \param IV Initial vector (IV) used during decryption. Can be NULL, in that
 * case and IV of all zeroes is assumed.
 * \param IVLength Length of IV buffer (in bytes).
 * \param keyID keyID to use for decryption
 * \param keyIDLength Length of keyID buffer (in bytes).
 * \param initWithLast15 Whether decryption context needs to be initialized with
 * last 15 bytes. Currently this only applies to PlayReady DRM.
 * \return Zero on success, non-zero on error.
 */
#ifdef __cplusplus
EXTERNAL OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session,
    uint8_t encrypted[],
    const uint32_t encryptedLength,
    const uint8_t* IV, uint16_t IVLength,
    const uint8_t* keyId, const uint16_t keyIdLength,
    uint32_t initWithLast15 = 0);
#else
EXTERNAL OpenCDMError opencdm_session_decrypt(struct OpenCDMSession* session,
    uint8_t encrypted[],
    const uint32_t encryptedLength,
    const uint8_t* IV, uint16_t IVLength,
    const uint8_t* keyId, const uint16_t keyIdLength,
    uint32_t initWithLast15);
#endif // __cplusplus

#ifdef __cplusplus
}
#endif

#endif // __OCDM_WRAPPER_H_

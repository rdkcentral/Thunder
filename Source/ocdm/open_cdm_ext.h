#pragma once

#include "open_cdm.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////
// System
//////////////////////////////////////

// LEAN: Only netflix uses secure store, but is generic PlayReady functionality? 
struct OpenCDMAccessor* opencdm_create_system_netflix(const char readDir[], const char storeLocation[]);

// LEAN: - Is called seperately from create_system, so might be called more than once
//       - Sets a revocation buffer, only used in netflix case
OpenCDMError opencdm_init_system_netflix(struct OpenCDMAccessor* system);

// TODO: document we need at least 64 bytes in "versionStr"
// LEAN: GetVersionStr might only be used by Netflix, but appears to be generic PlayReady functionality.
OpenCDMError opencdm_system_get_version(struct OpenCDMAccessor* system, char versionStr[]);

// LEAN: uses "_Netflix"-suffixed PR call
OpenCDMError opencdm_system_get_ldl_session_limit(struct OpenCDMAccessor* system, uint32_t * ldlLimit);

// LEAN: Secure Stop might only be used by Netflix, but appears to be generic PlayReady functionality.
OpenCDMError opencdm_system_enable_secure_stop(struct OpenCDMAccessor* system, uint32_t use);
OpenCDMError opencdm_system_commit_secure_stop(struct OpenCDMAccessor* system, const unsigned char sessionID[], uint32_t sessionIDLength, const unsigned char serverResponse[], uint32_t serverResponseLength);

// LEAN: DrmSystemTime might only be used by Netflix, but appears to be generic PlayReady functionality.
OpenCDMError opencdm_system_get_drm_time(struct OpenCDMAccessor* system, time_t * time);

OpenCDMError opencdm_system_teardown(struct OpenCDMAccessor* system);

// LEAN: Secure Store might only be used by Netflix, but appears to be generic PlayReady functionality.
// TODO: document that buffer needs to be at least 256 bytes big
OpenCDMError opencdm_get_secure_store_hash(struct OpenCDMAccessor* system, uint8_t secureStoreHash[], uint32_t secureStoreHashLength);
OpenCDMError opencdm_delete_secure_store(struct OpenCDMAccessor* system);


//////////////////////////////////////
// Session
//////////////////////////////////////

// LEAN: can remove these args: session ID, license type, content id
OpenCDMError opencdm_create_session_netflix(struct OpenCDMAccessor* system, struct OpenCDMSession ** opencdmSession, const uint8_t drmHeader[], uint32_t drmHeaderLength);

// TODO: do we need a specific "opencdm_destroy_session" for Netflix?
// TODO: rename to "destruct"?
OpenCDMError opencdm_destroy_session_netflix(struct OpenCDMSession * opencdmSession);

// play levels
// LEAN: use json for this
uint16_t opencdm_session_get_playlevel_compressed_video(struct OpenCDMSession * mOpenCDMSession);
uint16_t opencdm_session_get_playlevel_uncompressed_video(struct OpenCDMSession * mOpenCDMSession);
uint16_t opencdm_session_get_playlevel_analog_video(struct OpenCDMSession * mOpenCDMSession);
uint16_t opencdm_session_get_playlevel_compressed_audio(struct OpenCDMSession * mOpenCDMSession);
uint16_t opencdm_session_get_playlevel_uncompressed_audio(struct OpenCDMSession * mOpenCDMSession);

// LEAN: this one is more complicated to remove, is needed at a few sports: initdecryptcontext and genchallenge
OpenCDMError opencdm_session_set_drm_header(struct OpenCDMSession * opencdmSession, const uint8_t drmHeader[], uint32_t drmHeaderSize);

// TODO: document that this is a two-pass system (first get size, then get data).
// LEAN: this one uses a playready "_netflix" suffix
OpenCDMError opencdm_session_get_challenge_data_netflix(struct OpenCDMSession * mOpenCDMSession, uint8_t * challenge, uint32_t * challengeSize, uint32_t isLDL);

// TODO: document that "secureStopId" should be 16 bytes.
// LEAN: this one uses a playready "_Netflix" suffix
OpenCDMError opencdm_session_store_license_data(struct OpenCDMSession * mOpenCDMSession, const uint8_t licenseData[], uint32_t licenseDataSize, unsigned char * secureStopId);

// LEAN: this one uses a playready "_Netflix" suffix
OpenCDMError opencdm_session_init_decrypt_context_by_kid(struct OpenCDMSession * mOpenCDMSession);

#ifdef __cplusplus
} // extern "C"
#endif

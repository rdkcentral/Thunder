/*
 * Copyright 2016-2019 TATA ELXSI
 * Copyright 2016-2019 Metrological
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

#ifndef __OPEN_OCDM_EXT_H_
#define __OPEN_OCDM_EXT_H_

#include "open_cdm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns maximum number of concurrent LDLs (limited duration licenses).
 * \param system Extended OCDM system handle.
 * \param system ldlLimit Output parameter that will contain number of allowed
 * concurrent LDLs.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_system_ext_get_ldl_session_limit(struct OpenCDMSystem* system,
    uint32_t* ldlLimit);

/**
 * Check Secure Stop is enabled or not.
 * \param system Extended OCDM system handle.
 * \return True if enabled, fals otherwise.
 */
uint32_t opencdm_system_ext_is_secure_stop_enabled(struct OpenCDMSystem* system);

/**
 * Enables/disables Secure Stop.
 * \param system Extended OCDM system handle.
 * \param use Whether to enable Secure Stop (1: true, 0: false).
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_system_ext_enable_secure_stop(struct OpenCDMSystem* system,
    uint32_t use);

/**
 * Reset Secure stop.
 * \param system Extended OCDM system handle.
 */
uint32_t opencdm_system_ext_reset_secure_stop(struct OpenCDMSystem* system);

/**
 * Get a secure stop ids.
 * \param system Extended OCDM system handle.
 * \param Ids secure stop Ids.
 * \param count number of valid stop Ids
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_system_ext_get_secure_stop_ids(struct OpenCDMSystem* system,
    uint8_t Ids[], uint16_t idsLength,
    uint32_t* count);

/**
 * Get a secure stop.
 * \param system Extended OCDM system handle.
 * \param sessionID Session ID.
 * \param sessionIDLength Session ID length (in bytes).
 * \param rawData secure stop info
 * \param rawSize secure stop info length (in bytes).
 * \return Zero if successful, non-zero otherwise.
 */
// TODO: is this the session ID, shouldn't we pass OCDMSession?
//  If so, OpenCDMSession arg and remove first three (so only three args)
OpenCDMError opencdm_system_ext_get_secure_stop(struct OpenCDMSystem* system,
    const uint8_t sessionID[],
    uint32_t sessionIDLength,
    uint8_t rawData[],
    uint16_t* rawSize);

/**
 * Commits a secure stop.
 * \param system Extended OCDM system handle.
 * \param sessionID Session ID.
 * \param sessionIDLength Session ID length (in bytes).
 * \param serverResponse Server response.
 * \param serverResponseLength Server response length (in bytes).
 * \return Zero if successful, non-zero otherwise.
 */
// TODO: also here only OpenCDMSession*?
OpenCDMError opencdm_system_ext_commit_secure_stop(
    struct OpenCDMSystem* system, const uint8_t sessionID[],
    uint32_t sessionIDLength, const uint8_t serverResponse[],
    uint32_t serverResponseLength);

/**
 * Gets Secure key hash.
 * \param system Extended OCDM system handle.
 * \param keyStoreHash Buffer that will contain key store hash.
 * \param keyStoreHashLength Size of keyStoreHash (make sure is at least 64
 * bytes).
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_get_key_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t keyStoreHash[],
    uint32_t keyStoreHashLength);

/**
 * Gets Secure Store hash.
 * \param system Extended OCDM system handle.
 * \param secureStoreHash Buffer that will contain secure store hash.
 * \param secureStoreHashLength Size of secureStoreHash (make sure is at least
 * 64 bytes).
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_get_secure_store_hash_ext(struct OpenCDMSystem* system,
    uint8_t secureStoreHash[],
    uint32_t secureStoreHashLength);

/**
 * Deletes key store.
 * \param system Extended OCDM system handle.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_delete_key_store(struct OpenCDMSystem* system);

/**
 * Deletes secure store.
 * \param system Extended OCDM system handle.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_delete_secure_store(struct OpenCDMSystem* system);

/**
 * Sets DRM header.
 * \param opencdmSession OCDM Session.
 * \param drmHeader Buffer containing DRM header.
 * \param drmHeaderSize Size of buffer containing DRM header (in bytes).
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_session_set_drm_header(struct OpenCDMSession* opencdmSession,
    const uint8_t drmHeader[],
    uint32_t drmHeaderSize);

/**
 * Gets challenge data for session.
 * Use this function in two steps: first set \ref challenge to NULL and pass a
 * valid pointer to \ref challengeSize. This will
 * give you the minimum buffer size. Secondly, allocate \ref challengeSize bytes
 * and pass via \ref challenge while also passing
 * allocated number of bytes in unsigned integer pointed to by \ref
 * challengeSize.
 * \param opencdmSession OCDM Session.
 * \param challenge Output buffer that will contain the challenge data, or NULL
 * during the first pass to get required buffer size.
 * \param challengeSize Pointer to unsigned int that will either receive the
 * required buffer size, or allocated number of bytes in \ref challenge.
 * \param isLDL Whether we want a Limited Duration License of not (1: yes, 0:
 * no).
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_session_get_challenge_data(struct OpenCDMSession* mOpenCDMSession,
    uint8_t* challenge, uint32_t* challengeSize,
    uint32_t isLDL);

/**
 * Cancel Challenge data in progress
 * \param opencdmSession OCDM Session.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_session_cancel_challenge_data(struct OpenCDMSession* mOpenCDMSession);

/**
 * Stores challenge data for session.
 * \param opencdmSession OCDM Session.
 * \param licenseData Buffer containing license data.
 * \param licenseDataSize Size of buffer containing license data (in bytes).
 * \param secureStopId Secure stop ID.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_session_store_license_data(
    struct OpenCDMSession* mOpenCDMSession, const uint8_t licenseData[],
    uint32_t licenseDataSize, uint8_t* secureStopId);

/**
 * Initializes the decryption context of a session via (unused Key ID).
 * Some applications (e.g. Netflix) require to call this function for some DRM
systems (e.g. PlayReady 2.0).
 :75
* \param opencdmSession OCDM Session.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_session_select_key_id(
    struct OpenCDMSession* mOpenCDMSession, uint8_t keyLenght, const uint8_t keyId[]);

/**
 * Tear down the drm system
 * \param system Extended OCDM system handle.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError opencdm_system_teardown(struct OpenCDMSystem* system);

/**
 * Deinitializes the decryption context of a session via (unused Key ID).
 * Some applications (e.g. Netflix) require to call this function for some DRM
 * systems (e.g. PlayReady 2.0).
 * \param opencdmSession OCDM Session.
 * \return Zero if successful, non-zero otherwise.
 */
OpenCDMError
opencdm_session_clean_decrypt_context(struct OpenCDMSession* mOpenCDMSession);
#ifdef __cplusplus
} // extern "C"
#endif

#endif // __OPEN_OCDM_EXT_H_

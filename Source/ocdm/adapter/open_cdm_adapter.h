
#ifndef __OPEN_CDM_ADAPTER_H
#define __OPEN_CDM_ADAPTER_H

#include "open_cdm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Performs decryption based on adapter implementation.
 *
 * This method accepts encrypted data and will typically decrypt it out-of-process (for security reasons). The actual data copying is performed
 * using a memory-mapped file (for performance reasons). If the DRM system allows access to decrypted data (i.e. decrypting is not
 * performed in a TEE), the decryption is performed in-place.
 * \param session \ref OpenCDMSession instance.
 * \param buffer Gstreamer buffer containing encrypted data and related meta data. If applicable, decrypted data will be stored here after this call returns.
 * \param subSample Gstreamer buffer containing subsamples size which has been parsed from protection meta data.
 * \param subSampleCount count of subsamples
 * \param IV Initial vector (IV) used during decryption.
 * \param IVLength Length of IV buffer (in bytes).
 * \return Zero on success, non-zero on error.
 */
    OpenCDMError adapter_session_decrypt(struct OpenCDMSession * session, void* buffer, void* subSample, const uint32_t subSampleCount, const uint8_t IV[], uint16_t IVLength);

#ifdef __cplusplus
}
#endif

#endif // __OPEN_CDM_ADAPTER_H

 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#ifndef __OPEN_CDM_ADAPTER_H
#define __OPEN_CDM_ADAPTER_H

#include "open_cdm.h"

struct _GstBuffer;
typedef struct _GstBuffer GstBuffer;

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
 * \param IV Gstreamer buffer containing initial vector (IV) used during decryption.
 * \param keyID Gstreamer buffer containing keyID to use for decryption
 * \return Zero on success, non-zero on error.
 */
    OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
                                                   GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15);

#ifdef __cplusplus
}
#endif

#endif // __OPEN_CDM_ADAPTER_H

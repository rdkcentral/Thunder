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
 
#include "open_cdm_adapter.h"

#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

#include <nexus_memory.h>
#include <gst_brcm_svp_meta.h>
#include <assert.h>
#include "Trace.h"

static const uint8_t nalUnit[] = {0x00, 0x00, 0x00, 0x01};

inline bool mappedBuffer(GstBuffer *buffer, bool writable, uint8_t **data, uint32_t *size)
{
    GstMapInfo map;

    if (!gst_buffer_map (buffer, &map, writable ? GST_MAP_WRITE : GST_MAP_READ)) {
        return false;
    }

    *data = reinterpret_cast<uint8_t* >(map.data);
    *size = static_cast<uint32_t >(map.size);
    gst_buffer_unmap (buffer, &map);

    return true;
}

OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSample, const uint32_t subSampleCount,
                                               GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15)
{
    OpenCDMError result (ERROR_INVALID_SESSION);

    if (session != nullptr) {

        uint8_t *mappedData = nullptr;
        uint32_t mappedDataSize = 0;
        if (mappedBuffer(buffer, true, &mappedData, &mappedDataSize) == false) {

            TRACE_L1("adapter_session_decrypt: Invalid buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        uint8_t *mappedSubSample = nullptr;
        uint32_t mappedSubSampleSize = 0;
        if (subSample != nullptr && mappedBuffer(subSample, false, &mappedSubSample, &mappedSubSampleSize) == false) {

            TRACE_L1("adapter_session_decrypt: Invalid subsample buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        uint8_t *mappedIV = nullptr;
        uint32_t mappedIVSize = 0;
        if (mappedBuffer(IV, false, &mappedIV, &mappedIVSize) == false) {
            TRACE_L1("adapter_session_decrypt: Invalid IV buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        uint8_t *mappedKeyID = nullptr;
        uint32_t mappedKeyIDSize = 0;
        if (keyID != nullptr && mappedBuffer(keyID, false, &mappedKeyID, &mappedKeyIDSize) == false) {
            TRACE_L1("Invalid keyID buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        uint32_t bufferSize = mappedDataSize;

        uint32_t encryptedSampleCount = 0;
        if (mappedSubSample) {
            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            uint32_t totalEncrypted = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);
                totalEncrypted += inEncrypted;
                if (inEncrypted)
                    encryptedSampleCount++;
            }
            gst_byte_reader_free(reader);

            if (totalEncrypted == 0) {
                result = ERROR_NONE;
                goto exit;
            }

            bufferSize = totalEncrypted;
        }

        uint8_t* encryptedData = reinterpret_cast<uint8_t*> (::malloc(bufferSize));
        uint8_t* encryptedDataIter = encryptedData;

        brcm_svp_meta_data_t* svpMeta = static_cast<brcm_svp_meta_data_t *>(g_malloc(sizeof(brcm_svp_meta_data_t)));
        memset(svpMeta, 0, sizeof(brcm_svp_meta_data_t));
        svpMeta->sub_type = GST_META_BRCM_SVP_TYPE_3;
        svpMeta->u.u3.chunks_cnt = encryptedSampleCount > 0 ? encryptedSampleCount : 1;
        svpMeta->u.u3.chunk_info = static_cast<svp_chunk_info *>(g_malloc(svpMeta->u.u3.chunks_cnt * sizeof(svp_chunk_info)));

        if (mappedSubSample) {
            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            for (uint32_t indexClr = 0, index = 0, position = 0; index < subSampleCount; index++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                // Ignore whole clean subsamples, do not process them in decryption
                if (inEncrypted) {
                    ::memcpy(encryptedDataIter, mappedData + indexClr + inClear, inEncrypted);
                    svpMeta->u.u3.chunk_info[position].clear_size = inClear;
                    svpMeta->u.u3.chunk_info[position].encrypted_size = inEncrypted;
                    svpMeta->u.u3.chunk_info[position].offset = indexClr;
                    encryptedDataIter += inEncrypted;
                    position++;
                }
                indexClr += inClear + inEncrypted;
            }
            gst_byte_reader_free(reader);
        } else {
            svpMeta->u.u3.chunk_info[0].clear_size = 0;
            svpMeta->u.u3.chunk_info[0].encrypted_size = mappedDataSize;
            svpMeta->u.u3.chunk_info[0].offset = 0;

            ::memcpy(encryptedDataIter, mappedData , mappedDataSize);

        }
        result = opencdm_session_decrypt(session, encryptedData, bufferSize, mappedIV, mappedIVSize,
                                         mappedKeyID, mappedKeyIDSize, initWithLast15);
        if (result != ERROR_NONE) {
            TRACE_L1("adapter_session_decrypt: opencdm_session_decrypt failed!");
            ::free(encryptedData);
            goto exit;
        }

        uint32_t *tokenHandle = reinterpret_cast<uint32_t *>(encryptedData);
        NEXUS_MemoryBlockHandle block = nullptr;
        block = NEXUS_MemoryBlock_Clone (reinterpret_cast<NEXUS_MemoryBlockTokenHandle>(*tokenHandle));
        if (!block) {
            TRACE_L1("Memory token alloc error");
            ::free(encryptedData);
            goto exit;
        }
        ::free(encryptedData);
        void *opaqueData;
        NEXUS_MemoryBlock_Lock(block, &opaqueData);
        if (!opaqueData) {
            TRACE_L1("Memory token alloc error");
            goto exit;
        }

        svpMeta->u.u3.secbuf_ptr = reinterpret_cast<uintptr_t>(opaqueData);
        gst_buffer_add_brcm_svp_meta(buffer, svpMeta);
    }
exit:
    return (result);
}

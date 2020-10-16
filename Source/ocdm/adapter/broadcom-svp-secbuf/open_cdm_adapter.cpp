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

#include <gst_svp_meta.h>
#include "b_secbuf.h"

struct Rpc_Secbuf_Info {
    uint8_t *ptr;
    uint32_t type;
    size_t   size;
    void    *token;
};
OpenCDMError opencdm_gstreamer_session_decrypt(struct OpenCDMSession* session, GstBuffer* buffer, GstBuffer* subSampleBuffer, const uint32_t subSampleCount,
                                               GstBuffer* IV, GstBuffer* keyID, uint32_t initWithLast15)
{
    OpenCDMError result (ERROR_INVALID_SESSION);

    struct Rpc_Secbuf_Info sb_info;

    if (session != nullptr) {
        GstMapInfo dataMap;
        if (gst_buffer_map(buffer, &dataMap, (GstMapFlags) GST_MAP_READWRITE) == false) {
            fprintf(stderr, "Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        GstMapInfo ivMap;
        if (gst_buffer_map(IV, &ivMap, (GstMapFlags) GST_MAP_READ) == false) {
            gst_buffer_unmap(buffer, &dataMap);
            fprintf(stderr, "Invalid IV buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        GstMapInfo keyIDMap;
        if (gst_buffer_map(keyID, &keyIDMap, (GstMapFlags) GST_MAP_READ) == false) {
            gst_buffer_unmap(buffer, &dataMap);
            gst_buffer_unmap(IV, &ivMap);
            fprintf(stderr, "Invalid keyID buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        uint8_t *mappedData = reinterpret_cast<uint8_t* >(dataMap.data);
        uint32_t mappedDataSize = static_cast<uint32_t >(dataMap.size);
		 uint8_t *mappedIV = reinterpret_cast<uint8_t* >(ivMap.data);
        uint32_t mappedIVSize = static_cast<uint32_t >(ivMap.size);
        uint8_t *mappedKeyID = reinterpret_cast<uint8_t* >(keyIDMap.data);
        uint32_t mappedKeyIDSize = static_cast<uint32_t >(keyIDMap.size);

        if (subSampleBuffer != nullptr) {
            GstMapInfo sampleMap;

            if (gst_buffer_map(subSampleBuffer, &sampleMap, GST_MAP_READ) == false) {
                fprintf(stderr, "Invalid subsample buffer.\n");
                gst_buffer_unmap(keyID, &keyIDMap);
                gst_buffer_unmap(IV, &ivMap);
                gst_buffer_unmap(buffer, &dataMap);
                return (ERROR_INVALID_DECRYPT_BUFFER);
            }
            uint8_t *mappedSubSample = reinterpret_cast<uint8_t* >(sampleMap.data);
            uint32_t mappedSubSampleSize = static_cast<uint32_t >(sampleMap.size);

            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            uint32_t totalEncrypted = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);
                totalEncrypted += inEncrypted;
            }
            gst_byte_reader_set_pos(reader, 0);

            if(totalEncrypted > 0)
            {
                svp_meta_data_t * ptr = (svp_meta_data_t *) g_malloc(sizeof(svp_meta_data_t));
                if (ptr) {
                    uint32_t chunks_cnt = 0;
                    enc_chunk_data_t * ci = NULL;

                    memset((uint8_t *)ptr, 0, sizeof(svp_meta_data_t));
                    // The next line need to change to assign the opaque handle after calling ->processPayload()
                    ptr->secure_memory_ptr = NULL; //pData;
                    chunks_cnt = subSampleCount;
                    ptr->num_chunks = chunks_cnt;
                    if (chunks_cnt) {
                        ci = (enc_chunk_data_t *)g_malloc(chunks_cnt * sizeof(enc_chunk_data_t));
                        ptr->info = ci;
                    }
                }

                totalEncrypted += sizeof(Rpc_Secbuf_Info); //make sure enough data for metadata

                uint8_t* encryptedData = reinterpret_cast<uint8_t*> (g_malloc(totalEncrypted));
                uint8_t* encryptedDataIter = encryptedData;

                uint32_t index = 0;
                for (unsigned int position = 0; position < subSampleCount; position++) {

                    gst_byte_reader_get_uint16_be(reader, &inClear);
                    gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                    memcpy(encryptedDataIter, mappedData + index + inClear, inEncrypted);
                    index += inClear + inEncrypted;
                    encryptedDataIter += inEncrypted;

                    if (ptr && ptr->num_chunks > 0 && ptr->info) {
                        enc_chunk_data_t * ci = ptr->info;
                        ci[position].clear_data_size = inClear;
                        ci[position].enc_data_size = inEncrypted;
                    }
                }
                gst_byte_reader_set_pos(reader, 0);

                result = opencdm_session_decrypt(session, encryptedData, totalEncrypted, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize, initWithLast15);

                if(ptr && (result == ERROR_NONE)) {
                    memcpy(&sb_info, encryptedData, sizeof(Rpc_Secbuf_Info));
                    if (B_Secbuf_AllocWithToken(sb_info.size, (B_Secbuf_Type)sb_info.type, sb_info.token, (void**)&sb_info.ptr)) {
                        fprintf(stderr, "B_Secbuf_AllocWithToken() failed!\n");
                        fprintf(stderr, "%u subsamples, totalEncrypted: %u, sb_inf: ptr=%p, type=%i, size=%i, token=%p\n", subSampleCount, totalEncrypted, sb_info.ptr, sb_info.type, sb_info.size, sb_info.token);
                    }

                    ptr->secure_memory_ptr = (uintptr_t) sb_info.ptr; //assign the handle here!
                    gst_buffer_append_svp_metadata(buffer, ptr);
                    g_free(ptr->info);
                    g_free(ptr);
                }
                g_free(encryptedData);
            } else {
                // no encrypted data, skip decryption...
                result = ERROR_NONE;
            }

            gst_byte_reader_free(reader);
            gst_buffer_unmap(subSampleBuffer, &sampleMap);
        } else {
            uint8_t* encryptedData = NULL;
            uint8_t* fEncryptedData = NULL;
            uint32_t totalEncryptedSize = 0;

            svp_meta_data_t * ptr = (svp_meta_data_t *) g_malloc(sizeof(svp_meta_data_t));
            if (ptr) {
                enc_chunk_data_t *ci = NULL;
                memset((uint8_t *)ptr, 0, sizeof(svp_meta_data_t));
                // The next line need to change to assign the opaque handle after calling ->processPayload()
                ptr->secure_memory_ptr = NULL; //pData;
                ptr->num_chunks = 1;

                ci = (enc_chunk_data_t *)g_malloc(sizeof(enc_chunk_data_t));
                ptr->info = ci;
                ci[0].clear_data_size = 0;
                ci[0].enc_data_size = mappedDataSize;

                totalEncryptedSize = mappedDataSize + sizeof(Rpc_Secbuf_Info); //make sure it is enough for metadata
                encryptedData = (uint8_t*) g_malloc(totalEncryptedSize);
                fEncryptedData = encryptedData;
                memcpy(encryptedData, mappedData, mappedDataSize);

                result = opencdm_session_decrypt(session, encryptedData, totalEncryptedSize, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize, initWithLast15);

                if(result == ERROR_NONE){
                    memcpy(&sb_info, fEncryptedData, sizeof(Rpc_Secbuf_Info));
                    if (B_Secbuf_AllocWithToken(sb_info.size, (B_Secbuf_Type)sb_info.type, sb_info.token, (void**)&sb_info.ptr)) {
                        fprintf(stderr, "B_Secbuf_AllocWithToken() failed!\n");
                        fprintf(stderr, "no subsamples, encrypted size: %u, sb_inf: ptr=%p, type=%i, size=%i, token=%p\n", totalEncryptedSize, sb_info.ptr, sb_info.type, sb_info.size, sb_info.token);
                    }

                    ptr->secure_memory_ptr = (uintptr_t) sb_info.ptr; //assign the handle here!
                    gst_buffer_append_svp_metadata(buffer, ptr);
                    g_free(ptr->info);
                    g_free(ptr);
                }
                g_free(fEncryptedData);
            }
        }

        gst_buffer_unmap(keyID, &keyIDMap);
        gst_buffer_unmap(IV, &ivMap);
        gst_buffer_unmap(buffer, &dataMap);
    }

    return (result);
}


#include "open_cdm_adapter.h"

#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

#include <b_secbuf.h>
#include <gst_brcm_svp_meta.h>
#include <assert.h>
#include "Trace.h"

struct RPCSecureBufferInformation {
    uint32_t type;
    size_t size;
    void* token;
    void* token_enc;
};

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
            TRACE_L1("adapter_session_decrypt: Secbuf alloc failed!");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        uint8_t *mappedKeyID = nullptr;
        uint32_t mappedKeyIDSize = 0;
        if (keyID != nullptr) {
            if (mappedBuffer(keyID, false, &mappedKeyID, &mappedKeyIDSize) == false) {
                TRACE_L1("Invalid keyID buffer.");
                result = ERROR_INVALID_DECRYPT_BUFFER;
                goto exit;
            }
        }

        B_Secbuf_Info secureBufferInfo;
        void *opaqueData, *opaqueDataEnc;

        uint32_t sizeOfRPCInfo = sizeof(RPCSecureBufferInformation);
        RPCSecureBufferInformation* rpcSecureBufferInformation = reinterpret_cast<RPCSecureBufferInformation*> (g_alloca(sizeOfRPCInfo));

        uint32_t bufferSize = mappedDataSize;

        if (mappedSubSample) {
            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            uint32_t totalEncrypted = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);
                totalEncrypted += inEncrypted;
            }
            gst_byte_reader_free(reader);

            if (totalEncrypted == 0) {
                result = ERROR_NONE;
                goto exit;
            }

            bufferSize = totalEncrypted;
        }

        if(B_Secbuf_Alloc(bufferSize, B_Secbuf_Type_eGeneric, &opaqueDataEnc)) {
            TRACE_L1("adapter_session_decrypt: Secbuf alloc failed!");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }
        B_Secbuf_GetBufferInfo(opaqueDataEnc, &secureBufferInfo);

        rpcSecureBufferInformation->type      = secureBufferInfo.type;
        rpcSecureBufferInformation->size      = secureBufferInfo.size;
        rpcSecureBufferInformation->token_enc = secureBufferInfo.token;
        rpcSecureBufferInformation->token     = NULL;

        brcm_svp_meta_data_t* svpMeta = static_cast<brcm_svp_meta_data_t *>(g_malloc(sizeof(brcm_svp_meta_data_t)));
        memset(svpMeta, 0, sizeof(brcm_svp_meta_data_t));
        svpMeta->sub_type = GST_META_BRCM_SVP_TYPE_2;
        svpMeta->u.u2.chunks_cnt = subSampleCount > 0 ? subSampleCount : 1;
        svpMeta->u.u2.chunk_info = static_cast<svp_chunk_info *>(g_malloc(svpMeta->u.u2.chunks_cnt * sizeof(svp_chunk_info)));

        if (mappedSubSample) {
            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            for (uint32_t indexSec = 0, indexClr = 0, position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                B_Secbuf_ImportData(opaqueDataEnc, indexSec, mappedData + indexClr + inClear, inEncrypted, true);
                svpMeta->u.u2.chunk_info[position].clear_size = inClear;
                svpMeta->u.u2.chunk_info[position].encrypted_size = inEncrypted;
                indexSec += inEncrypted;
                indexClr += inClear + inEncrypted;
            }
            gst_byte_reader_free(reader);
        } else {
            svpMeta->u.u2.chunk_info[0].clear_size = 0;
            svpMeta->u.u2.chunk_info[0].encrypted_size = mappedDataSize;
            B_Secbuf_ImportData(opaqueDataEnc, 0, mappedData, mappedDataSize, true);
        }
        // opaqueDataEnc no need as more. OCDM will acces it via its token
        B_Secbuf_FreeDesc(opaqueDataEnc);

        result = opencdm_session_decrypt(session, reinterpret_cast<uint8_t*>(rpcSecureBufferInformation), sizeOfRPCInfo, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize, initWithLast15);
        if(result != ERROR_NONE) {
            TRACE_L1("adapter_session_decrypt: opencdm_session_decrypt failed!");
            goto exit;
        }

        // OCDM allocate opaqueData for secure decrypted buffer and will be freed in gstreamer
        if(B_Secbuf_AllocWithToken(bufferSize, B_Secbuf_Type_eSecure,  rpcSecureBufferInformation->token, &opaqueData)) {
            TRACE_L1("adapter_session_decrypt: Secbuf Alloc failed!");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        svpMeta->u.u2.secbuf_ptr = reinterpret_cast<uintptr_t>(opaqueData);
        gst_buffer_add_brcm_svp_meta(buffer, svpMeta);
    }
exit:
    return (result);
}

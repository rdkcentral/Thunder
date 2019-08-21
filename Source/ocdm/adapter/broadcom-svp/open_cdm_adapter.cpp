
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
    uint32_t subSamplesCount;
    uint32_t subSamples[]; // Array of clear and encrypted pairs of subsamples.
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

static void addSVPMetaData(GstBuffer* gstBuffer, uint8_t* opaqueData)
{
    brcm_svp_meta_data_t* svpMeta = reinterpret_cast<brcm_svp_meta_data_t*> (g_malloc0(sizeof(brcm_svp_meta_data_t)));
    assert(svpMeta);

    svpMeta->sub_type = GST_META_BRCM_SVP_TYPE_1;
    svpMeta->u.u1.secbuf_ptr = reinterpret_cast<unsigned>(opaqueData);
    gst_buffer_add_brcm_svp_meta(gstBuffer, svpMeta);
}

static void replaceLengthPrefixWithStartcodePrefix(uint8_t* buffer, size_t size)
{
    uint8_t* curr;
    uint8_t* end;
    uint32_t remain, slice_size = 0;

    curr =  buffer;
    end = buffer + size;
    remain = size;

    while (curr < end) {

        slice_size = (*curr) << 24;
        slice_size += (*(curr + 1)) << 16;
        slice_size += (*(curr + 2)) << 8;
        slice_size += (*(curr + 3)) ;

        if ((curr == buffer) && 
                (*curr       == nalUnit[0]) && 
                (*(curr + 1) == nalUnit[1]) && 
                (*(curr + 2) == nalUnit[2]) && 
                (*(curr + 3) == nalUnit[3])) {
            return;
        }

        if (slice_size > remain) {
            return;
        }

        *curr       = nalUnit[0];
        *(curr + 1) = nalUnit[1];
        *(curr + 2) = nalUnit[2];
        *(curr + 3) = nalUnit[3];

        curr   += slice_size + sizeof(nalUnit);
        remain -= slice_size + sizeof(nalUnit);
    }
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
        if (mappedBuffer(keyID, false, &mappedKeyID, &mappedKeyIDSize) == false) {
            TRACE_L1("Invalid keyID buffer.");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        B_Secbuf_Info secureBufferInfo;
        void *opaqueData, *opaqueDataEnc;

        // If there is no subsample, only allocate one region for clear+enc, otherwise, number of subsamples.
        uint32_t rpcSubSampleTotalSize = (subSampleCount ? subSampleCount * 2 : 2) * sizeof(uint32_t);
        uint32_t sizeOfRPCInfo = sizeof(RPCSecureBufferInformation) + rpcSubSampleTotalSize;
        RPCSecureBufferInformation* rpcSecureBufferInformation = reinterpret_cast<RPCSecureBufferInformation*> (g_alloca(sizeOfRPCInfo));

        if(B_Secbuf_Alloc(mappedDataSize, B_Secbuf_Type_eGeneric, &opaqueDataEnc)) {
            TRACE_L1("adapter_session_decrypt: Secbuf alloc failed!");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }
        B_Secbuf_GetBufferInfo(opaqueDataEnc, &secureBufferInfo);

        rpcSecureBufferInformation->type      = secureBufferInfo.type;
        rpcSecureBufferInformation->size      = secureBufferInfo.size;
        rpcSecureBufferInformation->token_enc = secureBufferInfo.token;
        rpcSecureBufferInformation->token     = NULL;

        if (mappedSubSample) {

            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            for (unsigned int position = 0, index = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                rpcSecureBufferInformation->subSamples[2*position] = static_cast<uint32_t>(inClear);
                rpcSecureBufferInformation->subSamples[2*position + 1] = inEncrypted;

                assert( sizeof(nalUnit) < (inClear+inEncrypted));
                // replace length prefiex NALU length into startcode prefix
                replaceLengthPrefixWithStartcodePrefix(mappedData+index, inClear+inEncrypted);
                B_Secbuf_ImportData(opaqueDataEnc, index, mappedData + index, inClear + inEncrypted, true);
                index += inClear + inEncrypted;
            }
            gst_byte_reader_free(reader);

            rpcSecureBufferInformation->subSamplesCount = subSampleCount * 2; // In order of clear+enc+clear+enc...
        } else {

            B_Secbuf_ImportData(opaqueDataEnc, 0, mappedData, mappedDataSize, true);

            rpcSecureBufferInformation->subSamples[0] = 0; // No clear.
            rpcSecureBufferInformation->subSamples[1] = mappedDataSize; // All encrypted.
            rpcSecureBufferInformation->subSamplesCount = 2; // One pair of clear_enc.
        }
        // opaqueDataEnc no need as more. OCDM will acces it via its token
        B_Secbuf_FreeDesc(opaqueDataEnc);

        result = opencdm_session_decrypt(session, reinterpret_cast<uint8_t*>(rpcSecureBufferInformation), sizeOfRPCInfo, mappedIV, mappedIVSize, mappedKeyID, mappedKeyIDSize);
        if(result != ERROR_NONE) {
            TRACE_L1("adapter_session_decrypt: opencdm_session_decrypt failed!");
            goto exit;
        }

        // OCDM allocate opaqueData for secure decrypted buffer and will be freed in gstreamer
        if(B_Secbuf_AllocWithToken(mappedDataSize, B_Secbuf_Type_eSecure,  rpcSecureBufferInformation->token, &opaqueData)) {
            TRACE_L1("adapter_session_decrypt: Secbuf Alloc failed!");
            result = ERROR_INVALID_DECRYPT_BUFFER;
            goto exit;
        }

        addSVPMetaData(buffer, reinterpret_cast<uint8_t*>(opaqueData));
    }
exit:
    return (result);
}

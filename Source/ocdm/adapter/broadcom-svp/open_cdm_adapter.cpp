
#include "open_cdm_adapter.h"

#include <b_secbuf.h>
#include <gst_brcm_svp_meta.h>
#include <assert.h>

struct RPCSecureBufferInformation {
    uint32_t type;
    size_t size;
    void* token;
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
    //assert(svpMeta);

    svpMeta->sub_type = GST_META_BRCM_SVP_TYPE_1;
    svpMeta->u.u1.secbuf_ptr = reinterpret_cast<unsigned>(opaqueData);
    gst_buffer_add_brcm_svp_meta(gstBuffer, svpMeta);
}

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
OpenCDMError adapter_session_decrypt(struct OpenCDMSession * session, void* buffer, void* subSample, const uint32_t subSampleCount, const uint8_t IV[], uint16_t IVLength) {
    OpenCDMError result (ERROR_INVALID_SESSION);

    if (session != nullptr) {

        uint8_t *mappedData = nullptr;
        uint32_t mappedDataSize = 0;
        if (mappedBuffer(reinterpret_cast<GstBuffer*>(buffer), true, &mappedData, &mappedDataSize) == false) {

            printf("Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        uint8_t *mappedSubSample = nullptr;
        uint32_t mappedSubSampleSize = 0;
        if (subSample != nullptr && mappedBuffer(reinterpret_cast<GstBuffer*>(subSample), true, &mappedSubSample, &mappedSubSampleSize) == false) {

            printf("Invalid subsample buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        B_Secbuf_Info secureBufferInfo;
        void* opaqueData;

        // If there is no subsample, only allocate one region for clear+enc, otherwise, number of subsamples.
        uint32_t rpcSubSampleTotalSize = (subSampleCount ? subSampleCount * 2 : 2) * sizeof(uint32_t);
        uint32_t sizeOfRPCInfo = sizeof(RPCSecureBufferInformation) + rpcSubSampleTotalSize;
        RPCSecureBufferInformation* rpcSecureBufferInformation = reinterpret_cast<RPCSecureBufferInformation*> (g_alloca(sizeOfRPCInfo));

        B_Secbuf_Alloc(mappedDataSize, B_Secbuf_Type_eSecure, &opaqueData);
        B_Secbuf_GetBufferInfo(opaqueData, &secureBufferInfo);

        rpcSecureBufferInformation->type = secureBufferInfo.type;
        rpcSecureBufferInformation->size = secureBufferInfo.size;
        rpcSecureBufferInformation->token = secureBufferInfo.token;

        if (mappedSubSample) {

            GstByteReader* reader = gst_byte_reader_new(mappedSubSample, mappedSubSampleSize);
            uint16_t inClear = 0;
            uint32_t inEncrypted = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                rpcSecureBufferInformation->subSamples[2*position] = static_cast<uint32_t>(inClear);
                rpcSecureBufferInformation->subSamples[2*position + 1] = inEncrypted;
            }
            gst_byte_reader_free(reader);

            B_Secbuf_ImportData(opaqueData, 0, const_cast<uint8_t*>(nalUnit), sizeof(nalUnit), 0);
            B_Secbuf_ImportData(opaqueData, sizeof(nalUnit), mappedData + sizeof(nalUnit), mappedDataSize - sizeof(nalUnit), 0);
            B_Secbuf_ImportData(opaqueData, 0, nullptr, 0, 1);

            rpcSecureBufferInformation->subSamplesCount = subSampleCount * 2; // In order of clear+enc+clear+enc...
        } else {

            B_Secbuf_ImportData(opaqueData, 0, mappedData, mappedDataSize, 1);

            rpcSecureBufferInformation->subSamples[0] = 0; // No clear.
            rpcSecureBufferInformation->subSamples[1] = mappedDataSize; // All encrypted.
            rpcSecureBufferInformation->subSamplesCount = 2; // One pair of clear_enc.
        }

        result = opencdm_session_decrypt(session, reinterpret_cast<uint8_t*>(rpcSecureBufferInformation), sizeOfRPCInfo, IV, IVLength);

        addSVPMetaData(static_cast<GstBuffer*>(buffer), reinterpret_cast<uint8_t*>(opaqueData));
    }

    return (result);
}


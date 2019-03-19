
#include "open_cdm_adapter.h"

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
        uint32_t mappedDataSize;
        if (mappedBuffer(reinterpret_cast<GstBuffer*>(buffer), true, &mappedData, &mappedDataSize) == false) {

            printf("Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        uint8_t *mappedSubSample = nullptr;
        uint32_t mappedSubSampleSize;

        if (subSample != nullptr && mappedBuffer(reinterpret_cast<GstBuffer*>(subSample), true, &mappedSubSample, &mappedSubSampleSize) == false) {

            printf("Invalid subsample buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

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

            uint8_t* encryptedData = reinterpret_cast<uint8_t*> (malloc(totalEncrypted));

            uint32_t index = 0;

            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(encryptedData, mappedData + index + inClear, inEncrypted);
                index += inClear + inEncrypted;
                encryptedData += inEncrypted;
            }
            gst_byte_reader_set_pos(reader.get(), 0);

            result = opencdm_session_decrypt(session, encryptedData, totalEncrypted, IV, IVLength);

            // Re-build sub-sample data.
            index = 0;
            unsigned total = 0;
            for (position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(mappedData + total + inClear, encryptedData + index, inEncrypted);
                index += inEncrypted;
                total += inClear + inEncrypted;
            }

            gst_byte_reader_free(reader);
            free(encryptedData);

        } else {

            result = opencdm_session_decrypt(session, mappedData, mappedDataSize, IV, IVLength);
        }
    }

    return (result);
}

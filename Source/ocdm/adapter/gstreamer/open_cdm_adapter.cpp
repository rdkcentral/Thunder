
#include "open_cdm_adapter.h"

#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

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


#include "open_cdm_adapter.h"

#include <gst/gst.h>
#include <gst/base/gstbytereader.h>

OpenCDMError adapter_session_decrypt(struct OpenCDMSession * session, void* buffer, void* subSample, const uint32_t subSampleCount,
                                     const uint8_t IV[], uint16_t IVLength, const uint8_t keyID[], uint16_t keyIDLength) {
    OpenCDMError result (ERROR_INVALID_SESSION);

    if (session != nullptr) {
        GstMapInfo dataMap;
        GstBuffer* dataBuffer = reinterpret_cast<GstBuffer*>(buffer);
        if (gst_buffer_map(dataBuffer, &dataMap, (GstMapFlags) GST_MAP_READWRITE) == false) {
            printf("Invalid buffer.\n");
            return (ERROR_INVALID_DECRYPT_BUFFER);
        }

        uint8_t *mappedData = reinterpret_cast<uint8_t* >(dataMap.data);
        uint32_t mappedDataSize = static_cast<uint32_t >(dataMap.size);
        if (subSample != nullptr) {
            GstMapInfo sampleMap;
            GstBuffer* subSampleBuffer = reinterpret_cast<GstBuffer*>(subSample);
            if (gst_buffer_map(subSampleBuffer, &sampleMap, GST_MAP_READ) == false) {
                printf("Invalid subsample buffer.\n");
                gst_buffer_unmap(dataBuffer, &dataMap);
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

            uint8_t* encryptedData = reinterpret_cast<uint8_t*> (malloc(totalEncrypted));
            uint8_t* encryptedDataIter = encryptedData;

            uint32_t index = 0;
            for (unsigned int position = 0; position < subSampleCount; position++) {

                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(encryptedDataIter, mappedData + index + inClear, inEncrypted);
                index += inClear + inEncrypted;
                encryptedDataIter += inEncrypted;
            }
            gst_byte_reader_set_pos(reader, 0);

            result = opencdm_session_decrypt(session, encryptedData, totalEncrypted, IV, IVLength, keyID, keyIDLength);
            // Re-build sub-sample data.
            index = 0;
            unsigned total = 0;
            for (uint32_t position = 0; position < subSampleCount; position++) {
                gst_byte_reader_get_uint16_be(reader, &inClear);
                gst_byte_reader_get_uint32_be(reader, &inEncrypted);

                memcpy(mappedData + total + inClear, encryptedData + index, inEncrypted);
                index += inEncrypted;
                total += inClear + inEncrypted;
            }

            gst_byte_reader_free(reader);
            free(encryptedData);
            gst_buffer_unmap(subSampleBuffer, &sampleMap);
        } else {
            result = opencdm_session_decrypt(session, mappedData, mappedDataSize, IV, IVLength, keyID, keyIDLength);
        }

        gst_buffer_unmap(dataBuffer, &dataMap);
    }

    return (result);
}

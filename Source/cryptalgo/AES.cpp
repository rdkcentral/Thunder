 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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
 
#include "AES.h"

namespace Thunder {
namespace Crypto {

    AESEncryption::AESEncryption(const aesType type)
        : _type(type)
        , _context()
        , _offset(0)
    {
        ::memset(_iv, 0, sizeof(_iv));
    }

    AESEncryption::~AESEncryption()
    {
    }

    uint32_t AESEncryption::Key(const uint8_t length, const uint8_t key[])
    {
        ASSERT((length == 16 /* 128 bits */) || (length == 24 /* 192 bits */) || (length == 32 /* 256 bits */));
        mbedtls_aes_init(&_context);
        return (mbedtls_aes_setkey_enc(&_context, key, (length << 3)));
    }

    uint32_t AESEncryption::Encrypt(const uint32_t length, const uint8_t input[], uint8_t output[])
    {

        uint32_t result = Core::ERROR_UNAVAILABLE;

        switch (Type()) {
        case AES_ECB: {
            uint32_t offset = 0;
            uint32_t blockSize = ((length / 16) * 16);

            while (offset < blockSize) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_ecb(&_context, MBEDTLS_AES_ENCRYPT, &(input[offset]), &(output[offset]));
                offset += 16;
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                uint8_t results[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_ecb(&_context, MBEDTLS_AES_ENCRYPT, nextBlock, results);
                memcpy(&output[length - blockSize], &results, blockSize);
            }
            break;
        }

#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case AES_CBC: {
            uint32_t blockSize = ((length / 16) * 16);

            if (blockSize > 0) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cbc(&_context, MBEDTLS_AES_ENCRYPT, blockSize, _iv, input, output);
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                uint8_t results[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_cbc(&_context, MBEDTLS_AES_ENCRYPT, sizeof(nextBlock), _iv, nextBlock, results);
                memcpy(&output[length - blockSize], &results, blockSize);
            }
            break;
        } break;
#endif

#if defined(MBEDTLS_CIPHER_MODE_CFB)
        case AES_CFB8: {
            uint32_t blockSize = ((length / 16) * 16);

            if (blockSize > 0) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cfb8(&_context, MBEDTLS_AES_ENCRYPT, blockSize, _iv, input, output);
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_cfb8(&_context, MBEDTLS_AES_ENCRYPT, sizeof(nextBlock), _iv, nextBlock, nextBlock);
                memcpy(&output[length - blockSize], &nextBlock, blockSize);
            }
            break;
        }
        case AES_CFB128: {
            uint32_t delta = (16 - _offset) % 16;

            if (((delta <= length) && ((_offset + length) >= 16))) {

                uint32_t blockSize = static_cast<uint32_t>(((length - _offset) / 16) * 16) + delta;
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cfb128(&_context, MBEDTLS_AES_ENCRYPT, length, &_offset, _iv, input, output);
                delta = length - blockSize;
            } else {
                delta = length;
            }

            if (delta > 0) {

                ASSERT(delta < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - delta], delta);
                ::memset(&nextBlock[delta], 0, 16 - delta);
                _offset = 0;
                result = mbedtls_aes_crypt_cfb128(&_context, MBEDTLS_AES_ENCRYPT, sizeof(nextBlock), &_offset, _iv, nextBlock, nextBlock);
                _offset = delta;
                memcpy(&output[length - delta], &nextBlock, delta);
            }
            break;
        }
#endif
#if defined(MBEDTLS_CIPHER_MODE_OFB)
        case AES_OFB: {
            uint32_t delta = (16 - _offset) % 16;

            if ((delta <= length) && ((_offset + length) >= 16)) {

                uint32_t blockSize = static_cast<uint32_t>(((length - _offset) / 16) * 16) + delta;
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_ofb(&_context, blockSize, &_offset, _iv, input, output);
                delta = length - blockSize;
            } else {
                delta = length;
            }

            if (delta > 0) {

                ASSERT(delta < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - delta], delta);
                ::memset(&nextBlock[delta], 0, 16 - delta);
                _offset = 0;
                result = mbedtls_aes_crypt_ofb(&_context, sizeof(nextBlock), &_offset, _iv, nextBlock, nextBlock);
                _offset = delta;
                memcpy(&output[length - delta], &nextBlock, delta);
            }
            break;
        }
            result = mbedtls_aes_crypt_ofb(&_context, length, &_offset, _iv, input, output);
            break;
#endif
        default:
            ASSERT(false);
            break;
        }

        return (result);
    }

    AESDecryption::AESDecryption(const aesType type)
        : _type(type)
        , _context()
        , _offset(0)
    {
        ::memset(_iv, 0, sizeof(_iv));
    }

    AESDecryption::~AESDecryption()
    {
    }

    uint32_t AESDecryption::Key(const uint8_t length, const uint8_t key[])
    {

        ASSERT((length == 16 /* 128 bits */) || (length == 24 /* 192 bits */) || (length == 32 /* 256 bits */));

        mbedtls_aes_init(&_context);
        if ((Type() == AES_CBC) || (Type() == AES_ECB)) {
            // TODO: Check if for ECB encryption we also need a decryption ckey or if we
            // should ude the encryption key. Not sure !!!!!
            return (mbedtls_aes_setkey_dec(&_context, key, length << 3));
        }
        return (mbedtls_aes_setkey_enc(&_context, key, (length << 3)));
    }

    uint32_t AESDecryption::Decrypt(const uint32_t length, const uint8_t input[], uint8_t output[])
    {
        uint32_t result = Core::ERROR_UNAVAILABLE;

        switch (Type()) {
        case AES_ECB: {
            uint32_t offset = 0;
            uint32_t blockSize = ((length / 16) * 16);

            while (offset < blockSize) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_ecb(&_context, MBEDTLS_AES_DECRYPT, &(input[offset]), &(output[offset]));
                offset += 16;
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                uint8_t results[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_ecb(&_context, MBEDTLS_AES_DECRYPT, nextBlock, results);
                memcpy(&output[length - blockSize], &results, blockSize);
            }
            break;
        }

#if defined(MBEDTLS_CIPHER_MODE_CBC)
        case AES_CBC: {
            uint32_t blockSize = ((length / 16) * 16);

            if (blockSize > 0) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cbc(&_context, MBEDTLS_AES_DECRYPT, blockSize, _iv, input, output);
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                uint8_t results[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_cbc(&_context, MBEDTLS_AES_DECRYPT, sizeof(nextBlock), _iv, nextBlock, results);
                memcpy(&output[length - blockSize], &results, blockSize);
            }
            break;
        }
#endif

#if defined(MBEDTLS_CIPHER_MODE_CFB)
        case AES_CFB8: {
            uint32_t blockSize = ((length / 16) * 16);

            if (blockSize > 0) {
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cfb8(&_context, MBEDTLS_AES_DECRYPT, blockSize, _iv, input, output);
            }

            if (blockSize < length) {

                blockSize = (length - blockSize);
                ASSERT(blockSize < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - blockSize], blockSize);
                ::memset(&nextBlock[blockSize], 0, 16 - blockSize);
                result = mbedtls_aes_crypt_cfb8(&_context, MBEDTLS_AES_DECRYPT, sizeof(nextBlock), _iv, nextBlock, nextBlock);
                memcpy(&output[length - blockSize], &nextBlock, blockSize);
            }
            break;
        }
        case AES_CFB128: {
            uint32_t delta = (16 - _offset) % 16;

            if ((delta <= length) && ((_offset + length) >= 16)) {

                uint32_t blockSize = static_cast<uint32_t>(((length - _offset) / 16) * 16) + delta;
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_cfb128(&_context, MBEDTLS_AES_DECRYPT, length, &_offset, _iv, input, output);
                delta = length - blockSize;
            } else {
                delta = length;
            }

            if (delta > 0) {

                ASSERT(delta < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - delta], delta);
                ::memset(&nextBlock[delta], 0, 16 - delta);
                _offset = 0;
                result = mbedtls_aes_crypt_cfb128(&_context, MBEDTLS_AES_DECRYPT, sizeof(nextBlock), &_offset, _iv, nextBlock, nextBlock);
                _offset = delta;
                memcpy(&output[length - delta], &nextBlock, delta);
            }
            break;
        }
#endif
#if defined(MBEDTLS_CIPHER_MODE_OFB)
        case AES_OFB: {
            uint32_t delta = (16 - _offset) % 16;

            if ((delta <= length) && ((_offset + length) >= 16)) {

                uint32_t blockSize = static_cast<uint32_t>(((length - _offset) / 16) * 16) + delta;
                // First encrypt the whole blocks. No padding needed, yet
                result = mbedtls_aes_crypt_ofb(&_context, blockSize, &_offset, _iv, input, output);
                delta = length - blockSize;
            } else {
                delta = length;
            }

            if (delta > 0) {

                ASSERT(delta < 16);
                uint8_t nextBlock[16];
                ::memcpy(nextBlock, &input[length - delta], delta);
                ::memset(&nextBlock[delta], 0, 16 - delta);
                _offset = 0;
                result = mbedtls_aes_crypt_ofb(&_context, sizeof(nextBlock), &_offset, _iv, nextBlock, nextBlock);
                _offset = delta;
                memcpy(&output[length - delta], &nextBlock, delta);
            }
            break;
        }
#endif
        default:
            ASSERT(false);
            break;
        }

        return (result);
    }
}
} // namespace Thunder::Crypto

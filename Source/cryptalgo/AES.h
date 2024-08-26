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

#ifndef __AES_H
#define __AES_H

#include "AESImplementation.h"
#include "Module.h"

namespace Thunder {
namespace Crypto {

    enum aesType : uint8_t {
        AES_ECB,
        AES_CBC,
        AES_CFB8,
        AES_CFB128,
        AES_OFB
    };

    enum bitLength : uint16_t {
        BITLENGTH_128 = 128,
        BITLENGTH_192 = 192,
        BITLENGTH_256 = 256
    };

    class EXTERNAL AESEncryption {
    private:
        AESEncryption() = delete;
        AESEncryption(const AESEncryption&) = delete;
        AESEncryption& operator=(const AESEncryption&) = delete;

    public:
        AESEncryption(const aesType type);
        ~AESEncryption();

    public:
        inline aesType Type() const
        {
            return (_type);
        }
        inline const uint8_t* InitialVector() const
        {
            return (_iv);
        }
        inline void InitialVector(const uint8_t iv[16])
        {
            _offset = 0;
            ::memcpy(_iv, iv, sizeof(_iv));
        }
        uint32_t Key(const uint8_t length, const uint8_t key[]);

        uint32_t Encrypt(const uint32_t length, const uint8_t input[], uint8_t output[]);

    private:
        aesType _type;
        mbedtls_aes_context _context;
        uint8_t _iv[16];
        size_t _offset;
    };

    class EXTERNAL AESDecryption {
    private:
        AESDecryption() = delete;
        AESDecryption(const AESDecryption&) = delete;
        AESDecryption& operator=(const AESDecryption&) = delete;

    public:
        AESDecryption(const aesType type);
        ~AESDecryption();

    public:
        inline aesType Type() const
        {
            return (_type);
        }
        inline const uint8_t* InitialVector() const
        {
            return (_iv);
        }
        inline void InitialVector(const uint8_t iv[16])
        {
            _offset = 0;
            if (_iv != iv) {
                ::memcpy(_iv, iv, sizeof(_iv));
            }
        }
        uint32_t Key(const uint8_t length, const uint8_t key[]);

        uint32_t Decrypt(const uint32_t length, const uint8_t input[], uint8_t output[]);

    private:
        aesType _type;
        mbedtls_aes_context _context;
        uint8_t _iv[16];
        size_t _offset;
    };
}
} // namespace Crypto

#endif // __AES_H

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

#include "Random.h"

#ifdef __LINUX__
#include <arpa/inet.h>
#endif // __LINUX__

#ifdef __WINDOWS__
#include "Winsock2.h"
#endif // __WINDOWS__

#if defined(SECURESOCKETS_ENABLED)
#include <openssl/rand.h>
#endif // SECURESOCKETS_ENABLED

namespace WPEFramework {
namespace Crypto {
    // --------------------------------------------------------------------------------------------
    // RANDOM functionality
    // --------------------------------------------------------------------------------------------
    void Reseed()
    {
#ifdef __LINUX__
#if defined(SECURESOCKETS_ENABLED)
        if (RAND_poll() == 1) {
            return;
        }
#endif // SECURESOCKETS_ENABLED
        // RAND_poll() failed, fallback to legacy random.
        srandom(static_cast<unsigned int>(time(nullptr)));
#endif // __LINUX__
#ifdef __WINDOWS__
        srand(static_cast<unsigned int>(time(nullptr)));
#endif // __WINDOWS__

    }

    void Random(uint8_t& value)
    {
#if defined(SECURESOCKETS_ENABLED)
#ifdef __LINUX__
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) == 1) {
            return;
        }
        // RAND_bytes() failed, fallback to legacy random.
#endif // __LINUX__
#endif // SECURESOCKETS_ENABLED
#if RAND_MAX >= 0xFF
#ifdef __WINDOWS__
        value = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        value = static_cast<uint8_t>(random() & 0xFF);
#endif // __LINUX__
#else
#error "Can not create random functionality"
#endif
    }

    void Random(uint16_t& value)
    {
#if defined(SECURESOCKETS_ENABLED)
#ifdef __LINUX__
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) == 1) {
            return;
        }
        // RAND_bytes() failed, fallback to legacy random.
#endif // __LINUX__
#endif // SECURESOCKETS_ENABLED
#if RAND_MAX >= 0xFFFF
#ifdef __WINDOWS__
        value = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        value = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFF
#ifdef __WINDOWS__
        uint8_t hsb = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        uint8_t hsb = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb = static_cast<uint8_t>(random() & 0xFF);
#endif // __LINUX__
        value = (value << 8) | lsb;
#else
#error "Can not create random functionality"
#endif
    }

    void Random(uint32_t& value)
    {
#if defined(SECURESOCKETS_ENABLED)
#ifdef __LINUX__
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) == 1) {
            return;
        }
        // RAND_bytes() failed, fallback to legacy random.
#endif // __LINUX__
#endif // SECURESOCKETS_ENABLED
#if RAND_MAX >= 0xFFFFFFFF
#ifdef __WINDOWS__
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFFFF
#ifdef __WINDOWS__
        uint16_t hsw = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        uint16_t hsw = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
        value = (hsw << 16) | lsw;
#elif RAND_MAX >= 0xFF
#ifdef __WINDOWS__
        uint8_t hsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        uint8_t hsb1 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(random() & 0xFF);
#endif // __LINUX__
        value = (hsb1 << 24) | (lsb1 < 16) | (hsb2 << 8) | lsb2;
#else
#error "Can not create random functionality"
#endif
    }

    void Random(uint64_t& value)
    {
#if defined(SECURESOCKETS_ENABLED)
#ifdef __LINUX__
        if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) == 1) {
            return;
        }
        // RAND_bytes() failed, fallback to legacy random.
#endif // __LINUX__
#endif // SECURESOCKETS_ENABLED
#if RAND_MAX >= 0xFFFFFFFF
#ifdef __WINDOWS__
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFFFF
#ifdef __WINDOWS__
        uint16_t hsw1 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw1 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t hsw2 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw2 = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        uint16_t hsw1 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw1 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t hsw2 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw2 = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
        value = (hsw1 << 16) | lsw1;
        value = (value << 32) | (hsw2 << 16) | lsw2;
#elif RAND_MAX >= 0xFF
#ifdef __WINDOWS__
        uint8_t hsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb3 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb3 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb4 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb4 = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WINDOWS__
#ifdef __LINUX__
        uint8_t hsb1 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t hsb3 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb3 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t hsb4 = static_cast<uint8_t>(random() & 0xFF);
        uint8_t lsb4 = static_cast<uint8_t>(random() & 0xFF);
#endif // __LINUX__
        value = (hsb1 << 24) | (lsb1 < 16) | (hsb2 << 8) | lsb2;
        value = (value << 32) | (hsb3 << 24) | (lsb3 < 16) | (hsb4 << 8) | lsb4;
#else
#error "Can not create random functionality"
#endif
    }
}
} // namespace WPEFramework::Crypto

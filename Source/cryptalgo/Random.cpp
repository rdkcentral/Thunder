#include "Random.h"

#ifdef __LINUX__
#include <arpa/inet.h>
#endif // __LINUX__

#ifdef __WIN32__
#include "Winsock2.h"
#endif // __WIN32__

namespace WPEFramework {
namespace Crypto {
    // --------------------------------------------------------------------------------------------
    // RANDOM functionality
    // --------------------------------------------------------------------------------------------
    void Reseed()
    {
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    void Random(uint8_t& value)
    {
#if RAND_MAX >= 0xFF
        srand(static_cast<unsigned int>(time(nullptr)));

#ifdef __WIN32__
        value = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WIN32__
#ifdef __LINUX__
        value = static_cast<uint8_t>(random() & 0xFF);
#endif // __LINUX__
#else
#error "Can not create random functionality"
#endif
    }

    void Random(uint16_t& value)
    {
#if RAND_MAX >= 0xFFFF
#ifdef __WIN32__
        value = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WIN32__
#ifdef __LINUX__
        value = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFF
#ifdef __WIN32__
        uint8_t hsb = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WIN32__
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
#if RAND_MAX >= 0xFFFFFFFF
#ifdef __WIN32__
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
#endif // __WIN32__
#ifdef __LINUX__
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFFFF
#ifdef __WIN32__
        uint16_t hsw = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WIN32__
#ifdef __LINUX__
        uint16_t hsw = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
        value = (hsw << 16) | lsw;
#elif RAND_MAX >= 0xFF
#ifdef __WIN32__
        uint8_t hsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WIN32__
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
#if RAND_MAX >= 0xFFFFFFFF
#ifdef __WIN32__
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
        value = static_cast<uint32_t>(rand() & 0xFFFFFFFF);
#endif // __WIN32__
#ifdef __LINUX__
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
        value = static_cast<uint32_t>(random() & 0xFFFFFFFF);
#endif // __LINUX__
#elif RAND_MAX >= 0xFFFF
#ifdef __WIN32__
        uint16_t hsw1 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw1 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t hsw2 = static_cast<uint16_t>(rand() & 0xFFFF);
        uint16_t lsw2 = static_cast<uint16_t>(rand() & 0xFFFF);
#endif // __WIN32__
#ifdef __LINUX__
        uint16_t hsw1 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw1 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t hsw2 = static_cast<uint16_t>(random() & 0xFFFF);
        uint16_t lsw2 = static_cast<uint16_t>(random() & 0xFFFF);
#endif // __LINUX__
        value = (hsw1 << 16) | lsw1;
        value = (value << 32) | (hsw2 << 16) | lsw2;
#elif RAND_MAX >= 0xFF
#ifdef __WIN32__
        uint8_t hsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb1 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb2 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb3 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb3 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t hsb4 = static_cast<uint8_t>(rand() & 0xFF);
        uint8_t lsb4 = static_cast<uint8_t>(rand() & 0xFF);
#endif // __WIN32__
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

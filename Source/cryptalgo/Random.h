#ifndef __RANDOM_H
#define __RANDOM_H

// ---- Include system wide include files ----
#include <time.h>

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Crypto {
    extern EXTERNAL void Reseed();
    extern EXTERNAL void Random(uint8_t& value);
    extern EXTERNAL void Random(uint16_t& value);
    extern EXTERNAL void Random(uint32_t& value);
    extern EXTERNAL void Random(uint64_t& value);

    inline void Random(int8_t& value)
    {
        Random(reinterpret_cast<uint8_t&>(value));
    }
    inline void Random(int16_t& value)
    {
        Random(reinterpret_cast<uint16_t&>(value));
    }
    inline void Random(int32_t& value)
    {
        Random(reinterpret_cast<uint32_t&>(value));
    }
    inline void Random(int64_t& value)
    {
        Random(reinterpret_cast<uint64_t&>(value));
    }

    template <typename DESTTYPE>
    void Random(DESTTYPE& value, const DESTTYPE minimum, const DESTTYPE maximum)
    {
        Crypto::Random(value);

        value = ((value % (maximum - minimum)) + minimum);
    }
}
}

#endif // __RANDOM_H

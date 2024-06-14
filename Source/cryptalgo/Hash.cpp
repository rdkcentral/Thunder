/*
        * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
        * MD5 Message-Digest Algorithm (RFC 1321).
        *
        * Homepage:
        * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
        *
        * Author:
        * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
        *
        * This software was written by Alexander Peslyak in 2001.  No copyright is
        * claimed, and the software is hereby placed in the public domain.
        * In case this attempt to disclaim copyright and place the software in the
        * public domain is deemed null and void, then the software is
        * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
        * general public under the following terms:
        *
        * Redistribution and use in source and binary forms, with or without
        * modification, are permitted.
        *
        * There's ABSOLUTELY NO WARRANTY, express or implied.
        *
        * (This is a heavily cut-down "BSD license".)
        *
        * This differs from Colin Plumb's older public domain implementation in that
        * no exactly 32-bit integer data type is required (any 32-bit or wider
        * unsigned integer data type will do), there's no compile-time endianness
        * configuration, and the function prototypes match OpenSSL's.  No code from
        * Colin Plumb's implementation has been reused; this comment merely compares
        * the properties of the two independent implementations.
        *
        * The primary goals of this implementation are portability and ease of use.
        * It is meant to be fast, but not as fast as possible.  Some known
        * optimizations are not included to reduce source code size and avoid
        * compile-time configuration.
        */

/*
        * The SHA-256 algorith is updated to handle upto 32 bit size data
        * Reference: https://github.com/B-Con/crypto-algorithms
        *
        * Author:
        * Brad Conte (brad AT bradconte.com)
        *
        * This code is released into the public domain free of any restrictions.
        * The author requests acknowledgement if the code is used, but does not require it.
        * This code is provided free of any liability and without any quality claims by the author.
        *
        */

#include "Hash.h"

#ifdef __LINUX__
#include <arpa/inet.h>
#endif // __LINUX__

#ifdef __WINDOWS__
#include "Winsock2.h"
#endif // __WINDOWS__

// --------------------------------------------------------------------------------------------
// MD5 functionality
// --------------------------------------------------------------------------------------------
/*
 * The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z) ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z) ((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z) (((x) ^ (y)) ^ (z))
#define H2(x, y, z) ((x) ^ ((y) ^ (z)))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

/*
 * The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s)                         \
    (a) += f((b), (c), (d)) + (x) + (t);                     \
    (a) = (((a) << (s)) | (((a)&0xffffffff) >> (32 - (s)))); \
    (a) += (b);

/*
 * SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define SET(n) \
    (*(uint32_t*)&ptr[(n)*4])
#define GET(n) \
    SET(n)
#else
#define SET(n) \
    (ctx->block[(n)] = (uint32_t)ptr[(n)*4] | ((uint32_t)ptr[(n)*4 + 1] << 8) | ((uint32_t)ptr[(n)*4 + 2] << 16) | ((uint32_t)ptr[(n)*4 + 3] << 24))
#define GET(n) \
    (ctx->block[(n)])
#endif

// --------------------------------------------------------------------------------------------
// SHA224/SHA256/AHS384/SHA512 functionality
// --------------------------------------------------------------------------------------------
#if 0
#define UNROLL_LOOPS /* Enable loops unrolling */
#endif

#define SHA224_DIGEST_SIZE (224 / 8)
#define SHA256_DIGEST_SIZE (256 / 8)
#define SHA384_DIGEST_SIZE (384 / 8)
#define SHA512_DIGEST_SIZE (512 / 8)

#define SHA256_BLOCK_SIZE (512 / 8)
#define SHA512_BLOCK_SIZE (1024 / 8)
#define SHA384_BLOCK_SIZE SHA512_BLOCK_SIZE
#define SHA224_BLOCK_SIZE SHA256_BLOCK_SIZE

#define SHFR(x, n) (x >> n)
#define ROTR(x, n) ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define ROTL(x, n) ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define CH(x, y, z) ((x & y) ^ (~x & z))
#define MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))

#define SHA256_F1(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SHA256_F2(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SHA256_F3(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHFR(x, 3))
#define SHA256_F4(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHFR(x, 10))

#define SHA512_F1(x) (ROTR(x, 28) ^ ROTR(x, 34) ^ ROTR(x, 39))
#define SHA512_F2(x) (ROTR(x, 14) ^ ROTR(x, 18) ^ ROTR(x, 41))
#define SHA512_F3(x) (ROTR(x, 1) ^ ROTR(x, 8) ^ SHFR(x, 7))
#define SHA512_F4(x) (ROTR(x, 19) ^ ROTR(x, 61) ^ SHFR(x, 6))

#define UNPACK32(x, str)                     \
    {                                        \
        *((str) + 3) = (uint8_t)((x));       \
        *((str) + 2) = (uint8_t)((x) >> 8);  \
        *((str) + 1) = (uint8_t)((x) >> 16); \
        *((str) + 0) = (uint8_t)((x) >> 24); \
    }

#define PACK32(str, x)                          \
    {                                           \
        *(x) = ((uint32_t) * ((str) + 3))       \
            | ((uint32_t) * ((str) + 2) << 8)   \
            | ((uint32_t) * ((str) + 1) << 16)  \
            | ((uint32_t) * ((str) + 0) << 24); \
    }

#define UNPACK64(x, str)                     \
    {                                        \
        *((str) + 7) = (uint8_t)((x));       \
        *((str) + 6) = (uint8_t)((x) >> 8);  \
        *((str) + 5) = (uint8_t)((x) >> 16); \
        *((str) + 4) = (uint8_t)((x) >> 24); \
        *((str) + 3) = (uint8_t)((x) >> 32); \
        *((str) + 2) = (uint8_t)((x) >> 40); \
        *((str) + 1) = (uint8_t)((x) >> 48); \
        *((str) + 0) = (uint8_t)((x) >> 56); \
    }

#define PACK64(str, x)                          \
    {                                           \
        *(x) = ((uint64_t) * ((str) + 7))       \
            | ((uint64_t) * ((str) + 6) << 8)   \
            | ((uint64_t) * ((str) + 5) << 16)  \
            | ((uint64_t) * ((str) + 4) << 24)  \
            | ((uint64_t) * ((str) + 3) << 32)  \
            | ((uint64_t) * ((str) + 2) << 40)  \
            | ((uint64_t) * ((str) + 1) << 48)  \
            | ((uint64_t) * ((str) + 0) << 56); \
    }

/* Macros used for loops unrolling */

#define SHA256_SCR(i)                           \
    {                                           \
        w[i] = SHA256_F4(w[i - 2]) + w[i - 7]   \
            + SHA256_F3(w[i - 15]) + w[i - 16]; \
    }

#define SHA512_SCR(i)                           \
    {                                           \
        w[i] = SHA512_F4(w[i - 2]) + w[i - 7]   \
            + SHA512_F3(w[i - 15]) + w[i - 16]; \
    }

#define SHA256_EXP(a, b, c, d, e, f, g, h, j)                   \
    {                                                           \
        t1 = wv[h] + SHA256_F2(wv[e]) + CH(wv[e], wv[f], wv[g]) \
            + sha256_k[j] + w[j];                               \
        t2 = SHA256_F1(wv[a]) + MAJ(wv[a], wv[b], wv[c]);       \
        wv[d] += t1;                                            \
        wv[h] = t1 + t2;                                        \
    }

#define SHA512_EXP(a, b, c, d, e, f, g, h, j)                   \
    {                                                           \
        t1 = wv[h] + SHA512_F2(wv[e]) + CH(wv[e], wv[f], wv[g]) \
            + sha512_k[j] + w[j];                               \
        t2 = SHA512_F1(wv[a]) + MAJ(wv[a], wv[b], wv[c]);       \
        wv[d] += t1;                                            \
        wv[h] = t1 + t2;                                        \
    }

namespace Thunder {
namespace Crypto {
    // --------------------------------------------------------------------------------------------
    // MD5 functionality
    // --------------------------------------------------------------------------------------------
    /*
     * This processes one or more 64-byte data blocks, but does NOT update
     * the bit counters.  There are no alignment requirements.
     */
    static const void* body(MD5::Context* ctx, const void* data, unsigned long size)
    {
        const unsigned char* ptr;
        uint32_t a, b, c, d;
        uint32_t saved_a, saved_b, saved_c, saved_d;

        ptr = (const unsigned char*)data;

        a = static_cast<uint32_t>(ctx->h[0]);
        b = static_cast<uint32_t>(ctx->h[1]);
        c = static_cast<uint32_t>(ctx->h[2]);
        d = static_cast<uint32_t>(ctx->h[3]);

        do {
            saved_a = a;
            saved_b = b;
            saved_c = c;
            saved_d = d;

            /* Round 1 */
            STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
            STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
            STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
            STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
            STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
            STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
            STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
            STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
            STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
            STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
            STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
            STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
            STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
            STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
            STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
            STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

            /* Round 2 */
            STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
            STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
            STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
            STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
            STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
            STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
            STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
            STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
            STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
            STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
            STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
            STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
            STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
            STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
            STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
            STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

            /* Round 3 */
            STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
            STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11)
            STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
            STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23)
            STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
            STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11)
            STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
            STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23)
            STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
            STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11)
            STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
            STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23)
            STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
            STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11)
            STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
            STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23)

            /* Round 4 */
            STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
            STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
            STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
            STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
            STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
            STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
            STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
            STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
            STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
            STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
            STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
            STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
            STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
            STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
            STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
            STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

            a += saved_a;
            b += saved_b;
            c += saved_c;
            d += saved_d;

            ptr += 64;
        } while (size -= 64);

        ctx->h[0] = a;
        ctx->h[1] = b;
        ctx->h[2] = c;
        ctx->h[3] = d;

        return ptr;
    }

    static void MD5_Update(MD5::Context* ctx, const void* data, unsigned long size)
    {
        uint32_t saved_lo;
        unsigned long used, available;

        /*
         * Split lenght to lower and higher portion
         */
        uint32_t lengthLow = static_cast<uint32_t>(ctx->length & 0xFFFFFFFF);
        uint32_t lengthHigh = reinterpret_cast<uint32_t*>(&ctx->length)[1];

        saved_lo = lengthLow;
        if ((lengthLow = (saved_lo + size) & 0x1fffffff) < saved_lo) {
            lengthHigh++;
        }
        lengthHigh += size >> 29;

        /*
         * Combine lower and higher portion of length to single field
         */
        ctx->length = (static_cast<uint64_t>(lengthHigh) << 32 | lengthLow);

        used = saved_lo & 0x3f;
        if (used) {
            available = 64 - used;

            if (size < available) {
                ::memcpy(&ctx->block[used], data, size);
                return;
            }

            ::memcpy(&ctx->block[used], data, available);
            data = (const unsigned char*)data + available;
            size -= available;
            body(ctx, ctx->block, 64);
        }

        if (size >= 64) {
            data = body(ctx, data, size & ~(unsigned long)0x3f);
            size &= 0x3f;
        }

        ::memcpy(ctx->block, data, size);
    }

    void MD5::CloseContext()
    {
        unsigned long used, available;
        /*
         * Split lenght to lower and higher portion
         */
        uint32_t lengthLow = static_cast<uint32_t>(_context.length & 0xFFFFFFFF);
        uint32_t lengthHigh = reinterpret_cast<uint32_t*>(&_context.length)[1];

        used = lengthLow & 0x3f;

        _context.block[used++] = 0x80;

        available = 64 - used;

        if (available < 8) {
            memset(&_context.block[used], 0, available);
            body(&_context, _context.block, 64);
            used = 0;
            available = 64;
        }

        memset(&_context.block[used], 0, available - 8);

        lengthLow <<= 3;
        _context.block[56] = lengthLow;
        _context.block[57] = lengthLow >> 8;
        _context.block[58] = lengthLow >> 16;
        _context.block[59] = lengthLow >> 24;
        _context.block[60] = lengthHigh;
        _context.block[61] = lengthHigh >> 8;
        _context.block[62] = lengthHigh >> 16;
        _context.block[63] = lengthHigh >> 24;

        body(&_context, _context.block, 64);

        _context.block[0] = static_cast<uint32_t>(_context.h[0]);
        _context.block[1] = static_cast<uint32_t>(_context.h[0]) >> 8;
        _context.block[2] = static_cast<uint32_t>(_context.h[0]) >> 16;
        _context.block[3] = static_cast<uint32_t>(_context.h[0]) >> 24;
        _context.block[4] = static_cast<uint32_t>(_context.h[1]);
        _context.block[5] = static_cast<uint32_t>(_context.h[1]) >> 8;
        _context.block[6] = static_cast<uint32_t>(_context.h[1]) >> 16;
        _context.block[7] = static_cast<uint32_t>(_context.h[1]) >> 24;
        _context.block[8] = static_cast<uint32_t>(_context.h[2]);
        _context.block[9] = static_cast<uint32_t>(_context.h[2]) >> 8;
        _context.block[10] = static_cast<uint32_t>(_context.h[2]) >> 16;
        _context.block[11] = static_cast<uint32_t>(_context.h[2]) >> 24;
        _context.block[12] = static_cast<uint32_t>(_context.h[3]);
        _context.block[13] = static_cast<uint32_t>(_context.h[3]) >> 8;
        _context.block[14] = static_cast<uint32_t>(_context.h[3]) >> 16;
        _context.block[15] = static_cast<uint32_t>(_context.h[3]) >> 24;

        /*
         * Combine lower and higher portion of length to single field
         */
        _context.length = (static_cast<uint64_t>(lengthHigh) << 32 | lengthLow);
    }

    void MD5::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t sizeToHandle = length;
        const uint8_t* source = &message_array[0];

        while (sizeToHandle > 0) {
            if (sizeToHandle > 512) {
                MD5_Update(&_context, source, 512);
                source += 512;
                sizeToHandle -= 512;
            } else {
                MD5_Update(&_context, source, sizeToHandle);
                sizeToHandle = 0;
            }
        }
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the MD5 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    MD5& MD5::operator<<(const uint8_t message_array[])
    {
        uint16_t length = 0;

        while (message_array[length] != '\0') {
            length++;
        }

        Input(message_array, length);

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    MD5& MD5::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    // SHA1 functionality
    // --------------------------------------------------------------------------------------------
    /*
     *  Input
     *
     *  Description:
     *      This function accepts an array of octets as the next portion of
     *      the message.
     *
     *  Parameters:
     *      message_array: [in]
     *          An array of characters representing the next portion of the
     *          message.
     *
     *  Returns:
     *      Nothing.
     *
     *  Comments:
     *
     */
    void SHA1::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t counter = length;
        const uint8_t* current = &(message_array[0]);

        /*
         * Split lenght to lower and higher portion
         */
        uint32_t lengthLow = static_cast<uint32_t>(_context.length & 0xFFFFFFFF);
        uint32_t lengthHigh = reinterpret_cast<uint32_t*>(&_context.length)[1];

        ASSERT((_computed == false) || (_corrupted == false));

        while (counter-- && !_corrupted) {
            _context.block[_context.index++] = *current;

            lengthLow++;
            if (lengthLow == 0x20000000) {
                lengthLow = 0;
                lengthHigh++;
                if (lengthHigh == 0) {
                    _corrupted = true; // Message is too long
                }
            }

            if (_context.index == 64) {
                ProcessMessageBlock();
                _context.index = 0;
            }

            current++;
        }

        /*
         * Combine lower and higher portion of length to single field
         */
        _context.length = (static_cast<uint64_t>(lengthHigh) << 32 | lengthLow);
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the SHA1 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    SHA1& SHA1::operator<<(const uint8_t message_array[])
    {
        const uint8_t* p = &(message_array[0]);

        while (*p != 0) {
            Input(p, 1);
            p++;
        }

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    SHA1& SHA1::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }

    /*
     *  ProcessMessageBlock
     *
     *  Description:
     *      This function will process the next 512 bits of the message
     *      stored in the _block array.
     *
     *  Parameters:
     *      None.
     *
     *  Returns:
     *      Nothing.
     *
     *  Comments:
     *      Many of the variable names in this function, especially the single
     *      character names, were used because those were the names used
     *      in the publication.
     *
     */
    void SHA1::ProcessMessageBlock()
    {
        const unsigned K[] = { // Constants defined for SHA-1
            0x5A827999,
            0x6ED9EBA1,
            0x8F1BBCDC,
            0xCA62C1D6
        };
        int t; // Loop counter
        unsigned temp; // Temporary word value
        unsigned W[80]; // Word sequence
        unsigned A, B, C, D, E; // Word buffers

        /*
         *  Initialize the first 16 words in the array W
         */
        for (t = 0; t < 16; t++) {
            W[t] = ((unsigned)_context.block[t * 4]) << 24;
            W[t] |= ((unsigned)_context.block[t * 4 + 1]) << 16;
            W[t] |= ((unsigned)_context.block[t * 4 + 2]) << 8;
            W[t] |= ((unsigned)_context.block[t * 4 + 3]);
        }

        for (t = 16; t < 80; t++) {
            W[t] = CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
        }

        A = static_cast<unsigned int>(_context.h[0]);
        B = static_cast<unsigned int>(_context.h[1]);
        C = static_cast<unsigned int>(_context.h[2]);
        D = static_cast<unsigned int>(_context.h[3]);
        E = static_cast<unsigned int>(_context.h[4]);

        for (t = 0; t < 20; t++) {
            temp = CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = CircularShift(30, B);
            B = A;
            A = temp;
        }

        for (t = 20; t < 40; t++) {
            temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = CircularShift(30, B);
            B = A;
            A = temp;
        }

        for (t = 40; t < 60; t++) {
            temp = CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = CircularShift(30, B);
            B = A;
            A = temp;
        }

        for (t = 60; t < 80; t++) {
            temp = CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
            temp &= 0xFFFFFFFF;
            E = D;
            D = C;
            C = CircularShift(30, B);
            B = A;
            A = temp;
        }

        _context.h[0] = (_context.h[0] + A) & 0xFFFFFFFF;
        _context.h[1] = (_context.h[1] + B) & 0xFFFFFFFF;
        _context.h[2] = (_context.h[2] + C) & 0xFFFFFFFF;
        _context.h[3] = (_context.h[3] + D) & 0xFFFFFFFF;
        _context.h[4] = (_context.h[4] + E) & 0xFFFFFFFF;
    }

    /*
     *  PadMessage
     *
     *  Description:
     *      According to the standard, the message must be padded to an even
     *      512 bits.  The first padding bit must be a '1'.  The last 64 bits
     *      represent the length of the original message.  All bits in between
     *      should be 0.  This function will pad the message according to those
     *      rules by filling the message_block array accordingly.  It will also
     *      call ProcessMessageBlock() appropriately.  When it returns, it
     *      can be assumed that the message digest has been computed.
     *
     *  Parameters:
     *      None.
     *
     *  Returns:
     *      Nothing.
     *
     *  Comments:
     *
     */
    void SHA1::PadMessage()
    {
        /*
         *  Check to see if the current message block is too small to hold
         *  the initial padding bits and length.  If so, we will pad the
         *  block, process it, and then continue padding into a second block.
         */
        if (_context.index > 55) {
            _context.block[_context.index++] = 0x80;

            ::memset(&(_context.block[_context.index]), 0, (64 - _context.index));

            ProcessMessageBlock();

            _context.index = 0;
        } else {
            _context.block[_context.index++] = 0x80;
        }

        ::memset(&(_context.block[_context.index]), 0, (56 - _context.index));

        /*
         * Split lenght to lower and higher portion
         */
        uint32_t lengthLow = static_cast<uint32_t>(_context.length & 0xFFFFFFFF);
        uint32_t lengthHigh = reinterpret_cast<uint32_t*>(&_context.length)[1];

        /*
         *  Store the message length as the last 8 octets
         */
        _context.block[56] = (lengthHigh >> 24) & 0xFF;
        _context.block[57] = (lengthHigh >> 16) & 0xFF;
        _context.block[58] = (lengthHigh >> 8) & 0xFF;
        _context.block[59] = (lengthHigh)&0xFF;
        _context.block[60] = (lengthLow >> 21) & 0xFF;
        _context.block[61] = (lengthLow >> 13) & 0xFF;
        _context.block[62] = (lengthLow >> 5) & 0xFF;
        _context.block[63] = (lengthLow << 3) & 0xFF;

        ProcessMessageBlock();

        uint32_t* writer = reinterpret_cast<uint32_t*>(&_context.block[0]);

        for (uint8_t teller = 0; teller < 5; teller++) {
            *writer++ = htonl(static_cast<uint32_t>(_context.h[teller]));
        }

        _computed = true;
    }

    // --------------------------------------------------------------------------------------------
    // SHA224/SHA256/SHA384/SHA512 constants functionality
    // --------------------------------------------------------------------------------------------
    static uint32_t sha224_h0[8] = {
        0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
        0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
    };

    static uint32_t sha256_h0[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    static uint64_t sha384_h0[8] = {
        0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
        0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
        0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
        0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL
    };

    static uint64_t sha512_h0[8] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
    };

    static uint32_t sha256_k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    static uint64_t sha512_k[80] = {
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
        0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
        0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
        0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
        0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
        0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
        0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
        0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
        0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
        0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
        0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
        0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
        0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
        0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
        0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
        0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
        0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
        0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
        0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
        0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
        0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
    };

    // --------------------------------------------------------------------------------------------
    // SHA256 functionality
    // --------------------------------------------------------------------------------------------
    static void sha256_trans_block(SHA256::Context* ctx, const unsigned char* message, unsigned int block_nb)
    {
        uint32_t w[64];
        uint32_t wv[8];
        uint32_t t1, t2;
        const unsigned char* sub_block;
        int i;

#ifndef UNROLL_LOOPS
        int j;
#endif

        for (i = 0; i < (int)block_nb; i++) {
            sub_block = message + (i << 6);

#ifndef UNROLL_LOOPS
            for (j = 0; j < 16; j++) {
                PACK32(&sub_block[j << 2], &w[j]);
            }

            for (j = 16; j < 64; j++) {
                SHA256_SCR(j);
            }

            for (j = 0; j < 8; j++) {
                wv[j] = static_cast<uint32_t>(ctx->h[j]);
            }

            for (j = 0; j < 64; j++) {
                t1 = wv[7] + SHA256_F2(wv[4]) + CH(wv[4], wv[5], wv[6])
                    + sha256_k[j] + w[j];
                t2 = SHA256_F1(wv[0]) + MAJ(wv[0], wv[1], wv[2]);
                wv[7] = wv[6];
                wv[6] = wv[5];
                wv[5] = wv[4];
                wv[4] = wv[3] + t1;
                wv[3] = wv[2];
                wv[2] = wv[1];
                wv[1] = wv[0];
                wv[0] = t1 + t2;
            }

            for (j = 0; j < 8; j++) {
                ctx->h[j] += wv[j];
            }
#else
            PACK32(&sub_block[0], &w[0]);
            PACK32(&sub_block[4], &w[1]);
            PACK32(&sub_block[8], &w[2]);
            PACK32(&sub_block[12], &w[3]);
            PACK32(&sub_block[16], &w[4]);
            PACK32(&sub_block[20], &w[5]);
            PACK32(&sub_block[24], &w[6]);
            PACK32(&sub_block[28], &w[7]);
            PACK32(&sub_block[32], &w[8]);
            PACK32(&sub_block[36], &w[9]);
            PACK32(&sub_block[40], &w[10]);
            PACK32(&sub_block[44], &w[11]);
            PACK32(&sub_block[48], &w[12]);
            PACK32(&sub_block[52], &w[13]);
            PACK32(&sub_block[56], &w[14]);
            PACK32(&sub_block[60], &w[15]);

            SHA256_SCR(16);
            SHA256_SCR(17);
            SHA256_SCR(18);
            SHA256_SCR(19);
            SHA256_SCR(20);
            SHA256_SCR(21);
            SHA256_SCR(22);
            SHA256_SCR(23);
            SHA256_SCR(24);
            SHA256_SCR(25);
            SHA256_SCR(26);
            SHA256_SCR(27);
            SHA256_SCR(28);
            SHA256_SCR(29);
            SHA256_SCR(30);
            SHA256_SCR(31);
            SHA256_SCR(32);
            SHA256_SCR(33);
            SHA256_SCR(34);
            SHA256_SCR(35);
            SHA256_SCR(36);
            SHA256_SCR(37);
            SHA256_SCR(38);
            SHA256_SCR(39);
            SHA256_SCR(40);
            SHA256_SCR(41);
            SHA256_SCR(42);
            SHA256_SCR(43);
            SHA256_SCR(44);
            SHA256_SCR(45);
            SHA256_SCR(46);
            SHA256_SCR(47);
            SHA256_SCR(48);
            SHA256_SCR(49);
            SHA256_SCR(50);
            SHA256_SCR(51);
            SHA256_SCR(52);
            SHA256_SCR(53);
            SHA256_SCR(54);
            SHA256_SCR(55);
            SHA256_SCR(56);
            SHA256_SCR(57);
            SHA256_SCR(58);
            SHA256_SCR(59);
            SHA256_SCR(60);
            SHA256_SCR(61);
            SHA256_SCR(62);
            SHA256_SCR(63);

            wv[0] = ctx->h[0];
            wv[1] = ctx->h[1];
            wv[2] = ctx->h[2];
            wv[3] = ctx->h[3];
            wv[4] = ctx->h[4];
            wv[5] = ctx->h[5];
            wv[6] = ctx->h[6];
            wv[7] = ctx->h[7];

            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 0);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 1);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 2);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 3);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 4);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 5);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 6);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 7);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 8);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 9);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 10);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 11);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 12);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 13);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 14);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 15);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 16);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 17);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 18);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 19);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 20);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 21);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 22);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 23);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 24);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 25);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 26);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 27);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 28);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 29);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 30);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 31);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 32);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 33);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 34);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 35);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 36);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 37);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 38);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 39);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 40);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 41);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 42);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 43);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 44);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 45);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 46);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 47);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 48);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 49);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 50);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 51);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 52);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 53);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 54);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 55);
            SHA256_EXP(0, 1, 2, 3, 4, 5, 6, 7, 56);
            SHA256_EXP(7, 0, 1, 2, 3, 4, 5, 6, 57);
            SHA256_EXP(6, 7, 0, 1, 2, 3, 4, 5, 58);
            SHA256_EXP(5, 6, 7, 0, 1, 2, 3, 4, 59);
            SHA256_EXP(4, 5, 6, 7, 0, 1, 2, 3, 60);
            SHA256_EXP(3, 4, 5, 6, 7, 0, 1, 2, 61);
            SHA256_EXP(2, 3, 4, 5, 6, 7, 0, 1, 62);
            SHA256_EXP(1, 2, 3, 4, 5, 6, 7, 0, 63);

            ctx->h[0] += wv[0];
            ctx->h[1] += wv[1];
            ctx->h[2] += wv[2];
            ctx->h[3] += wv[3];
            ctx->h[4] += wv[4];
            ctx->h[5] += wv[5];
            ctx->h[6] += wv[6];
            ctx->h[7] += wv[7];
#endif /* !UNROLL_LOOPS */
        }
    }

    static void sha256_trans(SHA256::Context* ctx, const unsigned char* message)
    {
       uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];
        for (i = 0, j = 0; i < 16; ++i, j += 4) {
             PACK32(&message[i << 2], &m[i]);
        }
        for ( ; i < 64; ++i) {
                m[i] = SHA256_F4(m[i - 2]) + m[i - 7] + SHA256_F3(m[i - 15]) + m[i - 16];
        }

        a = static_cast<uint32_t>(ctx->h[0]);
        b = static_cast<uint32_t>(ctx->h[1]);
        c = static_cast<uint32_t>(ctx->h[2]);
        d = static_cast<uint32_t>(ctx->h[3]);
        e = static_cast<uint32_t>(ctx->h[4]);
        f = static_cast<uint32_t>(ctx->h[5]);
        g = static_cast<uint32_t>(ctx->h[6]);
        h = static_cast<uint32_t>(ctx->h[7]);

        for (i = 0; i < 64; ++i) {
            t1 = h + SHA256_F2(e) + CH(e,f,g) + sha256_k[i] + m[i];
            t2 = SHA256_F1(a) + MAJ(a,b,c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        ctx->h[0] += a;
        ctx->h[1] += b;
        ctx->h[2] += c;
        ctx->h[3] += d;
        ctx->h[4] += e;
        ctx->h[5] += f;
        ctx->h[6] += g;
        ctx->h[7] += h;
    }

    void SHA256::Reset()
    {
#ifndef UNROLL_LOOPS
        int i;
        for (i = 0; i < 8; i++) {
            _context.h[i] = sha256_h0[i];
        }
#else
        _context.h[0] = sha256_h0[0];
        _context.h[1] = sha256_h0[1];
        _context.h[2] = sha256_h0[2];
        _context.h[3] = sha256_h0[3];
        _context.h[4] = sha256_h0[4];
        _context.h[5] = sha256_h0[5];
        _context.h[6] = sha256_h0[6];
        _context.h[7] = sha256_h0[7];
#endif /* !UNROLL_LOOPS */

        _context.index = 0;
        _context.length = 0;
        _computed = false;
    }

    static void sha256_update(SHA256::Context* ctx, const unsigned char* message, unsigned int len)
    {
        uint32_t i;

        for (i = 0; i < len; ++i) {
            ctx->block[ctx->index] = message[i];
            ctx->index++;
            if (ctx->index == 64) {
                sha256_trans(ctx, ctx->block);
                ctx->length += 512;
                ctx->index = 0;
            }
        }
    }

    void SHA256::CloseContext()
    {
        uint32_t i = _context.index;

        // Pad whatever block is left in the buffer.
        if (_context.index < 56) {
            _context.block[i++] = 0x80;
            while (i < 56) {
               _context.block[i++] = 0x00;
            }
        } else {
            _context.block[i++] = 0x80;
            while (i < 64) {
                _context.block[i++] = 0x00;
            }
            sha256_trans(&_context, _context.block);
            memset(_context.block, 0, 56);
        }

        // Append to the padding the total message's length in bits and transform.
        _context.length += _context.index * 8;
        _context.block[63] = static_cast<uint8_t>((_context.length) & 0xFF);
        _context.block[62] = static_cast<uint8_t>((_context.length >> 8) & 0xFF);
        _context.block[61] = static_cast<uint8_t>((_context.length >> 16) & 0xFF);
        _context.block[60] = static_cast<uint8_t>((_context.length >> 24) & 0xFF);
        _context.block[59] = static_cast<uint8_t>((_context.length >> 32) & 0xFF);
        _context.block[58] = static_cast<uint8_t>((_context.length >> 40) & 0xFF);
        _context.block[57] = static_cast<uint8_t>((_context.length >> 48) & 0xFF);
        _context.block[56] = static_cast<uint8_t>((_context.length >> 56) & 0xFF);
        sha256_trans(&_context, _context.block);

        // Since this implementation uses little endian byte ordering and SHA uses big endian,
        // reverse all the bytes when copying the final h to the output hash.
        for (i = 0; i < 4; ++i) {
            _context.block[i]      = (_context.h[0] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 4]  = (_context.h[1] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 8]  = (_context.h[2] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 12] = (_context.h[3] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 16] = (_context.h[4] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 20] = (_context.h[5] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 24] = (_context.h[6] >> (24 - i * 8)) & 0x000000ff;
            _context.block[i + 28] = (_context.h[7] >> (24 - i * 8)) & 0x000000ff;
        }
    }

    void SHA256::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t sizeToHandle = length;
        const uint8_t* source = &message_array[0];

        while (sizeToHandle > 0) {
            if (sizeToHandle > SHA256_BLOCK_SIZE) {
                sha256_update(&_context, source, SHA256_BLOCK_SIZE);
                source += SHA256_BLOCK_SIZE;
                sizeToHandle -= SHA256_BLOCK_SIZE;
            } else {
                sha256_update(&_context, source, sizeToHandle);
                sizeToHandle = 0;
            }
        }
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the SHA1 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    SHA256& SHA256::operator<<(const uint8_t message_array[])
    {
        uint16_t length = 0;

        while (message_array[length] != '\0') {
            length++;
        }

        Input(message_array, length);

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    SHA256& SHA256::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    // SHA224 functionality
    // --------------------------------------------------------------------------------------------
    void SHA224::Reset()
    {
#ifndef UNROLL_LOOPS
        int i;
        for (i = 0; i < 8; i++) {
            _context.h[i] = sha224_h0[i];
        }
#else
        _context.h[0] = sha224_h0[0];
        _context.h[1] = sha224_h0[1];
        _context.h[2] = sha224_h0[2];
        _context.h[3] = sha224_h0[3];
        _context.h[4] = sha224_h0[4];
        _context.h[5] = sha224_h0[5];
        _context.h[6] = sha224_h0[6];
        _context.h[7] = sha224_h0[7];
#endif /* !UNROLL_LOOPS */

        _context.index = 0;
        _context.length = 0;
        _computed = false;
    }

    static void sha224_update(SHA224::Context* ctx, const unsigned char* message, unsigned int len)
    {
        unsigned int block_nb;
        unsigned int new_len, rem_len, tmp_len;
        const unsigned char* shifted_message;

        tmp_len = SHA224_BLOCK_SIZE - ctx->index;
        rem_len = len < tmp_len ? len : tmp_len;

        memcpy(&ctx->block[ctx->index], message, rem_len);

        if (ctx->index + len < SHA224_BLOCK_SIZE) {
            ctx->index += len;
            return;
        }

        new_len = len - rem_len;
        block_nb = new_len / SHA224_BLOCK_SIZE;

        shifted_message = message + rem_len;

        sha256_trans_block(ctx, ctx->block, 1);
        sha256_trans_block(ctx, shifted_message, block_nb);

        rem_len = new_len % SHA224_BLOCK_SIZE;

        memcpy(ctx->block, &shifted_message[block_nb << 6],
            rem_len);

        ctx->index = rem_len;
        ctx->length += (block_nb + 1) << 6;
    }

    void SHA224::CloseContext()
    {
        unsigned int block_nb;
        unsigned int pm_len;
        unsigned int len_b;

#ifndef UNROLL_LOOPS
        int i;
#endif

        block_nb = (1 + ((SHA224_BLOCK_SIZE - 9) < (_context.index % SHA224_BLOCK_SIZE)));

        len_b = static_cast<unsigned int>((_context.length + _context.index) << 3);
        pm_len = block_nb << 6;

        memset(_context.block + _context.index, 0, pm_len - _context.index);
        _context.block[_context.index] = 0x80;
        UNPACK32(len_b, _context.block + pm_len - 4);

        sha256_trans_block(&_context, _context.block, block_nb);

#ifndef UNROLL_LOOPS
        for (i = 0; i < 7; i++) {
            UNPACK32(_context.h[i], &_context.block[i << 2]);
        }
#else
        UNPACK32(_context.h[0], &_context.block[0]);
        UNPACK32(_context.h[1], &_context.block[4]);
        UNPACK32(_context.h[2], &_context.block[8]);
        UNPACK32(_context.h[3], &_context.block[12]);
        UNPACK32(_context.h[4], &_context.block[16]);
        UNPACK32(_context.h[5], &_context.block[20]);
        UNPACK32(_context.h[6], &_context.block[24]);
#endif /* !UNROLL_LOOPS */
    }

    void SHA224::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t sizeToHandle = length;
        const uint8_t* source = &message_array[0];

        while (sizeToHandle > 0) {
            if (sizeToHandle > SHA224_BLOCK_SIZE) {
                sha224_update(&_context, source, SHA224_BLOCK_SIZE);
                source += SHA224_BLOCK_SIZE;
                sizeToHandle -= SHA224_BLOCK_SIZE;
            } else {
                sha224_update(&_context, source, sizeToHandle);
                sizeToHandle = 0;
            }
        }
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the SHA1 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    SHA224& SHA224::operator<<(const uint8_t message_array[])
    {
        uint16_t length = 0;

        while (message_array[length] != '\0') {
            length++;
        }

        Input(message_array, length);

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    SHA224& SHA224::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    // SHA512 functionality
    // --------------------------------------------------------------------------------------------
    static void sha512_transf(SHA512::Context* ctx, const unsigned char* message, unsigned int block_nb)
    {
        uint64_t w[80];
        uint64_t wv[8];
        uint64_t t1, t2;
        const unsigned char* sub_block;
        int i, j;

        for (i = 0; i < (int)block_nb; i++) {
            sub_block = message + (i << 7);

#ifndef UNROLL_LOOPS
            for (j = 0; j < 16; j++) {
                PACK64(&sub_block[j << 3], &w[j]);
            }

            for (j = 16; j < 80; j++) {
                SHA512_SCR(j);
            }

            for (j = 0; j < 8; j++) {
                wv[j] = ctx->h[j];
            }

            for (j = 0; j < 80; j++) {
                t1 = wv[7] + SHA512_F2(wv[4]) + CH(wv[4], wv[5], wv[6])
                    + sha512_k[j] + w[j];
                t2 = SHA512_F1(wv[0]) + MAJ(wv[0], wv[1], wv[2]);
                wv[7] = wv[6];
                wv[6] = wv[5];
                wv[5] = wv[4];
                wv[4] = wv[3] + t1;
                wv[3] = wv[2];
                wv[2] = wv[1];
                wv[1] = wv[0];
                wv[0] = t1 + t2;
            }

            for (j = 0; j < 8; j++) {
                ctx->h[j] += wv[j];
            }
#else
            PACK64(&sub_block[0], &w[0]);
            PACK64(&sub_block[8], &w[1]);
            PACK64(&sub_block[16], &w[2]);
            PACK64(&sub_block[24], &w[3]);
            PACK64(&sub_block[32], &w[4]);
            PACK64(&sub_block[40], &w[5]);
            PACK64(&sub_block[48], &w[6]);
            PACK64(&sub_block[56], &w[7]);
            PACK64(&sub_block[64], &w[8]);
            PACK64(&sub_block[72], &w[9]);
            PACK64(&sub_block[80], &w[10]);
            PACK64(&sub_block[88], &w[11]);
            PACK64(&sub_block[96], &w[12]);
            PACK64(&sub_block[104], &w[13]);
            PACK64(&sub_block[112], &w[14]);
            PACK64(&sub_block[120], &w[15]);

            SHA512_SCR(16);
            SHA512_SCR(17);
            SHA512_SCR(18);
            SHA512_SCR(19);
            SHA512_SCR(20);
            SHA512_SCR(21);
            SHA512_SCR(22);
            SHA512_SCR(23);
            SHA512_SCR(24);
            SHA512_SCR(25);
            SHA512_SCR(26);
            SHA512_SCR(27);
            SHA512_SCR(28);
            SHA512_SCR(29);
            SHA512_SCR(30);
            SHA512_SCR(31);
            SHA512_SCR(32);
            SHA512_SCR(33);
            SHA512_SCR(34);
            SHA512_SCR(35);
            SHA512_SCR(36);
            SHA512_SCR(37);
            SHA512_SCR(38);
            SHA512_SCR(39);
            SHA512_SCR(40);
            SHA512_SCR(41);
            SHA512_SCR(42);
            SHA512_SCR(43);
            SHA512_SCR(44);
            SHA512_SCR(45);
            SHA512_SCR(46);
            SHA512_SCR(47);
            SHA512_SCR(48);
            SHA512_SCR(49);
            SHA512_SCR(50);
            SHA512_SCR(51);
            SHA512_SCR(52);
            SHA512_SCR(53);
            SHA512_SCR(54);
            SHA512_SCR(55);
            SHA512_SCR(56);
            SHA512_SCR(57);
            SHA512_SCR(58);
            SHA512_SCR(59);
            SHA512_SCR(60);
            SHA512_SCR(61);
            SHA512_SCR(62);
            SHA512_SCR(63);
            SHA512_SCR(64);
            SHA512_SCR(65);
            SHA512_SCR(66);
            SHA512_SCR(67);
            SHA512_SCR(68);
            SHA512_SCR(69);
            SHA512_SCR(70);
            SHA512_SCR(71);
            SHA512_SCR(72);
            SHA512_SCR(73);
            SHA512_SCR(74);
            SHA512_SCR(75);
            SHA512_SCR(76);
            SHA512_SCR(77);
            SHA512_SCR(78);
            SHA512_SCR(79);

            wv[0] = ctx->h[0];
            wv[1] = ctx->h[1];
            wv[2] = ctx->h[2];
            wv[3] = ctx->h[3];
            wv[4] = ctx->h[4];
            wv[5] = ctx->h[5];
            wv[6] = ctx->h[6];
            wv[7] = ctx->h[7];

            j = 0;

            do {
                SHA512_EXP(0, 1, 2, 3, 4, 5, 6, 7, j);
                j++;
                SHA512_EXP(7, 0, 1, 2, 3, 4, 5, 6, j);
                j++;
                SHA512_EXP(6, 7, 0, 1, 2, 3, 4, 5, j);
                j++;
                SHA512_EXP(5, 6, 7, 0, 1, 2, 3, 4, j);
                j++;
                SHA512_EXP(4, 5, 6, 7, 0, 1, 2, 3, j);
                j++;
                SHA512_EXP(3, 4, 5, 6, 7, 0, 1, 2, j);
                j++;
                SHA512_EXP(2, 3, 4, 5, 6, 7, 0, 1, j);
                j++;
                SHA512_EXP(1, 2, 3, 4, 5, 6, 7, 0, j);
                j++;
            } while (j < 80);

            ctx->h[0] += wv[0];
            ctx->h[1] += wv[1];
            ctx->h[2] += wv[2];
            ctx->h[3] += wv[3];
            ctx->h[4] += wv[4];
            ctx->h[5] += wv[5];
            ctx->h[6] += wv[6];
            ctx->h[7] += wv[7];
#endif /* !UNROLL_LOOPS */
        }
    }

    void SHA512::Reset()
    {
#ifndef UNROLL_LOOPS
        int i;
        for (i = 0; i < 8; i++) {
            _context.h[i] = sha512_h0[i];
        }
#else
        _context.h[0] = sha512_h0[0];
        _context.h[1] = sha512_h0[1];
        _context.h[2] = sha512_h0[2];
        _context.h[3] = sha512_h0[3];
        _context.h[4] = sha512_h0[4];
        _context.h[5] = sha512_h0[5];
        _context.h[6] = sha512_h0[6];
        _context.h[7] = sha512_h0[7];
#endif /* !UNROLL_LOOPS */

        _context.index = 0;
        _context.length = 0;
        _computed = false;
    }

    static void sha512_update(SHA512::Context* ctx, const unsigned char* message,
        unsigned int len)
    {
        unsigned int block_nb;
        unsigned int new_len, rem_len, tmp_len;
        const unsigned char* shifted_message;

        tmp_len = SHA512_BLOCK_SIZE - ctx->index;
        rem_len = len < tmp_len ? len : tmp_len;

        memcpy(&ctx->block[ctx->index], message, rem_len);

        if (ctx->index + len < SHA512_BLOCK_SIZE) {
            ctx->index += len;
            return;
        }

        new_len = len - rem_len;
        block_nb = new_len / SHA512_BLOCK_SIZE;

        shifted_message = message + rem_len;

        sha512_transf(ctx, ctx->block, 1);
        sha512_transf(ctx, shifted_message, block_nb);

        rem_len = new_len % SHA512_BLOCK_SIZE;

        memcpy(ctx->block, &shifted_message[block_nb << 7],
            rem_len);

        ctx->index = rem_len;
        ctx->length += (block_nb + 1) << 7;
    }

    void SHA512::CloseContext()
    {
        unsigned int block_nb;
        unsigned int pm_len;
        unsigned int len_b;

#ifndef UNROLL_LOOPS
        int i;
#endif

        block_nb = 1 + ((SHA512_BLOCK_SIZE - 17) < (_context.index % SHA512_BLOCK_SIZE));

        len_b = static_cast<unsigned int>(_context.length + _context.index) << 3;
        pm_len = block_nb << 7;

        memset(_context.block + _context.index, 0, pm_len - _context.index);
        _context.block[_context.index] = 0x80;
        UNPACK32(len_b, _context.block + pm_len - 4);

        sha512_transf(&_context, _context.block, block_nb);

#ifndef UNROLL_LOOPS
        for (i = 0; i < 8; i++) {
            UNPACK64(_context.h[i], &_context.block[i << 3]);
        }
#else
        UNPACK64(_context.h[0], &_context.block[0]);
        UNPACK64(_context.h[1], &_context.block[8]);
        UNPACK64(_context.h[2], &_context.block[16]);
        UNPACK64(_context.h[3], &_context.block[24]);
        UNPACK64(_context.h[4], &_context.block[32]);
        UNPACK64(_context.h[5], &_context.block[40]);
        UNPACK64(_context.h[6], &_context.block[48]);
        UNPACK64(_context.h[7], &_context.block[56]);
#endif /* !UNROLL_LOOPS */
    }

    void SHA512::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t sizeToHandle = length;
        const uint8_t* source = &message_array[0];

        while (sizeToHandle > 0) {
            if (sizeToHandle > SHA512_BLOCK_SIZE) {
                sha512_update(&_context, source, SHA512_BLOCK_SIZE);
                source += SHA512_BLOCK_SIZE;
                sizeToHandle -= SHA512_BLOCK_SIZE;
            } else {
                sha512_update(&_context, source, sizeToHandle);
                sizeToHandle = 0;
            }
        }
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the SHA1 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    SHA512& SHA512::operator<<(const uint8_t message_array[])
    {
        uint16_t length = 0;

        while (message_array[length] != '\0') {
            length++;
        }

        Input(message_array, length);

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    SHA512& SHA512::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }

    // --------------------------------------------------------------------------------------------
    // SHA384 functionality
    // --------------------------------------------------------------------------------------------

    void SHA384::Reset()
    {
#ifndef UNROLL_LOOPS
        int i;
        for (i = 0; i < 8; i++) {
            _context.h[i] = sha384_h0[i];
        }
#else
        _context.h[0] = sha384_h0[0];
        _context.h[1] = sha384_h0[1];
        _context.h[2] = sha384_h0[2];
        _context.h[3] = sha384_h0[3];
        _context.h[4] = sha384_h0[4];
        _context.h[5] = sha384_h0[5];
        _context.h[6] = sha384_h0[6];
        _context.h[7] = sha384_h0[7];
#endif /* !UNROLL_LOOPS */

        _context.index = 0;
        _context.length = 0;
        _computed = false;
    }

    static void sha384_update(SHA384::Context* ctx, const unsigned char* message, unsigned int len)
    {
        unsigned int block_nb;
        unsigned int new_len, rem_len, tmp_len;
        const unsigned char* shifted_message;

        tmp_len = SHA384_BLOCK_SIZE - ctx->index;
        rem_len = len < tmp_len ? len : tmp_len;

        memcpy(&ctx->block[ctx->index], message, rem_len);

        if (ctx->index + len < SHA384_BLOCK_SIZE) {
            ctx->index += len;
            return;
        }

        new_len = len - rem_len;
        block_nb = new_len / SHA384_BLOCK_SIZE;

        shifted_message = message + rem_len;

        sha512_transf(ctx, ctx->block, 1);
        sha512_transf(ctx, shifted_message, block_nb);

        rem_len = new_len % SHA384_BLOCK_SIZE;

        memcpy(ctx->block, &shifted_message[block_nb << 7],
            rem_len);

        ctx->index = rem_len;
        ctx->length += (block_nb + 1) << 7;
    }

    void SHA384::CloseContext()
    {
        unsigned int block_nb;
        unsigned int pm_len;
        unsigned int len_b;

#ifndef UNROLL_LOOPS
        int i;
#endif

        block_nb = (1 + ((SHA384_BLOCK_SIZE - 17) < (_context.index % SHA384_BLOCK_SIZE)));

        len_b = static_cast<unsigned int>((_context.length + _context.index) << 3);
        pm_len = block_nb << 7;

        memset(_context.block + _context.index, 0, pm_len - _context.index);
        _context.block[_context.index] = 0x80;
        UNPACK32(len_b, _context.block + pm_len - 4);

        sha512_transf(&_context, _context.block, block_nb);

#ifndef UNROLL_LOOPS
        for (i = 0; i < 6; i++) {
            UNPACK64(_context.h[i], &_context.block[i << 3]);
        }
#else
        UNPACK64(_context.h[0], &_context.block[0]);
        UNPACK64(_context.h[1], &_context.block[8]);
        UNPACK64(_context.h[2], &_context.block[16]);
        UNPACK64(_context.h[3], &_context.block[24]);
        UNPACK64(_context.h[4], &_context.block[32]);
        UNPACK64(_context.h[5], &_context.block[40]);
#endif /* !UNROLL_LOOPS */
    }

    void SHA384::Input(const uint8_t message_array[], const uint16_t length)
    {
        uint16_t sizeToHandle = length;
        const uint8_t* source = &message_array[0];

        while (sizeToHandle > 0) {
            if (sizeToHandle > SHA384_BLOCK_SIZE) {
                sha384_update(&_context, source, SHA384_BLOCK_SIZE);
                source += SHA384_BLOCK_SIZE;
                sizeToHandle -= SHA384_BLOCK_SIZE;
            } else {
                sha384_update(&_context, source, sizeToHandle);
                sizeToHandle = 0;
            }
        }
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This operator makes it convenient to provide character strings to
     *      the SHA1 object for processing.
     *
     *  Parameters:
     *      message_array: [in]
     *          The character array to take as input.
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      Each character is assumed to hold 8 bits of information.
     *
     */
    SHA384& SHA384::operator<<(const uint8_t message_array[])
    {
        uint16_t length = 0;

        while (message_array[length] != '\0') {
            length++;
        }

        Input(message_array, length);

        return *this;
    }

    /*
     *  operator<<
     *
     *  Description:
     *      This function provides the next octet in the message.
     *
     *  Parameters:
     *      message_element: [in]
     *          The next octet in the message
     *
     *  Returns:
     *      A reference to the SHA1 object.
     *
     *  Comments:
     *      The character is assumed to hold 8 bits of information.
     *
     */
    SHA384& SHA384::operator<<(const uint8_t message_element)
    {
        Input(&message_element, 1);

        return *this;
    }
}
} // namespace Thunder::Crypto

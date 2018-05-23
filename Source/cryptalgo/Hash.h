/*
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Last update: 02/02/2007
 * Issue date:  04/30/2005
 *
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

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
* See md5.c for more information.
*/

#ifndef __HASH_H
#define __HASH_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Crypto {
    enum EnumHashType {
        HASH_MD5 = 16,
        HASH_SHA1 = 20,
        HASH_SHA224 = 28,
        HASH_SHA256 = 32,
        HASH_SHA384 = 48,
        HASH_SHA512 = 64
    };

    class EXTERNAL SHA1 {
    private:
        SHA1(const SHA1&);
        SHA1& operator=(const SHA1&);

    public:
        inline SHA1()
        {
            Reset();
        }
        inline SHA1(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~SHA1()
        {
        }

    public:
        static const EnumHashType Type = HASH_SHA1;
        static const uint8_t Length = 20;

        void Reset()
        {
            _lengthLow = 0;
            _lengthHigh = 0;
            _messageIndex = 0;

            H[0] = 0x67452301;
            H[1] = 0xEFCDAB89;
            H[2] = 0x98BADCFE;
            H[3] = 0x10325476;
            H[4] = 0xC3D2E1F0;

            _computed = false;
            _corrupted = false;
        }
        const uint8_t* Result()
        {
            const uint8_t* result = nullptr;

            if (_corrupted == false) {
                if (_computed == false) {
                    PadMessage();
                }

                result = &_messageBlock[0];
            }

            return result;
        }

        /*
         *  Provide input to SHA1
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        SHA1& operator<<(const uint8_t message_array[]);
        SHA1& operator<<(const uint8_t message_element);

    private:
        /*
         *  Process the next 512 bits of the message
         */
        void ProcessMessageBlock();

        /*
         *  Pads the current message block to 512 bits
         */
        void PadMessage();

        /*
         *  CircularShift
         *
         *  Description:
         *      This member function will perform a circular shifting operation.
         *
         *  Parameters:
         *      bits: [in]
         *          The number of bits to shift (1-31)
         *      word: [in]
         *          The value to shift (assumes a 32-bit integer)
         *
         *  Returns:
         *      The shifted value.
         *
         *  Comments:
         *
         */
        inline uint32_t CircularShift(const uint8_t bits, const uint32_t word)
        {
            return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - bits));
        }

        uint32_t H[5]; // Message digest buffers

        uint32_t _lengthLow; // Message length in bits
        uint32_t _lengthHigh; // Message length in bits

        uint8_t _messageBlock[64]; // 512-bit message blocks
        uint32_t _messageIndex; // Index into message block array

        mutable bool _computed; // Is the digest computed?
        bool _corrupted; // Is the message digest corruped?
    };

    class EXTERNAL MD5 {
    private:
        MD5(const MD5&);
        MD5& operator=(const MD5&);

    public:
        typedef struct {
            uint32_t lo, hi;
            uint32_t a, b, c, d;
            uint8_t buffer[64];
            uint32_t block[16];
        } Context;

    public:
        inline MD5()
        {
            Reset();
        }
        inline MD5(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~MD5()
        {
        }

    public:
        static const EnumHashType Type = HASH_MD5;
        static const uint8_t Length = 16;

        void Reset()
        {
            ::memset(&_context, 0, sizeof(MD5::Context));

            _context.a = 0x67452301;
            _context.b = 0xefcdab89;
            _context.c = 0x98badcfe;
            _context.d = 0x10325476;

            _context.lo = 0;
            _context.hi = 0;

            _computed = false;
            _corrupted = false;
        }
        const uint8_t* Result()
        {
            const uint8_t* result = nullptr;

            if (_corrupted == false) {
                if (_computed == false) {
                    CloseContext();
                }
                result = &(_context.buffer[0]);
            }

            return result;
        }

        /*
         *  Provide input to MD5
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        MD5& operator<<(const uint8_t message_array[]);
        MD5& operator<<(const uint8_t message_element);

    private:
        void CloseContext();

    private:
        Context _context;
        bool _corrupted;
        bool _computed;
    };

    class EXTERNAL SHA256 {
    public:
        typedef struct {
            uint32_t tot_len;
            uint32_t len;
            uint8_t block[2 * (512 / 8)];
            uint32_t h[8];
        } Context;

    private:
        SHA256(const SHA256&);
        SHA256& operator=(const SHA256&);

    public:
        inline SHA256()
        {
            Reset();
        }
        inline SHA256(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~SHA256()
        {
        }

    public:
        void Reset();
        static const EnumHashType Type = HASH_SHA256;
        static const uint8_t Length = 32;
        inline const uint8_t* Result()
        {
            if (_computed == false) {
                _computed = true;
                CloseContext();
            }

            return &_context.block[0];
        }

        /*
         *  Provide input to SHA1
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        SHA256& operator<<(const uint8_t message_array[]);
        SHA256& operator<<(const uint8_t message_element);

    private:
        void CloseContext();

    private:
        Context _context;
        mutable bool _computed; // Is the digest computed?
    };

    class EXTERNAL SHA224 {
    private:
        SHA224(const SHA224&);
        SHA224& operator=(const SHA224&);

    public:
        inline SHA224()
        {
            Reset();
        }
        inline SHA224(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~SHA224()
        {
        }

    public:
        void Reset();
        static const EnumHashType Type = HASH_SHA224;
        static const uint8_t Length = 28;
        inline const uint8_t* Result()
        {
            if (_computed == false) {
                _computed = true;
                CloseContext();
            }

            return &_context.block[0];
        }

        /*
         *  Provide input to SHA224
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        SHA224& operator<<(const uint8_t message_array[]);
        SHA224& operator<<(const uint8_t message_element);

    private:
        void CloseContext();

    private:
        SHA256::Context _context;
        mutable bool _computed; // Is the digest computed?
    };

    class EXTERNAL SHA512 {
    public:
        typedef struct {
            uint32_t tot_len;
            uint32_t len;
            uint8_t block[2 * (1024 / 8)];
            uint64_t h[8];
        } Context;

    private:
        SHA512(const SHA512&);
        SHA512& operator=(const SHA512&);

    public:
        inline SHA512()
        {
            Reset();
        }
        inline SHA512(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~SHA512()
        {
        }

    public:
        void Reset();
        static const EnumHashType Type = HASH_SHA512;
        static const uint8_t Length = 64;
        inline const uint8_t* Result()
        {
            if (_computed == false) {
                _computed = true;
                CloseContext();
            }

            return &_context.block[0];
        }

        /*
         *  Provide input to SHA512
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        SHA512& operator<<(const uint8_t message_array[]);
        SHA512& operator<<(const uint8_t message_element);

    private:
        void CloseContext();

    private:
        Context _context;
        mutable bool _computed; // Is the digest computed?
    };

    class EXTERNAL SHA384 {
    private:
        SHA384(const SHA384&);
        SHA384& operator=(const SHA384&);

    public:
        inline SHA384()
        {
            Reset();
        }
        inline SHA384(const uint8_t message_array[], const uint16_t length)
        {
            Reset();

            Input(message_array, length);
        }
        inline ~SHA384()
        {
        }

    public:
        void Reset();
        static const EnumHashType Type = HASH_SHA384;
        static const uint8_t Length = 48;
        inline const uint8_t* Result()
        {
            if (_computed == false) {
                _computed = true;
                CloseContext();
            }

            return &_context.block[0];
        }

        /*
         *  Provide input to SHA384
         */
        void Input(const uint8_t message_array[], const uint16_t length);

        SHA384& operator<<(const uint8_t message_array[]);
        SHA384& operator<<(const uint8_t message_element);

    private:
        void CloseContext();

    private:
        SHA512::Context _context;
        mutable bool _computed; // Is the digest computed?
    };
}
}

#endif // HASH_H

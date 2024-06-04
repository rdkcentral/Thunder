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

/* The SHA-256 algorith is updated to handle upto 32 bit size data
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
#ifndef __HASH_H
#define __HASH_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace Thunder {
namespace Crypto {
    enum EnumHashType {
        HASH_MD5 = 16,
        HASH_SHA1 = 20,
        HASH_SHA224 = 28,
        HASH_SHA256 = 32,
        HASH_SHA384 = 48,
        HASH_SHA512 = 64
    };
    typedef struct EXTERNAL Context {
        uint64_t length;
        uint64_t h[8];
        uint32_t index;
        uint8_t block[2 * (1024 / 8)];
    } Context;

    class EXTERNAL MD5 {
    public:
        typedef Crypto::Context Context;

    public:
        MD5(MD5&&) = delete;
        MD5(const MD5&) = delete;
        MD5& operator=(MD5&&) = delete;
        MD5& operator=(const MD5&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
        }

        void Reset()
        {
            ::memset(&_context, 0, sizeof(Context));

            _context.h[0] = 0x67452301;
            _context.h[1] = 0xefcdab89;
            _context.h[2] = 0x98badcfe;
            _context.h[3] = 0x10325476;

            _context.length = 0;

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
                result = &(_context.block[0]);
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

    class EXTERNAL SHA1 {
    public:
        typedef Crypto::Context Context;

    public:
        SHA1(SHA1&&) = delete;
        SHA1(const SHA1&) = delete;
        SHA1& operator=(SHA1&&) = delete;
        SHA1& operator=(const SHA1&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
            ASSERT(_context.index <= sizeof(_context.block));
        }

        void Reset()
        {
            _context.length = 0;
            _context.index = 0;

            _context.h[0] = 0x67452301;
            _context.h[1] = 0xEFCDAB89;
            _context.h[2] = 0x98BADCFE;
            _context.h[3] = 0x10325476;
            _context.h[4] = 0xC3D2E1F0;

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

                result = &_context.block[0];
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

        mutable bool _computed; // Is the digest computed?
        bool _corrupted; // Is the message digest corruped?

        Context _context;
    };


    class EXTERNAL SHA256 {
    public:
        typedef Crypto::Context Context;

    public:
        SHA256(SHA256&&) = delete;
        SHA256(const SHA256&) = delete;
        SHA256& operator=(SHA256&&) = delete;
        SHA256& operator=(const SHA256&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
            ASSERT(_context.index <= sizeof(_context.block));
        }

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
    public:
        typedef Crypto::Context Context;

    public:
        SHA224(SHA224&&) = delete;
        SHA224(const SHA224&) = delete;
        SHA224& operator=(SHA224&&) = delete;
        SHA224& operator=(const SHA224&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
            ASSERT(_context.index <= sizeof(_context.block));
        }

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
        Context _context;
        mutable bool _computed; // Is the digest computed?
    };

    class EXTERNAL SHA512 {
    public:
        typedef Crypto::Context Context;

    public:
        SHA512(SHA512&&) = delete;
        SHA512(const SHA512&) = delete;
        SHA512& operator=(SHA512&&) = delete;
        SHA512& operator=(const SHA512&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
            ASSERT(_context.index <= sizeof(_context.block));
        }

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
    public:
        typedef Crypto::Context Context;

    public:
        SHA384(SHA384&&) = delete;
        SHA384(const SHA384&) = delete;
        SHA384& operator=(SHA384&&) = delete;
        SHA384& operator=(const SHA384&) = delete;

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

        inline const Context& CurrentContext() const
        {
            return _context;
        }

        inline void Load(const Context& context)
        {
            _context = context;
            ASSERT(_context.index <= sizeof(_context.block));
        }

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
        Context _context;
        mutable bool _computed; // Is the digest computed?
    };
}
}

#endif // HASH_H

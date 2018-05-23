#ifndef __HMAC_H
#define __HMAC_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "Hash.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Crypto {
    template <typename HASHALGORITHM>
    class HMACType {
    private:
        HMACType();
        HMACType(const HMACType&);
        HMACType& operator=(const HMACType&);

    public:
        HMACType(const string& key)
            : _algorithm()
        {
            uint8_t keyLength;
            const uint8_t* encryptionKey;
            HASHALGORITHM hashKey;

            if (key.length() > sizeof(_innerKeyPad)) {
                keyLength = HASHALGORITHM::Length;

                // Calculate the Hash over the key to use that i.s.o. the actual key.
                hashKey.Input(reinterpret_cast<const uint8_t*>(key.c_str()), static_cast<uint16_t>(key.length()));
                encryptionKey = hashKey.Result();
            }
            else {
                keyLength = static_cast<uint8_t>(key.length());

                // Use the key as is..
                encryptionKey = reinterpret_cast<const uint8_t*>(key.c_str());
            }

            // We have a suitable key, move it to the inner and outer pads
            ::memset(&_innerKeyPad[keyLength], 0x36, sizeof(_innerKeyPad) - keyLength);
            ::memset(&_outerKeyPad[keyLength], 0x5C, sizeof(_outerKeyPad) - keyLength);

            /* XOR key with inner keypad and outer key pad values */
            for (uint8_t index = 0; index < keyLength; index++) {
                _innerKeyPad[index] = encryptionKey[index] ^ 0x36;
                _outerKeyPad[index] = encryptionKey[index] ^ 0x5c;
            }

            // Reset the algorithm. We start from scratch..
            _algorithm.Input(_innerKeyPad, sizeof(_innerKeyPad));
            _computed = false;
        }
        ~HMACType()
        {
        }

    public:
        static const EnumHashType Type = HASHALGORITHM::Type;
        static const uint8_t Length = HASHALGORITHM::Length;
        inline static uint8_t BlockLength()
        {
            return (sizeof(_innerKeyPad));
        }
        void Reset()
        {
            _algorithm.Reset();
            _algorithm.Input(_innerKeyPad, sizeof(_innerKeyPad));
            _computed = false;
        }
        const uint8_t* Result()
        {
            if (_computed == false) {
                uint8_t hashKey[HASHALGORITHM::Length];

                _computed = true;

                ::memcpy(hashKey, _algorithm.Result(), sizeof(hashKey));

                // Now use the newly generated key to calulate the outer value..
                _algorithm.Reset();
                _algorithm.Input(_outerKeyPad, sizeof(_outerKeyPad));
                _algorithm.Input(hashKey, sizeof(hashKey));
            }

            return _algorithm.Result();
        }

        /*
         *  Provide input to HMACType
         */
        inline void Input(const uint8_t message_array[], const uint16_t length)
        {
            _algorithm.Input(message_array, length);
        }

        inline HMACType<HASHALGORITHM>& operator<<(const uint8_t message_array[])
        {
            uint16_t length = 0;

            while (message_array[length] != '\0') {
                length++;
            }

            _algorithm.Input(&message_array, length);

            return (*this);
        }

        inline HMACType<HASHALGORITHM>& operator<<(const uint8_t message_element)
        {
            _algorithm.Input(&message_element, 1);

            return (*this);
        }

    private:
        bool _computed;
        uint8_t _innerKeyPad[64];
        uint8_t _outerKeyPad[64];
        HASHALGORITHM _algorithm;
    };

    typedef HMACType<Crypto::SHA1> SHA1HMAC;
    typedef HMACType<Crypto::SHA224> SHA224HMAC;
    typedef HMACType<Crypto::SHA256> SHA256HMAC;
    typedef HMACType<Crypto::SHA384> SHA384HMAC;
    typedef HMACType<Crypto::SHA512> SHA512HMAC;
    typedef HMACType<Crypto::MD5> MD5HMAC;
}
}

#endif // __HMAC_H

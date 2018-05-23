#ifndef __ASN1_H
#define __ASN1_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Portability.h"
#include "Module.h"
#include "DataStorage.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Core {
    class HeapStorage {
    private:
        HeapStorage();

    public:
        HeapStorage(const uint16_t length)
            : _length(length)
            , _buffer(new uint8_t[length])
        {
        }
        HeapStorage(const HeapStorage& copy)
            : _length(copy._length)
            , _buffer(new uint8_t[copy._length])
        {
        }
        ~HeapStorage()
        {
        }

        HeapStorage& operator=(const HeapStorage& RHS)
        {
            if (RHS.Size() >= _length) {
                // We can increase....
                delete[] _buffer;
                _buffer = new uint8_t[RHS.Size()];
                _length = RHS._length;
            }
            ::memcpy(_buffer, RHS._buffer, RHS.Size());
            return (*this);
        }

    public:
        inline uint32_t Size() const
        {
            return (_length);
        }
        inline uint8_t& operator[](const uint32_t index)
        {
            ASSERT((_buffer != nullptr) && (index < _length));
            return (_buffer[index]);
        }
        inline const uint8_t& operator[](const uint32_t index) const
        {
            ASSERT((_buffer != nullptr) && (index < _length));

            return (_buffer[index]);
        }

    private:
        uint32_t _length;
        uint8_t* _buffer;
    };

    template <const unsigned int STACKSIZE>
    class StackStorage {
    private:
        StackStorage();

    public:
        StackStorage(const uint16_t length)
        {
            ASSERT(length <= STACKSIZE);
        }
        StackStorage(const HeapStorage& copy)
        {
            ASSERT(copy.Size() <= STACKSIZE);
        }
        ~StackStorage()
        {
        }

        StackStorage& operator=(const StackStorage& RHS)
        {
            ASSERT(RHS.Size() <= STACKSIZE);

            return (*this);
        }

    public:
        inline uint32_t Size() const
        {
            return (STACKSIZE);
        }
        inline uint8_t& operator[](const uint32_t index)
        {
            ASSERT(index < STACKSIZE);
            return (_buffer[index]);
        }
        inline const uint8_t& operator[](const uint32_t index) const
        {
            ASSERT(index < STACKSIZE);

            return (_buffer[index]);
        }

    private:
        uint8_t _buffer[STACKSIZE];
    };

    template <typename STORAGE>
    class OIDType {
    public:
        template <typename COPYSTORAGE>
        class IteratorType {
        public:
            IteratorType()
                : _length(0)
                , _index(0xFFFF)
                , _buffer(0)
            {
            }
            IteratorType(const COPYSTORAGE& copy, const uint16_t length)
                : _length(length)
                , _index(0xFFFF)
                , _buffer(copy)
            {
            }
            IteratorType(const IteratorType<COPYSTORAGE>& copy)
                : _length(copy._length)
                , _index(copy._index)
                , _buffer(copy._buffer)
            {
            }
            ~IteratorType()
            {
            }

            IteratorType<COPYSTORAGE>& operator=(const IteratorType<COPYSTORAGE>& RHS)
            {
                _length = RHS._length;
                _index = RHS._index;
                _buffer = RHS._buffer;
                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return ((_index == 0xFFFE) && (_index < _length));
            }
            inline void Reset()
            {
                _index = 0xFFFF;
            }
            inline bool Next()
            {
                if (_index == 0xFFFF) {
                    _index = 0xFFFE;
                }
                else if (_index == 0xFFFE) {
                    _index = 0;
                }
                else
                    while ((_index < _length) && ((_buffer[_index] & 0x80) != 0)) {
                        _index++;
                    }

                return (IsValid());
            }
            inline uint16_t Number() const
            {
                ASSERT(IsValid());

                if (_index == 0xFFFE) {
                    return (_buffer[0] / 40);
                }
                else if (_index == 0) {
                    return (_buffer[0] % 40);
                }
                else if (_buffer[_index] <= 127) {
                    return (_buffer[_index]);
                }

                return (((_buffer[_index] & 0x7F) << 7) | (_buffer[_index + 1] & 0x7F));
            }
            inline uint16_t Count() const
            {
                uint16_t result = 0;
                uint16_t index = 1;

                while (index < _length) {
                    if (_buffer[index] > 127) {
                        index++;
                    }
                    index++;
                    result++;
                }

                return (2 + result);
            }

        private:
            uint16_t _length;
            uint16_t _index;
            COPYSTORAGE _buffer;
        };

    public:
        OIDType(const uint8_t identifier[], const uint16_t length)
            : _buffer(length)
        {
            CopyBuffer(identifier, length);
        }
        OIDType(const string& OID, const uint16_t bufferLength = 256)
            : _buffer(bufferLength)
            , _length(0)
        {
            uint16_t firstSet = 0;
            uint16_t index = 0;
            uint32_t value = 0;
            uint16_t digits = 0;
            uint8_t base = 10;

            ASSERT(_buffer.length > 0);

            // Encode the string
            while ((index < OID.length()) && (_length != 0xFFFF)) {
                if (OID[index] == '.') {
                    if (_length == 0) {
                        _length = 1;
                        firstSet = value;
                    }
                    else if (_length == 1) {
                        _length = ((value < 40) && (firstSet < 7) ? 1 : 0xFFFF);
                        _buffer[0] = (firstSet * 40) + value;
                    }
                    else {
                        _length = ((value <= 127 ? (_length < (_buffer.length - 1)) : (_length < (_buffer.length - 2))) ? _length : 0xFFFF);

                        if (_length != 0xFFFF) {
                            if (value <= 127) {
                                _buffer[_length] = value;
                                _length++;
                            }
                            else {
                                _buffer[_length] = 0x80 | ((value >> 7) & 0xFF);
                                _length++;
                                _buffer[_length] = (value & 0x7F);
                                _length++;

                                _length = (value <= 0x7FFF ? _length : 0xFFFF);
                            }
                        }
                    }
                    // Store it we have a digit
                    value = 0;
                    digits = 0;
                    base = 10;
                }
                else if (toupper(OID[index]) == 'X') {
                    if ((value == 0) && (digits == 1)) {
                        base = 16;
                    }
                }
                else if (isdigit(OID[index])) {
                    value = (value * base) + (OID[index] - '0');
                }
                else if ((base > 10) && (toupper(OID[index]) >= 'A') && (toupper(OID[index]) <= 'F')) {
                    value = (value * base) + (toupper(OID[index]) - 'A' + 10);
                }

                digits++;
                index++;
            }
        }
        OIDType(const OIDType<STORAGE>& copy)
            : _buffer(copy._buffer)
        {
            CopyBuffer(&(copy._buffer[0]), copy._length);
        }
        ~OIDType()
        {
        }

        OIDType<STORAGE>& operator=(const OIDType<STORAGE>& RHS)
        {
            _buffer = RHS.Buffer;

            return (*this);
        }

    public:
        inline IteratorType<STORAGE> Iterator() const
        {
            return (IteratorType<STORAGE>(_buffer, _length));
        }
        inline uint16_t Length() const
        {
            return (_length);
        }
        inline const uint8_t* Buffer() const
        {
            return (&(_buffer[0]));
        }
        inline string Text() const
        {
            string result;
            IteratorType<STORAGE> index(_buffer, _length);

            while (index.Next() == true) {
                string textValue;
                uint16_t value = index.Number();

                while (value > 0) {
                    textValue += static_cast<char>((value % 10) + '0') + textValue;
                    value /= 10;
                }

                if (result.empty() == true) {
                    result = textValue;
                }
                else {
                    result = result + '.' + textValue;
                }
            }
            return (result);
        }
        inline bool operator==(const OIDType<STORAGE>& RHS) const
        {
            return (RHS._length == _length ? (memcmp(&(_buffer[0]), &(RHS._buffer[0]), _length) == 0) : false);
        }
        inline bool operator!=(const OIDType<STORAGE>& RHS) const
        {
            return (!operator==(RHS));
        }

    private:
        inline void CopyBuffer(const uint8_t identifier[], const uint16_t length)
        {
            ASSERT(_buffer.Length() >= length);
            memcpy(_buffer, &(identifier[0]), length);
            _length = length;
        }

    private:
        STORAGE _buffer;
        uint16_t _length;
    };

    typedef OIDType<StackStorage<256> > StackOID;

    template <typename STORAGE>
    class ASN1SequenceType {
    public:
        /**
         * name ASN1 Error codes
         * These error codes are OR'ed to X509 error codes for
         * higher error granularity.
         * ASN1 is a standard to specify data structures.
         */
        enum enumError {
            ASN1_OK = 0x00,
            ASN1_OUT_OF_DATA = 0x60, /**< Out of data when parsing an ASN1 data structure. */
            ASN1_UNEXPECTED_TAG = 0x62, /**< ASN1 tag was of an unexpected value. */
            ASN1_INVALID_LENGTH = 0x64, /**< Error when trying to determine the length or invalid length. */
            ASN1_LENGTH_MISMATCH = 0x66, /**< Actual length differs from expected length. */
            ASN1_INVALID_DATA = 0x68, /**< Data is invalid. (not used) */
            ASN1_MALLOC_FAILED = 0x6A, /**< Memory allocation failed */
            ASN1_BUF_TOO_SMALL = 0x6C /**< Buffer too small when writing ASN.1 data structure. */
        };
        /**
         * name DER constants
         * These constants comply with DER encoded the ANS1 type tags.
         * DER encoding uses hexadecimal representation.
         * An example DER sequence is:\n
         * - 0x02 -- tag indicating INTEGER
         * - 0x01 -- length in octets
         * - 0x05 -- value
         * Such sequences are typically read into \c ::x509_buf.
         */
        enum enumType {
            ASN1_BOOLEAN = 0x01,
            ASN1_INTEGER = 0x02,
            ASN1_BIT_STRING = 0x03,
            ASN1_OCTET_STRING = 0x04,
            ASN1_NULL = 0x05,
            ASN1_OID = 0x06,
            ASN1_UTF8_STRING = 0x0C,
            ASN1_SEQUENCE = 0x10,
            ASN1_SET = 0x11,
            ASN1_PRINTABLE_STRING = 0x13,
            ASN1_T61_STRING = 0x14,
            ASN1_IA5_STRING = 0x16,
            ASN1_UTC_TIME = 0x17,
            ASN1_GENERALIZED_TIME = 0x18,
            ASN1_UNIVERSAL_STRING = 0x1C,
            ASN1_BMP_STRING = 0x1E,
            ASN1_PRIMITIVE = 0x00,
            ASN1_CONSTRUCTED = 0x20,
            ASN1_CONTEXT_SPECIFIC = 0x80
        };

    public:
        ASN1SequenceType()
            : _buffer(0)
            , _index(0)
            , _length(0)
        {
        }
        ASN1SequenceType(const uint8_t buffer[], const uint16_t length)
            : _buffer(length)
            , _index(0)
            , _length(0)
        {
            CopyBuffer(&(buffer[0]), length);
        }
        ASN1SequenceType(const ASN1SequenceType<STORAGE>& copy)
            : _buffer(copy._buffer)
            , _index(copy._index)
            , _length(copy._length)
        {
            CopyBuffer(&(copy._buffer[0]), copy.Length());
        }
        ~ASN1SequenceType()
        {
        }

        ASN1SequenceType<STORAGE>& operator=(const ASN1SequenceType<STORAGE>& RHS)
        {
            _buffer = RHS._buffer;

            return (*this);
        }

    public:
        inline void Reset()
        {
            _index = 0;
        }
        inline bool IsValid() const
        {
            return ((_index > 0) && (_index < _length));
        }
        inline bool Next()
        {
            if (_index == 0) {
                _index++;
            }
            else if (_index < _length) {
                // Time to jump over the section, if possible.
                _index += (Length() + 1);
            }

            return (IsValid() == true);
        }
        inline enumType Tag() const
        {
            ASSERT(IsValid() == true);

            return (static_cast<enumType>(_buffer[_index - 1]));
        }
        inline uint8_t Length() const
        {
            ASSERT(IsValid() == true);

            return (static_cast<enumType>(_buffer[_index]));
        }
        //-----------------------------------------------------
        // BOOLEAN to BOOL VALUE (8Bits)
        //-----------------------------------------------------
        inline enumError Value(bool& value) const
        {
            ASSERT(Tag() == ASN1_BOOLEAN);
            ASSERT(Length() == 1);

            value = (_buffer[_index + 1] != 0);

            return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        //-----------------------------------------------------
        // INTEGER to SCALAR VALUE (8Bits)
        //-----------------------------------------------------
        inline enumError Value(signed char& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        inline enumError Value(unsigned char& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        //-----------------------------------------------------
        // INTEGER to SCALAR VALUE (16Bits)
        //-----------------------------------------------------
        inline enumError Value(signed short& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            return (Length() <= 2 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        inline enumError Value(unsigned short& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            return (Length() <= 2 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        //-----------------------------------------------------
        // INTEGER to SCALAR VALUE (32Bits)
        //-----------------------------------------------------
        inline enumError Value(unsigned int& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            if (Length() > 2) {
                value = (value << 8) | _buffer[_index + 3];
            }
            if (Length() > 3) {
                value = (value << 8) | _buffer[_index + 4];
            }
            return (Length() <= 4 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        inline enumError Value(signed int& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            if (Length() > 2) {
                value = (value << 8) | _buffer[_index + 3];
            }
            if (Length() > 3) {
                value = (value << 8) | _buffer[_index + 4];
            }
            return (Length() <= 4 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        //-----------------------------------------------------
        // INTEGER to SCALAR VALUE (64Bits)
        //-----------------------------------------------------
        inline enumError Value(unsigned long long& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            if (Length() > 2) {
                value = (value << 8) | _buffer[_index + 3];
            }
            if (Length() > 3) {
                value = (value << 8) | _buffer[_index + 4];
            }
            if (Length() > 4) {
                value = (value << 8) | _buffer[_index + 5];
            }
            if (Length() > 5) {
                value = (value << 8) | _buffer[_index + 6];
            }
            if (Length() > 6) {
                value = (value << 8) | _buffer[_index + 7];
            }
            if (Length() > 7) {
                value = (value << 8) | _buffer[_index + 8];
            }
            return (Length() <= 8 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        inline enumError Value(signed long long& value) const
        {
            ASSERT(Tag() == ASN1_INTEGER);
            value = _buffer[_index + 1];
            if (Length() > 1) {
                value = (value << 8) | _buffer[_index + 2];
            }
            if (Length() > 2) {
                value = (value << 8) | _buffer[_index + 3];
            }
            if (Length() > 3) {
                value = (value << 8) | _buffer[_index + 4];
            }
            if (Length() > 4) {
                value = (value << 8) | _buffer[_index + 5];
            }
            if (Length() > 5) {
                value = (value << 8) | _buffer[_index + 6];
            }
            if (Length() > 6) {
                value = (value << 8) | _buffer[_index + 7];
            }
            if (Length() > 7) {
                value = (value << 8) | _buffer[_index + 8];
            }
            return (Length() <= 4 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
        }
        //-----------------------------------------------------
        // OID to OID class
        //-----------------------------------------------------
        template <typename MYSTORAGE>
        inline enumError Value(OIDType<MYSTORAGE>& value) const
        {
            ASSERT(Tag() == ASN1_OID);
            value = OIDType<MYSTORAGE>(&_buffer[_index + 1], Length());

            return (ASN1_OK);
        }
        //-----------------------------------------------------
        // UTF8String to string class
        //-----------------------------------------------------
        inline enumError Value(string& value) const
        {
            ASSERT(Tag() == ASN1_UTF8_STRING);
            value = string(reinterpret_cast<const char*>(&_buffer[_index + 1]), Length());

            return (ASN1_OK);
        }
        //-----------------------------------------------------
        // SEQUENCE to ASN1Sequence class
        //-----------------------------------------------------
        template <typename MYSTORAGE>
        inline enumError Value(ASN1SequenceType<MYSTORAGE>& value) const
        {
            ASSERT(Tag() == ASN1_SEQUENCE);
            value = ASN1SequenceType<MYSTORAGE>(&(_buffer[_index + 1]), Length());

            return (ASN1_OK);
        }

    private:
        inline void CopyBuffer(const uint8_t identifier[], const uint16_t length)
        {
            ASSERT(_buffer.Length() >= length);
            memcpy(_buffer, &(identifier[0]), length);
            _length = length;
        }

    private:
        STORAGE _buffer;
        uint32_t _index;
        uint32_t _length;
    };
}
}

#endif // __ASN1_H

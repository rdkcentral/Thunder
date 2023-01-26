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

#ifndef __ASN1_H
#define __ASN1_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"
#include "Portability.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----
namespace WPEFramework {
namespace Core {
    namespace ASN1 {

        // ===================================================================
        // NOT THREAD SAFE !!!!! on RELEASE!!!!!
        // ===================================================================
        class Buffer {
        private:
            static constexpr uint8_t AdminSize = 5;

        public:
            Buffer()
                : _buffer(nullptr)
            {
            }
            Buffer(const uint16_t length)
                : _buffer(length == 0 ? nullptr : new uint8_t[length + AdminSize])
            {
                if (_buffer != nullptr) {
                    _buffer[0] = 1;
                    _buffer[1] = (length & 0xFF);
                    _buffer[2] = ((length >> 8) && 0xFF);
                    _buffer[3] = (length & 0xFF);
                    _buffer[4] = ((length >> 8) && 0xFF);
                }
            }
            Buffer(const Buffer& copy)
                : _buffer(copy._buffer)
            {
                AddRef();
            }
            ~Buffer()
            {
                Release();
            }

            Buffer& operator=(const Buffer& RHS)
            {
                if (&RHS != this) {
                    Release();
                    _buffer = RHS._buffer;
                    AddRef();
                }
                return (*this);
            }

        public:
            inline void Size(const uint16_t length)
            {
                ASSERT((_buffer != nullptr) && (length < ((_buffer[3] << 8) | _buffer[4])));

                if (_buffer != nullptr) {
                    _buffer[1] = (length & 0xFF);
                    _buffer[2] = ((length >> 8) && 0xFF);
                }
            }
            inline uint16_t Size() const
            {
                return (_buffer != nullptr ? (_buffer[2] << 8) | _buffer[1] : 0);
            }
            inline uint8_t& operator[](const uint32_t index)
            {
                ASSERT((_buffer != nullptr) && (index < Size()));
                return (_buffer[index + AdminSize]);
            }
            inline const uint8_t& operator[](const uint32_t index) const
            {
                ASSERT((_buffer != nullptr) && (index < Size()));

                return (_buffer[index + AdminSize]);
            }

        private:
            void AddRef()
            {
                if (_buffer != nullptr) {
                    _buffer[0] = _buffer[0] + 1;
                }
            }
            void Release()
            {
                if (_buffer != nullptr) {
                    if (_buffer[0] == 1) {
                        delete _buffer;
                    } else {
                        _buffer[0] = _buffer[0] - 1;
                    }
                }
            }

        private:
            uint8_t* _buffer;
        };

        class OID {
        public:
            class Iterator {
            public:
                Iterator()
                    : _length(0)
                    , _index(0xFFFF)
                    , _buffer(0)
                {
                }
                Iterator(const uint8_t* buffer, const uint16_t length)
                    : _length(length)
                    , _index(0xFFFF)
                    , _buffer(buffer)
                {
                }
                Iterator(const Iterator& copy)
                    : _length(copy._length)
                    , _index(copy._index)
                    , _buffer(copy._buffer)
                {
                }
                ~Iterator()
                {
                }

                Iterator& operator=(const Iterator& RHS)
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
                    } else if (_index == 0xFFFE) {
                        _index = 0;
                    } else
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
                    } else if (_index == 0) {
                        return (_buffer[0] % 40);
                    } else if (_buffer[_index] <= 127) {
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
                const uint8_t* _buffer;
            };

        public:
            OID(const uint8_t identifier[], const uint16_t length)
                : _length(length > sizeof(_buffer) ? sizeof(_buffer) : length)
            {
                ::memcpy(_buffer, identifier, _length);
            }
            OID(const string& OID)
                : _length(0)
            {
                _buffer[0] = 0;
                uint16_t firstSet = ~0;
                uint16_t index = 0;
                uint32_t value = 0;
                uint16_t digits = 0;
                uint8_t base = 10;

                // Decode the string
                while (index < OID.length()) {
                    if (OID[index] == '.') {
                        if (firstSet == static_cast<uint16_t>(~0)) {
                            if (firstSet >= 7) {
                                break;
                            }
                            firstSet = value;
                        } else if (_length == 0) {
                            if (value >= 40) {
                                break;
                            }

                            _buffer[0] = (firstSet * 40) + value;
                            _length++;
                        } else if (_length >= (sizeof(_buffer) - (value <= 127 ? 1 : 2))) {
                            break;
                        } else if (value <= 127) {
                            _buffer[_length] = value;
                            _length++;
                        } else {
                            _buffer[_length] = 0x80 | ((value >> 7) & 0xFF);
                            _length++;
                            _buffer[_length] = (value & 0x7F);
                            _length++;
                        }
                        // Store it we have a digit
                        value = 0;
                        digits = 0;
                        base = 10;
                    } else if (toupper(OID[index]) == 'X') {
                        if ((value == 0) && (digits == 1)) {
                            base = 16;
                        }
                    } else if (isdigit(OID[index])) {
                        value = (value * base) + (OID[index] - '0');
                    } else if ((base > 10) && (toupper(OID[index]) >= 'A') && (toupper(OID[index]) <= 'F')) {
                        value = (value * base) + (toupper(OID[index]) - 'A' + 10);
                    }

                    digits++;
                    index++;
                }
            }
            OID(const OID& copy)
            {
                ::memcpy(_buffer, copy._buffer, copy._length);
                _length = copy._length;
            }
            ~OID()
            {
            }

            OID& operator=(const OID& RHS)
            {
                ::memcpy(_buffer, RHS._buffer, RHS._length);
                _length = RHS._length;

                return (*this);
            }

        public:
            inline Iterator Elements() const
            {
                return (Iterator(_buffer, _length));
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
                Iterator index(_buffer, _length);

                while (index.Next() == true) {
                    string textValue;
                    uint16_t value = index.Number();

                    while (value > 0) {
                        textValue = static_cast<char>((value % 10) + '0') + textValue;
                        value /= 10;
                    }

                    if (result.empty() == true) {
                        result = textValue;
                    } else {
                        result = result + '.' + textValue;
                    }
                }
                return (result);
            }
            inline bool operator==(const OID& RHS) const
            {
                return (RHS._length == _length ? (memcmp(&(_buffer[0]), &(RHS._buffer[0]), _length) == 0) : false);
            }
            inline bool operator!=(const OID& RHS) const
            {
                return (!operator==(RHS));
            }

        private:
            uint8_t _buffer[255];
            uint16_t _length;
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
            TYPE_BOOLEAN = 0x01,
            TYPE_INTEGER = 0x02,
            TYPE_BIT_STRING = 0x03,
            TYPE_OCTET_STRING = 0x04,
            TYPE_NULL = 0x05,
            TYPE_OID = 0x06,
            TYPE_UTF8_STRING = 0x0C,
            TYPE_SEQUENCE = 0x10,
            TYPE_SET = 0x11,
            TYPE_PRINTABLE_STRING = 0x13,
            TYPE_T61_STRING = 0x14,
            TYPE_IA5_STRING = 0x16,
            TYPE_UTC_TIME = 0x17,
            TYPE_GENERALIZED_TIME = 0x18,
            TYPE_UNIVERSAL_STRING = 0x1C,
            TYPE_BMP_STRING = 0x1E,
            TYPE_PRIMITIVE = 0x00,
            TYPE_CONSTRUCTED = 0x20,
            TYPE_CONTEXT_SPECIFIC = 0x80
        };
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

        class Sequence {
        public:
            /**
         * name ASN1 Error codes
         * These error codes are OR-ed to X509 error codes for
         * higher error granularity.
         * ASN1 is a standard to specify data structures.
         */
        public:
            Sequence()
                : _buffer(0)
                , _index(0)
                , _length(0)
            {
            }
            Sequence(const Buffer& buffer, const uint16_t index = 0, const uint16_t length = ~0)
                : _buffer(buffer)
                , _index(index)
                , _length(length == static_cast<uint16_t>(~0) ? buffer.Size() : length)
            {
            }
            Sequence(const Sequence& copy)
                : _buffer(copy._buffer)
                , _index(copy._index)
                , _length(copy._length)
            {
            }
            ~Sequence()
            {
            }

            Sequence& operator=(const Sequence& RHS)
            {

                _buffer = RHS._buffer;
                _index = RHS._index;
                _length = RHS._length;

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
                } else if (_index < _length) {
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
            inline const uint8_t* Data() const
            {
                ASSERT(IsValid() == true);

                return (&(_buffer[_index + 1]));
            }

            //-----------------------------------------------------
            // BOOLEAN to BOOL VALUE (8Bits)
            //-----------------------------------------------------
            inline enumError Value(bool& value) const
            {
                ASSERT(Tag() == TYPE_BOOLEAN);
                ASSERT(Length() == 1);

                value = (_buffer[_index + 1] != 0);

                return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
            }
            //-----------------------------------------------------
            // INTEGER to SCALAR VALUE (8Bits)
            //-----------------------------------------------------
            inline enumError Value(signed char& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
                value = _buffer[_index + 1];
                return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
            }
            inline enumError Value(unsigned char& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
                value = _buffer[_index + 1];
                return (Length() <= 1 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
            }
            //-----------------------------------------------------
            // INTEGER to SCALAR VALUE (16Bits)
            //-----------------------------------------------------
            inline enumError Value(int16_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
                value = _buffer[_index + 1];
                if (Length() > 1) {
                    value = (value << 8) | _buffer[_index + 2];
                }
                return (Length() <= 2 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
            }
            inline enumError Value(uint16_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
                value = _buffer[_index + 1];
                if (Length() > 1) {
                    value = (value << 8) | _buffer[_index + 2];
                }
                return (Length() <= 2 ? ASN1_OK : ASN1_BUF_TOO_SMALL);
            }
            //-----------------------------------------------------
            // INTEGER to SCALAR VALUE (32Bits)
            //-----------------------------------------------------
            inline enumError Value(uint32_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
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
            inline enumError Value(int32_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
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
            inline enumError Value(uint64_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
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
            inline enumError Value(int64_t& value) const
            {
                ASSERT(Tag() == TYPE_INTEGER);
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
            inline enumError Value(OID& value) const
            {
                ASSERT(Tag() == TYPE_OID);
                value = OID(&_buffer[_index + 1], Length());

                return (ASN1_OK);
            }
            //-----------------------------------------------------
            // UTF8String to string class
            //-----------------------------------------------------
            inline enumError Value(string& value) const
            {
                ASSERT(Tag() == TYPE_UTF8_STRING);
                value = string(reinterpret_cast<const char*>(&_buffer[_index + 1]), Length());

                return (ASN1_OK);
            }
            //-----------------------------------------------------
            // SEQUENCE to ASN1Sequence class
            //-----------------------------------------------------
            inline enumError Value(Sequence& value) const
            {
                ASSERT(Tag() == TYPE_SEQUENCE);
                value = Sequence(_buffer, _index, Length());

                return (ASN1_OK);
            }

        private:
            Buffer _buffer;
            uint32_t _index;
            uint32_t _length;
        };
    }
}
}

#endif // __ASN1_H

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

#pragma once

#include "Module.h"
#include "Portability.h"
#include "Frame.h"

namespace WPEFramework {

namespace Core {

    namespace IPC {

        typedef Core::Void Void;

        template <typename SCALAR>
        class ScalarType {
        public:
            inline ScalarType()
                : _data(0)
            {
            }
            inline ScalarType(const SCALAR data)
                : _data(data)
            {
            }
            inline ScalarType(const ScalarType<SCALAR>& copy)
                : _data(copy._data)
            {
            }
            inline ~ScalarType() {}

            inline ScalarType<SCALAR>& operator=(const ScalarType<SCALAR>& rhs)
            {
                _data = rhs._data;
                return (*this);
            }
            inline ScalarType<SCALAR>& operator=(const SCALAR rhs)
            {
                _data = rhs;
                return (*this);
            }
            inline operator SCALAR&()
            {
                return (_data);
            }
            inline operator const SCALAR&() const
            {
                return (_data);
            }
            inline SCALAR Value() const
            {
                return (_data);
            }
            inline void Value(const SCALAR& value)
            {
                _data = value;
            }

        private:
            SCALAR _data;
        };

        template <>
        class ScalarType<string> {
        public:
            ScalarType()
            {
            }
            ScalarType(const string& info)
                : _data(info)
            {
            }
            ScalarType(const ScalarType<string>& copy)
                : _data(copy._data)
            {
            }
            ~ScalarType()
            {
            }

            ScalarType<string>& operator=(const ScalarType<string>& rhs)
            {
                _data = rhs._data;

                return (*this);
            }
            ScalarType<string>& operator=(const string& rhs)
            {
                _data = rhs;

                return (*this);
            }

        public:
            inline uint32_t Length() const
            {
                return (static_cast<uint32_t>(_data.length() * sizeof(TCHAR)));
            }
            inline operator string&()
            {
                return (_data);
            }
            inline operator const string&() const
            {
                return (_data);
            }
            inline const string& Value() const
            {
                return (_data);
            }
            inline uint16_t Serialize(uint8_t buffer[], const uint16_t length, const uint32_t offset) const
            {
                uint16_t size = ((Length() - offset) > length ? length : (Length() - offset));
                ::memcpy(buffer, &(reinterpret_cast<const uint8_t*>(_data.c_str())[offset]), size);

                return (size);
            }
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t length, const uint32_t /* offset */)
            {
                uint16_t teller = 0;

                for (; (teller + sizeof(TCHAR)) <= length; teller += sizeof(TCHAR)) {
                    _data.push_back(*(reinterpret_cast<const TCHAR*>(&(buffer[teller]))));
                }

                return (teller);
            }

        private:
            string _data;
        };

        template <const uint16_t LENGTH>
        class Text {
        public:
            inline Text()
                : _length(0)
            {
                _text[0] = '\0';
            }
            explicit inline Text(const TCHAR text[], const uint16_t length = 0)
                : _length(length == 0 ? _tcslen(text) : length)
            {
                if (_length > 0) {
                    if (length >= LENGTH) {
                        _length = LENGTH - 1;
                    }
                    ::memcpy(_text, text, (_length * sizeof(TCHAR)));
                }
                _text[_length] = '\0';
            }
            explicit inline Text(const string& text)
                : _length(text.length())
            {
                if (_length > 0) {
                    if (_length >= LENGTH) {
                        _length = LENGTH - 1;
                    }
                    ::memcpy(_text, text.c_str(), (_length * sizeof(TCHAR)));
                }
                _text[_length] = '\0';
            }
            inline Text(const Text<LENGTH>& copy)
                : _length(copy._length)
            {
                if (_length > 0) {
                    ::memcpy(_text, copy._text, (_length * sizeof(TCHAR)));
                }
                _text[_length] = '\0';
            }
            inline ~Text()
            {
            }

            inline Text<LENGTH>& operator=(const Text<LENGTH>& RHS)
            {
                _length = RHS._length;

                if (_length > 0) {
                    ::memcpy(_text, RHS._text, _length);
                }
                _text[_length] = '\0';

                return (*this);
            }
            inline Text<LENGTH>& operator=(const string& RHS)
            {
                _length = static_cast<uint16_t>(RHS.length());

                if (_length > 0) {
                    if (_length >= LENGTH) {
                        _length = LENGTH - 1;
                    }
                    ::memcpy(_text, RHS.c_str(), (_length * sizeof(TCHAR)));
                }
                _text[_length] = '\0';

                return (*this);
            }

        public:
            inline uint32_t Length() const
            {
                return (_length);
            }
            inline const TCHAR* Value() const
            {
                return (_text);
            }
            inline operator string() const
            {
                return (string(_text, _length));
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                uint16_t result = 0;

                if (offset < (LENGTH * sizeof(TCHAR))) {
                    result = (maxLength > static_cast<uint16_t>(((LENGTH - 1) * sizeof(TCHAR)) - offset) ? static_cast<uint16_t>(((LENGTH - 1) * sizeof(TCHAR)) - offset) : maxLength);
                    ::memcpy(&(reinterpret_cast<uint8_t*>(&_text)[offset]), stream, result);
                    _length = static_cast<uint16_t>((offset + maxLength) / sizeof(TCHAR));
                    _text[_length] = '\0';
                }
                return (result);
            }

        private:
            TCHAR _text[LENGTH];
            uint16_t _length;
        };

        template <const uint16_t LENGTH>
        class BufferType {
        public:
            BufferType(const BufferType<LENGTH>& copy) = delete;
            BufferType<LENGTH>& operator=(const BufferType<LENGTH>& RHS) = delete;

            inline BufferType()
                : _buffer()
            {
            }
            inline BufferType(const uint16_t length, const uint8_t buffer[])
                : _buffer()
            {
                _buffer.SetBufferType(0, length, buffer);
            }
            inline ~BufferType()
            {
            }

        public:
            inline void Clear() {
                _buffer.Size(0);
            }
            inline uint32_t Length() const
            {
                return (_buffer.Size());
            }
            inline const uint8_t* Value() const
            {
                return (&(_buffer[0]));
            }
            inline void Set (const uint16_t length, const uint8_t buffer[]) {
                _buffer.Copy(0, length, buffer);
            }
            inline uint16_t Serialize(uint8_t buffer[], const uint16_t length, const uint32_t offset) const
            {
                uint16_t size = ((_buffer.Size() - offset) > length ? length : (_buffer.Size() - offset));
                ::memcpy(buffer, &(_buffer[offset]), size);

                return (size);
            }
            uint16_t Deserialize(const uint8_t buffer[], const uint16_t length, const uint32_t offset)
            {
                ASSERT (offset == _buffer.Size());

                _buffer.Size(length + _buffer.Size());

                ::memcpy(&(_buffer[offset]), buffer, length);

                return (length);
            }

        private:
            Core::FrameType<LENGTH> _buffer;
        };
 
    }
}
} // namespace Core


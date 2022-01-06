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

namespace WPEFramework {
namespace Core {

    template <const uint32_t BLOCKSIZE, const bool BIG_ENDIAN_ORDERING = true, typename SIZE_CONTEXT = uint16_t>
    class FrameType {
    private:
        template <const uint32_t STARTSIZE, typename SIZETYPE>
        class AllocatorType {
        public:
            AllocatorType<STARTSIZE, SIZETYPE> operator=(const AllocatorType<STARTSIZE, SIZETYPE>&) = delete;

            AllocatorType()
                : _bufferSize(STARTSIZE)
                , _data(reinterpret_cast<uint8_t*>(::malloc(_bufferSize)))
            {
                // It looks like there is a bug in the windows compiler. It prepares a default/copy constructor
                // if if the template being instantiated is not really utilizing it!
                #ifndef __WINDOWS__
                static_assert(STARTSIZE != 0, "This method can only be called if you specify an initial blocksize");
                #endif  
            }
            AllocatorType(const AllocatorType<STARTSIZE, SIZETYPE>& copy)
                : _bufferSize(copy._bufferSize)
                , _data(reinterpret_cast<uint8_t*>(::malloc(_bufferSize)))
            {
                // It looks like there is a bug in the windows compiler. It prepares a default/copy constructor
                // if if the template being instantiated is not really utilizing it!
                #ifndef __WINDOWS__
                static_assert(STARTSIZE != 0, "This method can only be called if you specify an initial blocksize");
                #endif

                ::memcpy(_data, copy._data, _bufferSize);
            }
            AllocatorType(uint8_t buffer[], const SIZETYPE length)
                : _bufferSize(length)
                , _data(buffer)
            {
                static_assert(STARTSIZE == 0, "This method can only be called if you specify an initial blocksize of 0");
            }
            ~AllocatorType()
            {
                if ((STARTSIZE != 0) && (_data != nullptr)) {
                    ::free(_data);
                }
            }

        public:
            inline uint8_t& operator[](const SIZETYPE index)
            {
                ASSERT(_data != nullptr);
                ASSERT(index < _bufferSize);
                return (_data[index]);
            }
            inline const uint8_t& operator[](const SIZETYPE index) const
            {
                ASSERT(_data != nullptr);
                ASSERT(index < _bufferSize);
                return (_data[index]);
            }
            inline void Allocate(SIZETYPE requiredSize)
            {
                RealAllocate<STARTSIZE>(requiredSize, TemplateIntToType<STARTSIZE>());
            }

        private:
            template <const uint32_t NONZEROSIZE>
            inline void RealAllocate(const SIZETYPE requiredSize, const TemplateIntToType<NONZEROSIZE>& /* For compile time diffrentiation */)
            {
                if (requiredSize > _bufferSize) {

                    SIZETYPE bufferSize = static_cast<uint32_t>(((requiredSize / (STARTSIZE ? STARTSIZE : 1)) + 1) * STARTSIZE);

                    // oops we need to "reallocate".
                    uint8_t* data = reinterpret_cast<uint8_t*>(::realloc(_data, bufferSize));

                    if (data != nullptr) {
                        _data = data;
                        _bufferSize = bufferSize;
                    }
                }
            }
            inline void RealAllocate(const SIZETYPE requiredSize, const TemplateIntToType<0>& /* For compile time diffrentiation */)
            {
                ASSERT(requiredSize <= _bufferSize);
            }

        private:
            SIZETYPE _bufferSize;
            uint8_t* _data;
        };

    public:
        class Reader {
        public:
            Reader& operator=(const Reader&) = delete;

            Reader()
                : _offset(0)
                , _container(nullptr)
            {
            }
            Reader(const FrameType& data, const SIZE_CONTEXT offset)
                : _offset(offset)
                , _container(&data)
            {
            }
            Reader(const Reader& copy)
                : _offset(copy._offset)
                , _container(copy._container)
            {
            }
            ~Reader() = default;

        public:
            inline bool HasData() const
            {
                return ((_container != nullptr) && (_offset < _container->Size()));
            }
            inline uint32_t Length() const
            {
                return (_container == nullptr ? 0 : _container->Size() - _offset);
            }
            template <typename TYPENAME>
            TYPENAME LockBuffer(const uint8_t*& buffer) const
            {
                TYPENAME result;

                ASSERT(_container != nullptr);

                _offset += _container->GetNumber<TYPENAME>(_offset, result);
                buffer = &(_container->operator[](_offset));

                return (result);
            }
            template <typename TYPENAME>
            void UnlockBuffer(TYPENAME length) const
            {
                ASSERT(_container != nullptr);

                _offset += length;
            }
            template <typename TYPENAME>
            TYPENAME Buffer(const TYPENAME maxLength, uint8_t buffer[]) const
            {
                uint32_t result;

                ASSERT(_container != nullptr);

                result = _container->GetBuffer<TYPENAME>(_offset, maxLength, buffer);
                _offset += result;

                return (static_cast<TYPENAME>(result - sizeof(TYPENAME)));
            }
            void Copy(const SIZE_CONTEXT length, uint8_t buffer[]) const
            {
                ASSERT(_container != nullptr);

                _offset += _container->Copy(_offset, length, buffer);
            }
            template <typename TYPENAME>
            TYPENAME Number() const
            {
                TYPENAME result;

                ASSERT(_container != nullptr);

                _offset += _container->GetNumber<TYPENAME>(_offset, result);

                return (result);
            }
            bool Boolean() const
            {
                bool result;

                ASSERT(_container != nullptr);

                _offset += _container->GetBoolean(_offset, result);

                return (result);
            }
            string Text() const
            {
                string result;

                ASSERT(_container != nullptr);

                _offset += _container->GetText(_offset, result);

                return (result);
            }
            string NullTerminatedText() const
            {
                string result;

                ASSERT(_container != nullptr);

                _offset += _container->GetNullTerminatedText(_offset, result);

                return (result);
            }
            const uint8_t* Data() const {

                ASSERT(_container != nullptr);

                return (&(_container->Data()[_offset]));
            }
            void Forward (const SIZE_CONTEXT skip) {
                ASSERT(skip <= Length());
                _offset += skip;
            }

#ifdef __DEBUG__
            void Dump() const
            {
                _container->Dump(_offset);
            }
#endif

        private:
            mutable SIZE_CONTEXT _offset;
            const FrameType* _container;
        };
        class Writer {
        public:
            Writer& operator=(const Writer&) = delete;

            Writer()
                : _offset(0)
                , _container(nullptr)
            {
            }
            // TODO: should we make offset 0 by default?
            Writer(FrameType& data, const uint32_t offset)
                : _offset(offset)
                , _container(&data)
            {
            }
            Writer(const Writer& copy)
                : _offset(copy._offset)
                , _container(copy._container)
            {
            }
            ~Writer() = default;

        public:
            inline SIZE_CONTEXT Offset() const
            {
                return (_offset);
            }
            template <typename TYPENAME>
            void Buffer(const TYPENAME length, const uint8_t buffer[])
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetBuffer<TYPENAME>(_offset, length, buffer);
            }
            void Copy(const SIZE_CONTEXT length, const uint8_t buffer[])
            {
                ASSERT(_container != nullptr);

                _offset += _container->Copy(_offset, length, buffer);
            }
            template <typename TYPENAME>
            void Number(const TYPENAME value)
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetNumber<TYPENAME>(_offset, value);
            }
            void Boolean(const bool value)
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetBoolean(_offset, value);
            }
            void Text(const string& text)
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetText(_offset, text);
            }
            void NullTerminatedText(const string& text)
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetNullTerminatedText(_offset, text);
            }

        private:
            SIZE_CONTEXT _offset;
            FrameType* _container;
        };

    public:       
        FrameType()
            : _size(0)
            , _data() {
            // It looks like there is a bug in the windows compiler. It prepares a default/copy constructor
            // if if the template being instantiated is not really utilizing it!
            #ifndef __WINDOWS__
            static_assert(BLOCKSIZE != 0, "This method can only be called if you specify an initial blocksize");
            #endif
        }
        FrameType(const FrameType<BLOCKSIZE, BIG_ENDIAN_ORDERING, SIZE_CONTEXT>& copy) 
            : _size(copy._size)
            , _data(copy._data)
        {
            // It looks like there is a bug in the windows compiler. It prepares a default/copy constructor
            // if if the template being instantiated is not really utilizing it!
            #ifndef __WINDOWS__
            static_assert(BLOCKSIZE != 0, "This method can only be called if you allocate a new buffer");
            #endif
        }
        FrameType(uint8_t* buffer, const SIZE_CONTEXT length, const SIZE_CONTEXT loadedSize = 0)
            : _size(loadedSize)
            , _data(buffer, length)
        {
            static_assert(BLOCKSIZE == 0, "This method can only be called if you pass a buffer that can not be extended");
        }
        ~FrameType() = default;

        FrameType<BLOCKSIZE, BIG_ENDIAN_ORDERING, SIZE_CONTEXT>& operator=(const FrameType<BLOCKSIZE, BIG_ENDIAN_ORDERING, SIZE_CONTEXT>& rhs) {
            if (this != &rhs) {
                Size(rhs.Size());
                if (Size() > 0) {
                    ::memcpy(&(_data[0]), rhs.Data(), Size());
                }
            }
            return(*this);
        }

    public:
        inline void Clear()
        {
            _size = 0;
        }
        inline SIZE_CONTEXT Size() const
        {
            return (_size);
        }
        inline const uint8_t* Data() const {
            return (&_data[0]);
        }
        inline uint8_t& operator[](const SIZE_CONTEXT index)
        {
            return _data[index];
        }
        inline const uint8_t& operator[](const SIZE_CONTEXT index) const
        {
            return _data[index];
        }
        void Size(SIZE_CONTEXT size)
        {
            _data.Allocate(size);

            _size = size;
        }
        template <typename TYPENAME>
        uint32_t SetBuffer(const SIZE_CONTEXT offset, const TYPENAME& length, const uint8_t buffer[])
        {
            SIZE_CONTEXT requiredLength(static_cast<SIZE_CONTEXT>(sizeof(TYPENAME) + length));

            static_assert(sizeof(TYPENAME) <= sizeof(SIZE_CONTEXT), "Make sure the logic can handle the size (enlarge the SIZE_CONTEXT)");

            if ((offset + requiredLength) >= _size) {
                Size(offset + requiredLength);
            }

            SetNumber<TYPENAME>(offset, length);

            if (length != 0) {
                ASSERT(buffer != nullptr);
                ::memcpy(&(_data[offset + sizeof(TYPENAME)]), buffer, length);
            }

            return (requiredLength);
        }

        SIZE_CONTEXT Copy(const SIZE_CONTEXT offset, const SIZE_CONTEXT length, uint8_t buffer[]) const
        {
            ASSERT(offset + length <= _size);

            ::memcpy(buffer, &(_data[offset]), length);

            return (length);
        }
        SIZE_CONTEXT Copy(const SIZE_CONTEXT offset, const SIZE_CONTEXT length, const uint8_t buffer[])
        {
            Size(offset + length);

            ::memcpy(&(_data[offset]), buffer, length);

            return (length);
        }
        template <typename TYPENAME = uint16_t>
        SIZE_CONTEXT SetText(const SIZE_CONTEXT offset, const string& value)
        {
            std::string convertedText(Core::ToString(value));
            return (SetBuffer<TYPENAME>(offset, static_cast<TYPENAME>(convertedText.length()), reinterpret_cast<const uint8_t*>(convertedText.c_str())));
        }

        SIZE_CONTEXT SetNullTerminatedText(const SIZE_CONTEXT offset, const string& value)
        {
            std::string convertedText(Core::ToString(value));
            SIZE_CONTEXT requiredLength(static_cast< SIZE_CONTEXT>(convertedText.length() + 1));

            if ((offset + requiredLength) >= _size) {
                Size(offset + requiredLength);
            }

            ::memcpy(&(_data[offset]), convertedText.c_str(), convertedText.length() + 1);

            return (requiredLength);
        }

        template <typename TYPENAME>
        SIZE_CONTEXT GetBuffer(const SIZE_CONTEXT offset, const TYPENAME length, uint8_t buffer[]) const
        {
            TYPENAME textLength;

            ASSERT((offset + sizeof(TYPENAME)) <= _size);
            static_assert(sizeof(TYPENAME) <= sizeof(SIZE_CONTEXT), "Make sure the logic can handle the size (enlarge the SIZE_CONTEXT)");

            GetNumber<TYPENAME>(offset, textLength);

            ASSERT((textLength + offset + sizeof(TYPENAME)) <= _size);

            if ((textLength + offset + sizeof(TYPENAME)) > _size) {
                textLength = (_size - (offset + static_cast<uint32_t>(sizeof(TYPENAME))));
            }

            memcpy(buffer, &(_data[offset + sizeof(TYPENAME)]), (textLength > length ? length : textLength));

            return (static_cast<SIZE_CONTEXT>(sizeof(TYPENAME) + textLength));
        }

        template <typename TYPENAME = uint16_t>
        SIZE_CONTEXT GetText(const SIZE_CONTEXT offset, string& result) const
        {
            TYPENAME textLength;
            ASSERT((offset + sizeof(TYPENAME)) <= _size);
            static_assert(sizeof(TYPENAME) <= sizeof(SIZE_CONTEXT), "Make sure the logic can handle the size (enlarge the SIZE_CONTEXT)");

            GetNumber<TYPENAME>(offset, textLength);

            ASSERT((textLength + offset + sizeof(TYPENAME)) <= _size);

            if (textLength + offset + sizeof(TYPENAME) > _size) {
                textLength = (_size - (offset + sizeof(TYPENAME)));
            }

            std::string convertedText(reinterpret_cast<const char*>(&(_data[offset + sizeof(TYPENAME)])), textLength);

            result = Core::ToString(convertedText);

            return (static_cast<SIZE_CONTEXT>(sizeof(TYPENAME) + textLength));
        }

        SIZE_CONTEXT GetNullTerminatedText(const SIZE_CONTEXT offset, string& result) const
        {
            const char* text = reinterpret_cast<const char*>(&(_data[offset]));
            result = text;
            return (static_cast<SIZE_CONTEXT>(result.length() + 1));
        }

        SIZE_CONTEXT SetBoolean(const SIZE_CONTEXT offset, const bool value)
        {
            if ((offset + 1) >= _size) {
                Size(offset + 1);
            }

            _data[offset] = (value ? 1 : 0);

            return (1);
        }

        SIZE_CONTEXT GetBoolean(const SIZE_CONTEXT offset, bool& value) const
        {
            ASSERT(offset < _size);

            value = (_data[offset] != 0);

            return (1);
        }

        template <typename TYPENAME>
        inline SIZE_CONTEXT SetNumber(const SIZE_CONTEXT offset, const TYPENAME number)
        {
            return (SetNumber(offset, number, TemplateIntToType<sizeof(TYPENAME) == 1>()));
        }

        template <typename TYPENAME>
        inline SIZE_CONTEXT GetNumber(const SIZE_CONTEXT offset, TYPENAME& number) const
        {
            return (GetNumber(offset, number, TemplateIntToType<sizeof(TYPENAME) == 1>()));
        }

#ifdef __DEBUG__
        void Dump(const SIZE_CONTEXT offset) const
        {
            static const TCHAR character[] = "0123456789ABCDEF";
            string info;
            SIZE_CONTEXT index = offset;

            while (index < _size) {
                if (info.empty() == false) {
                    info += ':';
                }
                info += string("0x") + character[(_data[index] & 0xF0) >> 4] + character[(_data[index] & 0x0F)];
                index++;
            }

            TRACE_L1("MetaData: %s", info.c_str());
        }
#endif

    private:
        template <typename TYPENAME>
        SIZE_CONTEXT SetNumber(const SIZE_CONTEXT offset, const TYPENAME number, const TemplateIntToType<true>&)
        {
            if ((offset + 1) >= _size) {
                Size(offset + 1);
            }

            _data[offset] = static_cast<uint8_t>(number);

            return (1);
        }

        template <typename TYPENAME>
        void SetNumberLittleEndianPlatform(const SIZE_CONTEXT offset, const TYPENAME number) {
            const uint8_t* source = reinterpret_cast<const uint8_t*>(&number);
            uint8_t* destination = &(_data[offset + sizeof(TYPENAME) - 1]);

            *destination-- = *source++;
            *destination-- = *source++;

            if ((sizeof(TYPENAME) == 4) || (sizeof(TYPENAME) == 8)) {
                *destination-- = *source++;
                *destination-- = *source++;

                if (sizeof(TYPENAME) == 8) {
                    *destination-- = *source++;
                    *destination-- = *source++;
                    *destination-- = *source++;
                    *destination-- = *source++;
                }
            }
        }

        template <typename TYPENAME>
        void SetNumberBigEndianPlatform(const SIZE_CONTEXT offset, const TYPENAME number) {
            const uint8_t* source = reinterpret_cast<const uint8_t*>(&number);
            uint8_t* destination = &(_data[offset]);

            *destination++ = *source++;
            *destination++ = *source++;

            if ((sizeof(TYPENAME) == 4) || (sizeof(TYPENAME) == 8)) {
                *destination++ = *source++;
                *destination++ = *source++;

                if (sizeof(TYPENAME) == 8) {
                    *destination++ = *source++;
                    *destination++ = *source++;
                    *destination++ = *source++;
                    *destination++ = *source++;
                }
            }
        }


        template <typename TYPENAME>
        SIZE_CONTEXT SetNumber(const SIZE_CONTEXT offset, const TYPENAME number, const TemplateIntToType<false>&)
        {
            if ((offset + sizeof(TYPENAME)) >= _size) {
                Size(offset + sizeof(TYPENAME));
            }

            if (BIG_ENDIAN_ORDERING == true) {
#ifdef LITTLE_ENDIAN_PLATFORM
                SetNumberLittleEndianPlatform(offset, number);
#else
                SetNumberBigEndianPlatform(offset, number);
#endif
            }
            else {
#ifdef LITTLE_ENDIAN_PLATFORM
                SetNumberBigEndianPlatform(offset, number);
#else
                SetNumberLittleEndianPlatform(offset, number);
#endif
            }

            return (sizeof(TYPENAME));
        }

        template <typename TYPENAME>
        SIZE_CONTEXT GetNumber(const SIZE_CONTEXT offset, TYPENAME& number, const TemplateIntToType<true>&) const
        {
            // Only on package level allowed to pass the boundaries!!!
            ASSERT((offset + sizeof(TYPENAME)) <= _size);

            number = static_cast<TYPENAME>(_data[offset]);

            return (1);
        }

        template <typename TYPENAME>
        inline TYPENAME GetNumberLittleEndianPlatform(const SIZE_CONTEXT offset) const
        {
            TYPENAME result = static_cast<TYPENAME>(0);
            const uint8_t* source = &(_data[offset]);
            uint8_t* destination = &(reinterpret_cast<uint8_t*>(&result)[sizeof(TYPENAME) - 1]);

            *destination-- = *source++;
            *destination-- = *source++;

            if ((sizeof(TYPENAME) == 4) || (sizeof(TYPENAME) == 8)) {
                *destination-- = *source++;
                *destination-- = *source++;

                if (sizeof(TYPENAME) == 8) {
                    *destination-- = *source++;
                    *destination-- = *source++;
                    *destination-- = *source++;
                    *destination-- = *source++;
                }
            }

            return (result);
        }

        template <typename TYPENAME>
        inline TYPENAME GetNumberBigEndianPlatform(const SIZE_CONTEXT offset) const
        {
            TYPENAME result =static_cast<TYPENAME>(0);

            // If the sizeof > 1, the alignment could be wrong. Assume the worst, always copy !!!
            const uint8_t* source = &(_data[offset]);
            uint8_t* destination = reinterpret_cast<uint8_t*>(&result);

            *destination++ = *source++;
            *destination++ = *source++;

            if ((sizeof(TYPENAME) == 4) || (sizeof(TYPENAME) == 8)) {
                *destination++ = *source++;
                *destination++ = *source++;

                if (sizeof(TYPENAME) == 8) {
                    *destination++ = *source++;
                    *destination++ = *source++;
                    *destination++ = *source++;
                    *destination++ = *source++;
                }
            }

            return (result);
        }

        template <typename TYPENAME>
        inline SIZE_CONTEXT GetNumber(const SIZE_CONTEXT offset, TYPENAME& value, const TemplateIntToType<false>&) const
        {
            if ((offset + sizeof(TYPENAME)) > _size) {
                value = static_cast<TYPENAME>(0);
            }
            else if (BIG_ENDIAN_ORDERING == true) {
#ifdef LITTLE_ENDIAN_PLATFORM
                value = GetNumberLittleEndianPlatform<TYPENAME>(offset);
#else
                value = GetNumberBigEndianPlatform<TYPENAME>(offset);
#endif
            }
            else {
#ifdef LITTLE_ENDIAN_PLATFORM
                value = GetNumberBigEndianPlatform<TYPENAME>(offset);
#else
                value = GetNumberLittleEndianPlatform<TYPENAME>(offset);
#endif
            }

            return (sizeof(TYPENAME));
        }

    private:
        mutable SIZE_CONTEXT _size;
        AllocatorType<BLOCKSIZE,SIZE_CONTEXT> _data;
    };
}
}

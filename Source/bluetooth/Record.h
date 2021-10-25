/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
#include <limits>
#include <type_traits>


namespace WPEFramework {

namespace Bluetooth {

    using Buffer = std::basic_string<uint8_t>;

    // Architecture netural version
    class Record  {
    public:
        Record(const Record& buffer) = delete;
        Record& operator=(const Record&) = delete;

        Record()
            : _buffer(nullptr)
            , _bufferSize(0)
            , _filledSize(0)
            , _readerOffset(0)
            , _writerOffset(0)
        {
        }
        explicit Record(const Buffer& buffer)
            // It is ensured that the buffer will never be written.
            : _buffer(const_cast<uint8_t*>(buffer.data()))
            , _bufferSize(buffer.size())
            , _filledSize(_bufferSize)
            , _readerOffset(0)
            , _writerOffset(_bufferSize)
        {
            ASSERT(buffer.size() < 0x10000);
            ASSERT(_buffer != nullptr);
            ASSERT(_bufferSize >= _filledSize);
        }
        Record(const uint8_t buffer[], const uint16_t bufferSize)
            // It is ensured that the buffer will never be written.
            : _buffer(const_cast<uint8_t*>(buffer))
            , _bufferSize(bufferSize)
            , _filledSize(bufferSize)
            , _readerOffset(0)
            , _writerOffset(bufferSize)
        {
            ASSERT(_buffer != nullptr);
            ASSERT(_bufferSize != 0);
            ASSERT(_bufferSize >= _filledSize);
        }
        Record(uint8_t scratchPad[], const uint16_t scratchPadSize, const uint16_t filledSize = 0)
            : _buffer(scratchPad)
            , _bufferSize(scratchPadSize)
            , _filledSize(filledSize)
            , _readerOffset(0)
            , _writerOffset(filledSize)
        {
            ASSERT(_buffer != nullptr);
            ASSERT(_bufferSize != 0);
            ASSERT(_bufferSize >= _filledSize);
        }
        virtual ~Record() = default;

    public:
        bool IsEmpty() const
        {
            return (Available() == 0);
        }
        uint16_t Length() const
        {
            return (_writerOffset);
        }
        uint16_t Capacity() const
        {
            return (_bufferSize);
        }
        uint16_t Free() const
        {
            return (_bufferSize > _writerOffset? (_bufferSize - _writerOffset) : 0);
        }
        uint16_t Available() const
        {
            return (_writerOffset > _readerOffset? (_writerOffset - _readerOffset) : 0);
        }
        const uint8_t* Data() const
        {
            return (_buffer);
        }
        void Assign(uint8_t buffer[], const uint32_t bufferSize)
        {
            _buffer = buffer;
            _bufferSize = bufferSize;
            _filledSize = bufferSize;
            _writerOffset = bufferSize;
            Rewind();
        }
        void Clear()
        {
            _writerOffset = 0;
            _filledSize = 0;
            Rewind();
        }
        void Rewind() const
        {
            _readerOffset = 0;
        }
        const string ToString() const
        {
            string val;
            Core::ToHexString(Data(), Length(), val);
            if (val.empty() == true) {
                val = _T("<empty>");
            }
            return (val);
        }
        void Export(Buffer& output) const
        {
            output.assign(Data(), Length());
        }

    public:
        void Push(const bool value)
        {
            Push(static_cast<uint8_t>(value));
        }
        void Push(const string& value)
        {
            ASSERT(value.size() < 0x10000);
            Push(reinterpret_cast<const uint8_t*>(value.data()), static_cast<uint16_t>(value.length()));
        }
        void Push(const Buffer& value)
        {
            ASSERT(value.size() < 0x10000);
            Push(value.data(), static_cast<uint16_t>(value.length()));
        }
        void Push(const uint8_t value[], const uint16_t size)
        {
            ASSERT(Free() >= size);
            ::memcpy(WritePtr(), value, size);
            _writerOffset += size;
        }
        template<typename TYPE, /* if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
        void Push(const TYPE value)
        {
            ASSERT(sizeof(TYPE) <= 4);
            PushIntegerValue(static_cast<typename std::make_unsigned<TYPE>::type>(value));
        }
        template<typename TYPE, /* if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        void Push(const TYPE value)
        {
            ASSERT(sizeof(TYPE) <= 4);
            PushIntegerValue(static_cast<typename std::underlying_type<TYPE>::type>(value));
        }
        void Push(const Record& element)
        {
            ASSERT(Free() >= element.Length());
            ::memcpy(WritePtr(), element.Data(), element.Length());
            _writerOffset += element.Length();
        }

    public:
        void Pop(string& value, const uint16_t length) const
        {
            if (Available() >= length) {
                value.assign(reinterpret_cast<const char*>(ReadPtr()), length);
                _readerOffset += length;
            } else {
                TRACE_L1("Record: String truncated!");
                _readerOffset = _writerOffset;
            }
        }
        void Pop(Buffer& value, const uint16_t length) const
        {
            if (Available() >= length) {
                value.assign(ReadPtr(), length);
                _readerOffset += length;
            } else {
                TRACE_L1("Record: Buffer truncated!");
                _readerOffset = _writerOffset;
            }
        }
        template<typename TYPE, /* if enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        void Pop(TYPE& value)
        {
            ASSERT(sizeof(TYPE) <= 4);
            typename std::underlying_type<TYPE>::type temp;
            Pop(temp);
            value = static_cast<TYPE>(temp);
        }
        template<typename TYPE, /* if integer */ typename std::enable_if<std::is_integral<TYPE>::value, int>::type = 0>
        void Pop(TYPE& value) const
        {
            PopIntegerValue(value);
        }
        void Pop(Record& element, const uint32_t size) const
        {
            if (Available() >= size) {
                element.Push(ReadPtr(), size);
                _readerOffset += size;
            } else {
                TRACE_L1("Record: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }

    public:
        void Peek(Record& element, const uint32_t size) const
        {
            if (Available() >= size) {
                element.Assign(const_cast<uint8_t*>(ReadPtr()), size);
                _readerOffset += size;
            } else {
                TRACE_L1("Record: Truncated payload");
                _readerOffset = _writerOffset;
            }
        }

    protected:
        const uint8_t* ReadPtr() const
        {
            return (&_buffer[_readerOffset]);
        }
        uint8_t* WritePtr()
        {
            return (&_buffer[_writerOffset]);
        }

    protected:
        void PushIntegerValue(const uint8_t value)
        {
            ASSERT(Free() >= 1);
            _buffer[_writerOffset++] = value;
        }
        void PopIntegerValue(uint8_t& value) const
        {
            value = _buffer[_readerOffset++];
        }
        virtual void PushIntegerValue(const uint16_t value)
        {
            ASSERT(false && "Push uint16_t");
        }
        virtual void PushIntegerValue(const uint32_t value)
        {
            ASSERT(false && "Push uint32_t");
        }
        virtual void PopIntegerValue(uint16_t& value) const
        {
            ASSERT(false && "Pop uint16_t");
        }
        virtual void PopIntegerValue(uint32_t& value) const
        {
            ASSERT(false && "Pop uint32_t");
        }

    protected:
        uint8_t* _buffer;
        uint16_t _bufferSize;
        uint16_t _filledSize;
        mutable uint16_t _readerOffset;
        uint16_t _writerOffset;
    }; // class Record

    // Big-Endian version
    class RecordBE : public Record {
    public:
        using Record::Record;
        ~RecordBE() = default;

    protected:
        void PushIntegerValue(const uint16_t value) override
        {
            ASSERT(Free() >= 2);
            _buffer[_writerOffset++] = (value >> 8);
            _buffer[_writerOffset++] = value;
        }
        void PushIntegerValue(const uint32_t value) override
        {
            ASSERT(Free() >= 4);
            _buffer[_writerOffset++] = (value >> 24);
            _buffer[_writerOffset++] = (value >> 16);
            _buffer[_writerOffset++] = (value >> 8);
            _buffer[_writerOffset++] = value;
        }
        void PopIntegerValue(uint16_t& value) const override
        {
            value = ((_buffer[_readerOffset] << 8) | _buffer[_readerOffset + 1]);
            _readerOffset += 2;
        }
        void PopIntegerValue(uint32_t& value) const override
        {
            value = ((_buffer[_readerOffset] << 24) | (_buffer[_readerOffset + 1] << 16)
                        | (_buffer[_readerOffset + 2] << 8) | _buffer[_readerOffset + 3]);
            _readerOffset += 4;
        }
    }; // class RecordLE

    // Little-Endian version
    class RecordLE : public Record {
    public:
        using Record::Record;
        ~RecordLE() = default;

    protected:
        void PushIntegerValue(const uint16_t value) override
        {
            ASSERT(Free() >= 2);
            _buffer[_writerOffset++] = value;
            _buffer[_writerOffset++] = (value >> 8);
        }
        void PushIntegerValue(const uint32_t value) override
        {
            ASSERT(Free() >= 4);
            _buffer[_writerOffset++] = value;
            _buffer[_writerOffset++] = (value >> 8);
            _buffer[_writerOffset++] = (value >> 16);
            _buffer[_writerOffset++] = (value >> 24);
        }
        void PopIntegerValue(uint16_t& value) const override
        {
            value = ((_buffer[_readerOffset + 1] << 8) | _buffer[_readerOffset]);
            _readerOffset += 2;
        }
        void PopIntegerValue(uint32_t& value) const override
        {
            value = ((_buffer[_readerOffset + 3] << 24) | (_buffer[_readerOffset + 2] << 16)
                        | (_buffer[_readerOffset + 1] << 8) | _buffer[_readerOffset]);
            _readerOffset += 4;
        }
    }; // class RecordLE

} // namespace Bluetooth

}
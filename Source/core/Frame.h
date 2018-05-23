#ifndef __GENERICS_FRAME_H
#define __GENERICS_FRAME_H

#include "Module.h"

namespace WPEFramework {
namespace Core {

    template <const uint16_t BLOCKSIZE>
    class FrameType {
    private:
        template <const uint16_t STARTSIZE>
        class AllocatorType {
        private:
            AllocatorType<STARTSIZE> operator=(const AllocatorType<STARTSIZE>&) = delete;

        public:
            AllocatorType()
                : _bufferSize(STARTSIZE)
                , _data(new uint8_t[_bufferSize])
            {
                static_assert(STARTSIZE != 0, "This method can only be called if you specify an initial blocksize");
            }
            AllocatorType(const AllocatorType<STARTSIZE>& copy) 
                : _bufferSize(copy._bufferSize)
                , _data (STARTSIZE == 0 ? copy._data : new uint8_t[_bufferSize]) {

                if (STARTSIZE != 0) {
                    ::memcpy (_data, copy._data, _bufferSize);
                }
            }
            AllocatorType(uint8_t buffer[], const uint32_t length)
                : _bufferSize(length)
                , _data(buffer)
            {
                static_assert(STARTSIZE == 0, "This method can only be called if you specify an initial blocksize of 0");
            }
            ~AllocatorType()
            {
                if ((STARTSIZE != 0) && (_data != nullptr)) {
                    delete[] _data;
                }
            }

        public:
            inline uint8_t& operator[](const uint16_t index)
            {
                ASSERT(_data != nullptr);
                ASSERT(index < _bufferSize);
                return (_data[index]);
            }
            inline const uint8_t& operator[](const uint16_t index) const
            {
                ASSERT(_data != nullptr);
                ASSERT(index < _bufferSize);
                return (_data[index]);
            }
            inline void Allocate(uint32_t requiredSize)
            {
		RealAllocate<STARTSIZE>(requiredSize, TemplateIntToType<STARTSIZE>());
            }

		private:
			template<const uint16_t NONZEROSIZE>
			inline void RealAllocate(const uint32_t requiredSize, const TemplateIntToType<NONZEROSIZE>& /* For compile time diffrentiation */) {
				if (requiredSize > _bufferSize) {

					_bufferSize = static_cast<uint16_t>(((requiredSize / (STARTSIZE ? STARTSIZE : 1)) + 1) * STARTSIZE);

					// oops we need to "reallocate".
					_data = reinterpret_cast<uint8_t*>(::realloc(_data, _bufferSize));
				}
			}
			inline void RealAllocate(const uint32_t requiredSize, const TemplateIntToType<0>& /* For compile time diffrentiation */) {
				ASSERT(requiredSize <= _bufferSize);
			}

        private:
            uint16_t _bufferSize;
            uint8_t* _data;
        };

    private:
        FrameType& operator=(const FrameType&);

    public:
        class Reader {
        private:
            Reader& operator=(const Reader&);

        public:
            Reader()
                : _offset(0)
                , _container(nullptr)
            {
            }
            Reader(const FrameType& data, const uint16_t offset)
                : _offset(offset)
                , _container(&data)
            {
            }
            Reader(const Reader& copy)
                : _offset(copy._offset)
                , _container(copy._container)
            {
            }
            ~Reader()
            {
            }

        public:
            inline bool HasData() const
            {
                return ((_container != nullptr) && (_offset < _container->Size()));
            }
			inline uint16_t Length() const {
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
                uint16_t result;

                ASSERT(_container != nullptr);

                result   = _container->GetBuffer<TYPENAME>(_offset, maxLength, buffer);
                _offset += result;

                return (static_cast<TYPENAME>(result - sizeof (TYPENAME)));
            }
			void Copy(const uint16_t length, uint8_t buffer[]) const
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
#ifdef __DEBUG__
            void Dump() const
            {
                _container->Dump(_offset);
            }
#endif

        private:
            mutable uint16_t _offset;
            const FrameType* _container;
        };
        class Writer {
        private:
            Writer& operator=(const Writer&);

        public:
            Writer()
                : _offset(0)
                , _container(nullptr)
            {
            }
            // TODO: should we make offset 0 by default?
            Writer(FrameType& data, const uint16_t offset)
                : _offset(offset)
                , _container(&data)
            {
            }
            Writer(const Writer& copy)
                : _offset(copy._offset)
                , _container(copy._container)
            {
            }
            ~Writer()
            {
            }

        public:
			inline uint16_t Offset() const {
				return (_offset);
			}
            template <typename TYPENAME>
            void Buffer(const TYPENAME length, const uint8_t buffer[])
            {
                ASSERT(_container != nullptr);

                _offset += _container->SetBuffer<TYPENAME>(_offset, length, buffer);
            }
			void Copy(const uint16_t length, const uint8_t buffer[])
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
            uint16_t _offset;
            FrameType* _container;
        };

    public:
        FrameType()
            : _size(0)
            , _data()
        {
            static_assert(BLOCKSIZE != 0, "This method can only be called if you specify an initial blocksize");
        }
        FrameType(const FrameType<BLOCKSIZE>& copy)
            : _size (copy._size)
            , _data (copy._data) {
        }
        FrameType(uint8_t buffer[], const uint32_t length, const uint32_t loadedSize = 0)
            : _size(loadedSize)
            , _data(buffer, length)
        {
            static_assert(BLOCKSIZE == 0, "This method can only be called if you pass a buffer that can not be extended");
        }
        ~FrameType()
        {
        }

    public:
        inline void Clear()
        {
            _size = 0;
        }
        inline uint32_t Size() const
        {
            return (_size);
        }
        inline uint8_t& operator[](const uint16_t index)
        {
            return _data[index];
        }
        inline const uint8_t& operator[](const uint16_t index) const
        {
            return _data[index];
        }
        void Size(uint16_t size)
        {
            _data.Allocate(size);

            _size = size;
        }
#ifdef __DEBUG__
        void Dump(const unsigned int offset) const
        {
            static const TCHAR character[] = "0123456789ABCDEF";
            string info;
            uint16_t index = offset;

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

        friend class Reader;
        friend class Writer;

        template <typename TYPENAME>
        uint16_t SetBuffer (const uint16_t offset, const TYPENAME& length, const uint8_t buffer[])
        {
            uint16_t requiredLength(sizeof(TYPENAME) + length);

            if ((offset + requiredLength) >= _size) {
                Size(offset + requiredLength);
            }

            SetNumber<TYPENAME>(offset, length);
            ::memcpy(&(_data[offset + sizeof(TYPENAME)]), buffer, length);

            return (requiredLength);
        }
 
		uint16_t Copy(const uint16_t offset, const uint16_t length, uint8_t buffer[]) const
		{
			ASSERT(offset + length <= _size);

			::memcpy(buffer, &(_data[offset]), length);

			return (length);
		}
		uint16_t Copy(const uint16_t offset, const uint16_t length, const uint8_t buffer[])
		{
			Size(offset + length);

			::memcpy(&(_data[offset]), buffer, length);

			return (length);
		}
        uint16_t SetText(const uint16_t offset, const string& value)
        {
            std::string convertedText(Core::ToString(value));
            return (SetBuffer<uint16_t>(offset, static_cast<uint16_t>(convertedText.length()), reinterpret_cast<const uint8_t*>(convertedText.c_str())));
        }

        uint16_t SetNullTerminatedText(const uint16_t offset, const string& value)
        {
            std::string convertedText(Core::ToString(value));
            uint16_t requiredLength(convertedText.length() + 1);

            if ((offset + requiredLength) >= _size) {
                Size(offset + requiredLength);
            }

            ::memcpy(&(_data[offset]), convertedText.c_str(), convertedText.length() + 1);

            return (requiredLength);
        }

        template <typename TYPENAME>
        uint16_t GetBuffer(const uint16_t offset, const TYPENAME length, uint8_t buffer[]) const
        {
            TYPENAME textLength;

            ASSERT((offset + sizeof(TYPENAME)) <= _size);

            GetNumber<TYPENAME>(offset, textLength);

            ASSERT((textLength + offset + sizeof(TYPENAME)) <= _size);

            if (textLength + offset + sizeof(TYPENAME) > _size) {
                textLength = (_size - (offset + sizeof(TYPENAME)));
            }

            memcpy (buffer, &(_data[offset + sizeof(TYPENAME)]), (textLength >  length ? length : textLength));

            return (sizeof(TYPENAME) + textLength);
        }

        uint16_t GetText(const uint16_t offset, string& result) const
        {
            uint16_t textLength;

            ASSERT((offset + sizeof(uint16_t)) <= _size);

            GetNumber<uint16_t>(offset, textLength);

            ASSERT((textLength + offset + sizeof(uint16_t)) <= _size);

            if (textLength + offset + sizeof(uint16_t) > _size) {
                textLength = (_size - (offset + sizeof(uint16_t)));
            }

            std::string convertedText(reinterpret_cast<const char*>(&(_data[offset + sizeof(uint16_t)])), textLength);

            result = Core::ToString(convertedText);

            return (sizeof(uint16_t) + textLength);
        }

        uint16_t GetNullTerminatedText(const uint16_t offset, string& result) const
        {
            const char* text = reinterpret_cast<const char*>(&(_data[offset]));
            result = text;
            return (result.length() + 1);
        }

        uint16_t SetBoolean(const uint16_t offset, const bool value)
        {
            if ((offset + 1) >= _size) {
                Size(offset + 1);
            }

            _data[offset] = (value ? 1 : 0);

            return (1);
        }

        uint16_t GetBoolean(const uint16_t offset, bool& value) const
        {
            ASSERT(offset < _size);

            value = (_data[offset] != 0);

            return (1);
        }

        template <typename TYPENAME>
        inline uint16_t SetNumber(const uint16_t offset, const TYPENAME number)
        {
            return (SetNumber(offset, number, TemplateIntToType<sizeof(TYPENAME) == 1>()));
        }

        template <typename TYPENAME>
        inline uint16_t GetNumber(const uint16_t offset, TYPENAME& number) const
        {
            return (GetNumber(offset, number, TemplateIntToType<sizeof(TYPENAME) == 1>()));
        }

    private:
        template <typename TYPENAME>
        uint16_t SetNumber(const uint16_t offset, const TYPENAME number, const TemplateIntToType<true>&)
        {
            if ((offset + 1) >= _size) {
                Size(offset + 1);
            }

            _data[offset] = static_cast<uint8_t>(number);

            return (1);
        }

        template <typename TYPENAME>
        uint16_t SetNumber(const uint16_t offset, const TYPENAME number, const TemplateIntToType<false>&)
        {
            if ((offset + sizeof(TYPENAME)) >= _size) {
                Size(offset + sizeof(TYPENAME));
            }

#ifdef LITTLE_ENDIAN_PLATFORM
            {
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
#else
            {
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
#endif

            return (sizeof(TYPENAME));
        }
 
        template <typename TYPENAME>
        uint16_t GetNumber(const uint16_t offset, TYPENAME& number, const TemplateIntToType<true>&) const
        {
            // Only on package level allowed to pass the boundaries!!!
            ASSERT((offset + sizeof(TYPENAME)) <= _size);

            number = static_cast<TYPENAME>(_data[offset]);

            return (1);
        }
 
        template <typename TYPENAME>
        inline uint16_t GetNumber(const uint16_t offset, TYPENAME& value, const TemplateIntToType<false>&) const
        {
            // Only on package level allowed to pass the boundaries!!!
            ASSERT((offset + sizeof(TYPENAME)) <= _size);

            TYPENAME result;

#ifdef LITTLE_ENDIAN_PLATFORM
            {
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
            }
#else
            {
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
            }
#endif

            value = result;

            return (sizeof(TYPENAME));
        }

	private:
        mutable uint16_t _size;
        AllocatorType<BLOCKSIZE> _data;
    };
}
}

#endif //__GENERICS_FRAME_H

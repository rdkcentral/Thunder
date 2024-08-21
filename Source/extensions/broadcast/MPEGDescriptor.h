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

#ifndef __MPEGDESCRIPTORS_H
#define __MPEGDESCRIPTORS_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace Thunder {
namespace Broadcast {
    namespace MPEG {
        class EXTERNAL Descriptor {
        public:
            inline Descriptor()
                : _descriptor()
            {
            }
            inline Descriptor(const Core::DataElement& data)
                : _descriptor(data)
            {
            }
            inline Descriptor(const Descriptor& copy)
                : _descriptor(copy._descriptor)
            {
            }
            inline ~Descriptor() {}

            inline Descriptor& operator=(const Descriptor& RHS)
            {
                _descriptor = RHS._descriptor;

                return (*this);
            }

        public:
            inline uint8_t Tag() const { return (_descriptor[0]); }
            inline uint8_t Length() const { return (_descriptor[1] + 2); }
            inline const uint8_t& operator[](const uint8_t index) const
            {
                ASSERT(index < _descriptor[1]);
                return (_descriptor[2 + index]);
            }

        private:
            Core::DataElement _descriptor;
        };

        class EXTERNAL DescriptorIterator {
        public:
            inline DescriptorIterator()
                : _descriptors()
                , _index(NUMBER_MAX_UNSIGNED(uint32_t))
            {
            }
            inline DescriptorIterator(const Core::DataElement& data)
                : _descriptors(data)
                , _index(NUMBER_MAX_UNSIGNED(uint32_t))
            {
            }
            inline DescriptorIterator(const DescriptorIterator& copy)
                : _descriptors(copy._descriptors)
                , _index(copy._index)
            {
            }
            inline ~DescriptorIterator() {}

            inline DescriptorIterator& operator=(const DescriptorIterator& rhs)
            {
                _descriptors = rhs._descriptors;
                _index = rhs._index;

                return (*this);
            }

        public:
            inline bool IsValid() const { return (_index < _descriptors.Size()); }
            inline void Reset() { _index = NUMBER_MAX_UNSIGNED(uint32_t); }
            bool Next()
            {
                uint8_t descriptorLength;

                if (_index == NUMBER_MAX_UNSIGNED(uint32_t)) {
                    _index = 0;
                    descriptorLength = _descriptors[1] + 2;
                } else if (_index < _descriptors.Size()) {
                    _index += (_descriptors[_index + 1] + 2);
                    if ((_index + 2) < _descriptors.Size()) {
                        descriptorLength = _descriptors[_index + 1] + 2;
                    }
                }

                // See if we have a valid descriptor, Does it fit the block we have ?
                if ((_index + descriptorLength) > _descriptors.Size()) {
                    // It's too big, Jump to the end..
                    _index = static_cast<uint32_t>(_descriptors.Size());
                }

                return (IsValid());
            }
            inline Descriptor Current()
            {
                return (Descriptor(Core::DataElement(_descriptors, _index)));
            }
            inline const Descriptor Current() const
            {
                return (Descriptor(Core::DataElement(_descriptors, _index)));
            }
            bool Tag(const uint8_t tagId)
            {

                if (_index == NUMBER_MAX_UNSIGNED(uint32_t)) {
                    _index = 0;
                }

                while (((_index + 2) < _descriptors.Size()) && (_descriptors[_index] != tagId)) {
                    _index += _descriptors[1] + 2;
                }

                // See if we have a valid descriptor, Does it fit the block we have ?
                if (((_index + 2) >= _descriptors.Size()) || ((_descriptors[_index + 1] + 2 + _index) > _descriptors.Size())) {
                    // It's too big, or none was found, jump to the end..
                    _index = static_cast<uint32_t>(_descriptors.Size());
                }

                return (IsValid());
            }
            uint32_t Count() const
            {
                uint32_t count = 0;
                uint32_t offset = 0;
                while (offset < _descriptors.Size()) {
                    count++;
                    offset += Descriptor(Core::DataElement(_descriptors, offset)).Length();
                }

                if (offset > _descriptors.Size()) {
                    // reduce the count by one, the last one is toooooooo big
                    count--;
                }

                return (count);
            }

        private:
            Core::DataElement _descriptors;
            uint32_t _index;
        };

    } // namespace MPEG
} // namespace Broadcast
} // namespace Thunder

#endif //__MPEGDESCRIPTORS_H

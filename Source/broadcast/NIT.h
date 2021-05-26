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

#ifndef DVB_NIT_TABLE_H
#define DVB_NIT_TABLE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "MPEGDescriptor.h"
#include "MPEGSection.h"
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Broadcast {
    namespace DVB {

        class EXTERNAL NIT {
        public:
            static const uint16_t ACTUAL = 0x40;
            static const uint16_t OTHER = 0x41;

        public:
            class NetworkIterator {
            public:
                NetworkIterator()
                    : _info()
                    , _offset(~0)
                {
                }
                NetworkIterator(const Core::DataElement& data)
                    : _info(data)
                    , _offset(~0)
                {
                }
                NetworkIterator(const NetworkIterator& copy)
                    : _info(copy._info)
                    , _offset(copy._offset)
                {
                }
                ~NetworkIterator() {}

                NetworkIterator& operator=(const NetworkIterator& RHS)
                {
                    _info = RHS._info;
                    _offset = RHS._offset;

                    return (*this);
                }

            public:
                inline bool IsValid() const { return (_offset < _info.Size()); }
                inline void Reset() { _offset = ~0; }
                inline bool Next()
                {
                    if (_offset == static_cast<uint16_t>(~0)) {
                        _offset = 0;
                    } else if (_offset < _info.Size()) {
                        _offset += (DescriptorSize() + 6);
                    }

                    return (IsValid());
                }
                inline uint16_t TransportStreamId() const
                {
                    return ((_info[_offset + 0] << 8) | _info[_offset + 1]);
                }
                inline uint16_t OriginalNetworkId() const
                {
                    return ((_info[_offset + 2] << 8) | _info[_offset + 3]);
                }
                inline MPEG::DescriptorIterator Descriptors() const
                {
                    return (MPEG::DescriptorIterator(
                        Core::DataElement(_info, _offset + 6, DescriptorSize())));
                }
                inline uint8_t Networks() const
                {
                    uint8_t count = 0;
                    uint16_t offset = 0;
                    while (offset < _info.Size()) {
                        offset += (((_info[offset + 4] << 8) | _info[offset + 5]) & 0x0FFF) + 6;
                        count++;
                    }
                    return (count);
                }

            private:
                inline uint16_t DescriptorSize() const
                {
                    return ((_info[_offset + 4] << 8) | _info[_offset + 5]) & 0x0FFF;
                }

            private:
                Core::DataElement _info;
                uint16_t _offset;
            };

        public:
            NIT()
                : _data()
                , _networkId(~0)
            {
            }
            NIT(const MPEG::Table& data)
                : _data(data.Data())
                , _networkId(data.Extension())
            {
            }
            NIT(const uint16_t networkId, const Core::DataElement& data)
                : _data(data)
                , _networkId(networkId)
            {
            }
            NIT(const NIT& copy)
                : _data(copy._data)
                , _networkId(copy._networkId)
            {
            }
            ~NIT() {}

            NIT& operator=(const NIT& rhs)
            {
                _data = rhs._data;
                _networkId = rhs._networkId;
                return (*this);
            }
            bool operator==(const NIT& rhs) const
            {
                return ((_networkId == rhs._networkId) && (_data == rhs._data));
            }
            bool operator!=(const NIT& rhs) const { return (!operator==(rhs)); }

        public:
            inline bool IsValid() const
            {
                return ((_networkId != static_cast<uint16_t>(~0)) && (_data.Size() >= 2));
            }
            inline uint16_t NetworkId() const { return (_networkId); }
            MPEG::DescriptorIterator Descriptors() const
            {
                uint16_t size(DescriptorSize());
                return (MPEG::DescriptorIterator(Core::DataElement(_data, 4, size)));
            }
            NetworkIterator Networks() const
            {
                uint16_t offset = DescriptorSize() + 2;
                uint16_t size = (_data.GetNumber<uint16_t, Core::ENDIAN_BIG>(offset) & 0x0FFF);
                ASSERT(size == (_data.Size() - offset - 2));
                return (NetworkIterator(Core::DataElement(_data, offset + 2, size)));
            }

        private:
            inline uint16_t DescriptorSize() const
            {
                return (_data.GetNumber<uint16_t, Core::ENDIAN_BIG>(2) & 0x0FFF);
            }

        private:
            Core::DataElement _data;
            uint16_t _networkId;
        };

    } // namespace DVB
} // namespace Broadcast
} // namespace WPEFramework

#endif // DVB_NIT_TABLE_H

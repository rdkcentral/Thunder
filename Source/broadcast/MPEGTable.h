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

#ifndef __MPEGTABLE_H
#define __MPEGTABLE_H

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
    namespace MPEG {
        class EXTERNAL PAT {
        private:
            PAT(const PAT&) = delete;
            PAT& operator=(const PAT&) = delete;

        public:
            static const uint8_t ID = 0x00;

        public:
            class ProgramIterator {
            public:
                ProgramIterator()
                    : _index(~0)
                    , _programs(0)
                    , _info()
                {
                }
                ProgramIterator(const Core::DataElement& sections)
                    : _index(~0)
                    , _programs(sections.Size() / 4)
                    , _info(sections)
                {
                }
                ProgramIterator(const ProgramIterator& copy)
                    : _index(copy._index)
                    , _programs(copy._programs)
                    , _info(copy._info)
                {
                }
                ~ProgramIterator() {}

                ProgramIterator& operator=(const ProgramIterator& RHS)
                {
                    _info = RHS._info;
                    _index = RHS._index;
                    _programs = RHS._programs;

                    return (*this);
                }

            public:
                inline bool IsValid() const { return (_index < _programs); }
                inline void Reset() { _index = ~0; }
                bool Next()
                {
                    if (_index < _programs) {
                        _index++;
                    } else if (_index == static_cast<uint32_t>(~0)) {
                        _index = 0;
                    }

                    return (IsValid());
                }
                inline uint16_t ProgramNumber() const
                {
                    return (_info.GetNumber<uint16_t, Core::ENDIAN_BIG>(_index * 4));
                }
                inline uint16_t Pid() const
                {
                    return (_info.GetNumber<uint16_t, Core::ENDIAN_BIG>((_index * 4) + 2) & 0x1FFF);
                }
                uint16_t Count() const { return (_programs); }

            private:
                uint32_t _index;
                uint32_t _programs;
                Core::DataElement _info;
            };

        public:
            PAT()
                : _data()
                , _transportId(~0)
            {
            }
            PAT(const Table& data)
                : _data(data.Data())
                , _transportId(data.Extension())
            {
            }
            ~PAT() {}

        public:
            inline uint16_t TransportStreamId() const { return (_transportId); }
            inline ProgramIterator Programs() const { return (ProgramIterator(_data)); }

        private:
            Core::DataElement _data;
            uint16_t _transportId;
        };

        class EXTERNAL PMT {
        public:
            static const uint16_t ID = 0x02;

        public:
            class StreamIterator {
            public:
                StreamIterator()
                    : _info()
                    , _offset(~0)
                {
                }
                StreamIterator(const Core::DataElement& data)
                    : _info(data)
                    , _offset(~0)
                {
                }
                StreamIterator(const StreamIterator& copy)
                    : _info(copy._info)
                    , _offset(copy._offset)
                {
                }
                ~StreamIterator() {}

                StreamIterator& operator=(const StreamIterator& RHS)
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
                        _offset += (DescriptorSize() + 5);
                    }

                    return (IsValid());
                }
                inline uint8_t StreamType() const { return (_info[_offset]); }
                inline uint16_t Pid() const
                {
                    return (((_info[_offset + 1] << 8) | _info[_offset + 2]) & 0x1FFF);
                }
                inline MPEG::DescriptorIterator Descriptors() const
                {
                    return (MPEG::DescriptorIterator(
                        Core::DataElement(_info, _offset + 5, DescriptorSize())));
                }
                inline uint8_t Streams() const
                {
                    uint8_t count = 0;
                    uint16_t offset = 0;
                    while (offset < _info.Size()) {
                        offset += (((_info[offset + 3] << 8) | _info[offset + 4]) & 0x03FF) + 5;
                        count++;
                    }
                    return (count);
                }

            private:
                inline uint16_t DescriptorSize() const
                {
                    return ((_info[_offset + 3] << 8) | _info[_offset + 4]) & 0x03FF;
                }

            private:
                Core::DataElement _info;
                uint16_t _offset;
            };

        public:
            PMT()
                : _data()
                , _programNumber(~0)
            {
            }
            PMT(const Table& data)
                : _data(data.Data())
                , _programNumber(data.Extension())
            {
            }
            PMT(const uint16_t programId, const Core::DataElement& data)
                : _data(data)
                , _programNumber(programId)
            {
            }
            PMT(const PMT& copy)
                : _data(copy._data)
                , _programNumber(copy._programNumber)
            {
            }
            ~PMT() {}

            PMT& operator=(const PMT& rhs)
            {
                _data = rhs._data;
                _programNumber = rhs._programNumber;
                return (*this);
            }
            bool operator==(const PMT& rhs) const
            {
                return ((_programNumber == rhs._programNumber) && (_data == rhs._data));
            }
            bool operator!=(const PMT& rhs) const { return (!operator==(rhs)); }

        public:
            inline bool IsValid() const
            {
                return ((_programNumber != static_cast<uint16_t>(~0)) && (_data.Size() >= 2));
            }
            inline uint16_t ProgramNumber() const { return (_programNumber); }
            uint16_t PCRPid() const
            {
                return (_data.GetNumber<uint16_t, Core::ENDIAN_BIG>(0) & 0x1FFF);
            }
            MPEG::DescriptorIterator Descriptors() const
            {
                uint16_t size(DescriptorSize());
                return (MPEG::DescriptorIterator(Core::DataElement(_data, 4, size)));
            }
            StreamIterator Streams() const
            {
                uint16_t offset(DescriptorSize() + 4);
                return (StreamIterator(
                    Core::DataElement(_data, offset, _data.Size() - offset)));
            }

        private:
            inline uint16_t DescriptorSize() const
            {
                return (_data.GetNumber<uint16_t, Core::ENDIAN_BIG>(2) & 0x03FF);
            }

        private:
            Core::DataElement _data;
            uint16_t _programNumber;
        };

    } // namespace MPEG
} // namespace Broadcast
} // namespace WPEFramework

#endif // __MPEGTABLE_

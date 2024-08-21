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

#ifndef __MPEGSECTION_H
#define __MPEGSECTION_H

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
        class EXTERNAL Section {
        public:
            inline Section()
                : _section()
            {
            }
            inline Section(const Core::DataElement& data)
                : _section(data)
            {
            }
            inline Section(const Section& copy)
                : _section(copy._section)
            {
            }
            inline ~Section() {}

            inline Section& operator=(const Section& RHS)
            {
                _section = RHS._section;

                return (*this);
            }

        public:
            inline bool IsValid() const
            {
                return ((_section.Size() >= Offset()) && (_section.Size() >= Length()) && (!HasSectionSyntax() || ValidCRC()));
            }
            inline uint8_t TableId() const { return (_section[0]); }
            inline bool HasSectionSyntax() const { return ((_section[1] & 0x80) != 0); }
            inline uint16_t Length() const { return (BaseLength() + 3); }
            inline uint16_t Extension() const
            {
                return (HasSectionSyntax() ? ((_section[3] << 8) | _section[4]) : ~0);
            }
            inline uint8_t Version() const
            {
                return (HasSectionSyntax() ? ((_section[5] & 0x3E) >> 1) : ~0);
            }
            inline bool IsCurrent() const
            {
                return (HasSectionSyntax() ? ((_section[5] & 0x01) != 0) : true);
            }
            inline bool IsNext() const { return (!IsCurrent()); }
            inline uint8_t SectionNumber() const { return (_section[6]); }
            inline uint8_t LastSectionNumber() const { return (_section[7]); }
            inline uint32_t Hash() const
            {
                // Extension(16)/TableId(8)/Version(5)/CurNext(1)/SectionIndex(1)
                return ((Extension() << 16) | (TableId() << 8) | (Version() << 3) | (IsCurrent() ? 0x04 : 0x00) | (HasSectionSyntax() ? 0x02 : 0x00));
            }
            inline Core::DataElement Data()
            {
                return (Core::DataElement(_section, Offset(), DataLength()));
            }
            inline const Core::DataElement Data() const
            {
                return (Core::DataElement(_section, Offset(), DataLength()));
            }
            template <typename TYPE>
            TYPE GetNumber(const uint16_t offset) const
            {
                return (_section.GetNumber<TYPE, Core::ENDIAN_BIG>(offset));
            }

        protected:
            inline bool ValidCRC() const
            {
                uint32_t size = Length() - 4;
                uint32_t counterCRC = GetNumber<uint32_t>(size);
                return (_section.CRC32(0, size) == counterCRC);
            }

        private:
            inline uint32_t Offset() const { return (HasSectionSyntax() ? 8 : 3); }
            inline uint16_t BaseLength() const
            {
                return ((_section[1] & 0x0F) << 8) | (_section[2]);
            }
            inline uint16_t DataLength() const
            {
                return (BaseLength() - (HasSectionSyntax() ? 9 : 0));
            }

        private:
            Core::DataElement _section;
        };

        class EXTERNAL Table {
        private:
            Table() = delete;
            Table(const Table&) = delete;
            Table& operator=(const Table&) = delete;

        public:
            Table(const Core::ProxyType<Core::DataStore>& data)
                : _extension(NUMBER_MAX_UNSIGNED(uint16_t))
                , _version(NUMBER_MAX_UNSIGNED(uint8_t))
                , _lastSectionNumber(NUMBER_MAX_UNSIGNED(uint8_t))
                , _tableId(0)
                , _sections()
                , _data(Core::DataElement(data, 0, 0))
            {
            }
            ~Table() {}

        public:
            inline void Storage(const Core::ProxyType<Core::DataStore>& data)
            {
                // Drop the current table, load a new storage
                _sections.clear();
                _data = Core::DataElement(data);
                _lastSectionNumber = NUMBER_MAX_UNSIGNED(uint8_t);
                _version = NUMBER_MAX_UNSIGNED(uint8_t);
            }
            inline bool IsValid() const
            {
                return ((_sections.size() > 0) && ((_sections.size() - 1) == _lastSectionNumber));
            }
            inline uint16_t TableId() const { return (_tableId); }
            inline uint16_t Extension() const { return (_extension); }
            template <typename TYPE>
            TYPE GetNumber(const uint16_t offset) const
            {
                return (_data.GetNumber<TYPE, Core::ENDIAN_BIG>(offset));
            }
            inline void Clear()
            {
                // Drop the current table, load a new storage
                _sections.clear();
                _data.Size(0);
                _lastSectionNumber = NUMBER_MAX_UNSIGNED(uint8_t);
                _version = NUMBER_MAX_UNSIGNED(uint8_t);
            }
            inline Core::DataElement& Data() { return (_data); }
            inline const Core::DataElement& Data() const { return (_data); }
            inline bool AddSection(const Section& section)
            {
                bool addSection = section.IsValid();

                if (addSection == true) {
                    if (_sections.size() != 0) {
                        // Starting something for TableId A and then continue with other
                        // TableId's Seems to me like a programming error.
                        if (_tableId != section.TableId()) {
                            TRACE_L1("Will not add a section, destined for: %d in table: %d", section.TableId(), _tableId);
                        }

                        addSection = (_tableId == section.TableId());

                        if ((addSection == true) && (section.Version() != _version)) {
                            // Give back all the elemts we do not use..
                            _sections.clear();
                            _data.Size(0);
                            _lastSectionNumber = section.LastSectionNumber();
                            _version = section.Version();

                            if (_extension != section.Extension()) {
                                printf("%s, %d -> Interesting the extensions differ\n",
                                    __FUNCTION__, __LINE__);
                            }
                        }
                    } else {
                        _tableId = section.TableId();
                        _data.Size(0);
                        _lastSectionNumber = section.LastSectionNumber();
                        _version = section.Version();
                        _extension = section.Extension();
                    }

                    if (addSection == true) {
                        uint32_t offset = 0;
                        uint32_t slotValue = (section.SectionNumber() << 16) | section.Length();

                        std::list<uint32_t>::iterator index(_sections.begin());

                        while (index != _sections.end()) {
                            uint16_t thisLength(*index & 0xFFFF);
                            uint8_t thisSection((*index >> 16) & 0xFF);

                            if (section.SectionNumber() == thisSection) {
                                // Replace it..
                                Insert(section.Data(), thisLength, offset);
                                *index = slotValue;
                                break;
                            } else if (section.SectionNumber() < thisSection) {
                                // We need to extend the last part
                                Insert(section.Data(), 0, offset);
                                index = _sections.insert(index, slotValue);
                                break;
                            }
                            offset += thisLength;
                        }

                        if (index == _sections.end()) {
                            ASSERT(offset == _data.Size());

                            Insert(section.Data(), 0, offset);
                            _sections.push_back(slotValue);
                        }
                    }
                }

                return (addSection);
            }

        private:
            void Insert(const Core::DataElement& data, const uint16_t allocatedLength,
                const uint16_t offset)
            {
                if (offset < _data.Size()) {
                    // We need to scrink or extend space...
                    int32_t needed = (data.Size() - allocatedLength);
                    uint32_t moveSize = (_data.Size() - offset - allocatedLength);
                    if (needed > 0) {
                        // Time to extend
                        _data.Size(_data.Size() + needed);
                        ::memmove(&(_data[offset + data.Size()]),
                            &(_data[offset + allocatedLength]), moveSize);
                    } else if (needed < 0) {
                        // time to schrink
                        ::memmove(&(_data[offset + data.Size()]),
                            &(_data[offset + allocatedLength]), moveSize);
                        _data.Size(_data.Size() + needed);
                    }
                } else {
                    _data.Size(_data.Size() + data.Size());
                }
                ::memcpy(&(_data[offset]), data.Buffer(), data.Size());
            }

        private:
            uint16_t _extension;
            uint8_t _version;
            uint8_t _lastSectionNumber;
            uint8_t _tableId;
            std::list<uint32_t> _sections;
            Core::DataElement _data;
        };

    } // namespace MPEG
} // namespace Broadcast
} // namespace Thunder

#endif // __MPEGSECTION_H

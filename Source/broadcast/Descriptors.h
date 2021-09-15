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

#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

#include "Definitions.h"
#include "MPEGDescriptor.h"

namespace WPEFramework {
namespace Broadcast {
    namespace DVB {
        namespace Descriptors {

            class EXTERNAL NetworkName {
            private:
                NetworkName operator=(const NetworkName& rhs) = delete;

            public:
                constexpr static uint8_t TAG = 0x40;

            public:
                NetworkName()
                    : _data()
                {
                }
                NetworkName(const NetworkName& copy)
                    : _data(copy._data)
                {
                }
                NetworkName(const MPEG::Descriptor& copy)
                    : _data(copy)
                {
                }
                ~NetworkName()
                {
                }

            public:
                string Name() const
                {
                    return (Core::ToString(reinterpret_cast<const char*>(&(_data[1])), _data[0]));
                }

            private:
                MPEG::Descriptor _data;
            };

            class EXTERNAL SatelliteDeliverySystem {
            private:
                SatelliteDeliverySystem operator=(const SatelliteDeliverySystem& rhs) = delete;

            public:
                constexpr static uint8_t TAG = 0x43;

            public:
                SatelliteDeliverySystem()
                    : _data()
                {
                }
                SatelliteDeliverySystem(const SatelliteDeliverySystem& copy)
                    : _data(copy._data)
                {
                }
                SatelliteDeliverySystem(const MPEG::Descriptor& copy)
                    : _data(copy)
                {
                }
                ~SatelliteDeliverySystem()
                {
                }

            public:
                // Frequency in KHz
                uint32_t Frequency() const
                {
                    return (Broadcast::ConvertBCD<uint32_t>(&(_data[0]), 8, true) * 10);
                }
                Broadcast::Modulation Modulation() const
                {
                    return (static_cast<Broadcast::Modulation>(((_data[6] >> 3) & 0x0C) | (_data[6] & 0x03)));
                }
                uint32_t SymbolRate() const
                {
                    return (Broadcast::ConvertBCD<uint32_t>(&(_data[7]), 7, true));
                }
                fec FECInner() const
                {
                    return (static_cast<fec>(_data[5] & 0xF));
                }

            private:
                MPEG::Descriptor _data;
            };

            class EXTERNAL CableDeliverySystem {
            private:
                CableDeliverySystem operator=(const CableDeliverySystem& rhs) = delete;

            public:
                constexpr static uint8_t TAG = 0x44;

            public:
                CableDeliverySystem()
                    : _data()
                {
                }
                CableDeliverySystem(const CableDeliverySystem& copy)
                    : _data(copy._data)
                {
                }
                CableDeliverySystem(const MPEG::Descriptor& copy)
                    : _data(copy)
                {
                }
                ~CableDeliverySystem()
                {
                }

            public:
                // Frequency in KHz
                uint32_t Frequency() const
                {
                    return (Broadcast::ConvertBCD<uint32_t>(&(_data[0]), 8, true) / 10);
                }
                Broadcast::Modulation Modulation() const
                {
                    uint8_t mod(_data[6]);
                    return (static_cast<Broadcast::Modulation>(((mod == 0) || (mod > 9)) ? 0 : (0x10 << (mod - 1))));
                }
                uint32_t SymbolRate() const
                {
                    return (Broadcast::ConvertBCD<uint32_t>(&(_data[7]), 7, true));
                }
                fec FECInner() const
                {
                    return (static_cast<fec>(_data[5] & 0xF));
                }
                fec_outer FECOuter() const
                {
                    return (static_cast<fec_outer>(_data[10] & 0x3));
                }

            private:
                MPEG::Descriptor _data;
            };

            class EXTERNAL Service {
            private:
                Service operator=(const Service& rhs) = delete;

            public:
                constexpr static uint8_t TAG = 0x48;

                enum type {
                    DIGITAL_TELEVISION = 0x01,
                    DIGITAL_RADIO = 0x02,
                    TELETEXT = 0x03,
                    NVOD_REFERENCE = 0x04,
                    NVOD_TIME_SHIFT = 0x05,
                    MOSAIC = 0x06,
                    ADVANCED_DIGITAL_RADIO = 0x0A,
                    ADVANCED_DIGITAL_MOSAIC = 0x0B,
                    DATA_BROADCAST_SERVICE = 0x0C,
                    ADVANCED_SD_TELEVISION = 0x16,
                    ADVANCED_SD_NVOD_TIME_SHIFT = 0x17,
                    ADVANCED_SD_NVOD_REFERENCE = 0x18,
                    ADVANCED_HD_TELEVISION = 0x19,
                    ADVANCED_HD_NVOD_TIME_SHIFT = 0x1A,
                    ADVANCED_HD_NVOD_REFERENCE = 0x1B
                };

            public:
                Service()
                    : _data()
                {
                }
                Service(const Service& copy)
                    : _data(copy._data)
                {
                }
                Service(const MPEG::Descriptor& copy)
                    : _data(copy)
                {
                }
                ~Service()
                {
                }

            public:
                type Type() const
                {
                    return (static_cast<type>(_data[0]));
                }
                string Provider() const
                {
                    return (Core::ToString(reinterpret_cast<const char*>(&(_data[2])), _data[1]));
                }
                string Name() const
                {
                    uint8_t offset = 1 /* service type */ + 1 /* length */ + _data[1];
                    return (Core::ToString(reinterpret_cast<const char*>(&(_data[offset + 1])), _data[offset]));
                }

            private:
                MPEG::Descriptor _data;
            };
        }
    }
}
} // namespace WPEFramework::Broadcast::DVB::Descriptors

#endif // DESCRIPTORS_H

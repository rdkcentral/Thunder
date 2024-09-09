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

#ifndef DVB_TDT_TABLE_H
#define DVB_TDT_TABLE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "MPEGDescriptor.h"
#include "MPEGSection.h"
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace Thunder {
namespace Broadcast {
    namespace DVB {

        class EXTERNAL TDT {
        private:
            TDT& operator=(const TDT& rhs) = delete;

        public:
            static const uint16_t ID = 0x70;

        public:
            TDT()
                : _time()
            {
            }
            TDT(const MPEG::Section& data)
                : _time()
            {
                if (data.IsValid() == true) {
                    const Core::DataElement info(data.Data());

                    // EXAMPLE: 93/10/13 12:45:00 is coded as "0xC079 124500".
                    uint16_t MJD = (info[0] << 8) | info[1];
                    uint32_t J, C, Y, M;

                    J = MJD + 2400001 + 68569;
                    C = 4 * J / 146097;
                    J = J - (146097 * C + 3) / 4;
                    Y = 4000 * (J + 1) / 1461001;
                    J = J - 1461 * Y / 4 + 31;
                    M = 80 * J / 2447;

                    uint8_t day = static_cast<uint8_t>(J - 2447 * M / 80);
                    J = M / 11;
                    uint8_t month = static_cast<uint8_t>(M + 2 - (12 * J));
                    uint16_t year = static_cast<uint16_t>(100 * (C - 49) + Y + J);
                    uint8_t uren = ((info[2] >> 4) * 10) + (info[2] & 0xF);
                    uint8_t minuten = ((info[3] >> 4) * 10) + (info[3] & 0xF);
                    uint8_t seconden = ((info[4] >> 4) * 10) + (info[4] & 0xF);

                    printf("Loadeded: %d-%d-%d %d:%d.%d\n", day, month, year, uren, minuten, seconden);
                    _time = Core::Time(year, month, day, uren, minuten, seconden, 0, false);
                }
            }
            TDT(const TDT& copy)
                : _time(copy._time)
            {
            }
            ~TDT() {}

        public:
            inline bool IsValid() const
            {
                return (_time.IsValid());
            }
            inline const Core::Time& Time() const { return (_time); }

        private:
            Core::Time _time;
        };

        class EXTERNAL TOT : public TDT {
        private:
            TOT& operator=(const TOT& rhs) = delete;

        public:
            static const uint16_t ID = 0x73;

        public:
            TOT()
                : TDT()
                , _data()
            {
            }
            TOT(const MPEG::Section& data)
                : TDT(data)
                , _data(data.Data())
            {
            }
            TOT(const TOT& copy)
                : TDT(copy)
                , _data(copy._data)
            {
            }
            ~TOT() {}

        public:
            MPEG::DescriptorIterator Descriptors() const
            {
                uint16_t size(((_data[6] & 0xF) << 8) | _data[7]);
                return (MPEG::DescriptorIterator(Core::DataElement(_data, 8, size)));
            }

        private:
            Core::DataElement _data;
        };

    } // namespace DVB
} // namespace Broadcast
} // namespace Thunder

#endif // DVB_TDT_TABLE_H

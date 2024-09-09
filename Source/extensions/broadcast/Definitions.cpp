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

#include "Definitions.h"

namespace Thunder {

ENUM_CONVERSION_BEGIN(Broadcast::ITuner::DTVStandard)
    { Broadcast::ITuner::DVB, _TXT("DVB") },
    { Broadcast::ITuner::ATSC, _TXT("ATSC") },
    { Broadcast::ITuner::ISDB, _TXT("ISDB") },
    { Broadcast::ITuner::DAB, _TXT("DAB") },
ENUM_CONVERSION_END(Broadcast::ITuner::DTVStandard)

ENUM_CONVERSION_BEGIN(Broadcast::ITuner::annex)
    { Broadcast::ITuner::NoAnnex, _TXT("None") },
    { Broadcast::ITuner::A, _TXT("A") },
    { Broadcast::ITuner::B, _TXT("B") },
    { Broadcast::ITuner::C, _TXT("C") },
ENUM_CONVERSION_END(Broadcast::ITuner::annex)

ENUM_CONVERSION_BEGIN(Broadcast::ITuner::modus)
    { Broadcast::ITuner::Cable, _TXT("Cable") },
    { Broadcast::ITuner::Terrestrial, _TXT("Terrestrial") },
    { Broadcast::ITuner::Satellite, _TXT("Satellite") },
ENUM_CONVERSION_END(Broadcast::ITuner::modus)
    
ENUM_CONVERSION_BEGIN(Broadcast::SpectralInversion)
    { Broadcast::Auto, _TXT("Auto") },
    { Broadcast::Normal, _TXT("Normal") },
    { Broadcast::Inverted, _TXT("Inverted") },
ENUM_CONVERSION_END(Broadcast::SpectralInversion)

ENUM_CONVERSION_BEGIN(Broadcast::Modulation)
    { Broadcast::HORIZONTAL_QPSK, _TXT("QPSK_H") },
    { Broadcast::HORIZONTAL_8PSK, _TXT("8PSK_H") },
    { Broadcast::HORIZONTAL_QAM16, _TXT("QAM16_H") },
    { Broadcast::VERTICAL_QPSK, _TXT("QPSK_V") },
    { Broadcast::VERTICAL_8PSK, _TXT("8PSK_V") },
    { Broadcast::VERTICAL_QAM16, _TXT("QAM16_V") },
    { Broadcast::LEFT_QPSK, _TXT("QPSK_L") },
    { Broadcast::LEFT_8PSK, _TXT("8PSK_L") },
    { Broadcast::LEFT_QAM16, _TXT("QAM16_L") },
    { Broadcast::RIGHT_QPSK, _TXT("QPSK_R") },
    { Broadcast::RIGHT_8PSK, _TXT("8PSK_R") },
    { Broadcast::RIGHT_QAM16, _TXT("QAM16_R") },
    { Broadcast::QAM16, _TXT("QAM16") },
    { Broadcast::QAM32, _TXT("QAM32") },
    { Broadcast::QAM64, _TXT("QAM64") },
    { Broadcast::QAM128, _TXT("QAM128") },
    { Broadcast::QAM256, _TXT("QAM256") },
    { Broadcast::QAM512, _TXT("QAM512") },
    { Broadcast::QAM1024, _TXT("QAM1024") },
    { Broadcast::QAM2048, _TXT("QAM2048") },
    { Broadcast::QAM4096, _TXT("QAM4096") },
ENUM_CONVERSION_END(Broadcast::Modulation)

} // namespace Thunder

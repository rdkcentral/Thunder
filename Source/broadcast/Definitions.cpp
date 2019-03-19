#include "Definitions.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Broadcast::ITuner::DTVStandard){ Broadcast::ITuner::DVB, _TXT("DVB") },
    { Broadcast::ITuner::ATSC, _TXT("ATSC") },
    ENUM_CONVERSION_END(Broadcast::ITuner::DTVStandard)

        ENUM_CONVERSION_BEGIN(Broadcast::ITuner::Annex){ Broadcast::ITuner::A, _TXT("A") },
    { Broadcast::ITuner::B, _TXT("B") },
    { Broadcast::ITuner::C, _TXT("C") },
    ENUM_CONVERSION_END(Broadcast::ITuner::Annex)

        ENUM_CONVERSION_BEGIN(Broadcast::SpectralInversion){ Broadcast::Auto, _TXT("Auto") },
    { Broadcast::Normal, _TXT("Normal") },
    { Broadcast::Inverted, _TXT("Inverted") },
    ENUM_CONVERSION_END(Broadcast::SpectralInversion)

        ENUM_CONVERSION_BEGIN(Broadcast::Modulation){ Broadcast::HORIZONTAL_QPSK, _TXT("QPSK_H") },
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

} // namespace WPEFramework

#include "Definitions.h"

namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::DTVStandard)
    { Broadcast::ITuner::DVB,     _TXT("DVB")       },
    { Broadcast::ITuner::ATSC,    _TXT("ATSC")      },
    ENUM_CONVERSION_END(Broadcast::ITuner::DTVStandard)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::Annex)
    { Broadcast::ITuner::A,     _TXT("A")       },
    { Broadcast::ITuner::B,     _TXT("B")       },
    { Broadcast::ITuner::C,     _TXT("C")       },
    ENUM_CONVERSION_END(Broadcast::ITuner::Annex)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::SpectralInversion)
    { Broadcast::ITuner::Auto,     _TXT("Auto")     },
    { Broadcast::ITuner::Normal,   _TXT("Normal")   },
    { Broadcast::ITuner::Inverted, _TXT("Inverted") },
    ENUM_CONVERSION_END(Broadcast::ITuner::SpectralInversion)

    ENUM_CONVERSION_BEGIN(Broadcast::ITuner::Modulation)
    { Broadcast::ITuner::QAM16,    _TXT("QAM16")     },
    { Broadcast::ITuner::QAM32,    _TXT("QAM32")     },
    { Broadcast::ITuner::QAM64,    _TXT("QAM64")     },
    { Broadcast::ITuner::QAM128,   _TXT("QAM128")    },
    { Broadcast::ITuner::QAM256,   _TXT("QAM256")    },
    { Broadcast::ITuner::QAM512,   _TXT("QAM512")    },
    { Broadcast::ITuner::QAM1024,  _TXT("QAM1024")   },
    { Broadcast::ITuner::QAM2048,  _TXT("QAM2048")   },
    { Broadcast::ITuner::QAM4096,  _TXT("QAM4096")   },
    ENUM_CONVERSION_END(Broadcast::ITuner::Modulation)

} // namespace WPEFramework


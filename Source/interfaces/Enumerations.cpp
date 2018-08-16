#include "IComposition.h"

namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(Exchange::IComposition::ScreenResolution)

    { Exchange::IComposition::ScreenResolution_Unknown,   _TXT("Unknown")   },
    { Exchange::IComposition::ScreenResolution_480i,      _TXT("480i")      },
    { Exchange::IComposition::ScreenResolution_480p,      _TXT("480p")      },
    { Exchange::IComposition::ScreenResolution_720p,      _TXT("720p")      },
    { Exchange::IComposition::ScreenResolution_720p50Hz,  _TXT("720p50Hz")  },
    { Exchange::IComposition::ScreenResolution_1080p24Hz, _TXT("1080p24Hz") },
    { Exchange::IComposition::ScreenResolution_1080i50Hz, _TXT("1080i50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p50Hz, _TXT("1080p50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p60Hz, _TXT("1080p60Hz") },
    { Exchange::IComposition::ScreenResolution_2160p50Hz, _TXT("2160p50Hz") },
    { Exchange::IComposition::ScreenResolution_2160p60Hz, _TXT("2160p60Hz") },

    ENUM_CONVERSION_END(Exchange::IComposition::ScreenResolution)

}

#include "IComposition.h"

namespace {

        struct ScreenResolutionWidthHeight {
            WPEFramework::Exchange::IComposition::ScreenResolution resolution;
            uint32_t width;
            uint32_t height;
        };

        ScreenResolutionWidthHeight resolutionWidthHeightTable[] = {

            { WPEFramework::Exchange::IComposition::ScreenResolution_Unknown,        0,         0 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_480i,          640,      480 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_480p,          640,      480 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_720p,         1280,      720 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_720p50Hz,     1280,      720 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_1080p24Hz,    1920,     1080 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_1080i50Hz,    1920,     1080 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_1080p50Hz,    1920,     1080 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_1080p60Hz,    1920,     1080 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_2160p50Hz,    3840,     2160 },
            { WPEFramework::Exchange::IComposition::ScreenResolution_2160p60Hz,    3840,     2160 },
        };

}

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

    uint32_t Exchange::IComposition::WidthFromResolution(const ScreenResolution resolution)  {
        return (static_cast<uint32_t>(resolution) < ((sizeof(resolutionWidthHeightTable)/sizeof(ScreenResolutionWidthHeight)) ? resolutionWidthHeightTable[static_cast<uint32_t>(resolution)].width : 0));
    }

    uint32_t Exchange::IComposition::HeightFromResolution(const ScreenResolution resolution) {
        return (static_cast<uint32_t>(resolution) < ((sizeof(resolutionWidthHeightTable)/sizeof(ScreenResolutionWidthHeight)) ? resolutionWidthHeightTable[static_cast<uint32_t>(resolution)].height : 0));
    }      
}



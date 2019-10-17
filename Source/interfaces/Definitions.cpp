/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "definitions.h"

namespace WPEFramework {

struct ScreenResolutionWidthHeight {
    Exchange::IComposition::ScreenResolution resolution;
    uint32_t width;
    uint32_t height;
};

ScreenResolutionWidthHeight resolutionWidthHeightTable[] = {

    { Exchange::IComposition::ScreenResolution_Unknown, 0, 0 },
    { Exchange::IComposition::ScreenResolution_480i, 640, 480 },
    { Exchange::IComposition::ScreenResolution_480p, 640, 480 },
    { Exchange::IComposition::ScreenResolution_720p, 1280, 720 },
    { Exchange::IComposition::ScreenResolution_720p50Hz, 1280, 720 },
    { Exchange::IComposition::ScreenResolution_1080p24Hz, 1920, 1080 },
    { Exchange::IComposition::ScreenResolution_1080i50Hz, 1920, 1080 },
    { Exchange::IComposition::ScreenResolution_1080p50Hz, 1920, 1080 },
    { Exchange::IComposition::ScreenResolution_1080p60Hz, 1920, 1080 },
    { Exchange::IComposition::ScreenResolution_2160p50Hz, 3840, 2160 },
    { Exchange::IComposition::ScreenResolution_2160p60Hz, 3840, 2160 },
};

ENUM_CONVERSION_BEGIN(Exchange::IComposition::ScreenResolution)
    { Exchange::IComposition::ScreenResolution_Unknown, _TXT("Unknown") },
    { Exchange::IComposition::ScreenResolution_480i, _TXT("480i") },
    { Exchange::IComposition::ScreenResolution_480p, _TXT("480p") },
    { Exchange::IComposition::ScreenResolution_720p, _TXT("720p") },
    { Exchange::IComposition::ScreenResolution_720p50Hz, _TXT("720p50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p24Hz, _TXT("1080p24Hz") },
    { Exchange::IComposition::ScreenResolution_1080i50Hz, _TXT("1080i50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p50Hz, _TXT("1080p50Hz") },
    { Exchange::IComposition::ScreenResolution_1080p60Hz, _TXT("1080p60Hz") },
    { Exchange::IComposition::ScreenResolution_2160p50Hz, _TXT("2160p50Hz") },
    { Exchange::IComposition::ScreenResolution_2160p60Hz, _TXT("2160p60Hz") },
ENUM_CONVERSION_END(Exchange::IComposition::ScreenResolution)

ENUM_CONVERSION_BEGIN(Exchange::IStream::streamtype)
    { Exchange::IStream::streamtype::Undefined, _TXT(_T("Undefined")) },
    { Exchange::IStream::streamtype::Cable, _TXT(_T("Cable")) },
    { Exchange::IStream::streamtype::Handheld, _TXT(_T("Handheld")) },
    { Exchange::IStream::streamtype::Satellite, _TXT(_T("Satellite")) },
    { Exchange::IStream::streamtype::Terrestrial, _TXT(_T("Terrestrial")) },
    { Exchange::IStream::streamtype::DAB, _TXT(_T("DAB")) },
    { Exchange::IStream::streamtype::RF, _TXT(_T("RF")) },
    { Exchange::IStream::streamtype::Unicast, _TXT(_T("Unicast")) },
    { Exchange::IStream::streamtype::Multicast, _TXT(_T("Multicast")) },
    { Exchange::IStream::streamtype::IP, _TXT(_T("IP")) },
ENUM_CONVERSION_END(Exchange::IStream::streamtype)

ENUM_CONVERSION_BEGIN(Exchange::IStream::state)
    { Exchange::IStream::state::Idle, _TXT(_T("Idle")) },
    { Exchange::IStream::state::Loading, _TXT(_T("Loading")) },
    { Exchange::IStream::state::Prepared, _TXT(_T("Prepared")) },
    { Exchange::IStream::state::Controlled, _TXT(_T("Controlled")) },
    { Exchange::IStream::state::Error, _TXT(_T("Error")) },
ENUM_CONVERSION_END(Exchange::IStream::state)

ENUM_CONVERSION_BEGIN(Exchange::IVoiceProducer::IProfile::codec)
    { Exchange::IVoiceProducer::IProfile::codec::UNDEFINED, _TXT(_T("undefined")) },
    { Exchange::IVoiceProducer::IProfile::codec::PCM, _TXT(_T("pcm")) },
    { Exchange::IVoiceProducer::IProfile::codec::ADPCM, _TXT(_T("adpcm")) },
ENUM_CONVERSION_END(Exchange::IVoiceProducer::IProfile::codec)

namespace Exchange
{

    uint32_t IComposition::WidthFromResolution(const IComposition::ScreenResolution resolution)
    {
        return ((static_cast<uint32_t>(resolution) < sizeof(resolutionWidthHeightTable) / sizeof(ScreenResolutionWidthHeight)) ? resolutionWidthHeightTable[static_cast<uint32_t>(resolution)].width : 0);
    }

    uint32_t IComposition::HeightFromResolution(const IComposition::ScreenResolution resolution)
    {
        return ((static_cast<uint32_t>(resolution) < sizeof(resolutionWidthHeightTable) / sizeof(ScreenResolutionWidthHeight)) ? resolutionWidthHeightTable[static_cast<uint32_t>(resolution)].height : 0);
    }
}
}

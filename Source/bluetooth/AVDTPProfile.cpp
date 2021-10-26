/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#include "Module.h"
#include "AVDTPProfile.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::category)
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::MEDIA_TRANSPORT,    _TXT("Media Transport") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::REPORTING,          _TXT("Reporting") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::RECOVERY,           _TXT("Recovery") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::CONTENT_PROTECTION, _TXT("Content Protection") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::HEADER_COMPRESSION, _TXT("Header Compression") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::MULTIPLEXING,       _TXT("Multiplexing") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::MEDIA_CODEC,        _TXT("Media Codec") },
    { Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::DELAY_REPORTING,    _TXT("Delay Reporting") },
ENUM_CONVERSION_END(Bluetooth::AVDTPProfile::StreamEndPoint::ServiceCapabilities::category)

namespace Bluetooth {

} // namespace Bluetooth

}

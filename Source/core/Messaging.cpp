/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological
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

#include "Messaging.h"
#include "Frame.h"

namespace Thunder {

namespace Core {

    namespace Messaging {

        uint16_t TextMessage::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            writer.NullTerminatedText(_text, bufferSize);

            return (std::min(bufferSize, static_cast<uint16_t>(_text.size() + 1)));
        }

        uint16_t TextMessage::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            Core::FrameType<0> frame(const_cast<uint8_t*>(buffer), bufferSize, bufferSize);
            Core::FrameType<0>::Reader reader(frame, 0);

            _text = reader.NullTerminatedText();

            return (static_cast<uint16_t>(_text.size() + 1));
        }
    }
}
}
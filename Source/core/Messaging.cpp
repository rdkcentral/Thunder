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

        uint16_t TelemetryMessage::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            uint16_t offset = 0;

            if (bufferSize > 0) {
                // First byte: value type tag
                buffer[0] = static_cast<uint8_t>(_type);
                offset = 1;

                switch (_type) {
                case ValueType::TEXT: {
                    if (offset < bufferSize) {
                        Core::FrameType<0> frame(buffer + offset, bufferSize - offset, bufferSize - offset);
                        Core::FrameType<0>::Writer writer(frame, 0);
                        writer.NullTerminatedText(_text, bufferSize - offset);
                        offset += std::min(static_cast<uint16_t>(bufferSize - offset), static_cast<uint16_t>(_text.size() + 1));
                    }
                    break;
                }
                case ValueType::INT8:    offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<int8_t>(_numericValue._signed));     break;
                case ValueType::UINT8:   offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<uint8_t>(_numericValue._unsigned));   break;
                case ValueType::INT16:   offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<int16_t>(_numericValue._signed));    break;
                case ValueType::UINT16:  offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<uint16_t>(_numericValue._unsigned));  break;
                case ValueType::INT32:   offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<int32_t>(_numericValue._signed));    break;
                case ValueType::UINT32:  offset += SerializeNumeric(buffer, offset, bufferSize, static_cast<uint32_t>(_numericValue._unsigned));  break;
                case ValueType::INT64:   offset += SerializeNumeric(buffer, offset, bufferSize, _numericValue._signed);                          break;
                case ValueType::UINT64:  offset += SerializeNumeric(buffer, offset, bufferSize, _numericValue._unsigned);                        break;
                case ValueType::FLOAT32: offset += SerializeNumeric(buffer, offset, bufferSize, _numericValue._float);                           break;
                case ValueType::FLOAT64: offset += SerializeNumeric(buffer, offset, bufferSize, _numericValue._double);                          break;
                }
            }

            return (offset);
        }

        uint16_t TelemetryMessage::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t offset = 0;

            if (bufferSize > 0) {
                _type = static_cast<ValueType>(buffer[0]);
                offset = 1;
                _numericValue = {};

                switch (_type) {
                case ValueType::TEXT: {
                    if (offset < bufferSize) {
                        Core::FrameType<0> frame(const_cast<uint8_t*>(buffer + offset), bufferSize - offset, bufferSize - offset);
                        Core::FrameType<0>::Reader reader(frame, 0);
                        _text = reader.NullTerminatedText();
                        offset += static_cast<uint16_t>(_text.size() + 1);
                    }
                    break;
                }
                case ValueType::INT8:    offset += DeserializeIntegral<int8_t>(buffer, offset, bufferSize);    break;
                case ValueType::UINT8:   offset += DeserializeIntegral<uint8_t>(buffer, offset, bufferSize);   break;
                case ValueType::INT16:   offset += DeserializeIntegral<int16_t>(buffer, offset, bufferSize);   break;
                case ValueType::UINT16:  offset += DeserializeIntegral<uint16_t>(buffer, offset, bufferSize);  break;
                case ValueType::INT32:   offset += DeserializeIntegral<int32_t>(buffer, offset, bufferSize);   break;
                case ValueType::UINT32:  offset += DeserializeIntegral<uint32_t>(buffer, offset, bufferSize);  break;
                case ValueType::INT64:   offset += DeserializeIntegral<int64_t>(buffer, offset, bufferSize);   break;
                case ValueType::UINT64:  offset += DeserializeIntegral<uint64_t>(buffer, offset, bufferSize);  break;
                case ValueType::FLOAT32: offset += DeserializeFloat<float>(buffer, offset, bufferSize);        break;
                case ValueType::FLOAT64: offset += DeserializeFloat<double>(buffer, offset, bufferSize);       break;
                default: {
                    _type = ValueType::TEXT;
                    break;
                }
                }
            }

            return (offset);
        }
    }
}
}
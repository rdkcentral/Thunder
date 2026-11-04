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

        TelemetryMessage::TelemetryMessage(const float value)
            : _type(ValueType::FLOAT32)
            , _text()
            , _numericValue{}
        {
            _numericValue._float = value;
            _text = Core::Format(_T("%g"), static_cast<double>(value));
        }

        TelemetryMessage::TelemetryMessage(const double value)
            : _type(ValueType::FLOAT64)
            , _text()
            , _numericValue{}
        {
            _numericValue._double = value;
            _text = Core::Format(_T("%g"), value);
        }

        void TelemetryMessage::Stringify(const int64_t value)
        {
            _text = Core::NumberType<int64_t>(value).Text();
        }

        void TelemetryMessage::Stringify(const uint64_t value)
        {
            _text = Core::NumberType<uint64_t>(value).Text();
        }

        uint16_t TelemetryMessage::Serialize(uint8_t buffer[], const uint16_t bufferSize) const
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            writer.Number(_type);

            switch (_type) {
            case ValueType::TEXT:    writer.NullTerminatedText(_text, bufferSize - writer.Offset()); break;
            case ValueType::INT8:    writer.Number(static_cast<int8_t>(_numericValue._signed));      break;
            case ValueType::UINT8:   writer.Number(static_cast<uint8_t>(_numericValue._unsigned));   break;
            case ValueType::INT16:   writer.Number(static_cast<int16_t>(_numericValue._signed));     break;
            case ValueType::UINT16:  writer.Number(static_cast<uint16_t>(_numericValue._unsigned));  break;
            case ValueType::INT32:   writer.Number(static_cast<int32_t>(_numericValue._signed));     break;
            case ValueType::UINT32:  writer.Number(static_cast<uint32_t>(_numericValue._unsigned));  break;
            case ValueType::INT64:   writer.Number(_numericValue._signed);                           break;
            case ValueType::UINT64:  writer.Number(_numericValue._unsigned);                         break;
            case ValueType::FLOAT32: writer.Number(_numericValue._float);                            break;
            case ValueType::FLOAT64: writer.Number(_numericValue._double);                           break;
            }

            return (writer.Offset());
        }

        uint16_t TelemetryMessage::Deserialize(const uint8_t buffer[], const uint16_t bufferSize)
        {
            uint16_t offset = 0;

            if (bufferSize > 0) {
                Core::FrameType<0> frame(const_cast<uint8_t*>(buffer), bufferSize, bufferSize);
                Core::FrameType<0>::Reader reader(frame, 0);

                _type = reader.Number<ValueType>();
                _text.clear();
                _numericValue = {};

                bool valid = true;

                switch (_type) {
                case ValueType::TEXT:
                    _text = reader.NullTerminatedText();
                    break;
                case ValueType::INT8: {
                    auto v = reader.Number<int8_t>();
                    _numericValue._signed = static_cast<int64_t>(v);
                    _text = Core::NumberType<int8_t>(v).Text();
                    break;
                }
                case ValueType::UINT8: {
                    auto v = reader.Number<uint8_t>();
                    _numericValue._unsigned = static_cast<uint64_t>(v);
                    _text = Core::NumberType<uint8_t>(v).Text();
                    break;
                }
                case ValueType::INT16: {
                    auto v = reader.Number<int16_t>();
                    _numericValue._signed = static_cast<int64_t>(v);
                    _text = Core::NumberType<int16_t>(v).Text();
                    break;
                }
                case ValueType::UINT16: {
                    auto v = reader.Number<uint16_t>();
                    _numericValue._unsigned = static_cast<uint64_t>(v);
                    _text = Core::NumberType<uint16_t>(v).Text();
                    break;
                }
                case ValueType::INT32: {
                    auto v = reader.Number<int32_t>();
                    _numericValue._signed = static_cast<int64_t>(v);
                    _text = Core::NumberType<int32_t>(v).Text();
                    break;
                }
                case ValueType::UINT32: {
                    auto v = reader.Number<uint32_t>();
                    _numericValue._unsigned = static_cast<uint64_t>(v);
                    _text = Core::NumberType<uint32_t>(v).Text();
                    break;
                }
                case ValueType::INT64: {
                    auto v = reader.Number<int64_t>();
                    _numericValue._signed = v;
                    _text = Core::NumberType<int64_t>(v).Text();
                    break;
                }
                case ValueType::UINT64: {
                    auto v = reader.Number<uint64_t>();
                    _numericValue._unsigned = v;
                    _text = Core::NumberType<uint64_t>(v).Text();
                    break;
                }
                case ValueType::FLOAT32: {
                    float v = reader.Number<float>();
                    _numericValue._float = v;
                    _text = Core::Format(_T("%g"), static_cast<double>(v));
                    break;
                }
                case ValueType::FLOAT64: {
                    double v = reader.Number<double>();
                    _numericValue._double = v;
                    _text = Core::Format(_T("%g"), v);
                    break;
                }
                default:
                    _type = ValueType::TEXT;
                    valid = false;
                    break;
                }

                if (valid == true) {
                    offset = static_cast<uint16_t>(bufferSize - reader.Length());
                }
            }

            return (offset);
        }
    }
}
}
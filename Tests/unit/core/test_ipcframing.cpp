/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Metrological
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

#include <gtest/gtest.h>

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include <core/IPCConnector.h>

#include <vector>

namespace Thunder {
namespace Tests {
    namespace Core {

        namespace {
            class Message : public ::Thunder::Core::IMessage {
            public:
                uint32_t Label() const override { return 1; }
                uint32_t Length() const override { return static_cast<uint32_t>(_payload.size()); }

                uint16_t Serialize(uint8_t[], const uint16_t, const uint32_t) const override
                {
                    return 0;
                }

                uint16_t Deserialize(const uint8_t stream[], const uint16_t length, const uint32_t offset) override
                {
                    EXPECT_EQ(_payload.size(), offset);
                    _payload.insert(_payload.end(), stream, stream + length);
                    return length;
                }

                const std::vector<uint8_t>& Payload() const { return _payload; }

            private:
                std::vector<uint8_t> _payload;
            };

            class Deserializer : public ::Thunder::Core::IMessage::Deserializer {
            public:
                Deserializer()
                    : _elements(0)
                    , _dispatches(0)
                {
                }

                ::Thunder::Core::IMessage* Element(const uint32_t& label) override
                {
                    EXPECT_EQ(1u, label);
                    ++_elements;
                    return &_message;
                }

                void Deserialized(::Thunder::Core::IMessage&) override
                {
                    ++_dispatches;
                }

                uint32_t Elements() const { return _elements; }
                uint32_t Dispatches() const { return _dispatches; }
                const Message& Received() const { return _message; }

            private:
                Message _message;
                uint32_t _elements;
                uint32_t _dispatches;
            };

            void Feed(Deserializer& deserializer, const std::vector<uint8_t>& data, const uint16_t offset, const uint16_t length)
            {
                ASSERT_EQ(length, deserializer.Deserialize(data.data() + offset, length));
            }
        }

        TEST(Core_IPCFraming, UndersizedCommandVarintDoesNotDispatchAndParserRecovers)
        {
            // The frame declares one byte after the length, but the command varint
            // requires another byte. The continuation byte exhausts the frame.
            const std::vector<uint8_t> malformed = { 0x01, 0x80 };
            const std::vector<uint8_t> valid = { 0x02, 0x01, 0xAB };

            for (uint16_t split = 0; split <= malformed.size(); ++split) {
                SCOPED_TRACE(split);
                Deserializer deserializer;

                Feed(deserializer, malformed, 0, split);
                Feed(deserializer, malformed, split, static_cast<uint16_t>(malformed.size() - split));

                EXPECT_EQ(0u, deserializer.Elements());
                EXPECT_EQ(0u, deserializer.Dispatches());

                Feed(deserializer, valid, 0, static_cast<uint16_t>(valid.size()));

                EXPECT_EQ(1u, deserializer.Elements());
                EXPECT_EQ(1u, deserializer.Dispatches());
                EXPECT_EQ((std::vector<uint8_t>{ 0xAB }), deserializer.Received().Payload());
            }
        }

    } // namespace Core
} // namespace Tests
} // namespace Thunder

#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Messaging {

    class EXTERNAL TextMessage : public Core::Messaging::IEvent {
    public:
        TextMessage() = default;
        TextMessage(const string& text)
            : _text(text)
        {
        }

        TextMessage(const TextMessage&) = delete;
        TextMessage& operator=(const TextMessage&) = delete;

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override
        {
            uint16_t length = 0;
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            if (bufferSize < _text.size() + 1) {
                string cutString(_text, 0, bufferSize - 1);
                writer.NullTerminatedText(cutString);
                length = bufferSize;

            } else {
                writer.NullTerminatedText(_text);
                length = _text.size() + 1;
            }

            return length;
        }
        uint16_t Deserialize(uint8_t buffer[], const uint16_t bufferSize) override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Reader reader(frame, 0);

            _text = reader.NullTerminatedText();

            return _text.size() + 1;
        }

        void ToString(string& text) const override
        {
            text = _text;
        }

    private:
        string _text;
    };
}
}
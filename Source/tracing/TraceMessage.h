#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Trace {

    class EXTERNAL TraceMessage : public Core::IMessageEvent {
    public:
        TraceMessage() = default;
        TraceMessage(const string& text)
            : _text(text)
        {
        }

        TraceMessage(const TraceMessage&) = delete;
        TraceMessage& operator=(const TraceMessage&) = delete;
        

        uint16_t Serialize(uint8_t buffer[], const uint16_t bufferSize) const override
        {
            Core::FrameType<0> frame(buffer, bufferSize, bufferSize);
            Core::FrameType<0>::Writer writer(frame, 0);

            writer.NullTerminatedText(_text);

            return _text.size() + 1;
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